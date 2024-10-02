
#include "mutation_generator.hpp"

namespace duckdb
{

    void MutationTreeNode::AddChild(const SQLStatement &child_query)
    {
        // Create a new child node with the given child_query
        auto child_node = make_uniq<MutationTreeNode>(child_query);

        // Add the new child node to the vector of children
        children.push_back(std::move(child_node));
    }

    void MutationTreeNode::PrintTree(int depth)
    {
        // Indentation based on depth
        for (int i = 0; i < depth; i++)
        {
            std::cout << "  ";
        }

        // Print the current node's query
        std::cout << "Query: " << statement.ToString() << std::endl;

        // Recursively print all child nodes
        for (const auto &child : children)
        {
            child->PrintTree(depth + 1);
        }
    }

    static duckdb_libpgquery::PGNodeTag findStatementType(duckdb_libpgquery::PGNode &stmt)
    {
        // std::cout << "Statement Type: " << stmt.type << std::endl;
        switch (stmt.type)
        {
        case duckdb_libpgquery::T_PGRawStmt:
        {
            auto &raw_stmt = Transformer::PGCast<duckdb_libpgquery::PGRawStmt>(stmt);
            // std::cout << "Indirect statement Type: " << raw_stmt.stmt->type << std::endl;
            // std::cout << "Direct statement Type: " << raw_stmt.type << std::endl;
            return findStatementType(*raw_stmt.stmt);
        }
        default:
            return stmt.type;
        }
    }

    MutationTreeNode::MutationTreeNode(const SQLStatement &statement) : statement(statement)
    {
    }

    // void GenerateSelectMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    // {
    //     vector<unique_ptr<SQLStatement>> statements;
    //     ParserOptions parserOptions;
    //     Transformer transformer(parserOptions);

    //     transformer.TransformParseTree(parse_tree_list, statements);
    //     vector<MutationOperatorTag> mutation_operators = {MutationOperatorTag::SEL};

    //     int i = 0;
    //     for (auto entry = parse_tree_list->head; entry != nullptr; entry = entry->next)
    //     {
    //         string initial_query = statements[i]->query;
    //         MutationTreeNode mutation_tree(initial_query);

    //         MutationTreeNode current = mutation_tree;
    //         for (auto operator_type : mutation_operators)
    //         {
    //             MutateSelect(&current, operator_type);
    //         }

    //         // MutationTreeNode

    //         i++;
    //     }

    //     for (auto &statement : statements)
    //     {
    //         MutationTreeNode mutation_tree(functionData->original_query);
    //     }

    //     vector<string> mutations;
    //     mutations.push_back(statements[0]->ToString());

    //     string mutation2 = statements[0]->ToString();
    //     size_t pos = mutation2.find("SELECT");
    //     if (pos != string::npos)
    //     {
    //         mutation2.replace(pos, 6, "SELECT DISTINCT");
    //         mutations.push_back(mutation2);
    //     }
    //     functionData->mutated_queries = mutations;
    //     // std::cout << "Mutated query: " << functionData->mutated_queries.get(0) << std::endl;
    // }

    // void GenerateSelectMutations(duckdb_libpgquery::PGSelectStmt &stmt, MutationTestFunctionData *functionData)
    // {
    //     D_ASSERT(stmt.type == duckdb_libpgquery::T_PGSelectStmt);
    //     D_ASSERT(!stmt.targetList);
    //     vector<string> mutations;
    //     ParserOptions parserOptions;
    //     MudTransformer mud_transformer(parserOptions);
    //     MutationTreeNode mutation_tree(functionData->original_query);
    //     mutation_tree.AddChild(functionData->original_query);

    //     // SQLStatement currrent_statement

    //     // if (stmt.distinctClause)
    //     // {
    //     //     stmt.distinctClause = nullptr;
    //     // }

    //     // std::cout << "A statement is going to be transformed from postgres to duck: " << std::endl;

    //     unique_ptr<SelectStatement> statement = mud_transformer.TransformSelect(stmt);

    //     // std::cout << "Transformed statement: " << statement->ToString() << std::endl;

    //     functionData->mutated_queries.push_back(functionData->original_query);
    //     functionData->mutated_queries.push_back(statement->ToString());
    //     // std::cout << "Mutated query: " << functionData->mutated_queries.get(0) << std::endl;
    // }

    MudStatementGenerator::MudStatementGenerator()
    {
    }

    MudStatementGenerator::~MudStatementGenerator()
    {
    }

    // unique_ptr<QueryNode> MudStatementGenerator::GenerateQueryNode()
    // {
    //     unique_ptr<QueryNode> result;
    //     bool is_distinct = false;
    //     if (RandomPercentage(70))
    //     {
    //         // select node
    //         auto select_node = make_uniq<SelectNode>();
    //         // generate CTEs
    //         GenerateCTEs(*select_node);

    //         is_distinct = RandomPercentage(30);
    //         if (RandomPercentage(95))
    //         {
    //             select_node->from_table = GenerateTableRef();
    //         }
    //         select_node->select_list = GenerateChildren(1, 10);
    //         select_node->where_clause = RandomExpression(60);
    //         select_node->having = RandomExpression(25);
    //         if (RandomPercentage(30))
    //         {
    //             select_node->groups.group_expressions = GenerateChildren(1, 5);
    //             auto group_count = select_node->groups.group_expressions.size();
    //             if (RandomPercentage(70))
    //             {
    //                 // single GROUP BY
    //                 GroupingSet set;
    //                 for (idx_t i = 0; i < group_count; i++)
    //                 {
    //                     set.insert(i);
    //                 }
    //                 select_node->groups.grouping_sets.push_back(std::move(set));
    //             }
    //             else
    //             {
    //                 // multiple grouping sets
    //                 while (true)
    //                 {
    //                     GroupingSet set;
    //                     while (true)
    //                     {
    //                         set.insert(RandomValue(group_count));
    //                         if (RandomPercentage(50))
    //                         {
    //                             break;
    //                         }
    //                     }
    //                     select_node->groups.grouping_sets.push_back(std::move(set));
    //                     if (RandomPercentage(70))
    //                     {
    //                         break;
    //                     }
    //                 }
    //             }
    //         }
    //         select_node->qualify = RandomExpression(10);
    //         select_node->aggregate_handling =
    //             RandomPercentage(10) ? AggregateHandling::FORCE_AGGREGATES : AggregateHandling::STANDARD_HANDLING;
    //         if (RandomPercentage(10))
    //         {
    //             auto sample = make_uniq<SampleOptions>();
    //             sample->is_percentage = RandomPercentage(50);
    //             if (sample->is_percentage)
    //             {
    //                 sample->sample_size = Value::BIGINT(RandomValue(100));
    //             }
    //             else
    //             {
    //                 sample->sample_size = Value::BIGINT(RandomValue(99999));
    //             }
    //             sample->method = Choose<SampleMethod>(
    //                 {SampleMethod::BERNOULLI_SAMPLE, SampleMethod::RESERVOIR_SAMPLE, SampleMethod::SYSTEM_SAMPLE});
    //             select_node->sample = std::move(sample);
    //         }
    //         result = std::move(select_node);
    //     }
    //     else
    //     {
    //         auto setop = make_uniq<SetOperationNode>();
    //         GenerateCTEs(*setop);
    //         setop->setop_type = Choose<SetOperationType>({SetOperationType::EXCEPT, SetOperationType::INTERSECT,
    //                                                       SetOperationType::UNION, SetOperationType::UNION_BY_NAME});
    //         setop->left = GenerateQueryNode();
    //         setop->right = GenerateQueryNode();
    //         switch (setop->setop_type)
    //         {
    //         case SetOperationType::EXCEPT:
    //         case SetOperationType::INTERSECT:
    //             is_distinct = true;
    //             break;
    //         case SetOperationType::UNION:
    //         case SetOperationType::UNION_BY_NAME:
    //             is_distinct = RandomBoolean();
    //             break;
    //         default:
    //             throw InternalException("Unsupported set operation type");
    //         }
    //         result = std::move(setop);
    //     }

    //     if (is_distinct)
    //     {
    //         result->modifiers.push_back(make_uniq<DistinctModifier>());
    //     }
    //     if (RandomPercentage(20))
    //     {
    //         result->modifiers.push_back(GenerateOrderBy());
    //     }
    //     if (RandomPercentage(20))
    //     {
    //         if (RandomPercentage(50))
    //         {
    //             auto limit_percent_modifier = make_uniq<LimitPercentModifier>();
    //             if (RandomPercentage(30))
    //             {
    //                 limit_percent_modifier->limit = GenerateExpression();
    //             }
    //             else if (RandomPercentage(30))
    //             {
    //                 limit_percent_modifier->offset = GenerateExpression();
    //             }
    //             else
    //             {
    //                 limit_percent_modifier->limit = GenerateExpression();
    //                 limit_percent_modifier->offset = GenerateExpression();
    //             }
    //             result->modifiers.push_back(std::move(limit_percent_modifier));
    //         }
    //         else
    //         {
    //             auto limit_modifier = make_uniq<LimitModifier>();
    //             if (RandomPercentage(30))
    //             {
    //                 limit_modifier->limit = GenerateExpression();
    //             }
    //             else if (RandomPercentage(30))
    //             {
    //                 limit_modifier->offset = GenerateExpression();
    //             }
    //             else
    //             {
    //                 limit_modifier->limit = GenerateExpression();
    //                 limit_modifier->offset = GenerateExpression();
    //             }
    //             result->modifiers.push_back(std::move(limit_modifier));
    //         }
    //     }
    //     return result;
    // }

    bool MudStatementGenerator::DistinctModifierExist(vector<unique_ptr<ResultModifier>> &modifiers, bool remove_modifier)
    {
        for (idx_t modifier_idx = modifiers.size(); modifier_idx > 0; modifier_idx--)
        {
            auto &modifier = *modifiers[modifier_idx - 1];
            if (modifier.type == ResultModifierType::DISTINCT_MODIFIER)
            {
                auto &distinct_modifier = modifier.Cast<DistinctModifier>();
                if (distinct_modifier.distinct_on_targets.empty())
                {
                    // we have a DISTINCT without an ON clause - this distinct does not need to be added
                    if (remove_modifier)
                        modifiers.erase(modifiers.begin() + (modifier_idx - 1));
                    return true;
                }
            }
            else if (modifier.type == ResultModifierType::LIMIT_MODIFIER ||
                     modifier.type == ResultModifierType::LIMIT_PERCENT_MODIFIER)
            {
                // we encountered a LIMIT or LIMIT PERCENT - these change the result of DISTINCT, so we do need to push a
                // DISTINCT relation
                return false;
            }
        }
        return false;
    }

    MutationTreeNode *MudStatementGenerator::GenerateSelectMutations(SelectStatement &statement, MutationTestFunctionData *functionData, MutationTreeNode *parent_node)
    {
        // auto n = Transformer::PGPointerCast<duckdb_libpgquery::PGNode>(entry->data.ptr_value);
        D_ASSERT(statement.TYPE == StatementType::SELECT_STATEMENT);
        // std::cout << "Gonna add child 1" << std::endl;
        functionData->mutated_queries.push_back(statement.ToString());
        if (!parent_node)
        { // calling the select mutation generator for the first time.
            // therefore we have to create the root node, because root node will initially be nullptr
            parent_node = new MutationTreeNode(statement);
            parent_node->AddChild(statement);

            // std::cout << "Casting SelectStatement" << std::endl;

            const auto &a = statement.Copy();
            const auto &dis_statement = a->Cast<SelectStatement>();
            // std::cout << "Casting SelectStatement completed" << std::endl;
            // std::cout << dis_statement.ToString() << std::endl;

            if (!DistinctModifierExist(dis_statement.node->modifiers, true))
            {
                // remove the distinct
                dis_statement.node->modifiers.push_back(make_uniq<DistinctModifier>());
            }

            // std::cout << "Gonna add child 2" << std::endl;
            functionData->mutated_queries.push_back(dis_statement.ToString());
        }

        return parent_node;

        // string original_query = statement.ToString();
        // unique_ptr<MutationTreeNode> mutation_tree = make_uniq<MutationTreeNode>("ROOT");
        // std::queue<unique_ptr<MutationTreeNode>> queue;

        // size_t pos = original_query.find("SELECT DISTINCT");
        // mutation_tree->AddChild(original_query);
        // queue.push(make_uniq<MutationTreeNode>(original_query));
        // functionData->mutated_queries.push_back(original_query);
        // if (pos != string::npos)
        // {
        //     original_query.replace(pos, 15, "SELECT");
        //     mutation_tree->AddChild(original_query);
        //     functionData->mutated_queries.push_back(original_query);
        //     queue.push(make_uniq<MutationTreeNode>(original_query));
        // }
        // else
        // {
        //     pos = original_query.find("SELECT");
        //     original_query.replace(pos, 6, "SELECT DISTINCT");
        //     mutation_tree->AddChild(original_query);
        //     functionData->mutated_queries.push_back(original_query);
        //     queue.push(make_uniq<MutationTreeNode>(original_query));
        // }

        // Parser parser;
        // original_query = queue.front()->query;
        // queue.pop();
        // const auto &tokens = PostgresParser::Tokenize(original_query);
        // for (size_t i = 0; i < tokens.size(); i++)
        // {
        //     idx_t start = tokens[i].start;
        //     idx_t end;

        //     if (i < tokens.size() - 1)
        //     {
        //         end = tokens[i + 1].start;
        //     }
        //     else
        //     {
        //         end = original_query.size();
        //     }
        //     string token_value = original_query.substr(start, end - start);
        // std::cout << "Token: " << token_value << std::endl;
    }

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
        int i = 0;
        // std::cout << "Parse Tree Type: " << parse_tree_list->type << std::endl;
        // std::cout << "Length of the parse tree: " << parse_tree_list->length << std::endl;
        for (auto entry = parse_tree_list->head; entry != nullptr; entry = entry->next)
        {

            auto n = Transformer::PGPointerCast<duckdb_libpgquery::PGNode>(entry->data.ptr_value);
            auto statement_type = findStatementType(*n);

            vector<unique_ptr<SQLStatement>> statements;
            ParserOptions parserOptions;
            Transformer transformer(parserOptions);

            transformer.TransformParseTree(parse_tree_list, statements);
            // std::cout << "Total Statement Count: " << statements.size() << std::endl;
            MudStatementGenerator statement_generator;

            switch (statement_type)
            {
            case duckdb_libpgquery::T_PGSelectStmt:
            {
                // std::cout << "Select Statment Mutation" << std::endl;
                // std::cout << "Statement " << i << " :" << statements[i]->ToString() << std::endl;
                auto &select_statement = statements[i]->Cast<SelectStatement>();
                statement_generator.GenerateSelectMutations(select_statement, functionData);
                // GenerateSelectMutations(Transformer::PGCast<duckdb_libpgquery::PGSelectStmt>(*n), functionData);
                // GenerateSelectMutations(parse_tree_list, functionData);
                break;
            }
            default:
                std::cout << "Undefined Query Type: " << static_cast<int>(n->type) << std::endl;
                break;
            }
            i++;
        }
    }
}