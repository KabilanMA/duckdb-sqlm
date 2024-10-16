
#include "mutation_generator.hpp"
#include <random>

namespace duckdb
{

    void MutationTreeNode::AddChild(SQLStatement &child_query)
    {
        auto child_node = make_uniq<MutationTreeNode>(child_query.Copy());
        children.push_back(std::move(child_node));
    }

    static duckdb_libpgquery::PGNodeTag findStatementType(duckdb_libpgquery::PGNode &stmt)
    {
        std::cout << "Calling findStatementType() with: " << stmt.type << std::endl;
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

    MutationTreeNode::MutationTreeNode(std::unique_ptr<SQLStatement> statement) : statement(std::move(statement))
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

    void MutateParsedExpression(MutationTreeNode *parent_node, SQLStatement *stmt, MutationTestFunctionData *functionData, ParsedExpression *expression, vector<ExpressionType> except_mutation = {})
    {
        switch (expression->type)
        {
        case ExpressionType::COMPARE_LESSTHAN:
        case ExpressionType::COMPARE_LESSTHANOREQUALTO:
        case ExpressionType::COMPARE_GREATERTHAN:
        case ExpressionType::COMPARE_GREATERTHANOREQUALTO:
        case ExpressionType::COMPARE_EQUAL:
        case ExpressionType::COMPARE_NOTEQUAL:
        {
            std::vector<std::function<void()>> condition_actions = {
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_LESSTHAN) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_LESSTHAN;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_LESSTHANOREQUALTO) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_LESSTHANOREQUALTO;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_GREATERTHAN) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_GREATERTHAN;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_GREATERTHANOREQUALTO) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_GREATERTHANOREQUALTO;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_EQUAL) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_EQUAL;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                },
                [&]()
                {
                    if (std::find(except_mutation.begin(), except_mutation.end(), ExpressionType::COMPARE_NOTEQUAL) == except_mutation.end())
                    {
                        expression->type = ExpressionType::COMPARE_NOTEQUAL;
                        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
                        parent_node->AddChild(*stmt);
                        except_mutation.push_back(expression->type);
                    }
                }};

            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(condition_actions.begin(), condition_actions.end(), g);

            for (auto &action : condition_actions)
            {
                action();
            }
            break;
        }
        case ExpressionType::CONJUNCTION_AND:
        case ExpressionType::CONJUNCTION_OR:
        {
            auto *conjunction_expr = dynamic_cast<ConjunctionExpression *>(expression);
            vector<ExpressionType> except = {};
            for (size_t i = 0; i < conjunction_expr->children.size(); i++)
            {
                std::cout << "Value of i in conjunction AND, OR: " << i << std::endl;
                auto &child = conjunction_expr->children.get(i);

                except.push_back(child->type);
                for (size_t j = i; j < conjunction_expr->children.size(); j++)
                {
                    std::cout << "Value of ij in conjunction AND, OR: " << i << " " << j << std::endl;
                    if (i < j)
                    {
                        auto &internal_child = conjunction_expr->children.get(j);

                        MutateParsedExpression(parent_node, stmt, functionData, internal_child.get(), {internal_child->type});
                        if (expression->type == ExpressionType::CONJUNCTION_AND)
                        {
                            expression->type = ExpressionType::CONJUNCTION_OR;
                            MutateParsedExpression(parent_node, stmt, functionData, internal_child.get(), {});
                            expression->type = ExpressionType::CONJUNCTION_AND;
                        }
                        else if (expression->type == ExpressionType::CONJUNCTION_OR)
                        {
                            expression->type = ExpressionType::CONJUNCTION_AND;
                            MutateParsedExpression(parent_node, stmt, functionData, internal_child.get(), {});
                            expression->type = ExpressionType::CONJUNCTION_OR;
                        }
                    }
                }
                MutateParsedExpression(parent_node, stmt, functionData, child.get(), except);
            }
            break;
        }
        default:
        {
            throw InternalException("Implemented expression type");
            break;
        }
        }
    }

    void MutateJoinType(JoinType new_type, SQLStatement *stmt, MutationTreeNode *parent_node, MutationTestFunctionData *functionData)
    {
        std::cout << "Calling the Mutate MutateJoinType function with type: " << std::endl;

        auto old_type = stmt->Cast<SelectStatement>().node->Cast<SelectNode>().from_table->Cast<JoinRef>().type;
        stmt->Cast<SelectStatement>().node->Cast<SelectNode>().from_table->Cast<JoinRef>().type = new_type;
        functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(stmt->Copy().release()))));
        parent_node->AddChild(*stmt);
        stmt->Cast<SelectStatement>().node->Cast<SelectNode>().from_table->Cast<JoinRef>().type = old_type;
        // join_ref.type = old_type;
    }

    void MutateTableRef(MutationTreeNode *parent_node, SQLStatement *stmt, MutationTestFunctionData *functionData, TableRef *from_table)
    {
        std::cout << "Calling the Mutate Table Ref function" << std::endl;
        switch (from_table->type)
        {
        case TableReferenceType::JOIN:
        {
            auto &join_ref = from_table->Cast<JoinRef>();
            switch (join_ref.type)
            {
            case JoinType::LEFT:
            case JoinType::RIGHT:
            case JoinType::INNER:
            case JoinType::OUTER:
            {
                std::vector<JoinType> mutation_types = {JoinType::LEFT, JoinType::RIGHT, JoinType::INNER, JoinType::OUTER};
                for (auto type : mutation_types)
                {
                    if (type != join_ref.type)
                    {
                        MutateJoinType(type, stmt, parent_node, functionData);
                        // stmt->Cast<SelectStatement>().node->Cast<SelectNode>().from_table->Cast<JoinRef>().type = JoinType::INNER;
                        // std::cout << "After mutation: " << static_cast<int>(stmt->Cast<SelectStatement>().node->Cast<SelectNode>().from_table->Cast<JoinRef>().type) << std::endl;
                        // std::cout << "After mutation: " << static_cast<int>(join_ref.type) << std::endl;
                    }
                }
                break;
            }
            default:
                throw InternalException("Implemented expression type");
                break;
            }

            break;
        }
        default:
            break;
        }
    }

    MutationTreeNode *MudStatementGenerator::GenerateSelectMutations(SelectStatement &statement, MutationTestFunctionData *functionData, MutationTreeNode *parent_node, MutationOperatorTag operator_type)
    {
        std::cout << "Generating Select Mutation Function Called with statement: " << statement.ToString() << std::endl;
        D_ASSERT(statement.TYPE == StatementType::SELECT_STATEMENT);
        // std::cout << "Where Operator 1: " << static_cast<int>(statement.node->Cast<SelectNode>().where_clause->type) << std::endl;

        if (!parent_node)
        { // calling the select mutation generator for the first time.
          // therefore we have to create the root node, because root node will initially be nullptr
            std::cout << "Calling the Generating Select Mutation Function for the first time" << std::endl;
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))));

            parent_node = new MutationTreeNode(statement.Copy());
            parent_node->AddChild(statement);

            const auto &a = statement.Copy();
            auto &dis_statement = a->Cast<SelectStatement>();

            // std::cout << "Where Operator 2: " << static_cast<int>(dis_statement.node->Cast<SelectNode>().where_clause->type) << std::endl;

            if (!DistinctModifierExist(dis_statement.node->modifiers, true))
            {
                dis_statement.node->modifiers.push_back(make_uniq<DistinctModifier>());
            }

            parent_node->AddChild(dis_statement);
            functionData->mutated_queries.push_back(std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(dis_statement.Copy().release()))));

            for (const auto &child : parent_node->children)
            {
                std::cout << "Running the for loop" << std::endl;
                const auto &a = child->statement->Copy();
                auto &child_statement = a->Cast<SelectStatement>();
                std::cout << "Casted select statement: " << child_statement.ToString() << std::endl;
                auto &child_statement_node = child_statement.node->Cast<SelectNode>();
                std::cout << "Select Node casting successful" << std::endl;
                std::cout << child->statement->Copy()->Cast<SelectStatement>().ToString() << std::endl;
                // std::cout << "Where Operator 3: " << static_cast<int>(child_statement_node.where_clause->type) << std::endl;
                if (child_statement_node.where_clause)
                    GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::WRO);
                std::cout << "Completed Processing the statement: " << child_statement.ToString() << std::endl;
            }
            std::cout << "Completed The Statment Processing" << std::endl;
        }
        else
        {
            std::cout << "Generating Select Mutation Function Recursively" << std::endl;

            if (operator_type == MutationOperatorTag::SEL)
            {
                throw InternalException("Unsupported mutation tree format requested");
            }
            else if (operator_type == MutationOperatorTag::WRO)
            {
                std::cout << "Generating mutants of (where clause) COMPARE operator" << std::endl;

                const auto &a = statement.Copy();
                auto &select_stmt = a->Cast<SelectStatement>();
                auto &select_stmt_node = select_stmt.node->Cast<SelectNode>();
                // MutateWhereClauseStatement(parent_node, std::move(std::unique_ptr<SelectStatement>(dynamic_cast<SelectStatement *>(statement.Copy().release()))), functionData);
                vector<ExpressionType> except_mutations = {select_stmt_node.where_clause->type};
                MutateParsedExpression(parent_node, a.get(), functionData, select_stmt_node.where_clause.get(), except_mutations);
                std::cout << "Completed where clause mutation" << std::endl;
                D_ASSERT(parent_node->children.size() != 0);
                std::cout << "Child statements after where clause: " << parent_node->children.size() << std::endl;
                std::cout << "Last child" << parent_node->children[parent_node->children.size() - 1]->statement->ToString() << std::endl;

                // int m = 0;
                for (const auto &child : parent_node->children)
                {
                    // std::cout << "For Loop of the join: " << m << std::endl;
                    const auto &a = child->statement->Copy();
                    auto &child_statement = a->Cast<SelectStatement>();
                    std::cout << child_statement.ToString() << std::endl;
                    auto &child_statement_node = child_statement.node->Cast<SelectNode>();

                    if (child_statement_node.from_table)
                    {
                        std::cout << "From table" << std::endl;
                        TableRef *from_table = child_statement_node.from_table.get();
                        if (from_table->type == TableReferenceType::JOIN)
                        {
                            auto &cp = from_table->Cast<JoinRef>();
                            std::cout << "Join mutation called inside the for loop" << std::endl;
                            GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::JOI);
                        }
                    }
                    // std::cout << "Completed For Loop of the join: " << m++ << std::endl;
                }
                std::cout << "Completed the for loop of the where clause completion mutation" << std::endl;
            }
            else if (operator_type == MutationOperatorTag::LCR)
            {
                std::cout << "Unimplemented Relational Operator Type" << std::endl;
                // throw InternalException("Unimplemented Relational Operator Type");
            }
            else if (operator_type == MutationOperatorTag::JOI)
            {
                const auto &a = statement.Copy();
                const auto &dis_statement = a->Cast<SelectStatement>();
                const auto &select_node = statement.node->Cast<SelectNode>();
                // auto &cp = select_node.from_table.get()->Cast<JoinRef>();
                MutateTableRef(parent_node, a.get(), functionData, select_node.from_table.get());
                // std::cout << static_cast<int>(select_node.from_table->type) << std::endl;
            }

            // for (const auto &child : parent_node->children)
            // {
            //     const auto &a = child->statement.Copy();
            //     auto &child_statement = a->Cast<SelectStatement>();
            //     auto &child_statement_node = child_statement.node->Cast<SelectNode>();

            //     if (child_statement_node.where_clause)
            //     {
            //         GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::WRO);
            //     }
            // else if (child_statement_node.from_table)
            // {
            //     TableRef *from_table = child_statement_node.from_table.get();
            //     if (from_table->type == TableReferenceType::JOIN)
            //     {
            //         auto &cp = from_table->Cast<JoinRef>();
            //         GenerateSelectMutations(child_statement, functionData, child.get(), MutationOperatorTag::JOI);
            //     }
            // }
            //     std::cout << "Completed Processing the statement: " << child_statement.ToString() << std::endl;
            // }
        }

        std::cout << "Completed generating Select MutatationN" << std::endl;

        return parent_node;
    }

    void GenerateMutations(duckdb_libpgquery::PGList *parse_tree_list, MutationTestFunctionData *functionData)
    {
        std::cout << "Starting the GenerateMutations() function with parse tree size: " << parse_tree_list->length << std::endl;

        vector<unique_ptr<SQLStatement>> statements;
        ParserOptions parserOptions;
        Transformer transformer(parserOptions);
        transformer.TransformParseTree(parse_tree_list, statements);

        MudStatementGenerator statement_generator;

        for (size_t i = 0; i < statements.size(); i++)
        {
            switch (statements[i]->type)
            {
            case StatementType::SELECT_STATEMENT:
            {
                auto &select_statement = static_cast<SelectStatement &>(*statements[i]);
                statement_generator.GenerateSelectMutations(select_statement, functionData);
                break;
            }
            default:
                break;
            }
        }

        std::cout << "exiting the generate mutation function" << std::endl;
    }
}