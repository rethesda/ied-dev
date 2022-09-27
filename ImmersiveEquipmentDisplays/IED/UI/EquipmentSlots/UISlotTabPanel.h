#pragma once

#include "IED/ConfigCommon.h"

#include "UISlotEditorActor.h"
#include "UISlotEditorGlobal.h"
#include "UISlotEditorNPC.h"
#include "UISlotEditorRace.h"

#include "IED/UI/UIEditorTabPanel.h"

namespace IED
{
	class Controller;

	namespace UI
	{
		class UISlotEditorTabPanel :
			public UIEditorTabPanel
		{
		public:
			inline static constexpr auto PANEL_ID = UIEditorPanel::Slot; 

			UISlotEditorTabPanel(Controller& a_controller);

			UISlotEditorActor  m_slotEditorActor;
			UISlotEditorRace   m_slotEditorRace;
			UISlotEditorGlobal m_slotEditorGlobal;
			UISlotEditorNPC    m_slotEditorNPC;

		private:
			virtual Data::SettingHolder::EditorPanel& GetEditorConfig() override;

			Controller& m_controller;
		};

	}
}