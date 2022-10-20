#include "pch.h"

#include "Controller.h"
#include "IED/EngineExtensions.h"
#include "IObjectManager.h"

#include <ext/Model.h>
#include <ext/Node.h>

namespace IED
{
	using namespace ::Util::Node;

	bool IObjectManager::RemoveObject(
		TESObjectREFR*                   a_actor,
		Game::ObjectRefHandle            a_handle,
		ObjectEntryBase&                 a_objectEntry,
		ActorObjectHolder&               a_data,
		stl::flag<ControllerUpdateFlags> a_flags)
	{
		if (auto& state = a_objectEntry.state)
		{
			if (
				m_playSound &&
				a_flags.test(ControllerUpdateFlags::kPlaySound) &&
				state->flags.test(ObjectEntryFlags::kPlaySound) &&
				a_actor &&
				a_actor->loadedState &&
				(a_actor == *g_thePlayer || m_playSoundNPC) &&
				state->nodes.rootNode->m_parent &&
				state->nodes.rootNode->IsVisible())
			{
				SoundPlay(
					state->form->formType,
					state->nodes.rootNode->m_parent,
					false);
			}

			if (!a_objectEntry.state->dbEntries.empty())
			{
				QueueDatabaseCleanup();
			}
		}

		/*if (a_objectEntry.state->weapAnimGraphManagerHolder)
		{
			a_data.UnregisterWeaponAnimationGraphManagerHolder(
				a_objectEntry.state->weapAnimGraphManagerHolder);
		}

		for (auto& e : a_objectEntry.state->groupObjects)
		{
			if (e.second.weapAnimGraphManagerHolder)
			{
				a_data.UnregisterWeaponAnimationGraphManagerHolder(
					e.second.weapAnimGraphManagerHolder);
			}
		}*/

		return a_objectEntry.reset(
			a_handle,
			a_data.m_root,
			a_data.m_root1p);
	}

	bool IObjectManager::RemoveActorImpl(
		TESObjectREFR*                   a_actor,
		Game::ObjectRefHandle            a_handle,
		stl::flag<ControllerUpdateFlags> a_flags)
	{
		auto it = m_objects.find(a_actor->formID);
		if (it == m_objects.end())
		{
			return false;
		}

		CleanupActorObjectsImpl(
			a_actor,
			a_handle,
			it->second,
			a_flags);

		EraseActor(it);

		return true;
	}

	bool IObjectManager::RemoveActorImpl(
		TESObjectREFR*                   a_actor,
		stl::flag<ControllerUpdateFlags> a_flags)
	{
		auto it = m_objects.find(a_actor->formID);
		if (it == m_objects.end())
		{
			return false;
		}

		auto handle = a_actor->GetHandle();

		CleanupActorObjectsImpl(
			a_actor,
			handle,
			it->second,
			a_flags);

		EraseActor(it);

		return true;
	}

	bool IObjectManager::RemoveActorImpl(
		Game::FormID                     a_actor,
		stl::flag<ControllerUpdateFlags> a_flags)
	{
		auto it = m_objects.find(a_actor);
		if (it == m_objects.end())
		{
			return false;
		}

		auto handle = it->second.GetHandle();

		NiPointer<TESObjectREFR> ref;
		(void)handle.LookupZH(ref);

		CleanupActorObjectsImpl(
			nullptr,
			handle,
			it->second,
			a_flags);

		EraseActor(it);

		return true;
	}

	/*void IObjectManager::QueueReSinkAnimationGraphs(
		Game::FormID a_actor)
	{
		ITaskPool::AddPriorityTask([this, a_actor]() {
			stl::scoped_lock lock(m_lock);

			auto it = m_objects.find(a_actor);
			if (it != m_objects.end())
			{
				it->second.ReSinkAnimationGraphs();
			}
		});
	}*/

	void IObjectManager::RequestEvaluate(
		Game::FormID a_actor,
		bool         a_defer,
		bool         a_xfrmUpdate,
		bool         a_xfrmUpdateNoDefer) const
	{
		stl::scoped_lock lock(m_lock);

		auto it = m_objects.find(a_actor);
		if (it != m_objects.end())
		{
			if (a_defer)
			{
				it->second.RequestEvalDefer();
			}
			else
			{
				it->second.RequestEval();
			}

			if (a_xfrmUpdate)
			{
				if (a_xfrmUpdateNoDefer)
				{
					it->second.RequestTransformUpdate();
				}
				else
				{
					it->second.RequestTransformUpdateDefer();
				}
			}
		}
	}

	void IObjectManager::QueueRequestEvaluate(
		Game::FormID a_actor,
		bool         a_defer,
		bool         a_xfrmUpdate,
		bool         a_xfrmUpdateNoDefer) const
	{
		ITaskPool::AddTask(
			[this,
		     a_actor,
		     a_defer,
		     a_xfrmUpdate,
		     a_xfrmUpdateNoDefer]() {
				RequestEvaluate(
					a_actor,
					a_defer,
					a_xfrmUpdate,
					a_xfrmUpdateNoDefer);
			});
	}

	void IObjectManager::QueueRequestEvaluate(
		TESObjectREFR* a_actor,
		bool           a_defer,
		bool           a_xfrmUpdate,
		bool           a_xfrmUpdateNoDefer) const
	{
		if (IsActorValid(a_actor))
		{
			QueueRequestEvaluate(
				a_actor->formID,
				a_defer,
				a_xfrmUpdate,
				a_xfrmUpdateNoDefer);
		}
	}

	void IObjectManager::QueueClearVariablesOnAll(bool a_requestEval)
	{
		ITaskPool::AddPriorityTask([this, a_requestEval] {
			stl::scoped_lock lock(m_lock);

			ClearVariablesOnAll(a_requestEval);
		});
	}

	void IObjectManager::QueueClearVariables(
		Game::FormID a_handle,
		bool         a_requestEval)
	{
		ITaskPool::AddPriorityTask([this, a_handle, a_requestEval] {
			stl::scoped_lock lock(m_lock);

			ClearVariables(a_handle, a_requestEval);
		});
	}

	void IObjectManager::QueueRequestVariableUpdateOnAll() const
	{
		ITaskPool::AddPriorityTask([this] {
			stl::scoped_lock lock(m_lock);

			RequestVariableUpdateOnAll();
		});
	}

	void IObjectManager::QueueRequestVariableUpdate(Game::FormID a_handle) const
	{
		ITaskPool::AddPriorityTask([this, a_handle] {
			stl::scoped_lock lock(m_lock);

			RequestVariableUpdate(a_handle);
		});
	}

	void IObjectManager::CleanupActorObjectsImpl(
		TESObjectREFR*                   a_actor,
		Game::ObjectRefHandle            a_handle,
		ActorObjectHolder&               a_holder,
		stl::flag<ControllerUpdateFlags> a_flags)
	{
		if (a_holder.m_actor->loadedState)
		{
			for (auto& e : a_holder.m_cmeNodes)
			{
				ResetNodeOverride(e.second);
			}

			for (auto& e : a_holder.m_weapNodes)
			{
				ResetNodePlacement(e, nullptr);
			}
		}

		RemoveActorGear(
			a_actor,
			a_handle,
			a_holder,
			a_flags);

		a_holder.m_cmeNodes.clear();
		a_holder.m_movNodes.clear();
		a_holder.m_weapNodes.clear();
		a_holder.m_monitorNodes.clear();
		a_holder.m_nodeMonitorEntries.clear();

		/*assert(a_holder.m_animationUpdateList->Empty());
		assert(a_holder.m_animEventForwardRegistrations.Empty());*/
	}

	void IObjectManager::RemoveActorGear(
		TESObjectREFR*                   a_actor,
		Game::ObjectRefHandle            a_handle,
		stl::flag<ControllerUpdateFlags> a_flags)
	{
		auto it = m_objects.find(a_actor->formID);
		if (it != m_objects.end())
		{
			RemoveActorGear(a_actor, a_handle, it->second, a_flags);
		}
	}

	void IObjectManager::RemoveActorGear(
		TESObjectREFR*                   a_actor,
		Game::ObjectRefHandle            a_handle,
		ActorObjectHolder&               a_holder,
		stl::flag<ControllerUpdateFlags> a_flags)
	{
		a_holder.visit([&](auto& a_object) {
			RemoveObject(
				a_actor,
				a_handle,
				a_object,
				a_holder,
				a_flags);
		});

		for (auto& e : a_holder.m_entriesCustom)
		{
			e.clear();
		}

		/*assert(a_holder.m_animationUpdateList->Empty());
		assert(a_holder.m_animEventForwardRegistrations.Empty());*/
	}

	bool IObjectManager::RemoveInvisibleObjects(
		ActorObjectHolder&    a_holder,
		Game::ObjectRefHandle a_handle)
	{
		bool result = false;

		for (auto& e : a_holder.m_entriesSlot)
		{
			if (!e.IsNodeVisible())
			{
				RemoveObject(
					nullptr,
					a_handle,
					e,
					a_holder,
					ControllerUpdateFlags::kNone);

				result = true;
			}
		}

		for (auto& e : a_holder.m_entriesCustom)
		{
			for (auto it1 = e.begin(); it1 != e.end();)
			{
				for (auto it2 = it1->second.begin(); it2 != it1->second.end();)
				{
					if (!it2->second.IsNodeVisible())
					{
						RemoveObject(
							nullptr,
							a_handle,
							it2->second,
							a_holder,
							ControllerUpdateFlags::kNone);

						result = true;

						it2 = it1->second.erase(it2);
					}
					else
					{
						++it2;
					}
				}

				if (it1->second.empty())
				{
					it1 = e.erase(it1);
				}
				else
				{
					++it1;
				}
			}
		}

		return result;
	}

	void IObjectManager::ClearObjectsImpl()
	{
		for (auto& e : m_objects)
		{
			auto handle = e.second.GetHandle();

			NiPointer<TESObjectREFR> ref;
			(void)handle.LookupZH(ref);

			CleanupActorObjectsImpl(
				nullptr,
				handle,
				e.second,
				ControllerUpdateFlags::kNone);
		}

		m_objects.clear();
	}

	bool IObjectManager::ConstructArmorNode(
		TESForm*                                          a_form,
		const stl::vector<TESObjectARMA*>&                a_in,
		bool                                              a_isFemale,
		stl::vector<ObjectDatabase::ObjectDatabaseEntry>& a_dbEntries,
		NiPointer<NiNode>&                                a_out)
	{
		bool result = false;

		for (auto& e : a_in)
		{
			auto texSwap = std::addressof(e->bipedModels[a_isFemale ? 1 : 0]);
			auto path    = texSwap->GetModelName();

			if (!path || path[0] == 0)
			{
				texSwap = std::addressof(e->bipedModels[a_isFemale ? 0 : 1]);
				path    = texSwap->GetModelName();

				if (!path || path[0] == 0)
				{
					continue;
				}
			}

			NiPointer<NiNode>   object;
			ObjectDatabaseEntry entry;

			if (!GetUniqueObject(path, entry, object))
			{
				continue;
			}

			if (!a_out)
			{
				a_out          = NiNode::Create(1);
				a_out->m_flags = NiAVObject::kFlag_SelectiveUpdate |
				                 NiAVObject::kFlag_SelectiveUpdateTransforms |
				                 NiAVObject::kFlag_kSelectiveUpdateController;
			}

			char buffer[NODE_NAME_BUFFER_SIZE];

			stl::snprintf(
				buffer,
				"OBJECT ARMA [%.8X/%.8X]",
				a_form->formID.get(),
				e->formID.get());

			object->m_name = buffer;

			EngineExtensions::ApplyTextureSwap(texSwap, object);

			a_out->AttachChild(object, true);

			if (entry)
			{
				a_dbEntries.emplace_back(std::move(entry));
			}

			result = true;
		}

		return result;
	}

	void IObjectManager::GetNodeName(
		TESForm*             a_form,
		const modelParams_t& a_params,
		char (&a_out)[NODE_NAME_BUFFER_SIZE])
	{
		switch (a_params.type)
		{
		case ModelType::kWeapon:
			GetWeaponNodeName(a_form->formID, a_out);
			break;
		case ModelType::kArmor:
			GetArmorNodeName(
				a_form->formID,
				a_params.arma ?
					a_params.arma->formID :
                    Game::FormID{},
				a_out);
			break;
		case ModelType::kMisc:
		case ModelType::kLight:
			GetMiscNodeName(a_form->formID, a_out);
			break;
		case ModelType::kAmmo:
			GetAmmoNodeName(a_form->formID, a_out);
			break;
		default:
			HALT("FIXME");
		}
	}

	bool IObjectManager::LoadAndAttach(
		processParams_t&                a_params,
		const Data::configBaseValues_t& a_activeConfig,
		const Data::configBase_t&       a_config,
		ObjectEntryBase&                a_objectEntry,
		TESForm*                        a_form,
		TESForm*                        a_modelForm,
		bool                            a_leftWeapon,
		bool                            a_visible,
		bool                            a_disableHavok,
		bool                            a_bhkAnims)
	{
		if (a_objectEntry.state)
		{
			return false;
		}

		if (!a_activeConfig.targetNode)
		{
			return false;
		}

		if (a_form->formID.IsTemporary())
		{
			return false;
		}

		const auto hasModelForm = static_cast<bool>(a_modelForm);

		if (!a_modelForm)
		{
			a_modelForm = a_form;
		}

		modelParams_t modelParams;

		if (!GetModelParams(
				a_params.actor,
				a_modelForm,
				a_params.race,
				a_params.configSex == Data::ConfigSex::Female,
				a_activeConfig.flags.test(Data::BaseFlags::kLoad1pWeaponModel),
				a_activeConfig.flags.test(Data::BaseFlags::kUseWorldModel),
				modelParams))
		{
			Debug(
				"[%.8X] [race: %.8X] [item: %.8X] couldn't get model params",
				a_params.actor->formID.get(),
				a_params.race->formID.get(),
				a_modelForm->formID.get());

			return false;
		}

		nodesRef_t targetNodes;

		if (!CreateTargetNode(
				a_activeConfig,
				a_activeConfig.targetNode,
				a_params.npcRoot,
				targetNodes))
		{
			Debug(
				"[%.8X] [race: %.8X] [item: %.8X] couldn't get target node: %s",
				a_params.actor->formID.get(),
				a_params.race->formID.get(),
				a_modelForm->formID.get(),
				a_activeConfig.targetNode.name.c_str());

			return false;
		}

		auto state = std::make_unique<ObjectEntryBase::State>();

		NiPointer<NiNode>   object;
		ObjectDatabaseEntry entry;

		if (!GetUniqueObject(modelParams.path, entry, object))
		{
			Warning(
				"[%.8X] [race: %.8X] [item: %.8X] failed to load model: %s",
				a_params.actor->formID.get(),
				a_params.race->formID.get(),
				a_modelForm->formID.get(),
				modelParams.path);

			return false;
		}

		if (entry)
		{
			state->dbEntries.emplace_back(std::move(entry));
		}

		if (modelParams.swap)
		{
			EngineExtensions::ApplyTextureSwap(
				modelParams.swap,
				object);
		}

		object->m_localTransform = {};

		a_params.state.ResetEffectShaders(a_params.handle);

		char buffer[NODE_NAME_BUFFER_SIZE];

		GetNodeName(a_modelForm, modelParams, buffer);

		auto itemRoot = CreateAttachmentNode(buffer);

		state->UpdateData(a_activeConfig);
		UpdateObjectTransform(
			state->transform,
			itemRoot,
			targetNodes.ref);

		targetNodes.rootNode->AttachChild(itemRoot, true);
		UpdateDownwardPass(itemRoot);

		auto ar = EngineExtensions::AttachObject(
			a_params.actor,
			a_params.root,
			itemRoot,
			object,
			modelParams.type,
			a_leftWeapon,
			modelParams.isShield,
			a_activeConfig.flags.test(Data::BaseFlags::kDropOnDeath),
			a_activeConfig.flags.test(Data::BaseFlags::kRemoveScabbard),
			a_activeConfig.flags.test(Data::BaseFlags::kKeepTorchFlame),
			a_disableHavok || a_activeConfig.flags.test(Data::BaseFlags::kDisableHavok));

		UpdateDownwardPass(itemRoot);

		FinalizeObjectState(
			state,
			a_form,
			itemRoot,
			object,
			targetNodes,
			a_activeConfig);

		if (a_activeConfig.flags.test(Data::BaseFlags::kPlaySequence))
		{
			state->UpdateAndPlayAnimation(
				a_params.actor,
				a_activeConfig.niControllerSequence);
		}
		else if (
			a_bhkAnims &&
			modelParams.type == ModelType::kWeapon &&
			!a_activeConfig.flags.test(Data::BaseFlags::kDisableWeaponAnims))
		{
			if (EngineExtensions::CreateWeaponBehaviorGraph(
					object,
					state->weapAnimGraphManagerHolder,
					[&](const char* a_path) {
						if (a_config.hkxFilter.empty())
						{
							return true;
						}
						else
						{
							return !a_config.hkxFilter.contains(a_path);
						}
					}))
			{
				if (a_activeConfig.flags.test(Data::BaseFlags::kAnimationEvent))
				{
					state->UpdateAndSendAnimationEvent(
						a_activeConfig.animationEvent);
				}
				else
				{
					state->currentAnimationEvent =
						StringHolder::GetSingleton().weaponSheathe;

					state->weapAnimGraphManagerHolder->NotifyAnimationGraph(
						BSStringHolder::GetSingleton()->m_weaponSheathe);
				}

				/*a_params.objects.RegisterWeaponAnimationGraphManagerHolder(
					state->weapAnimGraphManagerHolder,
					!a_activeConfig.flags.test(Data::BaseFlags::kDisableAnimEventForwarding));*/
			}
		}

		if (ar.test(AttachResultFlags::kScbLeft))
		{
			state->flags.set(ObjectEntryFlags::kScbLeft);
		}

		if (hasModelForm)
		{
			state->modelForm = a_modelForm->formID;
		}

		a_objectEntry.state = std::move(state);

		if (a_visible)
		{
			PlayObjectSound(
				a_params,
				a_activeConfig,
				a_objectEntry,
				true);
		}

		return true;
	}

	bool IObjectManager::LoadAndAttachGroup(
		processParams_t&                a_params,
		const Data::configBaseValues_t& a_config,
		const Data::configModelGroup_t& a_group,
		ObjectEntryBase&                a_objectEntry,
		TESForm*                        a_form,
		bool                            a_leftWeapon,
		bool                            a_visible,
		bool                            a_disableHavok,
		bool                            a_bhkAnims)
	{
		if (a_objectEntry.state)
		{
			return false;
		}

		if (!a_config.targetNode)
		{
			return false;
		}

		if (a_form->formID.IsTemporary())
		{
			return false;
		}

		auto it = a_group.entries.find({});
		if (it == a_group.entries.end())
		{
			return false;
		}

		if (it->second.form.get_id() != a_form->formID)
		{
			return false;
		}

		struct tmpdata_t
		{
			const Data::configModelGroup_t::data_type::value_type* entry{ nullptr };
			TESForm*                                               form{ nullptr };
			modelParams_t                                          params;
			NiPointer<NiNode>                                      object;
			ObjectEntryBase::State::GroupObject*                   grpObject{ nullptr };
		};

		stl::list<tmpdata_t> modelParams;

		for (auto& e : a_group.entries)
		{
			if (e.second.flags.test(Data::ConfigModelGroupEntryFlags::kDisabled))
			{
				continue;
			}

			auto form = e.second.form.get_form();
			if (!form)
			{
				continue;
			}

			if (form->formID.IsTemporary())
			{
				continue;
			}

			modelParams_t params;

			if (!GetModelParams(
					a_params.actor,
					form,
					a_params.race,
					a_params.configSex == Data::ConfigSex::Female,
					e.second.flags.test(Data::ConfigModelGroupEntryFlags::kLoad1pWeaponModel),
					a_config.flags.test(Data::BaseFlags::kUseWorldModel) ||
						e.second.flags.test(Data::ConfigModelGroupEntryFlags::kUseWorldModel),
					params))
			{
				Debug(
					"[%.8X] [race: %.8X] [item: %.8X] couldn't get model params",
					a_params.actor->formID.get(),
					a_params.race->formID.get(),
					form->formID.get());

				continue;
			}

			modelParams.emplace_back(
				std::addressof(e),
				form,
				std::move(params));
		}

		if (modelParams.empty())
		{
			return false;
		}

		nodesRef_t targetNodes;

		if (!CreateTargetNode(
				a_config,
				a_config.targetNode,
				a_params.npcRoot,
				targetNodes))
		{
			Debug(
				"[%.8X] [race: %.8X] [item: %.8X] couldn't get target node: %s",
				a_params.actor->formID.get(),
				a_params.race->formID.get(),
				a_form->formID.get(),
				a_config.targetNode.name.c_str());

			return false;
		}

		auto state = std::make_unique<ObjectEntryBase::State>();

		bool loaded = false;

		for (auto& e : modelParams)
		{
			ObjectDatabaseEntry entry;

			if (!GetUniqueObject(e.params.path, entry, e.object))
			{
				Warning(
					"[%.8X] [race: %.8X] [item: %.8X] failed to load model: %s",
					a_params.actor->formID.get(),
					a_params.race->formID.get(),
					e.form->formID.get(),
					e.params.path);

				continue;
			}

			if (entry)
			{
				state->dbEntries.emplace_back(std::move(entry));
			}

			loaded = true;
		}

		if (!loaded)
		{
			return false;
		}

		a_params.state.ResetEffectShaders(a_params.handle);

		char buffer[NODE_NAME_BUFFER_SIZE];

		stl::snprintf(
			buffer,
			StringHolder::FMT_NINODE_IED_GROUP,
			a_form->formID.get());

		auto groupRoot = CreateAttachmentNode(buffer);

		state->UpdateData(a_config);
		UpdateObjectTransform(
			state->transform,
			groupRoot,
			targetNodes.ref);

		targetNodes.rootNode->AttachChild(groupRoot, true);
		UpdateDownwardPass(groupRoot);

		for (auto& e : modelParams)
		{
			if (!e.object)
			{
				continue;
			}

			e.object->m_localTransform = {};

			if (e.params.swap)
			{
				EngineExtensions::ApplyTextureSwap(
					e.params.swap,
					e.object);
			}

			GetNodeName(e.form, e.params, buffer);

			auto itemRoot = CreateAttachmentNode(buffer);

			auto& n = state->groupObjects.try_emplace(
											 e.entry->first,
											 itemRoot,
											 e.object)
			              .first->second;

			n.transform.Update(e.entry->second.transform);

			UpdateObjectTransform(
				n.transform,
				n.rootNode,
				nullptr);

			groupRoot->AttachChild(itemRoot, true);
			UpdateDownwardPass(itemRoot);

			EngineExtensions::AttachObject(
				a_params.actor,
				a_params.root,
				itemRoot,
				e.object,
				e.params.type,
				a_leftWeapon ||
					e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kLeftWeapon),
				e.params.isShield,
				a_config.flags.test(Data::BaseFlags::kDropOnDeath) ||
					e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kDropOnDeath),
				a_config.flags.test(Data::BaseFlags::kRemoveScabbard) ||
					e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kRemoveScabbard),
				a_config.flags.test(Data::BaseFlags::kKeepTorchFlame) ||
					e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kKeepTorchFlame),
				a_disableHavok ||
					a_config.flags.test(Data::BaseFlags::kDisableHavok) ||
					e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kDisableHavok));

			e.grpObject = std::addressof(n);
		}

		UpdateDownwardPass(groupRoot);

		for (auto& e : modelParams)
		{
			if (!e.grpObject)
			{
				continue;
			}

			if (e.entry->second.flags.test(
					Data::ConfigModelGroupEntryFlags::kPlaySequence))
			{
				e.grpObject->PlayAnimation(
					a_params.actor,
					e.entry->second.niControllerSequence);
			}
			else if (
				a_bhkAnims &&
				e.params.type == ModelType::kWeapon &&
				!a_config.flags.test(Data::BaseFlags::kDisableWeaponAnims) &&
				!e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kDisableWeaponAnims))
			{
				if (EngineExtensions::CreateWeaponBehaviorGraph(
						e.grpObject->object,
						e.grpObject->weapAnimGraphManagerHolder,
						[](const char*) { return true; }))
				{
					if (e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kAnimationEvent))
					{
						e.grpObject->UpdateAndSendAnimationEvent(
							e.entry->second.animationEvent);
					}
					/*else if (a_activeConfig.flags.test(Data::BaseFlags::kAnimationEvent))
					{
						e.grpObject->UpdateAndSendAnimationEvent(
							a_activeConfig.animationEvent);
					}*/
					else
					{
						e.grpObject->currentAnimationEvent =
							StringHolder::GetSingleton().weaponSheathe;
						e.grpObject->weapAnimGraphManagerHolder->NotifyAnimationGraph(
							BSStringHolder::GetSingleton()->m_weaponSheathe);
					}

					/*a_params.objects.RegisterWeaponAnimationGraphManagerHolder(
						e.grpObject->weapAnimGraphManagerHolder,
						!a_activeConfig.flags.test(Data::BaseFlags::kDisableAnimEventForwarding) &&
							!e.entry->second.flags.test(Data::ConfigModelGroupEntryFlags::kDisableAnimEventForwarding));*/
				}
			}
		}

		FinalizeObjectState(
			state,
			a_form,
			groupRoot,
			nullptr,
			targetNodes,
			a_config);

		state->flags.set(ObjectEntryFlags::kIsGroup);

		a_objectEntry.state = std::move(state);

		if (a_visible)
		{
			PlayObjectSound(
				a_params,
				a_config,
				a_objectEntry,
				true);
		}

		return true;
	}

	void IObjectManager::FinalizeObjectState(
		std::unique_ptr<ObjectEntryBase::State>& a_state,
		TESForm*                                 a_form,
		NiNode*                                  a_rootNode,
		const NiPointer<NiNode>&                 a_objectNode,
		nodesRef_t&                              a_targetNodes,
		const Data::configBaseValues_t&          a_config)
	{
		a_state->form           = a_form;
		a_state->formid         = a_form->formID;
		a_state->nodes.rootNode = a_rootNode;
		a_state->nodes.ref      = std::move(a_targetNodes.ref);
		a_state->nodes.object   = a_objectNode;
		a_state->nodeDesc       = a_config.targetNode;
		a_state->created        = IPerfCounter::Query();
		a_state->atmReference   = a_config.targetNode.managed() ||
		                        a_config.flags.test(Data::BaseFlags::kReferenceMode);
	}

	void IObjectManager::PlayObjectSound(
		const processParams_t&          a_params,
		const Data::configBaseValues_t& a_config,
		const ObjectEntryBase&          a_objectEntry,
		bool                            a_equip)
	{
		if (a_objectEntry.state &&
		    a_params.flags.test(ControllerUpdateFlags::kPlaySound) &&
		    a_config.flags.test(Data::BaseFlags::kPlaySound) &&
		    m_playSound)
		{
			if (a_params.objects.IsPlayer() || m_playSoundNPC)
			{
				SoundPlay(
					a_objectEntry.state->form->formType,
					a_objectEntry.state->nodes.rootNode,
					a_equip);
			}
		}
	}

	bool IObjectManager::AttachNodeImpl(
		NiNode*                     a_root,
		const Data::NodeDescriptor& a_node,
		bool                        a_atmReference,
		ObjectEntryBase&            a_entry)
	{
		if (!a_entry.state)
		{
			return false;
		}

		bool result = AttachObjectToTargetNode(
			a_node,
			a_atmReference,
			a_root,
			a_entry.state->nodes.rootNode,
			a_entry.state->nodes.ref);

		if (result)
		{
			a_entry.state->nodeDesc     = a_node;
			a_entry.state->atmReference = a_atmReference;

			a_entry.state->flags.clear(ObjectEntryFlags::kRefSyncDisableFailedOrphan);
		}

		return result;
	}

}