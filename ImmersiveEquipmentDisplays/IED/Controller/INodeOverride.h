#pragma once

#include "CMENodeEntry.h"
#include "IED/CommonParams.h"
#include "IED/ConfigCommon.h"
#include "IED/FormCommon.h"
#include "IED/FormHolder.h"
#include "IED/Inventory.h"
#include "IED/NodeOverrideData.h"

namespace IED
{
	namespace Data
	{
		struct configNodeOverrideTransform_t;
	}

	class ActorObjectHolder;
	class Controller;
	struct WeaponNodeEntry;

	class INodeOverride
	{
	public:
		struct bipedInfoEntry_t
		{
			TESForm*     item;
			BIPED_OBJECT bip{ BIPED_OBJECT::kNone };
			float        weaponAdjust{ 0.0f };
		};

		struct nodeOverrideParams_t :
			CommonParams
		{
		public:
			template <class... Args>
			inline constexpr nodeOverrideParams_t(Args&&... a_args) noexcept :
				CommonParams(std::forward<Args>(a_args)...)
			{
			}

			inline nodeOverrideParams_t(
				CommonParams& a_cparams) noexcept :
				CommonParams(a_cparams)
			{
			}

			using item_container_type = stl::unordered_map<Game::FormID, bipedInfoEntry_t>;

			std::optional<float>                 weaponAdjust;
			std::optional<float>                 weightAdjust;
			std::unique_ptr<item_container_type> itemData;
			std::optional<bool>                  bipedHasArmor;
			std::uint64_t                        matchedSlotFlags{ 0 };

			auto get_biped_has_armor()
			{
				if (!bipedHasArmor)
				{
					bipedHasArmor = equipped_armor_visitor([](auto*) { return true; });
				}

				return *bipedHasArmor;
			}

			item_container_type& get_item_data();

			float get_weapon_adjust();

			inline constexpr float get_weight_adjust()
			{
				if (!weightAdjust)
				{
					weightAdjust = (actor->GetWeight() * 0.01f) * 0.5f;
				}

				return *weightAdjust;
			}

			inline constexpr void clear_matched_items() noexcept
			{
				matchedSlotFlags = 0;
			}

			inline constexpr void set_matched_item(BIPED_OBJECT a_object) noexcept
			{
				matchedSlotFlags |= 1ui64 << stl::underlying(a_object);
			}

			float get_matched_weapon_adjust() const noexcept
			{
				float result = 0.0f;

				if (itemData)
				{
					for (auto& e : *itemData)
					{
						if (matchedSlotFlags & (1ui64 << stl::underlying(e.second.bip)))
						{
							result += e.second.weaponAdjust;
						}
					}
				}

				return result;
			}

			template <class Tf>
			constexpr bool equipped_armor_visitor(
				Tf a_func)
			{
				auto bip = get_biped();
				if (!bip)
				{
					return false;
				}

				auto skin = get_actor_skin();

				using enum_type = std::underlying_type_t<BIPED_OBJECT>;

				for (enum_type i = stl::underlying(BIPED_OBJECT::kHead); i < stl::underlying(BIPED_OBJECT::kEditorTotal); i++)
				{
					if (is_av_ignored_slot(static_cast<BIPED_OBJECT>(i)))
					{
						continue;
					}

					auto& e = bip->objects[i];

					if (e.item &&
					    e.item != e.addon &&
					    e.item != skin)
					{
						if (auto armor = e.item->As<TESObjectARMO>())
						{
							if (a_func(armor) == true)
							{
								return true;
							}
						}
					}
				}

				return false;
			}

		private:
			inline constexpr bool is_av_ignored_slot(
				BIPED_OBJECT a_slot) noexcept
			{
				if (a_slot == get_shield_slot())
				{
					return true;
				}

				switch (a_slot)
				{
					// ??
				case BIPED_OBJECT::kDecapitateHead:
				case BIPED_OBJECT::kDecapitate:
				case BIPED_OBJECT::kFX01:
					return true;
				default:
					return false;
				}
			}
		};

		static void ResetNodeOverrideImpl(
			const CMENodeEntry::Node& a_node);

		static void ResetNodeOverride(
			const CMENodeEntry& a_entry,
			bool                a_defer);

		static void ResetNodePlacement(
			const WeaponNodeEntry& a_entry,
			nodeOverrideParams_t*  a_params,
			bool                   a_defer);

		static void ApplyNodeOverride(
			const stl::fixed_string&                   a_name,
			const CMENodeEntry&                        a_entry,
			const Data::configNodeOverrideTransform_t& a_data,
			nodeOverrideParams_t&                      a_params);

		static void ApplyNodeVisibility(
			const CMENodeEntry&                        a_entry,
			const Data::configNodeOverrideTransform_t& a_data,
			nodeOverrideParams_t&                      a_params);

		static void attach_node_to(
			const WeaponNodeEntry&   a_entry,
			const NiPointer<NiNode>& a_target,
			nodeOverrideParams_t*    a_params,
			WeaponPlacementID        a_placementID,
			bool                     a_defer);

		static void ApplyNodePlacement(
			const Data::configNodeOverridePlacement_t& a_data,
			const WeaponNodeEntry&                     a_entry,
			nodeOverrideParams_t&                      a_params);

		static const Data::configNodePhysicsValues_t& GetPhysicsConfig(
			const Data::configNodeOverridePhysics_t& a_data,
			nodeOverrideParams_t&                    a_params);

	private:
		static constexpr const stl::fixed_string& get_target_node(
			const Data::configNodeOverridePlacement_t& a_data,
			nodeOverrideParams_t&                      a_params);

		static bool process_offsets(
			const Data::configNodeOverrideOffsetList_t& a_data,
			NiTransform&                                a_out,
			NiPoint3&                                   a_posAccum,
			nodeOverrideParams_t&                       a_params);
	};

}