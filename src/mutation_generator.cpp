
#include "mutation_generator.hpp"

namespace duckdb
{

    void MutationTreeNode::AddChild(const SQLStatement &child_query)
    {
        auto child_node = make_uniq<MutationTreeNode>(child_query);
        children.push_back(std::move(child_node));
    }

    void MutationTreeNode::PrintTree(int depth)
    {
        for (int i = 0; i < depth; i++)
        {
            std::cout << "  ";
        }
        std::cout << "Query: " << statement.ToString() << std::endl;

        for (const auto &child : children)
        {
            child->PrintTree(depth + 1);
        }
    }

    static duckdb_libpgquery::PGNodeTag findStatementType(duckdb_libpgquery::PGNode &stmt)
    {
        switch (stmt.type)
        {
        case duckdb_libpgquery::T_PGRawStmt:
        {
            auto &raw_stmt = Transformer::PGCast<duckdb_libpgquery::PGRawStmt>(stmt);
            return findStatementType(*raw_stmt.stmt);
        }
        default:
            return stmt.type;
        }
    }

    MutationTreeNode::MutationTreeNode(const SQLStatement &statement) : statement(statement)
    {
    }

    MudStatementGenerator::MudStatementGenerator()
    {
    }

    MudStatementGenerator::~MudStatementGenerator()
    {
    }

    /**
     * @brief Check whether the given modifier has a distinct modifier; return true if it has distinct modifier, false otherwise.
     *
     * @param modifiers input modifier to check whether distinct modifier exists
     * @param remove_modifier input to confirm whether to delete the distinct modifier from the given modifiers vector if it exists.
     * @return true; if distinct modifier exists
     * @return false: otherwise
     */
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

    MutationTreeNode *MudStatementGenerator::GenerateSelectMutations(SelectStatement &statement, MutationTestFunctionData *functionData, MutationTreeNode *parent_node, MutationOperatorTag operator_type)
    {
        // std::cout << "Generating Select Mutation Function Called with statement: " << statement.ToString() << std::endl;
        D_ASSERT(statement.TYPE == StatementType::SELECT_STATEMENT);
        // std::cout << "Where Operator 1: " << static_cast<int>(statement.node->Cast<SelectNode>().where_clause->type) << std::endl;

        if (!parent_node)
        { // calling the select mutation generator for the first time.
          // therefore we have to create the root node, because root node will initially be nullptr
            // std::cout << "Calling the Generating Select Mutation Function for the first time" << std::endl;
            functionData->mutated_queries.push_back(statement.ToString());

            parent_node = new MutationTreeNode(statement);
            parent_node->AddChild(statement);

            const auto &a = statement.Copy();
            const auto &dis_statement = a->Cast<SelectStatement>();

            // std::cout << "Where Operator 2: " << static_cast<int>(dis_statement.node->Cast<SelectNode>().where_clause->type) << std::endl;

            if (!DistinctModifierExist(dis_statement.node->modifiers, true))
            {
                dis_statement.node->modifiers.push_back(make_uniq<DistinctModifier>());
            }

            parent_node->AddChild(dis_statement);
            functionData->mutated_queries.push_back(dis_statement.ToString());

            for (const auto &child : parent_node->children)
            {
                // std::cout << "Running the for loop" << std::endl;
                const auto &a = child->statement.Copy();
                auto &child_statement = a->Cast<SelectStatement>();
                // std::cout << "Casted select statement: " << child_statement.ToString() << std::endl;
                auto &child_statement_node = child_statement.node->Cast<SelectNode>();
                // std::cout << "Select Node casting successful" << std::endl;
                // std::cout << child->statement.Copy()->Cast<SelectStatement>().ToString() << std::endl;
                // std::cout << "Where Operator 3: " << static_cast<int>(child_statement_node.where_clause->type) << std::endl;
                GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::ROR);
            }
        }
        else
        {
            // std::cout << "Generating Select Mutation Function Recursively" << std::endl;

            if (operator_type == MutationOperatorTag::SEL)
            {
                throw InternalException("Unsupported mutation tree format requested");
            }
            else if (operator_type == MutationOperatorTag::ROR)
            {
                // std::cout << "Generating mutants of COMPARE operator" << std::endl;

                const auto &a = statement.Copy();
                // std::cout << "stmt1" << std::endl;
                const auto &dis_statement = a->Cast<SelectStatement>();
                // std::cout << "stmt1" << std::endl;
                auto &select_node = statement.node->Cast<SelectNode>();
                // std::cout << "stmt1" << std::endl;
                switch (select_node.where_clause->type)
                {
                case ExpressionType::COMPARE_LESSTHAN:
                {
                    auto &select_node_1 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_1.where_clause->type = ExpressionType::COMPARE_EQUAL;
                    // parent_node->AddChild(a->Cast<SelectStatement>());
                    functionData->mutated_queries.push_back(select_node_1.ToString());

                    auto &select_node_2 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_2.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                    functionData->mutated_queries.push_back(select_node_2.ToString());

                    auto &select_node_3 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_3.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_3.ToString());

                    auto &select_node_4 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_4.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_4.ToString());

                    auto &select_node_5 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_5.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                    functionData->mutated_queries.push_back(select_node_5.ToString());
                    break;
                }
                case ExpressionType::COMPARE_GREATERTHAN:
                {
                    auto &select_node_1 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_1.where_clause->type = ExpressionType::COMPARE_EQUAL;
                    functionData->mutated_queries.push_back(select_node_1.ToString());

                    auto &select_node_2 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_2.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                    functionData->mutated_queries.push_back(select_node_2.ToString());

                    auto &select_node_3 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_3.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_3.ToString());

                    auto &select_node_4 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_4.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_4.ToString());

                    auto &select_node_5 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_5.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                    functionData->mutated_queries.push_back(select_node_5.ToString());

                    break;
                }
                case ExpressionType::COMPARE_EQUAL:
                {
                    auto &select_node_1 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_1.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                    functionData->mutated_queries.push_back(select_node_1.ToString());

                    auto &select_node_2 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_2.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                    functionData->mutated_queries.push_back(select_node_2.ToString());

                    auto &select_node_3 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_3.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_3.ToString());

                    auto &select_node_4 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_4.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_4.ToString());

                    auto &select_node_5 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_5.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                    functionData->mutated_queries.push_back(select_node_5.ToString());

                    break;
                }
                case ExpressionType::COMPARE_LESSTHANOREQUALTO:
                {
                    auto &select_node_1 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_1.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                    functionData->mutated_queries.push_back(select_node_1.ToString());

                    auto &select_node_2 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_2.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                    functionData->mutated_queries.push_back(select_node_2.ToString());

                    auto &select_node_3 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_3.where_clause->type = ExpressionType::COMPARE_EQUAL;
                    functionData->mutated_queries.push_back(select_node_3.ToString());

                    auto &select_node_4 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_4.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_4.ToString());

                    auto &select_node_5 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_5.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                    functionData->mutated_queries.push_back(select_node_5.ToString());

                    break;
                }
                case ExpressionType::COMPARE_GREATERTHANOREQUALTO:
                {
                    auto &select_node_1 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_1.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                    functionData->mutated_queries.push_back(select_node_1.ToString());

                    auto &select_node_2 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_2.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                    functionData->mutated_queries.push_back(select_node_2.ToString());

                    auto &select_node_3 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_3.where_clause->type = ExpressionType::COMPARE_EQUAL;
                    functionData->mutated_queries.push_back(select_node_3.ToString());

                    auto &select_node_4 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_4.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_4.ToString());

                    auto &select_node_5 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_5.where_clause->type = ExpressionType::COMPARE_NOTEQUAL;
                    functionData->mutated_queries.push_back(select_node_5.ToString());

                    break;
                }
                case ExpressionType::COMPARE_NOTEQUAL:
                {
                    auto &select_node_1 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_1.where_clause->type = ExpressionType::COMPARE_GREATERTHAN;
                    functionData->mutated_queries.push_back(select_node_1.ToString());

                    auto &select_node_2 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_2.where_clause->type = ExpressionType::COMPARE_LESSTHAN;
                    functionData->mutated_queries.push_back(select_node_2.ToString());

                    auto &select_node_3 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_3.where_clause->type = ExpressionType::COMPARE_EQUAL;
                    functionData->mutated_queries.push_back(select_node_3.ToString());

                    auto &select_node_4 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_4.where_clause->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_4.ToString());

                    auto &select_node_5 = a->Cast<SelectStatement>().node->Cast<SelectNode>();
                    select_node_5.where_clause->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                    functionData->mutated_queries.push_back(select_node_5.ToString());

                    break;
                }
                default:
                    std::cout << "Unimplemented Operator Type" << std::endl;
                    break;
                }
            }
            else if (operator_type == MutationOperatorTag::LCR)
            {
            }
        }

        return parent_node;
    }

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
        int i = 0;
        for (auto entry = parse_tree_list->head; entry != nullptr; entry = entry->next)
        {

            auto n = Transformer::PGPointerCast<duckdb_libpgquery::PGNode>(entry->data.ptr_value);
            auto statement_type = findStatementType(*n);

            vector<unique_ptr<SQLStatement>> statements;
            ParserOptions parserOptions;
            Transformer transformer(parserOptions);

            transformer.TransformParseTree(parse_tree_list, statements);
            MudStatementGenerator statement_generator;

            switch (statement_type)
            {
            case duckdb_libpgquery::T_PGSelectStmt:
            {
                auto &select_statement = statements[i]->Cast<SelectStatement>();
                statement_generator.GenerateSelectMutations(select_statement, functionData);
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