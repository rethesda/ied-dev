#pragma once

namespace IED
{
	class SkeletonCache
	{
		struct Entry
		{
			NiTransform transform;
		};

	public:
		using actor_entry_type       = std::shared_ptr<stl::unordered_map<stl::fixed_string, Entry>>;
		using const_actor_entry_type = std::shared_ptr<const stl::unordered_map<stl::fixed_string, Entry>>;
		using data_type              = stl::unordered_map<stl::fixed_string, actor_entry_type>;

		[[nodiscard]] inline static constexpr auto& GetSingleton() noexcept
		{
			return m_Instance;
		}

		/*const Entry* GetNode(
			TESObjectREFR*           a_refr,
			const stl::fixed_string& a_name);*/

		std::optional<data_type::value_type> Get(
			TESObjectREFR* a_refr,
			bool           a_firstPerson = false);
		
		actor_entry_type Get2(
			TESObjectREFR* a_refr,
			bool           a_firstPerson = false);

		[[nodiscard]] inline auto GetSize() const noexcept
		{
			std::lock_guard lock(m_lock);
			return m_data.size();
		}

		[[nodiscard]] std::size_t GetTotalEntries() const noexcept;

	private:
		SkeletonCache() = default;

		static stl::fixed_string mk_key(
			TESObjectREFR* a_refr,
			bool           a_firstPerson);

		data_type::const_iterator get_or_create(
			const stl::fixed_string& a_key);

		void fill(
			const stl::fixed_string& a_key,
			data_type::iterator      a_it);

		mutable std::recursive_mutex m_lock;
		data_type                     m_data;

		static SkeletonCache m_Instance;
	};
}