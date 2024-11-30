#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/main/secret/secret_manager.hpp"
#include "notion_requests.hpp"
#include "notion_utils.hpp"
#include "notion_read.hpp"
#include <json.hpp>

namespace duckdb
{

    using json = nlohmann::json;

    std::string get_notion_token(ClientContext &context)
    {
        // Use the SecretManager to get the token
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
        return token;
    }

    // https://developers.notion.com/reference/property-object
    static LogicalType notion_type_to_duckdb_type(const std::string &notion_type)
    {
        if (notion_type == "title" || notion_type == "rich_text" ||
            notion_type == "url" || notion_type == "email" ||
            notion_type == "phone_number" || notion_type == "select" ||
            notion_type == "multi_select" || notion_type == "status")
        {
            return LogicalType::VARCHAR;
        }
        else if (notion_type == "number")
        {
            return LogicalType::DOUBLE;
        }
        else if (notion_type == "checkbox")
        {
            return LogicalType::BOOLEAN;
        }
        else if (notion_type == "date" || notion_type == "created_time" ||
                 notion_type == "last_edited_time")
        {
            return LogicalType::TIMESTAMP;
        }
        else if (notion_type == "people" || notion_type == "created_by" ||
                 notion_type == "last_edited_by" || notion_type == "files" ||
                 notion_type == "relation")
        {
            // These types contain complex objects, return as JSON strings
            return LogicalType::VARCHAR;
        }
        else if (notion_type == "formula" || notion_type == "rollup")
        {
            // Formula and rollup types can have different return types
            // For simplicity, return as VARCHAR and let the user parse if needed
            return LogicalType::VARCHAR;
        }

        // Default to VARCHAR for unknown types
        return LogicalType::VARCHAR;
    }

    // TODO
    void notion_read_function(ClientContext &context, TableFunctionInput &data_p, DataChunk &output)
    {
        auto &bind_data = const_cast<NotionReadFunctionData &>(data_p.bind_data->Cast<NotionReadFunctionData>());
        std::string database_id = bind_data.database_id;
        std::string token = get_notion_token(context);

        // TODO
    }

    // TODO
    unique_ptr<FunctionData> notion_bind_function(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names)
    {
        auto database_input = input.inputs[0].GetValue<string>();
        std::string database_id = extract_database_id(database_input);
        std::string token = get_notion_token(context);

        // TODO: Now fetch the metadata for the database, and use that to determine the columns
        std::string database_metadata = get_database(token, database_id);
        auto database_metadata_json = json::parse(database_metadata);
        auto properties = database_metadata_json["properties"];
        for (const auto &property : properties.items())
        {
            std::string property_name = property.key();
            std::string property_id = property.value()["id"];
            std::string property_type = property.value()["type"];

            names.push_back(property_name);
            return_types.push_back(notion_type_to_duckdb_type(property_type));

            // TODO: Add the property to the columns
        }

        auto bind_data = make_uniq<NotionReadFunctionData>(database_id);
        return bind_data;
    }
} // namespace duckdb
