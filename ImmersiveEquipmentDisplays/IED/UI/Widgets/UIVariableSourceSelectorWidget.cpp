#include "pch.h"

#include "UIVariableSourceSelectorWidget.h"

namespace IED
{
	namespace UI
	{
		UIVariableSourceSelectorWidget::data_type UIVariableSourceSelectorWidget::m_data{ {

			{ Data::VariableSource::kActor, UIVariableSourceSelectorWidgetStrings::Actor },
			{ Data::VariableSource::kPlayerHorse, UIVariableSourceSelectorWidgetStrings::PlayerHorse },

		} };

		UIVariableSourceSelectorWidget::UIVariableSourceSelectorWidget(
			Localization::ILocalization& a_localization) :
			UILocalizationInterface(a_localization)
		{
		}

		bool UIVariableSourceSelectorWidget::DrawVariableSourceSelectorWidget(
			Data::VariableSource& a_type)
		{
			bool result = false;

			if (ImGui::BeginCombo(
					LS(CommonStrings::Source, "ex_vs_sel"),
					variable_source_to_desc(a_type),
					ImGuiComboFlags_HeightLarge))
			{
				for (auto& e : m_data)
				{
					ImGui::PushID(stl::underlying(e.first));

					bool selected = (e.first == a_type);
					if (selected)
					{
						if (ImGui::IsWindowAppearing())
							ImGui::SetScrollHereY();
					}

					if (ImGui::Selectable(
							LS<UIVariableSourceSelectorWidgetStrings, 3>(e.second, "1"),
							selected))
					{
						a_type = e.first;
						result = true;
					}

					ImGui::PopID();
				}

				ImGui::EndCombo();
			}

			return result;
		}

		const char* UIVariableSourceSelectorWidget::variable_source_to_desc(
			Data::VariableSource a_type) const
		{
			switch (a_type)
			{
			case Data::VariableSource::kActor:
				return LS(UIVariableSourceSelectorWidgetStrings::Actor);
			case Data::VariableSource::kPlayerHorse:
				return LS(UIVariableSourceSelectorWidgetStrings::PlayerHorse);
			default:
				return nullptr;
			}
		}
	}
}