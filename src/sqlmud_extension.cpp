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

    // void GenerateSelectMutant(SelectStatement &statement, unique_ptr<MutationTestFunctionData> &functionData)
    // {
    //     duckdb_sqlmud::QueryNode *mutation_tree = new duckdb_sqlmud::QueryNode(statement);
    //     const auto &tokens = Parser::Tokenize(statement.ToString());
    //     for (size_t i = 0; i < tokens.size(); i++)
    //     {
    //         idx_t start = tokens[i].start;
    //         idx_t end;

    //         if (i < tokens.size() - 1)
    //         {
    //             end = tokens[i + 1].start;
    //         }
    //         else
    //         {
    //             end = statement.ToString().size();
    //         }
    //         string token_value = statement.ToString().substr(start, end - start);
    //         std::cout << "Token: " << token_value << " | Type: " << EnumUtil::ToChars(tokens[i].type) << std::endl;
    //     }

    //     // std::cout <<
    // }

    // unique_ptr<QueryResult> Query(ClientContext &context, unique_ptr<SQLStatement> statement){
    //     make_uniq<duckdb::ClientContextLock>
    //     // context.Query(statement, false);
    //     // auto lock = ClientContext::LockContext();
    // }

    duckdb_libpgquery::PGList *ParserQuery(ClientContext &context, const string &query)
    {
        // ParserOptions parserOptions;
        // Parser duckdb_parser(parserOptions);
        // Transformer transformer(parserOptions);

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

        // auto binder = Binder::CreateBinder(context, nullptr, BinderType::REGULAR_BINDER);

        // auto &statement = parser.statements[0];
        // auto bound_statement = binder->Bind(*statement);

        // Planner planner(context);
        // planner.CreatePlan(std::move(bound_statement));
        // auto logical_plan = std::move(planner.plan);

        // for (auto &statement : parser.statements)
        // {
        //     statement->
        // }
    }

    // vector<string> GenerateMutations(SQLStatement &statement)
    // {
    //     StatementType statementType = statement.type;
    //     vector<string> mutations;
    //     std::cout << "Calling the actual mutation function." << std::endl;

    //     switch (statementType)
    //     {
    //     case StatementType::SELECT_STATEMENT:
    //         auto &select_stmt = statement.Cast<SelectStatement>();
    //         mutations = GenerateSelectMutant(select_stmt);
    //         break;
    //     default:
    //         break;
    //     }

    // // Example mutation 1: Change "=" to "!="
    // string mutation1 = query;
    // size_t pos = mutation1.find("=");
    // if (pos != string::npos)
    // {
    //     mutation1.replace(pos, 1, "!=");
    //     mutations.push_back(mutation1);
    // }

    // // Example mutation 2: Replace "SELECT" with "SELECT DISTINCT"
    // string mutation2 = query;
    // pos = mutation2.find("SELECT");
    // if (pos != string::npos)
    // {
    //     mutation2.replace(pos, 6, "SELECT DISTINCT");
    //     mutations.push_back(mutation2);
    // }

    // // Example mutation 3: Add LIMIT 10 to the query
    // string mutation3 = query + " LIMIT 10";
    // mutations.push_back(mutation3);

    // // Add more mutations as needed...

    //     return mutations;
    // }

    // void GenerateMutations(SQLStatement &statement, unique_ptr<MutationTestFunctionData> functionData)
    // {
    //     StatementType statementType = statement.type;
    //     vector<string> mutations;
    //     // std::cout << "Calling the actual mutation function." << std::endl;

    //     switch (statementType)
    //     {
    //     case StatementType::SELECT_STATEMENT:
    //         auto &select_stmt = statement.Cast<SelectStatement>();
    //         GenerateSelectMutant(select_stmt, functionData);
    //         break;
    //     default:
    //         const char *stmt_type = EnumUtil::ToChars(StatementType::SELECT_STATEMENT);
    //         std::stringstream error_message;
    //         error_message << "SQL statement type " << stmt_type << " not supported";
    //         throw std::runtime_error(error_message.str());
    //         break;
    //     }
    // }

    // SELECT mutant FROM mutation_test("SELECT DISTINCT * FROM teacher WHERE salary > 10;");
    static duckdb::unique_ptr<FunctionData> MutationTestBind(ClientContext &context, TableFunctionBindInput &input,
                                                             vector<LogicalType> &return_types, vector<string> &names)
    {
        MutationTestFunctionData *functionData = new MutationTestFunctionData(StringValue::Get(input.inputs[0]));
        // std::cout << "Calling the mutation bind function with the query: " << functionData->original_query << std::endl;
        // StringValue::Get(input.inputs[0])
        // auto result = make_uniq<MutationTestFunctionData>(StringValue::Get(input.inputs[0]));
        // ParserOptions parserOptions;
        // Parser parser(parserOptions);

        duckdb_libpgquery::PGList *parse_trees = ParserQuery(context, functionData->original_query);
        // std::cout << "Completed Parsing the query: " << functionData->original_query << std::endl;
        GenerateMutations(parse_trees, functionData);
        // std::cout << "Completed Mutation Generation" << std::endl;
        // for (auto &statement : parser.statements)
        // {
        //     GenerateMutations(*statement, result);
        // }

        // auto &statement = parser.statements[0];
        // statement->type
        // std::cout << "Binding Mutation Test Function!" << std::endl;
        // Generate multiple mutations for the input query
        // result->mutated_queries = GenerateMutations(*statement);

        // Return type is a string for the mutated query
        return_types.emplace_back(LogicalType::VARCHAR);
        names.emplace_back("mutant");
        // std::cout << "Printing the generated mutant" << std::endl;
        // std::cout << functionData->mutated_queries[0] << std::endl;
        duckdb::unique_ptr<MutationTestFunctionData> result(functionData);
        // std::cout << "Printing the generated mutant query before returning" << std::endl;
        // std::cout << result->mutated_queries[0] << std::endl;
        // std::cout << "Returning result from mutation bind." << std::endl;
        return std::move(result);
    }

    static void MutationTestFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output)
    {
        // std::cout << "Calling the mutation test function" << std::endl;
        auto &data = data_p.bind_data->CastNoConst<MutationTestFunctionData>();
        // std::cout << "Printing the original query" << std::endl;
        // std::cout << data.original_query << std::endl;

        // TODO: execute the original query and cache the result

        // If all mutations are finished, set the output as empty
        if (data.current_index >= data.mutated_queries.size())
        {
            data.finished = true;
            return;
        }
        // std::cout << "Printing the current mutation" << std::endl;
        // std::cout << data.mutated_queries.get(data.current_index) << std::endl;

        // TODO: execute the current mutated query
        // TODO: compare it with the original query result
        // Output only the equivalent mutants

        // Set the output to the current mutation
        output.SetCardinality(1);
        output.SetValue(0, 0, Value(data.mutated_queries[data.current_index]));

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
