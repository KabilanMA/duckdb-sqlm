#define DUCKDB_EXTENSION_MAIN

#include "sqlmud_extension.hpp"

// OpenSSL linked through vcpkg
#include <openssl/opensslv.h>

namespace duckdb
{

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

    duckdb_libpgquery::PGList *ParserQuery(ClientContext &context, const string &query)
    {
        PostgresParser::SetPreserveIdentifierCase(true);
        PostgresParser parser;
        try
        {
            // duckdb_parser.ParseQuery(query);
            parser.Parse(query);
        }
        catch (ParserException &err)
        {
            throw std::runtime_error("Query sent is not able to be parsed by sqlmud.");
        }

        if (parser.success)
        {
            if (!parser.parse_tree)
            {
                throw std::runtime_error("Empty query cannot be parsed by sqlmud.");
            }
            return parser.parse_tree;
        }
        else
        {
            throw std::runtime_error("Query sent is not able to be parsed by sqlmud.");
        }
        return nullptr;
    }

    // SELECT mutant FROM mutation_test("SELECT DISTINCT * FROM teacher WHERE salary > 10;");
    static duckdb::unique_ptr<FunctionData> MutationTestBind(ClientContext &context, TableFunctionBindInput &input,
                                                             vector<LogicalType> &return_types, vector<string> &names)
    {
        MutationTestFunctionData *functionData = new MutationTestFunctionData(StringValue::Get(input.inputs[0]));

        duckdb_libpgquery::PGList *parse_trees = ParserQuery(context, functionData->original_query);
        GenerateMutations(parse_trees, functionData);

        std::cout << "Trying to create connection for querying original SQL statement" << std::endl;
        Connection con(context.db->GetDatabase(context));
        std::cout << "Querying with original Query: " << functionData->original_query << std::endl;
        auto result_ = con.Query(functionData->original_query);
        functionData->original_result = std::move(result_);

        names.emplace_back("all_mutants");
        return_types.emplace_back(LogicalType::VARCHAR);

        names.emplace_back("equivalent_mutants");
        return_types.emplace_back(LogicalType::VARCHAR);

        duckdb::unique_ptr<MutationTestFunctionData> result(functionData);
        return std::move(result);
    }

    static void MutationTestFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output)
    {
        auto &data = data_p.bind_data->CastNoConst<MutationTestFunctionData>();

        // If all mutations are finished, set the output as empty
        if (data.current_index >= data.mutated_queries.size())
        {
            data.finished = true;
            return;
        }

        Connection con(context.db->GetDatabase(context));
        std::cout << "Going to query the statement: " << data.mutated_queries[data.current_index]->ToString() << std::endl;
        auto ex_query = data.mutated_queries[data.current_index]->Copy();
        auto result = con.Query(std::move(ex_query));
        int count = 0;
        // if (result->Equals(*(data.original_result)))
        // {
        output.SetValue(0, count, Value(data.mutated_queries[data.current_index]->ToString()));
        // if (result->Equals(*(data.original_result)))
        // {
        //     output.SetValue(1, count, Value(data.mutated_queries[data.current_index]->ToString()));
        // }
        // else
        // {
        //     output.SetValue(1, count, Value("NULL"));
        // }
        output.SetValue(1, count, Value("NULL"));

        count++;
        // }
        // TODO: execute the current mutated query
        // TODO: compare it with the original query result
        // Output only the equivalent mutants

        output.SetCardinality(count);

        // Move to the next mutation for the next call
        data.current_index++;
    }

    static void LoadInternal(DatabaseInstance &instance)
    {
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
