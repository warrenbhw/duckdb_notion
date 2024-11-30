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
        auto read_notion_function = TableFunction("read_notion", {LogicalType::VARCHAR}, NotionReadFunction, NotionBindFunction);
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

    // struct NotionConnectionData
    // {
    //     string api_key;

    //     explicit NotionConnectionData(string api_key_p) : api_key(std::move(api_key_p)) {}
    // };

    // static NotionConnectionData *GetConnectionData(ClientContext &context)
    // {
    //     // auto api_key = GetNotionApiKey(context);
    //     string api_key = "your_key";
    //     if (!api_key.empty())
    //     {
    //         return new NotionConnectionData(api_key);
    //     }
    //     throw InvalidInputException("Notion API key not configured. Use SET notion_api_key='your_key';");
    // }

    // Table function to query Notion databases
    // struct NotionDatabaseScanData : public TableFunctionData
    // {
    //     string database_id;
    //     vector<NotionProperty> properties;
    //     string api_key;

    //     NotionDatabaseScanData(string database_id_p, vector<NotionProperty> properties_p, string api_key_p)
    //         : database_id(std::move(database_id_p)), properties(std::move(properties_p)),
    //           api_key(std::move(api_key_p)) {}
    // };

    // static unique_ptr<FunctionData> NotionDatabaseBind(ClientContext &context,
    //                                                    TableFunctionBindInput &input,
    //                                                    vector<LogicalType> &return_types,
    //                                                    vector<string> &names)
    // {
    //     auto database_id = StringValue::Get(input.inputs[0]);
    //     auto conn_data = unique_ptr<NotionConnectionData>(GetConnectionData(context));

    //     // Get database schema using notion_requests
    //     auto properties = GetDatabaseSchema(conn_data->api_key, database_id);

    //     // Set up return types based on property types
    //     for (auto &prop : properties)
    //     {
    //         names.push_back(prop.name);
    //         switch (prop.type)
    //         {
    //         case NotionPropertyType::TEXT:
    //             return_types.push_back(LogicalType::VARCHAR);
    //             break;
    //         case NotionPropertyType::NUMBER:
    //             return_types.push_back(LogicalType::DOUBLE);
    //             break;
    //         case NotionPropertyType::DATE:
    //             return_types.push_back(LogicalType::TIMESTAMP);
    //             break;
    //         case NotionPropertyType::CHECKBOX:
    //             return_types.push_back(LogicalType::BOOLEAN);
    //             break;
    //         case NotionPropertyType::SELECT:
    //         case NotionPropertyType::MULTI_SELECT:
    //             return_types.push_back(LogicalType::VARCHAR);
    //             break;
    //         case NotionPropertyType::RELATION:
    //             return_types.push_back(LogicalType::VARCHAR);
    //             break;
    //         default:
    //             return_types.push_back(LogicalType::VARCHAR);
    //             break;
    //         }
    //     }

    //     return make_uniq<NotionDatabaseScanData>(database_id, properties, conn_data->api_key);
    // }

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
