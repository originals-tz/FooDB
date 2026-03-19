#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include "store/bptree.h"
#include "store/trace.h"

void Test()
{
    BPTree tree("test-update.db", 3);
    // tree.Traverse(tree.GetRoot());

    tree.Insert("test", "hhh", 3);
    tree.Insert("test", "hello", 5);

    tree.Traverse(tree.GetRoot());
}

void Test2()
{
    BPTree tree("test.db", 2);
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
    auto data = tree.Search("08");
    if (data.has_value())
    {
        Trace("try to search a number");
        std::string str_data(data.value().m_data, data.value().m_data_size);
        std::cout << str_data << std::endl;
    }
}

int main()
{
    constexpr uint32_t kMetaPageType = 1;
    constexpr uint32_t kFileMagic = 0x42505431;
    constexpr uintmax_t kPageSize = 4096;

    std::filesystem::remove("test-update.db");
    std::filesystem::remove("test.db");
    Test();
    Test2();

    if (std::filesystem::file_size("test.db") % kPageSize != 0)
    {
        return 1;
    }

    {
        std::ifstream in("test.db", std::ios::binary);
        uint32_t page_type = 0;
        uint32_t magic = 0;
        in.read(reinterpret_cast<char*>(&page_type), sizeof(page_type));
        in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (!in.good() || page_type != kMetaPageType || magic != kFileMagic)
        {
            return 1;
        }
    }

    {
        BPTree tree("test.db", 2);
        auto data = tree.Search("08");
        if (!data.has_value())
        {
            return 1;
        }
        std::string str_data(data.value().m_data, data.value().m_data_size);
        if (str_data != "8")
        {
            return 1;
        }
    }

    std::filesystem::remove("test-update.db");
    std::filesystem::remove("test.db");
    return 0;
}
