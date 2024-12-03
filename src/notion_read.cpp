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

        // Query the database
        std::string response = query_database(token, database_id);
        auto json_response = parse_json(response);

        if (!json_response.contains("results"))
        {
            throw IOException("Invalid response from Notion API: no results found");
        }

        // Process each page/row
        auto &results = json_response["results"];
        idx_t row_index = 0;

        for (const auto &page : results)
        {
            if (!page.contains("properties"))
            {
                continue;
            }

            // Process each property/column
            auto &properties = page["properties"];
            idx_t col_index = 0;

            for (const auto &property : properties.items())
            {
                const auto &prop_value = property.value();
                const auto &prop_type = prop_value["type"].get<std::string>();

                // Extract value based on property type
                if (prop_type == "title" || prop_type == "rich_text")
                {
                    auto &text_array = prop_value[prop_type];
                    std::string text = text_array.empty() ? "" : text_array[0]["plain_text"].get<std::string>();
                    output.SetValue(col_index, row_index, Value(text));
                }
                else if (prop_type == "number")
                {
                    if (prop_value["number"].is_null())
                    {
                        output.SetValue(col_index, row_index, Value());
                    }
                    else
                    {
                        double number = prop_value["number"].get<double>();
                        output.SetValue(col_index, row_index, Value(number));
                    }
                }
                else if (prop_type == "date")
                {
                    if (prop_value["date"].is_null())
                    {
                        output.SetValue(col_index, row_index, Value());
                    }
                    else
                    {
                        std::string date = prop_value["date"]["start"].get<std::string>();
                        output.SetValue(col_index, row_index, Value::TIMESTAMP(Timestamp::FromString(date)));
                    }
                }
                else if (prop_type == "multi_select")
                {
                    auto tags = prop_value["multi_select"];
                    std::string tag_list;
                    for (size_t i = 0; i < tags.size(); i++)
                    {
                        if (i > 0)
                            tag_list += ", ";
                        tag_list += tags[i]["name"].get<std::string>();
                    }
                    output.SetValue(col_index, row_index, Value(tag_list));
                }
                else
                {
                    // Default to empty string for unsupported types
                    output.SetValue(col_index, row_index, Value());
                }

                col_index++;
            }
            row_index++;
        }

        output.SetCardinality(row_index);
    }

    unique_ptr<FunctionData> notion_bind_function(ClientContext &context, TableFunctionBindInput &input,
                                                  vector<LogicalType> &return_types, vector<string> &names)
    {
        auto database_input = input.inputs[0].GetValue<string>();
        std::string database_id = extract_database_id(database_input);
        std::string token = get_notion_token(context);

        // Get the database schema
        std::string database_metadata = get_database(token, database_id);
        auto database_metadata_json = parse_json(database_metadata);
        auto properties = database_metadata_json["properties"];
        for (const auto &property : properties.items())
        {
            std::string name = property.key();
            std::string type = property.value()["type"];

            names.push_back(name);
            return_types.push_back(notion_type_to_duckdb_type(type));
        }

        // Create the bind data
        auto bind_data = make_uniq<NotionReadFunctionData>(database_id);
        return bind_data;
    }
} // namespace duckdb
