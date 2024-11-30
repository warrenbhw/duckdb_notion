#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/main/config.hpp"
#include "duckdb/parser/expression/constant_expression.hpp"
#include "duckdb/parser/expression/function_expression.hpp"
#include "duckdb/parser/tableref/table_function_ref.hpp"

// Standard library
#include <string>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

// Notion extension
#include "notion_extension.hpp"
#include "notion_auth.hpp"
#include "notion_read.hpp"
// #include "notion_copy.hpp"

namespace duckdb
{

    static void LoadInternal(DatabaseInstance &instance)
    {
        // Initialize OpenSSL
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();

        // Register read_notion table function
        auto read_notion_function = TableFunction("read_notion", {LogicalType::VARCHAR}, notion_read_function, notion_bind_function);
        ExtensionUtil::RegisterFunction(instance, read_notion_function);

        // // Register COPY TO (FORMAT 'gsheet') function
        // GSheetCopyFunction gsheet_copy_function;
        // ExtensionUtil::RegisterFunction(instance, gsheet_copy_function);

        // Register Secret functions
        CreateNotionSecretFunctions::Register(instance);

        // Register replacement scan for read_gsheet
        // auto &config = DBConfig::GetConfig(instance);
        // config.replacement_scans.emplace_back(ReadSheetReplacement);
    }

    void NotionExtension::Load(DuckDB &db)
    {
        LoadInternal(*db.instance);
    }
    std::string NotionExtension::Name()
    {
        return "notion";
    }

    std::string NotionExtension::Version() const
    {
#ifdef EXT_VERSION_NOTION
        return EXT_VERSION_NOTION;
#else
        return "";
#endif
    }

} // namespace duckdb

extern "C"
{

    DUCKDB_EXTENSION_API void notion_init(duckdb::DatabaseInstance &db)
    {
        duckdb::DuckDB db_wrapper(db);
        db_wrapper.LoadExtension<duckdb::NotionExtension>();
    }

    DUCKDB_EXTENSION_API const char *notion_version()
    {
        return duckdb::DuckDB::LibraryVersion();
    }
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
