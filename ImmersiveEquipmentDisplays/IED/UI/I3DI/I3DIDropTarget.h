#pragma once

#include "IED/D3D/D3DObject.h"

namespace IED
{
	class D3DObject;

	namespace UI
	{
		class I3DIDraggable;
		class I3DIObjectController;

		class I3DIDropTarget
		{
			friend class I3DIObjectController;

		public:
			I3DIDropTarget(
				D3DObject& a_owner);

			virtual ~I3DIDropTarget() noexcept = default;

		private:
			virtual bool AcceptsDraggable(I3DIDraggable& a_item)   = 0;
			virtual bool ProcessDropRequest(I3DIDraggable& a_item) = 0;

			// parent
			D3DObject& m_owner;
		};
	}
}