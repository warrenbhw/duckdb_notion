#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/common/types/data_chunk.hpp"
#include "duckdb/main/client_context.hpp"

namespace duckdb
{

    struct NotionReadFunctionData : public TableFunctionData
    {
        string database_id;

        explicit NotionReadFunctionData(std::string database_id_p) : database_id(std::move(database_id_p)) {}
    };

    void notion_read_function(ClientContext &context, TableFunctionInput &data_p, DataChunk &output);

    unique_ptr<FunctionData> notion_bind_function(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names);
} // namespace duckdb