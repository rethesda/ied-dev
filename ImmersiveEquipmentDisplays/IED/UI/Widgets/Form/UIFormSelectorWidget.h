#pragma once

#include "IED/Controller/IForm.h"
#include "IED/UI/UIFormBrowser.h"
#include "IED/UI/UILocalizationInterface.h"
#include "IED/UI/UITips.h"

#include "../UIPopupToggleButtonWidget.h"

namespace IED
{
	class Controller;

	namespace UI
	{
		class UIFormSelectorWidget :
			public virtual UITipsInterface,
			public virtual UIPopupToggleButtonWidget,
			public virtual UILocalizationInterface
		{
			using on_open_func_t = std::function<void(UIFormSelectorWidget&, UIFormBrowser&)>;

		public:
			UIFormSelectorWidget(
				Controller& a_controller,
				FormInfoFlags a_requiredFlags,
				bool a_restrictTypes = false,
				bool a_enableFormBrowser = true,
				bool a_forceBase = false);

			bool DrawFormSelector(
				const char* a_label,
				Game::FormID& a_form,
				const char* a_tipText = nullptr);

			void Reset();

			void SetOnOpenFunc(on_open_func_t a_func);
			void SetAllowedTypes(std::initializer_list<UIFormBrowser::tab_filter_type::value_type> a_types);
			void SetAllowedTypes(const std::shared_ptr<const UIFormBrowser::tab_filter_type>& a_types);
			//void SetAllowedTypes(const UIFormBrowser::tab_filter_type& a_types);

			inline constexpr const auto& GetAllowedTypes() const noexcept
			{
				return m_types;
			}

			bool HasType(const formInfo_t& a_info) const;

			inline const auto& GetInfo() const noexcept
			{
				return m_state->m_currentInfo;
			}

			bool IsEntryValid(const IFormDatabase::entry_t& a_entry) const;

			bool IsCurrentValid() const noexcept;

		private:
			void DrawInfo(Game::FormID a_form);
			void QueueLookup(Game::FormID);
			void QueueGetCrosshairRef();
			Game::FormID GetFormIDFromInputBuffer();
			bool GetInputBufferChanged();
			void ErrorMessage(const char* a_text);

			inline constexpr auto& GetCurrentFormInfo() const noexcept
			{
				return !m_forceBase ?
                           m_state->m_currentInfo->form :
                           m_state->m_currentInfo->get_base();
			}

			struct state_t
			{
				std::unique_ptr<formInfoResult_t> m_currentInfo;
				Game::FormID m_bufferedFormID;
				Game::FormID m_lastInputFormID;

				char m_inputBuffer[9]{ 0 };
				char m_lastInputBuffer[9]{ 0 };
			};

			static void SetInputFormID(
				const std::shared_ptr<state_t>& a_state,
				Game::FormID a_form);

			bool m_nextGrabKeyboardFocus{ false };
			bool m_restrictTypes{ false };
			bool m_enableFormBrowser{ true };
			bool m_forceBase{ false };

			FormInfoFlags m_requiredFlags{ FormInfoFlags::kNone };
			std::shared_ptr<const UIFormBrowser::tab_filter_type> m_types;

			std::shared_ptr<state_t> m_state;

			Controller& m_controller;

		protected:
			on_open_func_t m_onOpenFunc;
		};

	}  // namespace UI
}  // namespace IED