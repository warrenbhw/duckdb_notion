#pragma once

#include <string>
#include <vector>

namespace duckdb
{

    enum class HttpMethod
    {
        GET,
        POST,
        PUT,
        PATCH,
        DELETE
    };

    std::string perform_https_request(const std::string &host, const std::string &path, const std::string &token,
                                      HttpMethod method, const std::string &body, const std::string &content_type);
    std::string call_notion_api(const std::string &token, HttpMethod method, const std::string &path, const std::string &body);
    std::string get_database(const std::string &token, const std::string &database_id);
    std::string query_database(const std::string &token, const std::string &database_id);

    // std::string list_databases(const std::string &token);
    // std::string create_page(const std::string &token, const std::string &database_id, const std::string &body);
    // std::string update_page_properties(const std::string &token, const std::string &page_id, const std::string &body);

} // namespace duckdb