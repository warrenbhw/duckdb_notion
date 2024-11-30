#include "notion_auth.hpp"
#include "notion_requests.hpp"
#include "notion_utils.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/main/secret/secret.hpp"
#include "duckdb/main/extension_util.hpp"
#include <fstream>
#include <cstdlib>

namespace duckdb
{

    std::string read_token_from_file(const std::string &file_path)
    {
        std::ifstream file(file_path);
        if (!file.is_open())
        {
            throw duckdb::IOException("Unable to open token file: " + file_path);
        }
        std::string token;
        std::getline(file, token);
        return token;
    }

    // This code is copied, with minor modifications from https://github.com/duckdb/duckdb_azure/blob/main/src/azure_secret.cpp
    static void CopySecret(const std::string &key, const CreateSecretInput &input, KeyValueSecret &result)
    {
        auto val = input.options.find(key);

        if (val != input.options.end())
        {
            result.secret_map[key] = val->second;
        }
    }

    static void RegisterCommonSecretParameters(CreateSecretFunction &function)
    {
        // Register google sheets common parameters
        function.named_parameters["token"] = LogicalType::VARCHAR;
    }

    static void RedactCommonKeys(KeyValueSecret &result)
    {
        result.redact_keys.insert("proxy_password");
    }

    // TODO: Maybe this should be a KeyValueSecret
    static unique_ptr<BaseSecret> CreateNotionSecretFromAccessToken(ClientContext &context, CreateSecretInput &input)
    {
        auto scope = input.scope;

        auto result = make_uniq<KeyValueSecret>(scope, input.type, input.provider, input.name);

        // Manage specific secret option
        CopySecret("token", input, *result);

        // Redact sensible keys
        RedactCommonKeys(*result);
        result->redact_keys.insert("token");

        return std::move(result);
    }

    static unique_ptr<BaseSecret> CreateNotionSecretFromOAuth(ClientContext &context, CreateSecretInput &input)
    {
        auto scope = input.scope;

        auto result = make_uniq<KeyValueSecret>(scope, input.type, input.provider, input.name);

        // Initiate OAuth flow
        string token = InitiateOAuthFlow();

        result->secret_map["token"] = token;

        // Redact sensible keys
        RedactCommonKeys(*result);
        result->redact_keys.insert("token");

        return std::move(result);
    }

    void CreateNotionSecretFunctions::Register(DatabaseInstance &instance)
    {
        string type = "notion";

        // Register the new type
        SecretType secret_type;
        secret_type.name = type;
        secret_type.deserializer = KeyValueSecret::Deserialize<KeyValueSecret>;
        secret_type.default_provider = "oauth";
        ExtensionUtil::RegisterSecretType(instance, secret_type);

        // Register the access_token secret provider
        CreateSecretFunction access_token_function = {type, "access_token", CreateNotionSecretFromAccessToken};
        access_token_function.named_parameters["access_token"] = LogicalType::VARCHAR;
        RegisterCommonSecretParameters(access_token_function);
        ExtensionUtil::RegisterFunction(instance, access_token_function);

        // Register the oauth secret provider
        CreateSecretFunction oauth_function = {type, "oauth", CreateNotionSecretFromOAuth};
        oauth_function.named_parameters["use_oauth"] = LogicalType::BOOLEAN;
        RegisterCommonSecretParameters(oauth_function);
        ExtensionUtil::RegisterFunction(instance, oauth_function);
    }

    std::string InitiateOAuthFlow()
    {
        // This is using the Web App OAuth flow, as I can't figure out desktop app flow.
        const std::string client_id = "14ed872b-594c-805b-ace4-0037cce1cc3e";
        const std::string redirect_uri = "https://duckdb-notion.com/oauth";
        const std::string auth_url = "https://api.notion.com/v1/oauth/authorize";

        // Generate a random state for CSRF protection
        std::string state = generate_random_string(10);

        std::string auth_request_url = auth_url + "?client_id=" + client_id +
                                       "&redirect_uri=" + redirect_uri +
                                       "&response_type=code" +
                                       "&owner=user" +
                                       "&state=" + state;

        // Instruct the user to visit the URL and grant permission
        std::cout << "Visit the below URL to authorize DuckDB Notion" << std::endl
                  << std::endl;
        std::cout << auth_request_url << std::endl
                  << std::endl;

// Open the URL in the user's default browser
#ifdef _WIN32
        system(("start \"\" \"" + auth_request_url + "\"").c_str());
#elif __APPLE__
        system(("open \"" + auth_request_url + "\"").c_str());
#elif __linux__
        system(("xdg-open \"" + auth_request_url + "\"").c_str());
#endif

        std::cout << "After granting permission, enter the token: ";
        std::string access_token;
        std::cin >> access_token;

        return access_token;
    }

} // namespace duckdb