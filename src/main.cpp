#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <parser/parser.hpp>
#include <duckdb.hpp>

int main(int argc, char **argv)
{
    std::string query = "SELECT * FROM teacher WHERE salary > 1000;";
    // std::string query = "SELECT e.employee_id, e.first_name, e.last_name, d.department_name, p.project_name, SUM(t.hours_worked) AS total_hours_worked, AVG(t.hours_worked) OVER (PARTITION BY e.department_id) AS avg_hours_in_dept, (SELECT COUNT(*) FROM employees e2 WHERE e2.manager_id = e.employee_id) AS num_reports FROM employees e JOIN departments d ON e.department_id = d.department_id JOIN project_assignments pa ON e.employee_id = pa.employee_id JOIN projects p ON pa.project_id = p.project_id JOIN timesheets t ON e.employee_id = t.employee_id WHERE t.date_worked BETWEEN '2024-01-01' AND '2024-12-31' GROUP BY e.employee_id, e.first_name, e.last_name, d.department_name, p.project_name HAVING SUM(t.hours_worked) > 1000 ORDER BY total_hours_worked DESC;";
    // void Parser::ParseQuery(const string &query);
    duckdb::ParserOptions options{true, false, 1000, nullptr};
    duckdb::Parser parser(options);
    parser.ParseQuery(query);
    // parser

    // Iterate through the vector
    for (const auto &stmt_ptr : parser.statements)
    {
        if (stmt_ptr)
        { // Check if the unique_ptr is not null
            std::cout << "Query: " << duckdb::EnumUtil::ToChars(stmt_ptr->type) << std::endl;
        }
    }

    // std::vector<std::string> query_statements;
    // auto tokens = duckdb::Parser::Tokenize(query);
    // uint64_t next_statement_start = 0;
    // std::cout << "Token Size: " << tokens.size() << std::endl;

    // for (uint64_t i = 1; i < tokens.size(); ++i)
    // {
    //     // std::cout << "Start of the Token: " << tokens[i - 1].start << std::endl
    //     //           << "Type of the Token: " << to_string(tokens[i - 1].type) << std::endl;
    //     auto &t_prev = tokens[i - 1];
    //     auto &t = tokens[i];
    //     std::cout << query.substr(t_prev.start, t.start - t_prev.start) << std::endl;
    //     if (t_prev.type == duckdb::SimplifiedTokenType::SIMPLIFIED_TOKEN_NUMERIC_CONSTANT)
    //     {
    //         for (uint64_t c = t_prev.start; c <= t.start; ++c)
    //         {
    //             if (query.c_str()[c] == ';')
    //             {
    //                 query_statements.emplace_back(query.substr(next_statement_start, t.start - next_statement_start));
    //                 next_statement_start = tokens[i].start;
    //             }
    //         }
    //     }
    // }
    // query_statements.emplace_back(query.substr(next_statement_start, query.size() - next_statement_start));

    // for (const std::string &query_statement : query_statements)
    // {
    //     std::cout << query_statement << std::endl;
    // }
    return 0;
}

// // vector<string> SplitQueryStringIntoStatements(const string &query) {
// // 	// Break sql string down into sql statements using the tokenizer
// // 	vector<string> query_statements;
// // 	auto tokens = Parser::Tokenize(query);
// // 	idx_t next_statement_start = 0;
// // 	for (idx_t i = 1; i < tokens.size(); ++i) {
// // 		auto &t_prev = tokens[i - 1];
// // 		auto &t = tokens[i];
// // 		if (t_prev.type == SimplifiedTokenType::SIMPLIFIED_TOKEN_OPERATOR) {
// // 			// LCOV_EXCL_START
// // 			for (idx_t c = t_prev.start; c <= t.start; ++c) {
// // 				if (query.c_str()[c] == ';') {
// // 					query_statements.emplace_back(query.substr(next_statement_start, t.start - next_statement_start));
// // 					next_statement_start = tokens[i].start;
// // 				}
// // 			}
// // 			// LCOV_EXCL_STOP
// // 		}
// // 	}
// // 	query_statements.emplace_back(query.substr(next_statement_start, query.size() - next_statement_start));
// // 	return query_statements;
// // }