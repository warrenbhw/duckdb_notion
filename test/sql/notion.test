# name: test/sql/notion.test
# description: test notion extension
# group: [notion]

require notion

# Test API key configuration
statement ok
SET notion_api_key='test_key';

# Test database query
query I
SELECT * FROM notion_scan_database('database_id');
----
<result>

# Test error handling
statement error
SELECT * FROM notion_scan_database(NULL);
----
Notion database ID cannot be NULL

# Test invalid API key
statement error
SET notion_api_key='invalid_key';
SELECT * FROM notion_scan_database('database_id');
----
Invalid Notion API key or insufficient permissions

# Load the extension
LOAD 'build/release/extension/duckdb_notion/duckdb_notion.duckdb_extension';

# Set up environment variables (this will be loaded from .env in practice)
SET notion_api_key='test_api_key';

# Test listing databases
# First try without API key (should fail)
statement error
SELECT * FROM notion_list_databases();

# Set invalid API key (should fail with auth error)
SET notion_api_key='invalid_key';
statement error
SELECT * FROM notion_list_databases();

# Test with valid API key (assuming .env has valid key)
SET notion_api_key='${NOTION_API_KEY}';
query I
SELECT COUNT(*) > 0 FROM notion_list_databases();
----
true

# Test database schema retrieval
# Invalid database ID should fail
statement error
SELECT * FROM notion_database_schema('invalid_id');

# Valid database ID should return schema
query III
SELECT 
    name,
    type,
    COUNT(*) > 0 as has_properties 
FROM notion_database_schema('${TEST_DATABASE_ID}')
GROUP BY name, type;
----

# Test database query
# Invalid database ID should fail
statement error
SELECT * FROM notion_database('invalid_id');

# Valid database ID should return data
query I
SELECT COUNT(*) > 0 FROM notion_database('${TEST_DATABASE_ID}');
----
true

# Test database query with specific columns
query II
SELECT 
    title,  # assuming there's a title column
    COUNT(*) as count
FROM notion_database('${TEST_DATABASE_ID}')
GROUP BY title
HAVING COUNT(*) > 0
LIMIT 1;
----
