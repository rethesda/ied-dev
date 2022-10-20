#pragma once

#include "IED/ConfigCommon.h"
#include "IED/TimeOfDay.h"

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
		public TaskDelegateFixed,
		public IFirstPersonState
	{
		struct animUpdateData_t
		{
			Game::Unk2f6b948::Steps steps;
			//BSAnimationUpdateData   data;
		};

	public:
		ActorProcessorTask(
			Controller& a_controller);

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

		[[nodiscard]] inline constexpr void SetProcessorTaskRunAUState(bool a_state) noexcept
		{
			m_runAnimationUpdates = a_state;
		}

	private:
		struct State
		{
			long long lastRun;

			RE::TESWeather*   currentWeather{ nullptr };
			Data::TimeOfDay   timeOfDay{ Data::TimeOfDay::kDay };
			bool              inFirstPerson{ false };
			Game::ActorHandle playerLastRidden;
#if defined(IED_ENABLE_CONDITION_EN)
			bool playerEnemiesNearby{ false };
#endif
		};

		inline static constexpr auto COMMON_STATE_CHECK_INTERVAL = 1000000ll;

		virtual void Run() override;

		SKMP_FORCEINLINE void UpdateNode(
			ActorObjectHolder& a_record,
			ObjectEntryBase&   a_entry);
		//const std::optional<animUpdateData_t>& a_animUpdateData);

		void ProcessTransformUpdateRequest(
			ActorObjectHolder& a_data);

		void ProcessEvalRequest(
			ActorObjectHolder& a_data);

		static constexpr bool CheckMonitorNodes(
			ActorObjectHolder& a_data) noexcept;

		void UpdateState();

		State m_state;

		PerfTimerInt m_timer{ 1000000LL };
		long long    m_currentTime{ 0LL };
		bool         m_run{ false };
		bool         m_runAnimationUpdates{ true };

		Controller& m_controller;
	};

}