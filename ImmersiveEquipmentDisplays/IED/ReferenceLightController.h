#pragma once

#include "Controller/ObjectLight.h"

namespace IED
{
	class ReferenceLightController :
		public BSTEventSink<TESCellAttachDetachEvent>
	{
		typedef bool (*updateRefrLight_t)(
			TESObjectLIGH*    a1,
			const REFR_LIGHT& a2,
			TESObjectREFR*    a3,
			float             a4) noexcept;

		typedef bool (*setNiPointLightAttenuation_t)(
			NiAVObject*  a_object,
			std::int32_t a_radius) noexcept;

		typedef bool (*shadowSceneNodeCleanupLights_t)(
			RE::ShadowSceneNode* a_ssn,
			NiNode*              a_node,
			bool                 a_lightNodeInvisibilitySwitch,
			bool                 a_remove) noexcept;

		typedef bool (*shadowSceneNodeCleanupCellMoveReAddLight_t)(
			RE::ShadowSceneNode* a_ssn,
			NiLight*             a2,
			bool                 a3) noexcept;

		typedef bool (*refreshLightOnSceneMove_t)(
			RE::ShadowSceneNode* a_ssn,
			RE::BSLight*         a_light) noexcept;

		typedef bool (*shadowSceneNode_UnkQueueBSLight_t)(
			RE::ShadowSceneNode* a_ssn,
			NiLight*             a_light) noexcept;

		struct Entry
		{
			Entry(
				TESObjectLIGH* a_form,
				NiPointLight*  a_light,
				RE::BSLight*   a_bsLight) :
				form(a_form),
				data{ a_light, -1.0f },
				bsLight(a_bsLight)
			{
			}

			TESObjectLIGH*         form;
			REFR_LIGHT             data;
			NiPointer<RE::BSLight> bsLight;
		};

		using lock_type   = std::shared_mutex;
		using shared_lock = std::shared_lock<lock_type>;
		using unique_lock = std::unique_lock<lock_type>;

	public:
		[[nodiscard]] static inline constexpr auto& GetSingleton() noexcept
		{
			return m_Instance;
		}

		void Initialize();

		void OnUpdatePlayerLight(PlayerCharacter* a_actor) const noexcept;
		void OnActorCrossCellBoundary(Actor* a_actor) const noexcept;
		void OnActorCellAttached(Actor* a_actor) const noexcept;
		void OnRefreshLightOnSceneMove(Actor* a_actor) const noexcept;
		void OnActorUpdate(Actor* a_actor, REFR_LIGHT* a_extraLight) const noexcept;

		void AddLight(
			Game::FormID       a_actor,
			TESObjectLIGH*     a_form,
			const ObjectLight& a_light) noexcept;

		void RemoveLight(
			Game::FormID  a_actor,
			NiPointLight* a_light) noexcept;

		void RemoveActor(Game::FormID a_actor) noexcept;

		[[nodiscard]] static ObjectLight CreateAndAttachPointLight(
			const TESObjectLIGH* a_lightForm,
			Actor*               a_actor,
			NiNode*              a_object) noexcept;

		static void CleanupLights(NiNode* a_node) noexcept;

		std::size_t GetNumLights() const noexcept;

		[[nodiscard]] inline constexpr bool GetEnabled() const noexcept
		{
			return m_initialized;
		}

		inline void SetNPCLightCellAttachFixEnabled(bool a_switch) noexcept
		{
			m_fixVanillaLightOnCellAttach.store(a_switch, std::memory_order_relaxed);
		}

		inline void SetNPCLightUpdateFixEnabled(bool a_switch) noexcept
		{
			m_fixVanillaNPCLightUpdates.store(a_switch, std::memory_order_relaxed);
		}
		
		inline void SetNPCLightUpdatesEnabled(bool a_switch) noexcept
		{
			m_npcLightUpdates.store(a_switch, std::memory_order_relaxed);
		}

	private:

		template <class Tf>
		constexpr void visit_lights(Actor* a_actor, Tf a_func) const noexcept
		{
			auto it = m_data.find(a_actor->formID);
			if (it != m_data.end())
			{
				for (auto& e : it->second)
				{
					a_func(e);
				}
			}
		}

		static void ReAddActorExtraLight(Actor* a_actor) noexcept;

		virtual EventResult ReceiveEvent(
			const TESCellAttachDetachEvent*           a_evn,
			BSTEventSource<TESCellAttachDetachEvent>* a_dispatcher) override;

		inline static const auto UpdateRefrLight            = IAL::Address<updateRefrLight_t>(17212, 17614);
		inline static const auto NiPointLightSetAttenuation = IAL::Address<setNiPointLightAttenuation_t>(17224, 17626);
		inline static const auto QueueRemoveAllLights       = IAL::Address<shadowSceneNodeCleanupLights_t>(99732, 106376);
		inline static const auto QueueAddLight              = IAL::Address<refreshLightOnSceneMove_t>(99693, 106327);
		inline static const auto UnkQueueBSLight            = IAL::Address<shadowSceneNode_UnkQueueBSLight_t>(99706, 106340);

		mutable lock_type                                          m_lock;
		stl::unordered_map<Game::FormID, std::forward_list<Entry>> m_data;

		bool             m_initialized{ false };
		std::atomic_bool m_fixVanillaLightOnCellAttach{ false };
		std::atomic_bool m_fixVanillaNPCLightUpdates{ false };
		std::atomic_bool m_npcLightUpdates{ false };

		static ReferenceLightController m_Instance;
	};
}