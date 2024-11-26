#include "notion_requests.hpp"
#include "duckdb/common/exception.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

namespace duckdb
{
    const std::string NOTION_API_HOST = "api.notion.com";

    std::string perform_https_request(const std::string &host, const std::string &path, const std::string &token,
                                      HttpMethod method, const std::string &body, const std::string &content_type)
    {
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

    // TODO: pagination - collect all databases if multiple requests are needed
    std::string list_databases(const std::string &token)
    {
        return call_notion_api(token, HttpMethod::POST, "/v1/search", "{\"filter\":{\"value\":\"database\", \"property\":\"object\"}}");
    }

    // This gets the schema of the database
    std::string get_database(const std::string &token, const std::string &database_id)
    {
        return call_notion_api(token, HttpMethod::GET, "/v1/databases/" + database_id, "");
    }

    // TODO: pagination, maybe filter if needed
    std::string query_database(const std::string &token, const std::string &database_id)
    {
        return call_notion_api(token, HttpMethod::POST, "/v1/databases/" + database_id + "/query", "{}");
    }

    // TODO: update database - CRUD on database rows
    std::string create_page(const std::string &token, const std::string &database_id, const std::string &body)
    {
        return call_notion_api(token, HttpMethod::POST, "/v1/databases/" + database_id + "/query", body);
    }

    std::string update_page_properties(const std::string &token, const std::string &page_id, const std::string &body)
    {
        return call_notion_api(token, HttpMethod::PATCH, "/v1/pages/" + page_id, body);
    }

    std::string delete_page(const std::string &token, const std::string &page_id)
    {
        return call_notion_api(token, HttpMethod::DELETE, "/v1/pages/" + page_id, "");
    }

    // TODO: create and delete databases?
}