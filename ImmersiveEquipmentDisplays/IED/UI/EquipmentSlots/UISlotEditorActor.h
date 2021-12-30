#pragma once

#include "IED/UI/UICommon.h"

#include "IED/UI/Widgets/Form/UIFormInfoTooltipWidget.h"
#include "IED/UI/Widgets/Lists/UIActorListWidget.h"
#include "IED/UI/Widgets/UIProfileSelectorWidget.h"

#include "UISlotEditorCommon.h"
#include "Widgets/UISlotEditorWidget.h"

#include "IED/GlobalProfileManager.h"

namespace IED
{
	class Controller;

	namespace UI
	{
		class UISlotEditorActor :
			// public UISlotEditorWidget<Game::FormID>,
			public UISlotEditorCommon<Game::FormID>,
			UIActorList<entrySlotData_t>,
			UIFormInfoTooltipWidget
		{
		public:
			UISlotEditorActor(Controller& a_controller);

			virtual void Initialize() override;
			virtual void Draw() override;

			virtual void OnOpen() override;
			virtual void OnClose() override;
			virtual void Reset() override;
			virtual void QueueUpdateCurrent() override;

		private:
			virtual constexpr Data::ConfigClass GetConfigClass() const override;

			virtual const ActorInfoHolder& GetActorInfoHolder() const override;

			std::uint64_t GetActorInfoUpdateID() const override;

			virtual Data::SettingHolder::EditorPanelActorSettings& GetActorSettings() const override;

			virtual SlotEditorCurrentData GetCurrentData() override;

			virtual Data::SettingHolder::EditorPanelCommon& GetEditorPanelSettings() override;
			virtual void OnEditorPanelSettingsChange() override;

			virtual void ListResetAllValues(
				Game::FormID a_handle) override;

			virtual entrySlotData_t GetData(
				Game::FormID a_formid) override;

			Data::configSlotHolder_t&
				GetOrCreateConfigSlotHolder(
					Game::FormID a_handle) const override;

			virtual void MergeProfile(
				profileSelectorParamsSlot_t<Game::FormID>& a_data,
				const SlotProfile& a_profile) override;

			virtual void OnBaseConfigChange(
				Game::FormID a_handle,
				const void* a_params,
				PostChangeAction a_action) override;

			virtual void OnFullConfigChange(
				Game::FormID a_handle,
				const SlotConfigUpdateParams& a_params) override;

			virtual void
				OnSingleSlotClear(Game::FormID a_handle, const void* a_params) override;

			virtual void OnFullConfigClear(Game::FormID a_handle) override;

			virtual void OnListChangeCurrentItem(
				const SetObjectWrapper<UIActorList<entrySlotData_t>::listValue_t>& a_oldHandle,
				const SetObjectWrapper<UIActorList<entrySlotData_t>::listValue_t>& a_newHandle) override;

			virtual void OnSexChanged(Data::ConfigSex a_newSex) override;

			virtual void OnListOptionsChange() override;

			virtual const SetObjectWrapper<Game::FormID>& GetCrosshairRef() override;

			virtual UIPopupQueue& GetPopupQueue() override;
			virtual UIPopupQueue& GetPopupQueue_ProfileBase() const override;

			virtual UIData::UICollapsibleStates& GetCollapsibleStatesData() override;
			virtual void OnCollapsibleStatesUpdate() override;

			virtual void DrawMenuBarItemsExtra() override;

			virtual void ListDrawExtraActorInfo(
				const listValue_t& a_entry) override;

			virtual bool DrawExtraSlotInfo(
				Game::FormID a_handle,
				Data::ObjectSlot a_slot,
				const Data::configStoreSlot_t::result_copy::result_entry& a_entry,
				bool a_infoDrawn) override;

			Controller& m_controller;
		};

	}  // namespace UI
}  // namespace IED