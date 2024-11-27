#include "notion_read.hpp"
#include "notion_utils.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/main/secret/secret_manager.hpp"
#include "notion_requests.hpp"
#include <json.hpp>

namespace duckdb
{

    using json = nlohmann::json;

    ReadSheetBindData::ReadSheetBindData(string database_id, string token, bool header)
        : database_id(database_id), token(token), finished(false), row_index(0), header(header)
    {
        // response = call_notion_api(token, HttpMethod::GET, "/v1/databases/" + database_id, "");
        response = "";
    }
} // namespace duckdb
