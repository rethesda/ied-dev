#include "pch.h"

#include "JSONConfigSlotHolderParser.h"
#include "JSONConfigSlotParser.h"
#include "JSONConfigSlotPriorityParser.h"
#include "JSONParsersCommon.h"

namespace IED
{
	namespace Serialization
	{
		using namespace Data;

		static SlotKeyParser s_slotKeyParser;

		static constexpr std::uint32_t CURRENT_VERSION = 1;

		template <>
		bool Parser<Data::configSlotHolder_t>::Parse(
			const Json::Value&        a_in,
			Data::configSlotHolder_t& a_out) const
		{
			JSON_PARSE_VERSION()

			Parser<configSlot_t> pslot(m_state);

			auto& data = a_in["data"];

			for (auto it = data.begin(); it != data.end(); ++it)
			{
				auto key = it.key().asString();

				auto slot = s_slotKeyParser.KeyToSlot(key);

				if (slot == ObjectSlot::kMax)
				{
					Error("%s: unrecognized slot key: '%s'", __FUNCTION__, key.c_str());
					return false;
				}

				auto& v = a_out.get(slot);

				if (!v)
				{
					v = std::make_unique<configSlotHolder_t::data_type>();
				}

				parserDesc_t<configSlot_t> desc[]{
					{ "m", (*v)(ConfigSex::Male) },
					{ "f", (*v)(ConfigSex::Female) }
				};

				for (auto& e : desc)
				{
					if (!pslot.Parse((*it)[e.member], e.data, version))
					{
						return false;
					}
				}
			}

			if (auto& prio = a_in["prio"])
			{
				Parser<configSlotPriority_t> pprio(m_state);

				auto& v = a_out.priority;

				if (!v)
				{
					v = std::make_unique<Data::configSlotHolder_t::prio_data_type>();
				}

				parserDesc_t<configSlotPriority_t> desc[]{
					{ "m", (*v)(ConfigSex::Male) },
					{ "f", (*v)(ConfigSex::Female) }
				};

				for (auto& e : desc)
				{
					if (!pprio.Parse(prio[e.member], e.data))
					{
						return false;
					}

					if (!e.data.validate())
					{
						Warning("%s: bad priority data", __FUNCTION__);
						SetHasErrors();
					}
				}
			}

			return true;
		}

		template <>
		void Parser<Data::configSlotHolder_t>::Create(
			const Data::configSlotHolder_t& a_data,
			Json::Value&                    a_out) const
		{
			auto& data = (a_out["data"] = Json::Value(Json::ValueType::objectValue));

			using enum_type = std::underlying_type_t<ObjectSlot>;

			Parser<configSlot_t> pslot(m_state);

			for (enum_type i = 0; i < stl::underlying(ObjectSlot::kMax); i++)
			{
				auto slot = static_cast<ObjectSlot>(i);

				auto& v = a_data.get(slot);

				if (!v)
				{
					continue;
				}

				auto key = s_slotKeyParser.SlotToKey(slot);

				assert(key);

				parserDesc_t<configSlot_t> desc[]{
					{ "m", (*v)(ConfigSex::Male) },
					{ "f", (*v)(ConfigSex::Female) }
				};

				auto& e = data[key];

				for (auto& f : desc)
				{
					pslot.Create(f.data, e[f.member]);
				}
			}

			if (auto& prio = a_data.priority)
			{
				Parser<configSlotPriority_t> pprio(m_state);

				auto& pout = (a_out["prio"] = Json::Value(Json::ValueType::objectValue));

				parserDesc_t<configSlotPriority_t> desc[]{
					{ "m", (*prio)(ConfigSex::Male) },
					{ "f", (*prio)(ConfigSex::Female) }
				};

				for (auto& e : desc)
				{
					pprio.Create(e.data, pout[e.member]);
				}
			}

			a_out["version"] = CURRENT_VERSION;
		}

	}
}