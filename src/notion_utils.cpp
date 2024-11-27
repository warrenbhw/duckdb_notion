#include "notion_utils.hpp"
#include "notion_requests.hpp"
#include "duckdb/common/exception.hpp"
#include <regex>
#include <json.hpp>
#include <iostream>
#include <sstream>

using json = nlohmann::json;
namespace duckdb
{
    // Examples inputs:
    // https://www.notion.so/1499ce5d31c980249613ee3558225560?v=51c255cb2ead4c539bf90457b849a66e
    // https://www.notion.so/1499ce5d31c980249613ee3558225560
    // 1499ce5d31c980249613ee3558225560
    //
    // Output: 1499ce5d31c980249613ee3558225560
    std::string extract_database_id(const std::string &input)
    {
        // If input is already just an ID (32 hex characters), return it
        std::regex id_only_pattern("^[a-fA-F0-9]{32}$");
        if (std::regex_match(input, id_only_pattern))
        {
            return input;
        }

        // Extract ID from Notion URL
        std::regex notion_url_pattern("notion\\.so/(?:[^/]+/)?([a-fA-F0-9]{32})(?:\\?.*)?$");
        std::smatch match;
        if (std::regex_search(input, match, notion_url_pattern) && match.size() > 1)
        {
            return match.str(1);
        }

        throw duckdb::InvalidInputException("Invalid Notion database URL or ID format");
    }

    std::string extract_spreadsheet_id(const std::string &input)
    {
        // Check if the input is already a sheet ID (no slashes)
        if (input.find('/') == std::string::npos)
        {
            return input;
        }

        // Regular expression to match the spreadsheet ID in a Google Sheets URL
        if (input.find("docs.google.com/spreadsheets/d/") != std::string::npos)
        {
            std::regex spreadsheet_id_regex("/d/([a-zA-Z0-9-_]+)");
            std::smatch match;

            if (std::regex_search(input, match, spreadsheet_id_regex) && match.size() > 1)
            {
                return match.str(1);
            }
        }

        throw duckdb::InvalidInputException("Invalid Google Sheets URL or ID");
    }

    json parseJson(const std::string &json_str)
    {
        try
        {
            // Find the start of the JSON object
            size_t start = json_str.find('{');
            if (start == std::string::npos)
            {
                throw std::runtime_error("No JSON object found in the response");
            }

            // Find the end of the JSON object
            size_t end = json_str.rfind('}');
            if (end == std::string::npos)
            {
                throw std::runtime_error("No closing brace found in the JSON response");
            }

            // Extract the JSON object
            std::string clean_json = json_str.substr(start, end - start + 1);

            json j = json::parse(clean_json);
            return j;
        }
        catch (const json::exception &e)
        {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
            std::cerr << "Raw JSON string: " << json_str << std::endl;
            throw;
        }
    }

    std::string generate_random_string(size_t length)
    {
        static const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<> distribution(0, sizeof(charset) - 2);

        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i)
        {
            result.push_back(charset[distribution(generator)]);
        }
        return result;
    }

    std::string url_encode(const std::string &str)
    {
        std::string encoded;
        for (char c : str)
        {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                encoded += c;
            }
            else
            {
                std::stringstream ss;
                ss << std::hex << std::uppercase << static_cast<int>(static_cast<unsigned char>(c));
                encoded += '%' + ss.str();
            }
        }
        return encoded;
    }

    std::string collect_paginated_results(
        std::function<std::string(const std::string &cursor)> fetch_page)
    {
        std::string result;
        std::string cursor;
        bool has_more = true;

        while (has_more)
        {
            // Use the provided function to fetch each page
            std::string response = fetch_page(cursor);
            auto json_response = nlohmann::json::parse(response);

            if (result.empty())
            {
                result = response;
            }
            else
            {
                auto result_json = nlohmann::json::parse(result);
                auto &results = result_json["results"];
                results.insert(results.end(), json_response["results"].begin(), json_response["results"].end());
                result = result_json.dump();
            }

            has_more = json_response["has_more"].get<bool>();
            if (has_more)
            {
                cursor = json_response["next_cursor"].get<std::string>();
            }
        }

        return result;
    }

} // namespace duckdb