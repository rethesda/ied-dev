#pragma once

namespace IED
{
	class Controller;
	class AnimationGraphManagerHolderList;

	class AnimationUpdateManager
	{
		using entry_type = std::pair<
			Game::FormID::held_type,
			std::shared_ptr<AnimationGraphManagerHolderList>>;

	public:
		void PrepareAnimationUpdateList(
			Controller* a_controller);

		void ClearAnimationUpdateList();

		void UpdateActorAnimations(
			Actor*                       a_actor,
			const BSAnimationUpdateData& a_data);

		static void UpdatePlayerAnimations(
			Actor*                       a_actor,
			const BSAnimationUpdateData& a_data,
			Controller*                  a_controller);

	private:
		static std::shared_ptr<AnimationGraphManagerHolderList> GetActorAnimationUpdateList(
			Actor*      a_actor,
			Controller* a_controller);

		std::vector<entry_type> m_data;

		std::atomic<bool>   m_running{ false };
		stl::fast_spin_lock m_lock;
	};
}