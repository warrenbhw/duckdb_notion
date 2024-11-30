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

    static std::string GetNotionToken(ClientContext &context)
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

    // TODO
    void NotionReadFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output)
    {
        auto &bind_data = const_cast<NotionReadFunctionData &>(data_p.bind_data->Cast<NotionReadFunctionData>());
        std::string database_id = bind_data.database_id;
        std::string token = GetNotionToken(context);

        // TODO
    }

    // TODO
    unique_ptr<FunctionData> NotionBindFunction(ClientContext &context, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names)
    {
        auto database_input = input.inputs[0].GetValue<string>();
        std::string database_id = extract_database_id(database_input);
        std::string token = GetNotionToken(context);

        // TODO: Now fetch the metadata for the database, and use that to determine the columns

        auto bind_data = make_uniq<NotionReadFunctionData>(database_id);
        return bind_data;
    }
} // namespace duckdb
