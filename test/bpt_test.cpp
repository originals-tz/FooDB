#include <cstring>
#include <memory>
#include "store/bptree.h"
#include "util/trace.h"

void Test()
{
    BPTree tree("test");
    tree.Traverse(tree.GetRoot());

    tree.Insert("test", "hhh", 3);
    tree.Insert("test", "hello", 5);

    tree.Traverse(tree.GetRoot());
}

void Test2()
{
    BPTree tree("test");
    tree.Insert("00", "0", 1);
    tree.Insert("05", "5", 1);
    tree.Insert("15", "15", 2);

    Trace("test 1");
    tree.Traverse(tree.GetRoot());

    tree.Insert("25", "25", 2);
    Trace("test 2");
    tree.Traverse(tree.GetRoot());

    tree.Insert("20", "20", 2);
    tree.Insert("10", "10", 2);

    Trace("test 3");
    tree.Traverse(tree.GetRoot());

    tree.Insert("07", "7", 1);
    tree.Insert("06", "6", 1);
    tree.Insert("03", "3", 1);
    tree.Insert("04", "4", 1);
    tree.Insert("02", "2", 1);
    tree.Insert("08", "8", 1);
    tree.Traverse(tree.GetRoot());
    Data data = tree.Search("08");
    Trace("try to search a number");
    std::string str_data(data.m_data, data.m_data_size);
    std::cout << str_data << std::endl;
    Trace("delete all");
    tree.Delete(tree.GetRoot());
    tree.Traverse(tree.GetRoot());
}

int main()
{
    // DataConf::GetInstance()->m_max_size = 3;
    Test();
    Test2();
    return 0;
}
