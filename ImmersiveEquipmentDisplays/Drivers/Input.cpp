#include "pch.h"

#include "Input.h"

#include <ext/IHook.h>
#include <ext/JITASM.h>

namespace IED
{
	namespace Drivers
	{
		using namespace ::Events;

		Input Input::m_Instance;

		bool Input::PlayerControls_InputEvent_ProcessEvent_Hook(
			const InputEvent** a_evns)
		{
			bool blocked = m_Instance.m_inputBlocked.load(
				std::memory_order_relaxed);

			if (blocked)
			{
				// nasty, implement this properly
				if (auto pc = PlayerControls::GetSingleton())
				{
					pc->lookInput = { 0, 0 };
					pc->moveInput = { 0, 0 };
					pc->prevMove = { 0, 0 };
					pc->prevLook = { 0, 0 };
				}
			}

			if (a_evns)
			{
				for (auto inputEvent = *a_evns; inputEvent; inputEvent = inputEvent->next)
				{
					auto buttonEvent = inputEvent->AsButtonEvent();
					if (!buttonEvent)
					{
						continue;
					}

					auto deviceType = buttonEvent->device;
					std::uint32_t keyCode;

					if (deviceType == INPUT_DEVICE::kMouse)
					{
						keyCode = InputMap::kMacro_MouseButtonOffset + buttonEvent->GetIDCode();
					}
					else if (deviceType == INPUT_DEVICE::kKeyboard)
					{
						keyCode = buttonEvent->GetIDCode();
					}
					else
					{
						continue;
					}

					if (!keyCode || keyCode >= InputMap::kMaxMacros)
					{
						continue;
					}

					//_DMESSAGE("%X | %f | %f", keyCode, buttonEvent->value, buttonEvent->heldDownSecs);

					if (buttonEvent->IsDown())
					{
						m_Instance.DispatchPriorityKeyEvent(
							KeyEventState::KeyDown,
							keyCode);
					}
					else if (buttonEvent->IsUpLF())
					{
						m_Instance.DispatchPriorityKeyEvent(
							KeyEventState::KeyUp,
							keyCode);
					}
				}
			}

			if (m_Instance.m_nextIEPCall)
			{
				blocked |= m_Instance.m_nextIEPCall(a_evns);
			}

			return blocked;
		}

		auto Input::ReceiveEvent(
			InputEvent* const* a_evns,
			BSTEventSource<InputEvent*>* a_dispatcher)
			-> EventResult
		{
			if (a_evns)
			{
				for (auto inputEvent = *a_evns; inputEvent; inputEvent = inputEvent->next)
				{
					auto buttonEvent = inputEvent->AsButtonEvent();
					if (!buttonEvent)
					{
						continue;
					}

					if (buttonEvent->device != INPUT_DEVICE::kKeyboard)
					{
						continue;
					}

					std::uint32_t keyCode = buttonEvent->GetIDCode();

					if (!keyCode || keyCode >= InputMap::kMaxMacros)
					{
						continue;
					}

					if (buttonEvent->IsDown())
					{
						DispatchKeyEvent(KeyEventState::KeyDown, keyCode);
					}
					else if (buttonEvent->IsUpLF())
					{
						DispatchKeyEvent(KeyEventState::KeyUp, keyCode);
					}
				}
			}

			return EventResult::kContinue;
		}

		template <bool _IsAE, class T>
		static constexpr bool ExtractHookCallAddr(
			std::uintptr_t a_dst,
			T& a_out)
		{
#pragma pack(push, 1)

			struct payload_se_t
			{
				/*	
					mov rcx, r12
				*/
				std::uint8_t m[3];
				// call ..
				std::uint8_t escape;
				std::uint8_t modrm;
				std::int32_t displ;
				// ...
			};

			struct payload_ae_t
			{
				/*	
					push rax
					push rcx
					push r11
					sub rsp, 0x28
					mov rcx, r12
				*/
				std::uint8_t m[11];
				// call ..
				std::uint8_t escape;
				std::uint8_t modrm;
				std::int32_t displ;
				// ...
			};

#pragma pack(pop)

			using payload_t = std::conditional_t<
				_IsAE,
				payload_ae_t,
				payload_se_t>;

			std::uintptr_t traddr;

			if (!Hook::GetDst5<0xE9>(a_dst, traddr))
			{
				return false;
			}

			const payload_t* pl{ nullptr };

			if (!Hook::GetDst6<0x25>(traddr, pl))
			{
				return false;
			}

			if constexpr (_IsAE)
			{
				if (!Patching::validate_mem(
						std::uintptr_t(pl->m),
						{ 0x50,
				          0x51,
				          0x41,
				          0x53,
				          0x48,
				          0x83,
				          0xEC,
				          0x28,
				          0x4C,
				          0x89,
				          0xE1 }))
				{
					return false;
				}
			}
			else
			{
				if (!Patching::validate_mem(
						std::uintptr_t(pl->m),
						{ 0x4C,
				          0x89,
				          0xE1 }))
				{
					return false;
				}
			}

			return Hook::GetDst6<0x15>(
				std::uintptr_t(std::addressof(pl->escape)),
				a_out);
		}

		void Input::InstallPriorityHook()
		{
			struct ProcessInputEvent : JITASM::JITASM
			{
				ProcessInputEvent(std::uintptr_t a_targetAddr) :
					JITASM(ISKSE::GetLocalTrampoline())
				{
					Xbyak::Label exitSkipLabel;
					Xbyak::Label exitContinueLabel;
					Xbyak::Label callLabel;

					Xbyak::Label skip;

					if (IAL::IsAE())
					{
						push(rax);
						push(rcx);
						push(r11);
						sub(rsp, 0x28);

						mov(rcx, r12);
						call(ptr[rip + callLabel]);
						mov(dl, al);

						add(rsp, 0x28);
						pop(r11);
						pop(rcx);
						pop(rax);

						test(dl, dl);
						jne(skip);

						cmp(rsi, rbx);
					}
					else
					{
						mov(rcx, r12);
						call(ptr[rip + callLabel]);
						test(al, al);
						jne(skip);

						cmp(rdi, rbx);
					}

					je(skip);

					jmp(ptr[rip + exitContinueLabel]);

					L(skip);
					jmp(ptr[rip + exitSkipLabel]);

					L(exitSkipLabel);
					dq(a_targetAddr + (IAL::IsAE() ? 0x66 : 0x6A));

					L(exitContinueLabel);
					dq(a_targetAddr + 0x5);

					L(callLabel);
					dq(std::uintptr_t(PlayerControls_InputEvent_ProcessEvent_Hook));
				}
			};

			if ((IAL::IsAE() ?
                     ExtractHookCallAddr<true>(
						 m_unkIEProc_a,
						 m_Instance.m_nextIEPCall) :
                     ExtractHookCallAddr<false>(
						 m_unkIEProc_a,
						 m_Instance.m_nextIEPCall)))
			{
				m_Instance.Debug(
					"%s: extracted previous hook call address @%p",
					__FUNCTION__,
					m_Instance.m_nextIEPCall);
			}
			else
			{
				if (IAL::IsAE())
				{
					ASSERT_STR(
						Patching::validate_mem(
							m_unkIEProc_a,
							{ 0x48, 0x3B, 0xF3, 0x74, 0x61 }),
						"Memory validation failed");
				}
				else
				{
					ASSERT_STR(
						Patching::validate_mem(
							m_unkIEProc_a,
							{ 0x48, 0x3B, 0xFB, 0x74, 0x65 }),
						"Memory validation failed");
				}
			}

			m_Instance.LogPatchBegin(__FUNCTION__);
			{
				ProcessInputEvent code(m_unkIEProc_a);

				ISKSE::GetBranchTrampoline().Write5Branch(m_unkIEProc_a, code.get());
			}
			m_Instance.LogPatchEnd(__FUNCTION__);
		}

		bool Input::SinkToInputDispatcher()
		{
			if (auto evd = InputEventDispatcher::GetSingleton())
			{
				evd->AddEventSink(std::addressof(m_Instance));
				return true;
			}
			else
			{
				return false;
			}
		}

		void Input::RegisterForPriorityKeyEvents(
			EventSink<Handlers::KeyEvent>* const a_handler)
		{
			m_Instance.m_prioHandlers.AddSink(a_handler);
		}

		void Input::RegisterForPriorityKeyEvents(
			EventSink<Handlers::KeyEvent>& a_handler)
		{
			m_Instance.m_prioHandlers.AddSink(std::addressof(a_handler));
		}

		void Input::RegisterForKeyEvents(
			EventSink<Handlers::KeyEvent>* const a_handler)
		{
			m_Instance.m_handlers.AddSink(a_handler);
		}

		void Input::RegisterForKeyEvents(
			EventSink<Handlers::KeyEvent>& a_handler)
		{
			m_Instance.m_handlers.AddSink(std::addressof(a_handler));
		}

		void Input::DispatchPriorityKeyEvent(
			KeyEventState a_event,
			std::uint32_t a_keyCode)
		{
			Handlers::KeyEvent evn{
				a_event,
				a_keyCode
			};

			m_prioHandlers.SendEvent(evn);
		}

		void Input::DispatchKeyEvent(
			KeyEventState a_event,
			std::uint32_t a_keyCode)
		{
			Handlers::KeyEvent evn{
				a_event,
				a_keyCode
			};

			m_handlers.SendEvent(evn);
		}
	}  // namespace Drivers
}  // namespace IED