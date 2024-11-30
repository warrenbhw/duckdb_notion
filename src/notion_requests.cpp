#include "notion_requests.hpp"
#include "duckdb/common/exception.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <json.hpp>
#include "notion_utils.hpp"
#include "duckdb/common/types/value.hpp"
#include <iostream>

namespace duckdb
{
    const std::string NOTION_API_HOST = "api.notion.com";
    const std::string NOTION_API_VERSION = "2022-02-22";

    std::string perform_https_request(const std::string &host, const std::string &path, const std::string &token,
                                      HttpMethod method, const std::string &body, const std::string &content_type = "application/json")
    {
        std::cout << "Performing HTTPS request to: " << host << path << std::endl;
        std::cout << "Token: " << token << std::endl;
        std::cout << "Body: " << body << std::endl;
        std::cout << "Content type: " << content_type << std::endl;
        // std::cout << "Method: " << method << std::endl;

        std::string response;
        SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx)
        {
            throw duckdb::IOException("Failed to create SSL context");
        }

        BIO *bio = BIO_new_ssl_connect(ctx);
        SSL *ssl;
        BIO_get_ssl(bio, &ssl);
        SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

        BIO_set_conn_hostname(bio, (host + ":443").c_str());

        if (BIO_do_connect(bio) <= 0)
        {
            BIO_free_all(bio);
            SSL_CTX_free(ctx);
            throw duckdb::IOException("Failed to connect");
        }

        std::string method_str;
        switch (method)
        {
        case HttpMethod::GET:
            method_str = "GET";
            break;
        case HttpMethod::POST:
            method_str = "POST";
            break;
        case HttpMethod::PUT:
            method_str = "PUT";
            break;
        case HttpMethod::PATCH:
            method_str = "PATCH";
            break;
        case HttpMethod::DELETE:
            method_str = "DELETE";
            break;
        }

        std::string request = method_str + " " + path + " HTTP/1.0\r\n";
        request += "Host: " + host + "\r\n";
        request += "Authorization: Bearer " + token + "\r\n";
        request += "Connection: close\r\n";

        if (!body.empty())
        {
            request += "Content-Type: " + content_type + "\r\n";
            request += "Content-Length: " + std::to_string(body.length()) + "\r\n";
        }

        request += "Notion-Version: " + NOTION_API_VERSION + "\r\n";

        request += "\r\n";

        if (!body.empty())
        {
            request += body;
        }

        if (BIO_write(bio, request.c_str(), request.length()) <= 0)
        {
            BIO_free_all(bio);
            SSL_CTX_free(ctx);
            throw duckdb::IOException("Failed to write request");
        }

        char buffer[1024];
        int len;
        while ((len = BIO_read(bio, buffer, sizeof(buffer))) > 0)
        {
            response.append(buffer, len);
        }

        BIO_free_all(bio);
        SSL_CTX_free(ctx);

        // Extract body from response
        size_t body_start = response.find("\r\n\r\n");
        if (body_start != std::string::npos)
        {
            return response.substr(body_start + 4);
        }

        return response;
    }

    std::string call_notion_api(const std::string &token, HttpMethod method, const std::string &path, const std::string &body)
    {
        return perform_https_request(NOTION_API_HOST, path, token, method, body);
    }

    std::string get_database(const std::string &token, const std::string &database_id)
    {
        std::cout << "Getting database: " << database_id << std::endl;
        std::cout << "Token: " << token << std::endl;

        auto response = call_notion_api(token, HttpMethod::GET, "/v1/databases/" + database_id, "");
        std::cout << "Response: " << response << std::endl;
        return response;
    }

    // std::string list_databases(const std::string &token)
    // {
    //     nlohmann::json initial_body = {
    //         {"filter", {{"value", "database"}, {"property", "object"}}}};

    //     return collect_paginated_results([&](const std::string &cursor)
    //                                      {
    //         nlohmann::json request_body = initial_body;
    //         if (!cursor.empty()) {
    //             request_body["start_cursor"] = cursor;
    //         }
    //         return call_notion_api(token, HttpMethod::POST, "/v1/search", request_body.dump()); });
    // }

    // // TODO: pagination, maybe filter if needed
    // std::string query_database(const std::string &token, const std::string &database_id)
    // {
    //     return call_notion_api(token, HttpMethod::POST, "/v1/databases/" + database_id + "/query", "{}");
    // }

    // // TODO: update database - CRUD on database rows
    // std::string create_page(const std::string &token, const std::string &database_id, const std::string &body)
    // {
    //     return call_notion_api(token, HttpMethod::POST, "/v1/databases/" + database_id + "/query", body);
    // }

    // std::string update_page_properties(const std::string &token, const std::string &page_id, const std::string &body)
    // {
    //     return call_notion_api(token, HttpMethod::PATCH, "/v1/pages/" + page_id, body);
    // }

    // std::string delete_page(const std::string &token, const std::string &page_id)
    // {
    //     return call_notion_api(token, HttpMethod::DELETE, "/v1/pages/" + page_id, "");
    // }

    // TODO: create and delete databases?

    // // Add function to extract property value from Notion response
    // Value ExtractPropertyValue(const nlohmann::json &property, NotionPropertyType type)
    // {
    //     switch (type)
    //     {
    //     case NotionPropertyType::TEXT:
    //     {
    //         if (property.contains("title"))
    //         {
    //             auto &title = property["title"];
    //             if (!title.empty() && title.contains("plain_text"))
    //             {
    //                 return Value(title["plain_text"].get<std::string>());
    //             }
    //         }
    //         else if (property.contains("rich_text"))
    //         {
    //             auto &text = property["rich_text"];
    //             if (!text.empty() && text.contains("plain_text"))
    //             {
    //                 return Value(text["plain_text"].get<std::string>());
    //             }
    //         }
    //         return Value();
    //     }
    //     case NotionPropertyType::NUMBER:
    //         return property.contains("number") ? Value(property["number"].get<double>()) : Value();
    //     case NotionPropertyType::DATE:
    //     {
    //         if (property.contains("date") && property["date"].contains("start"))
    //         {
    //             // Parse ISO 8601 date string
    //             std::string date_str = property["date"]["start"].get<std::string>();
    //             try
    //             {
    //                 return Value::TIMESTAMP(Timestamp::FromString(date_str));
    //             }
    //             catch (...)
    //             {
    //                 return Value();
    //             }
    //         }
    //         return Value();
    //     }
    //     case NotionPropertyType::CHECKBOX:
    //         return property.contains("checkbox") ? Value::BOOLEAN(property["checkbox"].get<bool>()) : Value();
    //     // Add other property type handling...
    //     default:
    //         return Value();
    //     }
    // }

    // std::vector<NotionProperty> GetDatabaseSchema(const std::string &token, const std::string &database_id)
    // {
    //     auto response = get_database(token, database_id);

    //     nlohmann::json d;
    //     d = nlohmann::json::parse(response);

    //     if (d.contains("error") || !d.contains("properties"))
    //     {
    //         throw IOException("Failed to parse Notion database schema");
    //     }

    //     std::vector<NotionProperty> properties;
    //     auto &props = d["properties"];

    //     for (auto &m : props.items())
    //     {
    //         NotionProperty property;
    //         property.name = m.key();
    //         property.type = ParseNotionPropertyType(m.value()["type"].get<std::string>());
    //         properties.push_back(property);
    //     }

    //     return properties;
    // }

    // std::vector<std::vector<Value>> QueryDatabaseRows(const std::string &token, const std::string &database_id,
    //                                                   const std::vector<NotionProperty> &properties)
    // {
    //     auto response = query_database(token, database_id);

    //     nlohmann::json d;
    //     d = nlohmann::json::parse(response);

    //     if (d.contains("error") || !d.contains("results"))
    //     {
    //         throw IOException("Failed to parse Notion database query results");
    //     }

    //     std::vector<std::vector<Value>> rows;
    //     auto &results = d["results"];

    //     for (auto &page : results)
    //     {
    //         std::vector<Value> row;
    //         auto &props = page["properties"];

    //         for (const auto &prop : properties)
    //         {
    //             if (props.contains(prop.name))
    //             {
    //                 row.push_back(ExtractPropertyValue(props[prop.name], prop.type));
    //             }
    //             else
    //             {
    //                 row.push_back(Value());
    //             }
    //         }

    //         rows.push_back(std::move(row));
    //     }

    //     return rows;
    // }

    // // Update create_page to handle property types correctly
    // std::string create_page(const std::string &token, const std::string &database_id,
    //                         const std::vector<std::pair<NotionProperty, Value>> &properties)
    // {
    //     nlohmann::json d = nlohmann::json::object();

    //     // Set parent database
    //     nlohmann::json parent(nlohmann::json::object());
    //     parent["database_id"] = database_id;
    //     d["parent"] = parent;

    //     // Add properties
    //     nlohmann::json props(nlohmann::json::object());
    //     for (const auto &prop : properties)
    //     {
    //         nlohmann::json prop_obj(nlohmann::json::object());
    //         // Convert DuckDB Value to Notion property format based on type
    //         // ... implement property conversion ...
    //         props[prop.first.name] = prop_obj;
    //     }
    //     d["properties"] = props;

    //     std::string buffer = d.dump();

    //     return call_notion_api(token, HttpMethod::POST, "/v1/pages", buffer);
    // }
}