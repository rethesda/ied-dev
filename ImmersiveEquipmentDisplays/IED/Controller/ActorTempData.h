#pragma once

#include "IED/Inventory.h"
#include "IED/NodeOverrideParams.h"
#include "IED/ConfigCommon.h"

namespace IED
{
	struct BipedSlotCacheEntry;

	struct ActorTempData
	{
		Data::CollectorData::container_type       idt;
		Data::CollectorData::eq_container_type    eqt;
		nodeOverrideParams_t::item_container_type nc;
		SlotResults                               sr;
		UseCountContainer                         uc;
		stl::vector<const BipedSlotCacheEntry*>   le;
		Data::configFormList_t                    fl;
	};
}