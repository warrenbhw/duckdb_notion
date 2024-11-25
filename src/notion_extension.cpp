#define DUCKDB_EXTENSION_MAIN

#include "notion_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

namespace duckdb {

inline void NotionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "Notion "+name.GetString()+" 🐥");;
        });
}

inline void NotionOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result) {
    auto &name_vector = args.data[0];
    UnaryExecutor::Execute<string_t, string_t>(
	    name_vector, result, args.size(),
	    [&](string_t name) {
			return StringVector::AddString(result, "Notion " + name.GetString() +
                                                     ", my linked OpenSSL version is " +
                                                     OPENSSL_VERSION_TEXT );;
        });
}

static void LoadInternal(DatabaseInstance &instance) {
    // Register a scalar function
    auto notion_scalar_function = ScalarFunction("notion", {LogicalType::VARCHAR}, LogicalType::VARCHAR, NotionScalarFun);
    ExtensionUtil::RegisterFunction(instance, notion_scalar_function);

    // Register another scalar function
    auto notion_openssl_version_scalar_function = ScalarFunction("notion_openssl_version", {LogicalType::VARCHAR},
                                                LogicalType::VARCHAR, NotionOpenSSLVersionScalarFun);
    ExtensionUtil::RegisterFunction(instance, notion_openssl_version_scalar_function);
}

void NotionExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string NotionExtension::Name() {
	return "notion";
}

std::string NotionExtension::Version() const {
#ifdef EXT_VERSION_NOTION
	return EXT_VERSION_NOTION;
#else
	return "";
#endif
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void notion_init(duckdb::DatabaseInstance &db) {
    duckdb::DuckDB db_wrapper(db);
    db_wrapper.LoadExtension<duckdb::NotionExtension>();
}

DUCKDB_EXTENSION_API const char *notion_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
