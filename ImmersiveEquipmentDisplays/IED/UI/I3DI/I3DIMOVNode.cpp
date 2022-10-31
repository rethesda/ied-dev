#include "pch.h"

#include "I3DIMOVNode.h"

#include "I3DIActorContext.h"
#include "I3DIActorObject.h"
#include "I3DICommonData.h"
#include "I3DIWeaponNode.h"

#include "Common/VectorMath.h"

#include "IED/Controller/Controller.h"

namespace IED
{
	namespace UI
	{
		I3DIMOVNode::I3DIMOVNode(
			ID3D11Device*                                  a_device,
			ID3D11DeviceContext*                           a_context,
			const std::shared_ptr<D3DModelData>&           a_data,
			const stl::fixed_string&                       a_name,
			const NodeOverrideData::weaponNodeListEntry_t& a_nodeInfo,
			const NodeOverrideData::overrideNodeEntry_t*   a_cmeNodeInfo,
			I3DIWeaponNode&                                a_weaponNode,
			I3DIActorContext&                              a_actorContext) :
			I3DIModelObject(
				a_device,
				a_context,
				BoundingShape::kSphere,
				a_data),
			I3DIDropTarget(*this),
			m_name(a_name),
			m_nodeInfo(a_nodeInfo),
			m_weaponNode(a_weaponNode),
			m_cmeNodeInfo(a_cmeNodeInfo),
			m_actorContext(a_actorContext)
		{
			SetAlpha(0.67f);
		}

		I3DIBoundObject* I3DIMOVNode::GetParentObject() const
		{
			return m_actorContext.GetActorObject().get();
		}

		void I3DIMOVNode::DrawTooltip(I3DICommonData& a_data)
		{
			ImGui::Text("Target: %s [%s]", m_nodeInfo.desc, m_name.c_str());

			if (auto& dragObject = a_data.objectController.GetDragObject())
			{
				if (auto weaponNode = dragObject->GetAsWeaponNode())
				{
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::Text(
						"Weapon node: %s [%s]",
						weaponNode->GetNodeDesc(),
						weaponNode->GetNodeName().c_str());
				}
			}
		}

		bool I3DIMOVNode::AcceptsDraggable(I3DIDraggable& a_item) const
		{
			if (m_weaponNodeAttached)
			{
				return false;
			}

			return std::addressof(static_cast<I3DIDraggable&>(m_weaponNode)) == std::addressof(a_item);
		}

		bool I3DIMOVNode::ProcessDropRequest(I3DIDraggable& a_item)
		{
			if (!AcceptsDraggable(a_item))
			{
				return false;
			}

			auto weaponNode = a_item.GetDraggableObject().GetAsWeaponNode();
			if (!weaponNode)
			{
				return false;
			}

			auto& controller = m_actorContext.GetController();
			auto& actorid    = m_actorContext.GetActorFormID();
			auto& objects    = controller.GetObjects();

			auto it = objects.find(actorid);
			if (it == objects.end())
			{
				return false;
			}

			auto r = controller.GetConfigStore().active.transforms.GetActorData().try_emplace(actorid);
			auto s = r.first->second.placementData.try_emplace(weaponNode->GetNodeName());

			auto& e = s.first->second.get(
				it->second.IsFemale() ?
					Data::ConfigSex::Female :
                    Data::ConfigSex::Male);

			e.targetNode = m_name;

			it->second.RequestTransformUpdate();

			_DMESSAGE("drop: %s %s", weaponNode->GetNodeName().c_str(), m_name.c_str());

			return false;
		}

		void I3DIMOVNode::OnDraggableMovingOver(I3DIDraggable& a_item)
		{
			if (!AcceptsDraggable(a_item))
			{
				return;
			}

			auto& dragObject = a_item.GetDraggableObject();

			if (auto weapon = dragObject.GetAsWeaponNode())
			{
				weapon->UpdateWorldMatrix(
					weapon->GetLocalMatrix() *
					GetOriginalWorldMatrix());

				weapon->UpdateBound();
			}
		}

		bool I3DIMOVNode::ShouldProcess(I3DICommonData& a_data)
		{
			if (auto& dragObject = a_data.objectController.GetDragObject())
			{
				return static_cast<bool>(dragObject->GetAsWeaponNode());
			}

			return false;
		}

		void I3DIMOVNode::OnMouseMoveOver(I3DICommonData& a_data)
		{
			SetAlpha(1.0f);
		}

		void I3DIMOVNode::OnMouseMoveOut(I3DICommonData& a_data)
		{
			if (!m_objectFlags.test_any(I3DIObjectFlags::kHSMask))
			{
				SetAlpha(0.67f);
			}
		}

		bool I3DIMOVNode::WantDrawTooltip()
		{
			return (m_objectFlags & I3DIObjectFlags::kHSMask) == I3DIObjectFlags::kHovered;
		}
	}
}