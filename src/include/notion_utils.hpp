#pragma once

#include <string>
#include <vector>
#include <json.hpp>
#include <random>
#include <functional>

using json = nlohmann::json;

namespace duckdb
{

    // https://developers.notion.com/reference/property-object
    // enum class NotionPropertyType
    // {
    //     CHECKBOX,
    //     CREATED_BY,
    //     CREATED_TIME,
    //     DATE,
    //     EMAIL,
    //     FILES,
    //     FORMULA,
    //     LAST_EDITED_BY,
    //     LAST_EDITED_TIME,
    //     MULTI_SELECT,
    //     NUMBER,
    //     PEOPLE,
    //     PHONE_NUMBER,
    //     RELATION,
    //     RICH_TEXT,
    //     ROLLUP,
    //     SELECT,
    //     STATUS,
    //     TITLE,
    //     URL,
    // };

    /**
     * Extracts the database ID from a Notion URL or returns the input if it's already a database ID.
     * @param input A Notion URL or database ID
     * @return The extracted database ID
     * @throws InvalidInputException if the input is neither a valid URL nor a database ID
     */
    std::string extract_database_id(const std::string &input);

    /**
     * Parses a JSON string into a json object
     * @param json_str The JSON string
     * @return The parsed json object
     */
    json parse_json(const std::string &json_str);

    /**
     * Generates a random string of specified length using alphanumeric characters.
     * @param length The length of the random string to generate
     * @return A random string of the specified length
     */
    std::string generate_random_string(size_t length);

    /**
     * Encodes a string to be used in a URL
     * @param str The string to encode
     * @return The encoded string
     */
    std::string url_encode(const std::string &str);

    std::string collect_paginated_results(
        std::function<std::string(const std::string &cursor)> fetch_page);

} // namespace duckdb