#pragma once

#include "ConfigSkeletonMatch.h"
#include "ConfigTransform.h"

namespace IED
{
	namespace Data
	{

		struct configExtraNodeEntrySkel_t
		{
			configSkeletonMatch_t     match;
			configTransform_t         transform_mov;
			configTransform_t         transform_node;
		};

		struct configExtraNodeEntry_t
		{
			stl::fixed_string                     name;
			stl::fixed_string                     parent;
			stl::fixed_string                     desc;
			stl::list<configExtraNodeEntrySkel_t> skel;
		};

		using configExtraNodeList_t = stl::list<configExtraNodeEntry_t>;
	}
}