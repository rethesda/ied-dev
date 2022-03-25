#include "pch.h"

#include "IUI.h"

#include "IED/UI/UIFormBrowser.h"
#include "IED/UI/UIFormInfoCache.h"
#include "IED/UI/UIMain.h"

#include "Drivers/UI.h"

namespace IED
{
	void IUI::UIInitialize(Controller& a_controller)
	{
		m_task = std::make_unique<IUIRenderTaskMain>(*this);
		m_task->InitializeContext<UI::UIMain>(a_controller);
	}

	bool IUI::UIIsInitialized() const noexcept
	{
		return m_task.get() != nullptr;
	}

	UI::UIPopupQueue& IUI::UIGetPopupQueue() noexcept
	{
		return m_task->GetContext<UI::UIMain>()->GetPopupQueue();
	}

	UI::UIFormBrowser& IUI::UIGetFormBrowser() noexcept
	{
		return m_task->GetContext<UI::UIMain>()->GetFormBrowser();
	}

	UI::UIFormInfoCache& IUI::UIGetFormLookupCache() noexcept
	{
		return m_task->GetContext<UI::UIMain>()->GetFormLookupCache();
	}

	void IUI::UIReset()
	{
		m_task->QueueReset();
	}

	auto IUI::UIToggle() -> UIOpenResult
	{
		IScopedLock lock(UIGetLock());

		if (!m_task || !m_safeToOpenUI)
		{
			return UIOpenResult::kResultNone;
		}

		if (m_task->IsRunning())
		{
			m_task->GetContext<UI::UIMain>()->SetOpenState(false);

			return UIOpenResult::kResultDisabled;
		}
		else
		{
			return UIOpenImpl();
		}
	}

	auto IUI::UIOpen() -> UIOpenResult
	{
		IScopedLock lock(UIGetLock());

		return UIOpenImpl();
	}

	auto IUI::UIOpenImpl() -> UIOpenResult
	{
		if (m_task &&
		    m_safeToOpenUI &&
		    m_task->RunEnableChecks())
		{
			if (Drivers::UI::AddTask(0, m_task))
			{
				return UIOpenResult::kResultEnabled;
			}
		}

		return UIOpenResult::kResultNone;
	}

	IUIRenderTask::IUIRenderTask(
		IUI& a_interface) :
		m_owner(a_interface)
	{
	}

	bool IUIRenderTask::Run()
	{
		IScopedLock lock(m_owner.UIGetLock());

		try
		{
			if (m_reset)
			{
				m_reset = false;
				m_context->Reset();
			}

			if (ShouldClose())
			{
				m_context->SetOpenState(false);
			}

			m_context->Draw();

			return m_context->IsWindowOpen();
		}
		catch (const std::exception& e)
		{
			HALT(e.what());
		}
	}

	void IUIRenderTask::OnTaskStop()
	{
		IScopedLock lock(m_owner.UIGetLock());

		try
		{
			m_context->SetOpenState(false);
			m_context->OnClose();
		}
		catch (const std::exception& e)
		{
			HALT(e.what());
		}
	}

	void IUIRenderTask::OnTaskStart()
	{
		IScopedLock lock(m_owner.UIGetLock());

		try
		{
			m_context->SetOpenState(true);
			m_context->OnOpen();
			OnStart();
		}
		catch (const std::exception& e)
		{
			HALT(e.what());
		}
	}

	bool IUIRenderTask::ShouldClose()
	{
		return false;
	}

	void IUIRenderTask::OnStart()
	{
	}

	void IUIRenderTaskMain::OnTaskStart()
	{
		IScopedLock lock(m_owner.UIGetLock());

		try
		{
			m_context->SetOpenState(true);
			m_owner.OnUIOpen();
			m_context->OnOpen();
			OnStart();
		}
		catch (const std::exception& e)
		{
			HALT(e.what());
		}
	}

	bool IUIRenderTaskMain::ShouldClose()
	{
		return GetContext<UI::UIMain>()->GetUISettings().closeOnESC &&
		           ImGui::GetIO().KeysDown[VK_ESCAPE] ||
		       (!GetEnabledInMenu() && Game::InPausedMenu());
	}

	
	IUITimedRenderTask::IUITimedRenderTask(
		IUI&      a_interface,
		long long a_lifetime) :
		IUIRenderTask(a_interface),
		m_lifetime(a_lifetime)
	{		
	}

	void IUITimedRenderTask::OnStart()
	{
		m_deadline = m_state.startTime + IPerfCounter::T(m_lifetime);
	}

	bool IUITimedRenderTask::ShouldClose()
	{
		return IPerfCounter::Query() >= m_deadline;
	}


}