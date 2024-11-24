#pragma once

#include "duckdb/main/client_context.hpp"

#include "duckdb.hpp"
#include "mutation_generator.hpp"

#include <iostream>

namespace duckdb
{
    class MudContext
    {
    private:
    public:
        shared_ptr<ClientContext> internalClientContext;

        MudContext(DatabaseInstance &database);
        ~MudContext();

        void ExecuteMutants(MutationTestFunctionData *functionData);
    };

    // MudContext::MudContext(/* args */)
    // {
    // }

    // MudContext::~MudContext()
    // {
    // }

} // namespace duckdb
