#include "pch.h"

#include "ConfigConditionsCommon.h"

namespace IED
{
	namespace Conditions
	{
		bool match_extra(
			CommonParams& a_params,
			Data::ExtraConditionType a_type)
		{
			switch (a_type)
			{
			case Data::ExtraConditionType::kCanDualWield:
				return a_params.can_dual_wield();
			case Data::ExtraConditionType::kIsDead:
				return a_params.get_actor_dead();
			case Data::ExtraConditionType::kInInterior:
				return a_params.get_in_interior();
			case Data::ExtraConditionType::kIsPlayerTeammate:
				return a_params.is_player_teammate();
			case Data::ExtraConditionType::kIsGuard:
				return a_params.actor->IsGuard();
			case Data::ExtraConditionType::kIsMount:
				return a_params.actor->IsMount();
			default:
				return false;
			}
		}

		bool match_form(
			Game::FormID a_formid,
			TESForm* a_form)
		{
			return a_formid && a_form->formID == a_formid;
		}

		bool is_in_location(
			BGSLocation* a_current,
			BGSLocation* a_loc)
		{
			for (auto it = a_current; it; it = it->parentLoc)
			{
				if (a_loc == it)
				{
					return true;
				}
			}

			return false;
		}

		bool is_in_location(
			BGSLocation* a_current,
			BGSKeyword* a_keyword,
			BGSLocation* a_matched)
		{
			for (auto it = a_current; it; it = it->parentLoc)
			{
				if (it->keyword.HasKeyword(a_keyword))
				{
					return true;
				}

				if (it == a_matched)
				{
					break;
				}
			}

			return false;
		}

		bool is_in_location(
			BGSLocation* a_current,
			BGSKeyword* a_keyword)
		{
			for (auto it = a_current; it; it = it->parentLoc)
			{
				if (it->keyword.HasKeyword(a_keyword))
				{
					return true;
				}
			}

			return false;
		}

		bool match_worldspace(
			TESWorldSpace* a_current,
			Game::FormID a_wsId,
			bool a_matchParent)
		{
			if (a_current->formID == a_wsId)
			{
				return true;
			}

			if (a_matchParent)
			{
				if (auto parent = a_current->GetParentWorldspace())
				{
					return parent->formID == a_wsId;
				}
			}

			return false;
		}
	}
}