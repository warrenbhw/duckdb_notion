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

    ReadDatabaseBindData::ReadDatabaseBindData(string database_id, string token, bool header)
        : database_id(database_id), token(token), finished(false), row_index(0), header(header)
    {
        // response = call_notion_api(token, HttpMethod::GET, "/v1/databases/" + database_id, "");
        response = "";
    }

    void NotionReadFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output)
    {
        // TODO
        auto &bind_data = const_cast<ReadDatabaseBindData &>(data_p.bind_data->Cast<ReadDatabaseBindData>());

        if (bind_data.finished)
        {
            return;
        }

        // json cleanJson = parseJson(bind_data.response);
        // DatabaseData database_data = getDatabaseData(cleanJson);
    }

    unique_ptr<FunctionData> NotionBindFunction(ClientContext &context, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names)
    {
        // TODO
    }
} // namespace duckdb
