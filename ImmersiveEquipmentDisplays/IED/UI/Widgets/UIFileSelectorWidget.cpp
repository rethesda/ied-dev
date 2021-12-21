#include "pch.h"

#include "UIFileSelectorWidget.h"
#include "UIFileSelectorWidgetStrings.h"

namespace IED
{
	namespace UI
	{
		UIFileSelector::SelectedFile::SelectedFile(
			const fs::path& a_root,
			const storage_type::value_type& a_filename) :
			m_fullpath(a_root / a_filename.second),
			m_filename(a_filename.second),
			m_key(a_filename.first) {}

		void UIFileSelector::SelectedFile::UpdateInfo()
		{
			// m_infoResult = SKMP::DCBP::GetImportInfo(m_fullpath, m_info);
		}

		UIFileSelector::UIFileSelector(
			Localization::ILocalization& a_loc,
			const fs::path& a_root) :
			UILocalizationInterface(a_loc),
			m_root(a_root)
		{}

		void UIFileSelector::DrawFileSelector()
		{
			const char* curSelName = m_selected ? m_selected->m_key.c_str() : nullptr;

			ImGui::PushItemWidth(ImGui::GetFontSize() * -8.0f);

			if (ImGui::BeginCombo(
					LS(CommonStrings::Files, "file_sel"),
					curSelName,
					ImGuiComboFlags_HeightLarge))
			{
				for (auto& e : m_files)
				{
					ImGui::PushID(std::addressof(e));

					bool selected = m_selected->m_key == e.first;
					if (selected)
						if (ImGui::IsWindowAppearing())
							ImGui::SetScrollHereY();

					if (ImGui::Selectable(e.first.c_str(), selected))
					{
						m_selected = { m_root, e };
						m_selected->UpdateInfo();
					}

					ImGui::PopID();
				}

				ImGui::EndCombo();
			}

			ImGui::PopItemWidth();
		}

		bool UIFileSelector::UpdateFileList(bool a_createPath, bool a_select)
		{
			try
			{
				storage_type tmp;

				if (!fs::exists(m_root))
				{
					fs::create_directories(m_root);
				}

				if (fs::exists(m_root) && !fs::is_directory(m_root))
				{
					throw std::exception(LS(UIFileSelectorWidgetStrings::ErrorRootPathNotDir));
				}

				for (const auto& entry : fs::directory_iterator(m_root))
				{
					try
					{
						if (!entry.is_regular_file())
							continue;

						auto file = entry.path().filename();

						tmp.emplace(file.stem().string(), file);
					}
					catch (const std::exception& e)
					{
						gLog.Warning(
							"%s: exception while iterating directory: %s",
							__FUNCTION__,
							e.what());
					}
				}

				if (tmp.empty())
				{
					m_selected.clear();
				}
				else
				{
					m_files = std::move(tmp);

					if (a_select)
					{
						if (!m_selected || !m_files.contains(m_selected->m_key))
						{
							m_selected = { m_root, *m_files.begin() };
						}

						if (m_selected)
						{
							m_selected->UpdateInfo();
						}
					}
				}

				return true;
			}
			catch (const std::exception& e)
			{
				m_lastExcept = e;
				return false;
			}
		}

		bool UIFileSelector::Delete(const fs::path& a_file)
		{
			try
			{
				if (!fs::exists(a_file) || !fs::is_regular_file(a_file))
				{
					throw std::exception(LS(UIFileSelectorWidgetStrings::ErrorInvalidPath));
				}

				if (!fs::remove(a_file))
				{
					throw std::exception(LS(UIFileSelectorWidgetStrings::ErrorFileDelFailed));
				}

				return true;
			}
			catch (const std::exception& e)
			{
				m_lastExcept = e;
				return false;
			}
		}

		bool UIFileSelector::DeleteSelected()
		{
			if (m_selected)
			{
				return DeleteItem(*m_selected);
			}
			else
			{
				return false;
			}
		}

		bool UIFileSelector::DeleteItem(const SelectedFile& a_item)
		{
			if (!Delete(a_item.m_fullpath))
			{
				return false;
			}

			m_files.erase(a_item.m_key);

			if (m_files.empty())
			{
				m_selected.clear();
			}
			else
			{
				if (m_selected->m_key == a_item.m_key)
				{
					m_selected = { m_root, *m_files.begin() };
					m_selected->UpdateInfo();
				}
			}

			return true;
		}

		bool UIFileSelector::RenameItem(
			const SelectedFile& a_item,
			const fs::path& a_newFileName)
		{
			fs::path newFile(a_newFileName);

			try
			{
				fs::rename(a_item.m_fullpath, m_root / a_newFileName);
			}
			catch (const std::exception& e)
			{
				m_lastExcept = e;
				return false;
			}

			m_files.erase(a_item.m_key);
			auto& r = m_files.emplace(newFile.stem().string(), newFile);

			if (m_selected->m_key == a_item.m_key)
			{
				m_selected->m_fullpath = m_root / r.first->second;
				m_selected->m_filename = r.first->second;
				m_selected->m_key = r.first->first;
			}

			return true;
		}

		void UIFileSelector::SelectItem(const stl::fixed_string& a_itemDesc)
		{
			auto it = m_files.find(a_itemDesc);
			if (it != m_files.end())
			{
				m_selected = { m_root, *it };
				m_selected->UpdateInfo();
			}
		}
	}
}