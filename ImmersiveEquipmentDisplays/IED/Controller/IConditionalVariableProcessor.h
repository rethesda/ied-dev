#pragma once

#include "IED/ConfigConditionalVars.h"

namespace IED
{
	struct processParams_t;
	class IEquipment;

	class IConditionalVariableProcessor
	{
	public:
		static bool UpdateVariableMap(
			processParams_t&                                a_params,
			const Data::configConditionalVariablesHolder_t& a_config,
			conditionalVariableMap_t&                       a_map) noexcept;

	private:
		static constexpr const Data::configConditionalVariable_t* GetOverrideVariable(
			processParams_t&                              a_params,
			const Data::configConditionalVariablesList_t& a_list) noexcept;

		static Game::FormID GetLastEquippedForm(
			processParams_t&                                  a_params,
			const Data::configConditionalVariableValueData_t& a_data) noexcept;

		static constexpr void UpdateVariable(
			processParams_t&                                  a_params,
			ConditionalVariableType                           a_type,
			const Data::configConditionalVariableValueData_t& a_src,
			conditionalVariableStorage_t&                     a_dst,
			bool&                                             a_modified) noexcept;
	};
}