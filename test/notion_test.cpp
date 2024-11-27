#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "notion_requests.hpp"
#include "notion_utils.hpp"
#include "duckdb/common/types/value.hpp"
#include <fstream>

using namespace duckdb;

// Helper to load API key from .env file
std::string load_api_key()
{
    std::ifstream env_file(".env");
    std::string line;
    while (std::getline(env_file, line))
    {
        if (line.find("NOTION_API_KEY=") == 0)
        {
            return line.substr(14); // Length of "NOTION_API_KEY="
        }
    }
    return "";
}

TEST_CASE("NotionUtils - Database ID extraction", "[notion]")
{
    SECTION("Extract from full Notion URL")
    {
        std::string url = "https://www.notion.so/workspace/1499ce5d31c980249613ee3558225560?v=51c255cb2ead4c539bf90457b849a66e";
        std::string id = extract_database_id(url);
        REQUIRE(id == "1499ce5d31c980249613ee3558225560");
    }

    SECTION("Extract from simple Notion URL")
    {
        std::string url = "https://www.notion.so/1499ce5d31c980249613ee3558225560";
        std::string id = extract_database_id(url);
        REQUIRE(id == "1499ce5d31c980249613ee3558225560");
    }

    SECTION("Pass through valid ID")
    {
        std::string id = "1499ce5d31c980249613ee3558225560";
        REQUIRE(extract_database_id(id) == id);
    }

    SECTION("Invalid input throws")
    {
        REQUIRE_THROWS(extract_database_id("invalid-id"));
    }
}

TEST_CASE("NotionUtils - Property type parsing", "[notion]")
{
    SECTION("Text types")
    {
        REQUIRE(ParseNotionPropertyType("title") == NotionPropertyType::TEXT);
        REQUIRE(ParseNotionPropertyType("rich_text") == NotionPropertyType::TEXT);
    }

    SECTION("Other types")
    {
        REQUIRE(ParseNotionPropertyType("number") == NotionPropertyType::NUMBER);
        REQUIRE(ParseNotionPropertyType("checkbox") == NotionPropertyType::CHECKBOX);
        REQUIRE(ParseNotionPropertyType("date") == NotionPropertyType::DATE);
    }

    SECTION("Unknown type defaults to TEXT")
    {
        REQUIRE(ParseNotionPropertyType("unknown_type") == NotionPropertyType::TEXT);
    }
}

TEST_CASE("NotionUtils - Pagination", "[notion]")
{
    SECTION("Single page")
    {
        int call_count = 0;
        auto result = collect_paginated_results([&](const std::string &cursor)
                                                {
            call_count++;
            return R"({
                "results": [{"id": "test1"}, {"id": "test2"}],
                "has_more": false
            })"; });

        auto parsed = nlohmann::json::parse(result);
        REQUIRE(call_count == 1);
        REQUIRE(parsed["results"].size() == 2);
    }

    SECTION("Multiple pages")
    {
        int call_count = 0;
        auto result = collect_paginated_results([&](const std::string &cursor)
                                                {
            call_count++;
            if (cursor.empty()) {
                return R"({
                    "results": [{"id": "page1"}],
                    "has_more": true,
                    "next_cursor": "cursor1"
                })";
            } else {
                return R"({
                    "results": [{"id": "page2"}],
                    "has_more": false
                })";
            } });

        auto parsed = nlohmann::json::parse(result);
        REQUIRE(call_count == 2);
        REQUIRE(parsed["results"].size() == 2);
        REQUIRE(parsed["results"][0]["id"] == "page1");
        REQUIRE(parsed["results"][1]["id"] == "page2");
    }
}

TEST_CASE("NotionRequests - API Integration", "[notion][integration]")
{
    std::string api_key = load_api_key();
    if (api_key.empty())
    {
        WARN("Skipping integration tests - no API key found in .env");
        return;
    }

    SECTION("List databases")
    {
        std::string result = list_databases(api_key);
        auto parsed = nlohmann::json::parse(result);
        REQUIRE(parsed.contains("results"));
        REQUIRE(parsed["results"].is_array());
    }

    SECTION("Get database schema")
    {
        // This requires a valid database ID from your workspace
        std::string database_id = "your_test_database_id";
        auto properties = GetDatabaseSchema(api_key, database_id);
        REQUIRE(!properties.empty());

        // Check that we have at least one property with a valid type
        bool has_valid_property = false;
        for (const auto &prop : properties)
        {
            if (!prop.name.empty() &&
                (prop.type == NotionPropertyType::TEXT ||
                 prop.type == NotionPropertyType::NUMBER ||
                 prop.type == NotionPropertyType::DATE))
            {
                has_valid_property = true;
                break;
            }
        }
        REQUIRE(has_valid_property);
    }
}