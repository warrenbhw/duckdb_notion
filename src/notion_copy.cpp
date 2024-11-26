#include "notion_copy.hpp"
#include "notion_requests.hpp"
#include "notion_auth.hpp"
#include "notion_utils.hpp"

#include "duckdb/common/serializer/buffered_file_writer.hpp"
#include "duckdb/common/file_system.hpp"
#include "duckdb/main/secret/secret_manager.hpp"
#include <json.hpp>

using json = nlohmann::json;

namespace duckdb
{

    NotionCopyFunction::NotionCopyFunction() : CopyFunction("notion")
    {
        copy_to_bind = NotionWriteBind;
        copy_to_initialize_global = NotionWriteInitializeGlobal;
        copy_to_initialize_local = NotionWriteInitializeLocal;
        copy_to_sink = NotionWriteSink;
    }

    unique_ptr<FunctionData> NotionCopyFunction::NotionWriteBind(ClientContext &context, CopyFunctionBindInput &input, const vector<string> &names, const vector<LogicalType> &sql_types)
    {
        string file_path = input.info.file_path;

        return make_uniq<NotionWriteBindData>(file_path, sql_types, names);
    }

    unique_ptr<GlobalFunctionData> NotionCopyFunction::NotionWriteInitializeGlobal(ClientContext &context, FunctionData &bind_data, const string &file_path)
    {
        auto &secret_manager = SecretManager::Get(context);
        auto transaction = CatalogTransaction::GetSystemCatalogTransaction(context);
        auto secret_match = secret_manager.LookupSecret(transaction, "notion", "notion");

        if (!secret_match.HasMatch())
        {
            throw InvalidInputException("No 'notion' secret found. Please create a secret with 'CREATE SECRET' first.");
        }

        auto &secret = secret_match.GetSecret();
        if (secret.GetType() != "notion")
        {
            throw InvalidInputException("Invalid secret type. Expected 'notion', got '%s'", secret.GetType());
        }

        const auto *kv_secret = dynamic_cast<const KeyValueSecret *>(&secret);
        if (!kv_secret)
        {
            throw InvalidInputException("Invalid secret format for 'notion' secret");
        }

        Value token_value;
        if (!kv_secret->TryGetValue("token", token_value))
        {
            throw InvalidInputException("'token' not found in 'notion' secret");
        }

        std::string token = token_value.ToString();
        std::string spreadsheet_id = extract_spreadsheet_id(file_path);
        std::string sheet_id = extract_sheet_id(file_path);
        std::string sheet_name = "Sheet1";

        sheet_name = get_sheet_name_from_id(spreadsheet_id, sheet_id, token);

        std::string encoded_sheet_name = url_encode(sheet_name);

        // If writing, clear out the entire sheet first.
        // Do this here in the initialization so that it only happens once
        std::string delete_response = delete_sheet_data(spreadsheet_id, token, encoded_sheet_name);

        // Write out the headers to the file here in the Initialize so they are only written once
        // Create object ready to write to Google Sheet
        json sheet_data;

        sheet_data["range"] = sheet_name;
        sheet_data["majorDimension"] = "ROWS";

        vector<string> headers = bind_data.Cast<NotionWriteBindData>().options.name_list;

        vector<vector<string>> values;
        values.push_back(headers);
        sheet_data["values"] = values;

        // Convert the JSON object to a string
        std::string request_body = sheet_data.dump();

        // Make the API call to write data to the Google Sheet
        // Today, this is only append.
        std::string response = call_sheets_api(spreadsheet_id, token, encoded_sheet_name, HttpMethod::POST, request_body);

        // Check for errors in the response
        json response_json = parseJson(response);
        if (response_json.contains("error"))
        {
            throw duckdb::IOException("Error writing to Google Sheet: " + response_json["error"]["message"].get<std::string>());
        }

        return make_uniq<NotionCopyGlobalState>(context, spreadsheet_id, token, encoded_sheet_name);
    }

    unique_ptr<LocalFunctionData> NotionCopyFunction::NotionWriteInitializeLocal(ExecutionContext &context, FunctionData &bind_data_p)
    {
        return make_uniq<LocalFunctionData>();
    }

    void NotionCopyFunction::NotionWriteSink(ExecutionContext &context, FunctionData &bind_data_p, GlobalFunctionData &gstate_p, LocalFunctionData &lstate, DataChunk &input)
    {
        input.Flatten();
        auto &gstate = gstate_p.Cast<NotionCopyGlobalState>();

        std::string sheet_id = extract_sheet_id(bind_data_p.Cast<NotionWriteBindData>().files[0]);

        std::string sheet_name = "Sheet1";

        sheet_name = get_sheet_name_from_id(gstate.spreadsheet_id, sheet_id, gstate.token);
        std::string encoded_sheet_name = url_encode(sheet_name);
        // Create object ready to write to Google Sheet
        json sheet_data;

        sheet_data["range"] = sheet_name;
        sheet_data["majorDimension"] = "ROWS";

        vector<vector<string>> values;

        for (idx_t r = 0; r < input.size(); r++)
        {
            vector<string> row;
            for (idx_t c = 0; c < input.ColumnCount(); c++)
            {
                auto &col = input.data[c];
                Value val = col.GetValue(r);
                if (val.IsNull())
                {
                    row.push_back("");
                }
                else
                {
                    row.push_back(val.ToString());
                }
            }
            values.push_back(row);
        }
        sheet_data["values"] = values;

        // Convert the JSON object to a string
        std::string request_body = sheet_data.dump();

        // Make the API call to write data to the Google Sheet
        // Today, this is only append.
        std::string response = call_sheets_api(gstate.spreadsheet_id, gstate.token, encoded_sheet_name, HttpMethod::POST, request_body);

        // Check for errors in the response
        json response_json = parseJson(response);
        if (response_json.contains("error"))
        {
            throw duckdb::IOException("Error writing to Google Sheet: " + response_json["error"]["message"].get<std::string>());
        }
    }
} // namespace duckdb