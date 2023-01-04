#pragma once

#include "IED/ConfigCommon.h"
#include "IED/TimeOfDay.h"

#include "EffectController.h"
#include "IFirstPersonState.h"

namespace RE
{
	class TESWeather;
}

namespace IED
{
	/*enum class TransformUpdateFlags : std::uint32_t
	{
		kSkipNext = 1u << 0
	};

	DEFINE_ENUM_CLASS_BITWISE(TransformUpdateFlags);*/

	class Controller;
	class ActorObjectHolder;

	struct ObjectEntryBase;

	class ActorProcessorTask :
		public EffectController,
		public IFirstPersonState,
		public TaskDelegateFixed
	{
		struct animUpdateData_t
		{
			Game::Unk2f6b948::Steps steps;
			//BSAnimationUpdateData   data;
		};

	public:
		ActorProcessorTask();

		[[nodiscard]] inline constexpr auto NodeProcessorGetTime() const noexcept
		{
			return m_currentTime;
		}

#if defined(IED_ENABLE_CONDITION_EN)
		[[nodiscard]] inline constexpr auto PlayerHasEnemiesNearby() const noexcept
		{
			return m_state.playerEnemiesNearby;
		}
#endif

		[[nodiscard]] inline constexpr void SetProcessorTaskRunState(bool a_state) noexcept
		{
			m_run = a_state;
		}

		[[nodiscard]] void SetProcessorTaskRunAUState(bool a_state) noexcept;

		inline constexpr void SetProcessorTaskParallelUpdates(bool a_switch) noexcept
		{
			m_parallelProcessing = a_switch;
		}

	private:
		struct GlobalState
		{
			long long nextRun;

			RE::TESWeather*   currentWeather{ nullptr };
			Data::TimeOfDay   timeOfDay{ Data::TimeOfDay::kDay };
			bool              inFirstPerson{ false };
			Game::ActorHandle playerLastRidden;
#if defined(IED_ENABLE_CONDITION_EN)
			bool playerEnemiesNearby{ false };
#endif
		};

		inline static constexpr auto COMMON_STATE_CHECK_INTERVAL = 1000000ll;

		[[nodiscard]] inline constexpr bool ParallelProcessingEnabled() const noexcept
		{
			return m_parallelProcessing;
		}

		virtual void Run() noexcept override;

		SKMP_FORCEINLINE Controller& GetController() noexcept;

		bool SyncRefParentNode(
			ActorObjectHolder& a_record,
			ObjectEntryBase&   a_entry) noexcept;

		SKMP_FORCEINLINE void DoObjectRefSync(
			ActorObjectHolder& a_record,
			ObjectEntryBase&   a_entry) noexcept;

		SKMP_FORCEINLINE void DoObjectRefSyncMTSafe(
			ActorObjectHolder& a_record,
			ObjectEntryBase&   a_entry) noexcept;

		//const std::optional<animUpdateData_t>& a_animUpdateData);

		void ProcessTransformUpdateRequest(
			ActorObjectHolder& a_data) noexcept;

		void ProcessEvalRequest(
			ActorObjectHolder& a_data) noexcept;

		void UpdateGlobalState() noexcept;

		template <bool _Mt>
		void UpdateHolderMTSafe(
			const float                          a_interval,
			const Game::Unk2f6b948::Steps&       a_stepMuls,
			const std::optional<PhysicsUpdateData>& a_physUpdData,
			ActorObjectHolder&                   a_holder,
			bool                                 a_updateEffects) noexcept;

		void RunPreUpdates(
			const Game::Unk2f6b948::Steps& a_stepMuls) noexcept;

		GlobalState m_globalState;

		PerfTimerInt m_timer{ 1000000LL };
		long long    m_currentTime{ 0LL };
		bool         m_run{ false };
		bool         m_runAnimationUpdates{ false };
		bool         m_parallelProcessing{ false };

		stl::fast_spin_lock                                          m_syncRefParentQueueWRLock;
		stl::vector<std::pair<ActorObjectHolder*, ObjectEntryBase*>> m_syncRefParentQueue;
		/*stl::fast_spin_lock                                          m_unloadQueueWRLock;
		stl::vector<std::pair<ActorObjectHolder*, ObjectEntryBase*>> m_unloadQueue;*/
	};

}