#include "pch.h"

#include "Render.h"

#include <skse64/NiRenderer.h>

namespace IED
{
	namespace Drivers
	{
		using namespace ::Events;

		Render Render::m_Instance;

		bool Render::Initialize()
		{
			if (!Hook::CheckDst5<0xE8>(m_Instance.m_createD3D11_a) ||
			    !Hook::CheckDst5<0xE8>(m_Instance.m_unkPresent_a))
			{
				m_Instance.Error("Unable to hook, one or more invalid targets");
				return false;
			}

			auto& skse = ISKSE::GetSingleton();

			ASSERT(Hook::Call5(
				skse.GetBranchTrampoline(),
				m_Instance.m_createD3D11_a,
				std::uintptr_t(CreateD3D11_Hook),
				m_Instance.m_createD3D11_o));

			ASSERT(Hook::Call5(
				skse.GetBranchTrampoline(),
				m_Instance.m_unkPresent_a,
				std::uintptr_t(Present_Pre),
				m_Instance.m_unkPresent_o));

			return true;
		}

		void Render::CreateD3D11_Hook()
		{
			m_Instance.m_createD3D11_o();
			m_Instance.InitializeD3D();
		}

		void Render::InitializeD3D()
		{
			auto renderManager = BSRenderManager::GetSingleton();

			ASSERT(renderManager != nullptr);

			auto swapChain = renderManager->swapChain;

			ASSERT(swapChain != nullptr);

			DXGI_SWAP_CHAIN_DESC sd{};
			if (FAILED(swapChain->GetDesc(std::addressof(sd))))
			{
				constexpr auto error_msg = "IDXGISwapChain::GetDesc failed";

				Error(error_msg);
				WinApi::MessageBoxError(PLUGIN_NAME, error_msg);

				return;
			}

			m_device       = renderManager->forwarder;
			m_context      = renderManager->context;
			m_swapChain    = swapChain;
			m_bufferSize.x = static_cast<float>(sd.BufferDesc.Width);
			m_bufferSize.y = static_cast<float>(sd.BufferDesc.Height);
			m_bufferSize.z = m_bufferSize.x / m_bufferSize.y;

			Debug("%s: initialized, sending event", __FUNCTION__);

			Events::D3D11CreateEventPost evd_post{
				std::addressof(sd),
				renderManager->forwarder,
				renderManager->context,
				swapChain
			};

			GetEventDispatcher<Events::D3D11CreateEventPost>().SendEvent(evd_post);
		}

		void Render::Present_Pre(std::uint32_t a_p1)
		{
			m_Instance.m_unkPresent_o(a_p1);

			Events::IDXGISwapChainPresent evn;

			m_Instance.GetEventDispatcher<Events::IDXGISwapChainPresent>().SendEvent(evn);
		}
	}
}