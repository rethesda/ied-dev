#pragma once

#include "EffectShaderData.h"
#include "INode.h"
#include "ObjectDatabase.h"
#include "ObjectManagerCommon.h"

#include "IED/ConfigBaseValues.h"
#include "IED/EngineExtensions.h"
#include "IED/Physics/SimComponent.h"

namespace IED
{

	enum class ObjectEntryFlags : std::uint32_t
	{
		kNone = 0,

		kRefSyncDisableFailedOrphan = 1u << 1,
		kScbLeft                    = 1u << 2,
		kSyncReferenceTransform     = 1u << 6,
		kPlaySound                  = 1u << 8,
		kIsGroup                    = 1u << 9,
		kInvisible                  = 1u << 24,
	};

	DEFINE_ENUM_CLASS_BITWISE(ObjectEntryFlags);

	enum class GroupObjectEntryFlags : std::uint32_t
	{
		kNone = 0,

		kHasLight = 1u << 1,
	};

	DEFINE_ENUM_CLASS_BITWISE(GroupObjectEntryFlags);

	class IObjectManager;

	struct ObjectEntryBase
	{
		ObjectEntryBase()  = default;
		~ObjectEntryBase() = default;

		ObjectEntryBase(const ObjectEntryBase&) = delete;
		ObjectEntryBase(ObjectEntryBase&&)      = delete;
		ObjectEntryBase& operator=(const ObjectEntryBase&) = delete;
		ObjectEntryBase& operator=(ObjectEntryBase&&) = delete;

		bool reset(
			Game::ObjectRefHandle    a_handle,
			const NiPointer<NiNode>& a_root,
			const NiPointer<NiNode>& a_root1p,
			IObjectManager&          a_db,
			bool                     a_defer);

		bool SetObjectVisible(const bool a_switch) const noexcept;
		bool DeferredHideObject(const std::uint8_t a_delay) const noexcept;
		void ResetDeferredHide() const noexcept;

		SKMP_FORCEINLINE auto IsNodeVisible() const
		{
			return data.state && !data.state->flags.test(ObjectEntryFlags::kInvisible);
		}

		SKMP_FORCEINLINE auto IsActive() const
		{
			return IsNodeVisible();
		}

		SKMP_FORCEINLINE auto GetFormIfActive() const
		{
			return IsActive() ? data.state->form : nullptr;
		}

		struct AnimationState
		{
			RE::WeaponAnimationGraphManagerHolderPtr weapAnimGraphManagerHolder;
			stl::fixed_string                        currentAnimationEvent;

			void UpdateAndSendAnimationEvent(const stl::fixed_string& a_event);
		};

		struct State :
			AnimationState
		{
			State() noexcept = default;
			~State()         = default;

			State(const State&) = delete;
			State(State&&)      = delete;
			State& operator=(const State&) = delete;
			State& operator=(State&&) = delete;

			struct GroupObject :
				AnimationState
			{
				GroupObject(
					NiNode*            a_rootNode,
					NiPointer<NiNode>& a_object) :
					rootNode(a_rootNode),
					object(a_object)
				{
				}

				NiPointer<NiNode>                   rootNode;
				NiPointer<NiNode>                   object;
				Data::cacheTransform_t              transform;
				ObjectDatabase::ObjectDatabaseEntry dbEntry;
				//NiPointer<NiPointLight>             light;

				void PlayAnimation(Actor* a_actor, const stl::fixed_string& a_sequence);
			};

			void UpdateData(
				const Data::configBaseValues_t& a_in)
			{
				UpdateFlags(a_in);
				transform.Update(a_in);

				resetTriggerFlags = a_in.flags & Data::BaseFlags::kResetTriggerFlags;
			}

			void UpdateFlags(
				const Data::configBaseValues_t& a_in) noexcept
			{
				// gross but efficient

				static_assert(
					std::is_same_v<std::underlying_type_t<ObjectEntryFlags>, std::underlying_type_t<Data::BaseFlags>> &&
					stl::underlying(ObjectEntryFlags::kPlaySound) == stl::underlying(Data::BaseFlags::kPlaySound) &&
					stl::underlying(ObjectEntryFlags::kSyncReferenceTransform) == stl::underlying(Data::BaseFlags::kSyncReferenceTransform));

				flags =
					(flags & ~(ObjectEntryFlags::kPlaySound | ObjectEntryFlags::kSyncReferenceTransform | ObjectEntryFlags::kRefSyncDisableFailedOrphan)) |
					static_cast<ObjectEntryFlags>((a_in.flags & (Data::BaseFlags::kPlaySound | Data::BaseFlags::kSyncReferenceTransform)));
			}

			SKMP_FORCEINLINE void UpdateAnimationGraphs(
				const BSAnimationUpdateData& a_data)
			{
				for (auto& e : groupObjects)
				{
					if (e.second.weapAnimGraphManagerHolder)
					{
						EngineExtensions::UpdateAnimationGraph(
							e.second.weapAnimGraphManagerHolder.get(),
							a_data);
					}
				}

				if (weapAnimGraphManagerHolder)
				{
					EngineExtensions::UpdateAnimationGraph(
						weapAnimGraphManagerHolder.get(),
						a_data);
				}
			}

			/*void UpdateGroupTransforms(const Data::configModelGroup_t& a_group)
			{
				for (auto& e : a_group.entries)
				{
					if (auto it = groupObjects.find(e.first);
					    it != groupObjects.end())
					{
						it->second.transform.Update(e.second.transform);
					}
				}
			}*/

			void Cleanup(Game::ObjectRefHandle a_handle);

			void UpdateAndPlayAnimation(
				Actor*                   a_actor,
				const stl::fixed_string& a_sequence);

			void SetVisible(bool a_switch);

			template <class Tf>
			void visit_db_entries(Tf a_func)
			{
				if (auto& d = dbEntry)
				{
					a_func(d);
				}

				for (auto& e : groupObjects)
				{
					if (auto& d = e.second.dbEntry)
					{
						a_func(d);
					}
				}
			}

			TESForm*                                           form{ nullptr };
			Game::FormID                                       formid;
			Game::FormID                                       modelForm;
			stl::flag<ObjectEntryFlags>                        flags{ ObjectEntryFlags::kNone };
			stl::flag<Data::BaseFlags>                         resetTriggerFlags{ Data::BaseFlags::kNone };
			Data::NodeDescriptor                               nodeDesc;
			nodesRef_t                                         nodes;
			Data::cacheTransform_t                             transform;
			ObjectDatabase::ObjectDatabaseEntry                dbEntry;
			stl::unordered_map<stl::fixed_string, GroupObject> groupObjects;
			std::shared_ptr<PHYSimComponent>                   simComponent;
			stl::fixed_string                                  currentSequence;
			long long                                          created{ 0 };
			std::optional<luid_tag>                            currentGeomTransformTag;
			//NiPointer<NiPointLight>                            light;
			Game::FormID                                       owner;
			std::uint8_t                                       hideCountdown{ 0 };
			bool                                               atmReference{ true };
		};

		struct ActiveData
		{
			//#if defined(DEBUG)
			//
			//			ActiveData()             = default;
			//			ActiveData(ActiveData&&) = default;
			//
			//			~ActiveData()
			//			{
			//				assert(!state);
			//				assert(!effectShaderData);
			//			}
			//
			//#endif

			void Cleanup(
				Game::ObjectRefHandle    a_handle,
				const NiPointer<NiNode>& a_root,
				const NiPointer<NiNode>& a_root1p,
				ObjectDatabase&          a_db);

			[[nodiscard]] inline explicit operator bool() const noexcept
			{
				return state || effectShaderData;
			}

			std::unique_ptr<State>            state;
			std::unique_ptr<EffectShaderData> effectShaderData;
		};

		ActiveData data;
	};

}