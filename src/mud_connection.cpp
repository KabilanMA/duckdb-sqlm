#include <mud_connection.hpp>

#include <duckdb/main/connection_manager.hpp>
#include "duckdb/main/connection.hpp"

namespace duckdb
{
    MudConnection::MudConnection(DuckDB &database, MutationTestFunctionData *functionData) : functionData(functionData), context(make_shared_ptr<MudContext>((*database.instance).shared_from_this()))
    {
        ConnectionManager::Get((*database.instance)).AddConnection(*context->internalClientContext);
    }

    void MudConnection::Query()
    {
        D_ASSERT(functionData);
        if (functionData->mutated_queries.size() < 2)
            return;
        context->ExecuteMutants(functionData);
    }
}