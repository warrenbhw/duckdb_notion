#pragma once

#include "duckdb.hpp"
#include "duckdb/function/table_function.hpp"
#include "duckdb/common/types/data_chunk.hpp"
#include "duckdb/main/client_context.hpp"

namespace duckdb
{

    struct ReadDatabaseBindData : public TableFunctionData
    {
        string database_id;
        string token;
        bool finished;
        idx_t row_index;
        string response;
        bool header;

        ReadDatabaseBindData(string database_id, string token, bool header);
    };

    // void ReadDatabaseFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output);

    // unique_ptr<FunctionData> ReadDatabaseBind(ClientContext &context, TableFunctionBindInput &input,
    //                                           vector<LogicalType> &return_types, vector<string> &names);

} // namespace duckdb