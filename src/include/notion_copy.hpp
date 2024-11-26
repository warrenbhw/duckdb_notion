// Taken with minor modifications from https://github.com/duckdb/postgres_scanner/blob/main/src/include/postgres_binary_copy.hpp

#pragma once

#include "duckdb/function/copy_function.hpp"

namespace duckdb
{
    struct NotionCopyGlobalState : public GlobalFunctionData
    {
        explicit NotionCopyGlobalState(ClientContext &context, const string &spreadsheet_id, const string &token, const string &sheet_name)
            : spreadsheet_id(spreadsheet_id), token(token), sheet_name(sheet_name)
        {
        }

    public:
        string spreadsheet_id;
        string token;
        string sheet_name;
    };

    struct NotionWriteOptions
    {
        vector<string> name_list;
    };

    struct NotionWriteBindData : public TableFunctionData
    {
        vector<string> files;
        NotionWriteOptions options;
        vector<LogicalType> sql_types;

        NotionWriteBindData(string file_path, vector<LogicalType> sql_types, vector<string> names)
            : sql_types(std::move(sql_types))
        {
            files.push_back(std::move(file_path));
            options.name_list = std::move(names);
        }
    };

    class NotionCopyFunction : public CopyFunction
    {
    public:
        NotionCopyFunction();

        static unique_ptr<FunctionData> NotionWriteBind(ClientContext &context, CopyFunctionBindInput &input, const vector<string> &names, const vector<LogicalType> &sql_types);

        static unique_ptr<GlobalFunctionData> NotionWriteInitializeGlobal(ClientContext &context, FunctionData &bind_data, const string &file_path);

        static unique_ptr<LocalFunctionData> NotionWriteInitializeLocal(ExecutionContext &context, FunctionData &bind_data_p);

        static void NotionWriteSink(ExecutionContext &context, FunctionData &bind_data_p, GlobalFunctionData &gstate, LocalFunctionData &lstate, DataChunk &input);
    };

} // namespace duckdb