#include "pch.h"

#include "JSONConfigNodeOverrideHolderParser.h"
#include "JSONConfigNodeOverrideTransformParser.h"
#include "JSONConfigNodeOverridePlacementParser.h"

namespace IED
{
	using namespace Data;

	namespace Serialization
	{
		static constexpr std::uint32_t CURRENT_VERSION = 1;

		template <>
		bool Parser<Data::configNodeOverrideHolder_t>::Parse(
			const Json::Value& a_in,
			Data::configNodeOverrideHolder_t& a_out) const
		{
			JSON_PARSE_VERSION()

			auto& data = a_in["data"];

			Parser<Data::configNodeOverrideTransform_t> parser(m_state);
			Parser<Data::configNodeOverridePlacement_t> pparser(m_state);

			a_out.flags = static_cast<Data::NodeOverrideHolderFlags>(
				data.get("flags", stl::underlying(Data::NodeOverrideHolderFlags::kNone)).asUInt());

			auto& vdata = data["data"];

			for (auto it = vdata.begin(); it != vdata.end(); ++it)
			{
				auto key = it.key().asString();

				auto& v = a_out.data.try_emplace(key).first->second;

				parserDesc_t<Data::configNodeOverrideTransform_t> desc[]{
					{ "m", v(ConfigSex::Male) },
					{ "f", v(ConfigSex::Female) }
				};

				for (auto& e : desc)
				{
					if (!parser.Parse((*it)[e.member], e.data, version))
					{
						return false;
					}
				}
			}

			auto& pdata = data["pdata"];

			for (auto it = pdata.begin(); it != pdata.end(); ++it)
			{
				auto key = it.key().asString();

				auto& v = a_out.placementData.try_emplace(key).first->second;

				parserDesc_t<Data::configNodeOverridePlacement_t> desc[]{
					{ "m", v(ConfigSex::Male) },
					{ "f", v(ConfigSex::Female) }
				};

				for (auto& e : desc)
				{
					if (!pparser.Parse((*it)[e.member], e.data, version))
					{
						return false;
					}
				}
			}

			return true;
		}

		template <>
		void Parser<Data::configNodeOverrideHolder_t>::Create(
			const Data::configNodeOverrideHolder_t& a_data,
			Json::Value& a_out) const
		{
			auto& data = (a_out["data"] = Json::Value(Json::ValueType::objectValue));

			Parser<Data::configNodeOverrideTransform_t> parser(m_state);
			Parser<Data::configNodeOverridePlacement_t> pparser(m_state);

			data["flags"] = stl::underlying(a_data.flags.value);

			auto& vdata = (data["data"] = Json::Value(Json::ValueType::objectValue));

			for (auto& e : a_data.data)
			{
				parserDescConst_t<Data::configNodeOverrideTransform_t> desc[]{
					{ "m", e.second(ConfigSex::Male) },
					{ "f", e.second(ConfigSex::Female) }
				};

				auto& v = vdata[e.first];

				for (auto& f : desc)
				{
					parser.Create(f.data, v[f.member]);
				}
			}

			auto& pdata = (data["pdata"] = Json::Value(Json::ValueType::objectValue));

			for (auto& e : a_data.placementData)
			{
				parserDescConst_t<Data::configNodeOverridePlacement_t> desc[]{
					{ "m", e.second(ConfigSex::Male) },
					{ "f", e.second(ConfigSex::Female) }
				};

				auto& v = pdata[e.first];

				for (auto& f : desc)
				{
					pparser.Create(f.data, v[f.member]);
				}
			}

			a_out["version"] = CURRENT_VERSION;
		}

		template <>
		void Parser<Data::configNodeOverrideHolder_t>::GetDefault(
			Data::configNodeOverrideHolder_t& a_out) const
		{
		}
	}
}