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

    // TODO
    void NotionReadFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output)
    {
        auto &bind_data = const_cast<NotionReadFunctionData &>(data_p.bind_data->Cast<NotionReadFunctionData>());
    }

    // TODO
    unique_ptr<FunctionData> NotionBindFunction(ClientContext &context, TableFunctionBindInput &input,
                                                vector<LogicalType> &return_types, vector<string> &names)
    {
    }
} // namespace duckdb
