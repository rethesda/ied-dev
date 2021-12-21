#pragma once

#include "../ConfigOverride.h"
#include "../ConfigOverrideDefault.h"
#include "ImportFlags.h"

namespace IED
{
	class IJSONSerialization :
		public virtual ILog
	{
	public:

		bool ImportData(Data::configStore_t&& a_in, stl::flag<ImportFlags> a_flags);

		bool ExportData(const fs::path& a_path, stl::flag<Data::ConfigStoreSerializationFlags> a_flags);

		bool LoadConfigStore(const fs::path& a_path, Data::configStore_t& a_out) const;
		bool SaveConfigStore(const fs::path& a_path, const Data::configStore_t& a_data) const;

		inline constexpr const auto& JSGetLastException() const noexcept
		{
			return m_lastException;
		}

		Data::configStore_t CreateExportData(
			const Data::configStore_t& a_data,
			stl::flag<Data::ConfigStoreSerializationFlags> a_flags);

		FN_NAMEPROC("JSONSerialization");

	private:
		bool DoImportOverwrite(
			Data::configStore_t&& a_in,
			[[maybe_unused]] stl::flag<ImportFlags> a_flags);

		bool DoImportMerge(
			Data::configStore_t&& a_in,
			[[maybe_unused]] stl::flag<ImportFlags> a_flags);

		mutable except::descriptor m_lastException;

		virtual constexpr WCriticalSection& JSGetLock() noexcept = 0;
		virtual constexpr Data::configStore_t& JSGetConfigStore() noexcept = 0;
		virtual void JSOnDataImport() = 0;
	};
}  // namespace IED