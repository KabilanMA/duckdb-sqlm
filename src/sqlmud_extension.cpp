#define DUCKDB_EXTENSION_MAIN

#include "sqlmud_extension.hpp"
#include "duckdb.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/function/scalar_function.hpp"
#include "duckdb/main/extension_util.hpp"
#include <duckdb/parser/parsed_data/create_scalar_function_info.hpp>

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

namespace duckdb
{

    inline void SqlmudScalarFun(DataChunk &args, ExpressionState &state, Vector &result)
    {
        auto &name_vector = args.data[0];
        UnaryExecutor::Execute<string_t, string_t>(
            name_vector, result, args.size(),
            [&](string_t name)
            {
                return StringVector::AddString(result, "Sqlmud " + name.GetString() + " 🐥");
                ;
            });
    }

    inline void SqlmudOpenSSLVersionScalarFun(DataChunk &args, ExpressionState &state, Vector &result)
    {
        auto &name_vector = args.data[0];
        UnaryExecutor::Execute<string_t, string_t>(
            name_vector, result, args.size(),
            [&](string_t name)
            {
                return StringVector::AddString(result, "Sqlmud " + name.GetString() +
                                                           ", my linked OpenSSL version is " +
                                                           OPENSSL_VERSION_TEXT);
                ;
            });
    }

    struct MutationTestFunctionData : public TableFunctionData
    {
        MutationTestFunctionData(string query) : original_query(query) {}

        string original_query;
        vector<string> mutated_queries;
        bool finished = false;
        size_t current_index = 0;
    };

    vector<string> GenerateMutations(const string &query)
    {
        vector<string> mutations;
        std::cout << "Calling the actual mutation function." << std::endl;

        // Example mutation 1: Change "=" to "!="
        string mutation1 = query;
        size_t pos = mutation1.find("=");
        if (pos != string::npos)
        {
            mutation1.replace(pos, 1, "!=");
            mutations.push_back(mutation1);
        }

        // Example mutation 2: Replace "SELECT" with "SELECT DISTINCT"
        string mutation2 = query;
        pos = mutation2.find("SELECT");
        if (pos != string::npos)
        {
            mutation2.replace(pos, 6, "SELECT DISTINCT");
            mutations.push_back(mutation2);
        }

        // Example mutation 3: Add LIMIT 10 to the query
        string mutation3 = query + " LIMIT 10";
        mutations.push_back(mutation3);

        // Add more mutations as needed...

        return mutations;
    }

    static duckdb::unique_ptr<FunctionData> MutationTestBind(ClientContext &context, TableFunctionBindInput &input,
                                                             vector<LogicalType> &return_types, vector<string> &names)
    {
        auto result = make_uniq<MutationTestFunctionData>(StringValue::Get(input.inputs[0]));
        std::cout << "Binding Mutation Test Function!" << std::endl;
        // Generate multiple mutations for the input query
        result->mutated_queries = GenerateMutations(result->original_query);

        // Return type is a string for the mutated query
        return_types.emplace_back(LogicalType::VARCHAR);
        names.emplace_back("mutant");
        return std::move(result);
    }

    static void MutationTestFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output)
    {
        auto &data = data_p.bind_data->CastNoConst<MutationTestFunctionData>();

        std::cout << "Calling the mutation test function" << std::endl;
        std::cout << data.mutated_queries[data.current_index] << std::endl;

        // If all mutations are finished, set the output as empty
        if (data.current_index >= data.mutated_queries.size())
        {
            data.finished = true;
            return;
        }

        // Set the output to the current mutation
        output.SetCardinality(1);
        output.SetValue(0, 0, Value(data.mutated_queries[data.current_index]));

        // Move to the next mutation for the next call
        data.current_index++;
    }

    static void LoadInternal(DatabaseInstance &instance)
    {
        // Register a scalar function
        auto sqlmud_scalar_function = ScalarFunction("sqlmud", {LogicalType::VARCHAR}, LogicalType::VARCHAR, SqlmudScalarFun);
        ExtensionUtil::RegisterFunction(instance, sqlmud_scalar_function);

        // Register another scalar function
        auto sqlmud_openssl_version_scalar_function = ScalarFunction("sqlmud_openssl_version", {LogicalType::VARCHAR},
                                                                     LogicalType::VARCHAR, SqlmudOpenSSLVersionScalarFun);
        ExtensionUtil::RegisterFunction(instance, sqlmud_openssl_version_scalar_function);

        // Register the mutation_test function
        TableFunction mutation_test_func("mutation_test", {LogicalType::VARCHAR}, MutationTestFunction, MutationTestBind);
        ExtensionUtil::RegisterFunction(instance, mutation_test_func);
    }

    void SqlmudExtension::Load(DuckDB &db)
    {
        LoadInternal(*db.instance);
    }
    std::string SqlmudExtension::Name()
    {
        return "sqlmud";
    }

    std::string SqlmudExtension::Version() const
    {
#ifdef EXT_VERSION_SQLMUD
        return EXT_VERSION_SQLMUD;
#else
        return "";
#endif
    }

} // namespace duckdb

extern "C"
{

    DUCKDB_EXTENSION_API void sqlmud_init(duckdb::DatabaseInstance &db)
    {
        duckdb::DuckDB db_wrapper(db);
        db_wrapper.LoadExtension<duckdb::SqlmudExtension>();
    }

    DUCKDB_EXTENSION_API const char *sqlmud_version()
    {
        return duckdb::DuckDB::LibraryVersion();
    }
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
