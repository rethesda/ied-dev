#pragma once

#include "IED/ConfigOverrideCustom.h"

#include "Serialization/Serialization.h"

namespace IED
{
	namespace Serialization
	{
		template <>
		bool Parser<Data::configCustom_t>::Parse(
			const Json::Value& a_in,
			Data::configCustom_t& a_out,
			const std::uint32_t a_version) const;

		template <>
		void Parser<Data::configCustom_t>::Create(
			const Data::configCustom_t& a_in,
			Json::Value& a_out) const;

	}  // namespace Serialization
}  // namespace IED