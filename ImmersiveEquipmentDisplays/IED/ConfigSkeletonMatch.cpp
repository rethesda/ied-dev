#include "pch.h"

#include "ConfigSkeletonMatch.h"

#include "IED/SkeletonID.h"

namespace IED
{
	namespace Data
	{
		bool configSkeletonMatch_t::test(
			const SkeletonID& a_id) const
		{
			if (!ids.empty())
			{
				auto& id = a_id.id();
				if (!id || !ids.contains(*id))
				{
					return false;
				}
			}

			if (!signatures.empty())
			{
				if (!signatures.contains(a_id.signature()))
				{
					return false;
				}
			}

			return true;
		}
	}
}