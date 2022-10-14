#include "pch.h"

#include "ObjectEntryBase.h"

#include "IED/EngineExtensions.h"

namespace IED
{
	void ObjectEntryBase::reset(
		Game::ObjectRefHandle  a_handle,
		NiPointer<NiNode>&     a_root,
		NiPointer<NiAVObject>& a_root1p)
	{
		if (state)
		{
			for (auto& e : state->dbEntries)
			{
				e->accessed = IPerfCounter::Query();
			}

			if (EngineExtensions::SceneRendering() ||
			    !ITaskPool::IsRunningOnCurrentThread())
			{
				struct DisposeStateTask :
					public TaskDelegate
				{
				public:
					DisposeStateTask(
						std::unique_ptr<State>&& a_state,
						Game::ObjectRefHandle    a_handle,
						NiPointer<NiNode>&       a_root,
						NiPointer<NiAVObject>&   a_root1p) :
						m_state(std::move(a_state)),
						m_handle(a_handle),
						m_root(a_root),
						m_root1p(a_root1p)
					{
					}

					virtual void Run() override
					{
						m_state->Cleanup(m_handle);

						m_state.reset();
					}

					virtual void Dispose() override
					{
						delete this;
					}

				private:
					std::unique_ptr<State> m_state;
					Game::ObjectRefHandle  m_handle;
					NiPointer<NiNode>      m_root;
					NiPointer<NiAVObject>  m_root1p;
				};

				ITaskPool::AddPriorityTask<DisposeStateTask>(
					std::move(state),
					a_handle,
					a_root,
					a_root1p);
			}
			else
			{
				state->Cleanup(a_handle);
				state.reset();
			}
		}

		if (effectShaderData)
		{
			if (EngineExtensions::SceneRendering() ||
			    !ITaskPool::IsRunningOnCurrentThread())
			{
				struct DisposeEffectDataTask :
					public TaskDelegate
				{
				public:
					DisposeEffectDataTask(
						std::unique_ptr<EffectShaderData>&& a_data,
						NiPointer<NiNode>&                  a_root,
						NiPointer<NiAVObject>&              a_root1p) :
						m_data(std::move(a_data)),
						m_root(a_root),
						m_root1p(a_root1p)
					{
					}

					virtual void Run() override
					{
						m_data->ClearEffectShaderDataFromTree(m_root);
						m_data->ClearEffectShaderDataFromTree(m_root1p);

						m_data.reset();
					}

					virtual void Dispose() override
					{
						delete this;
					}

				private:
					std::unique_ptr<EffectShaderData> m_data;
					NiPointer<NiNode>                 m_root;
					NiPointer<NiAVObject>             m_root1p;
				};

				ITaskPool::AddPriorityTask<DisposeEffectDataTask>(
					std::move(effectShaderData),
					a_root,
					a_root1p);
			}
			else
			{
				effectShaderData->ClearEffectShaderDataFromTree(a_root);
				effectShaderData->ClearEffectShaderDataFromTree(a_root1p);

				effectShaderData.reset();
			}
		}
	}

	void ObjectEntryBase::State::UpdateAnimationGraphs(
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

	void ObjectEntryBase::State::Cleanup(
		Game::ObjectRefHandle a_handle)
	{
		for (auto& e : groupObjects)
		{
			EngineExtensions::CleanupObjectImpl(
				a_handle,
				e.second.rootNode);

			if (e.second.weapAnimGraphManagerHolder)
			{
				EngineExtensions::CleanupWeaponBehaviorGraph(
					e.second.weapAnimGraphManagerHolder);
			}
		}

		EngineExtensions::CleanupObjectImpl(
			a_handle,
			nodes.rootNode);

		if (weapAnimGraphManagerHolder)
		{
			EngineExtensions::CleanupWeaponBehaviorGraph(
				weapAnimGraphManagerHolder);
		}
	}

	void ObjectEntryBase::State::GroupObject::PlayAnimation(
		Actor*                   a_actor,
		const stl::fixed_string& a_sequence)
	{
		if (a_sequence.empty())
		{
			return;
		}

		if (!object)
		{
			return;
		}

		if (auto controller = object->GetControllers())
		{
			if (auto manager = controller->AsNiControllerManager())
			{
				if (auto nseq = manager->GetSequenceByName(a_sequence.c_str()))
				{
					a_actor->PlayAnimation(
						manager,
						nseq,
						nseq);
				}
			}
		}
	}

	void ObjectEntryBase::State::UpdateAndPlayAnimation(
		Actor*                   a_actor,
		const stl::fixed_string& a_sequence)
	{
		if (a_sequence.empty())
		{
			return;
		}

		if (a_sequence == currentSequence)
		{
			return;
		}

		if (!nodes.object)
		{
			return;
		}

		if (auto controller = nodes.object->GetControllers())
		{
			if (auto manager = controller->AsNiControllerManager())
			{
				if (auto nseq = manager->GetSequenceByName(a_sequence.c_str()))
				{
					auto cseq = !currentSequence.empty() ?
					                manager->GetSequenceByName(currentSequence.c_str()) :
                                    nullptr;

					a_actor->PlayAnimation(
						manager,
						nseq,
						cseq ? cseq : nseq);

					currentSequence = a_sequence;
				}
			}
		}
	}

	void ObjectEntryBase::AnimationState::UpdateAndSendAnimationEvent(
		const stl::fixed_string& a_event)
	{
		assert(weapAnimGraphManagerHolder != nullptr);

		if (a_event.empty())
		{
			return;
		}

		if (a_event != currentAnimationEvent)
		{
			currentAnimationEvent = a_event;
			weapAnimGraphManagerHolder->NotifyAnimationGraph(a_event.c_str());
		}
	}

}