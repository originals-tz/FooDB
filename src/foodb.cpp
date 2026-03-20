#include "catalog/table.h"
#include "store/bptree.h"

int main()
{
    Schema schema({ { "id", ColumnType::kString, 0, false, true }, { "name", ColumnType::kString, 0, true, false } });
    Table table("users", schema);
    (void) table;
    return 0;
}
