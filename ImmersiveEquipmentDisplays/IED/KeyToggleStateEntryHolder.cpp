#include "pch.h"

#include "KeyToggleStateEntryHolder.h"

namespace IED
{
	namespace KB
	{
		auto KeyToggleStateEntryHolder::make_state_data() const
			-> state_data
		{
			state_data result;

			result.reserve(entries.size());

			for (auto& e : entries)
			{
				result.emplace_back(e.first, e.second.GetState());
			}

			return result;
		}
	}
}