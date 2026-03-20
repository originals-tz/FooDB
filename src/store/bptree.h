#ifndef _BPTREE_H_
#define _BPTREE_H_

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "node.h"

struct Data
{
    Data()
        : m_data_size(0)
        , m_data(nullptr)
    {
    }

    size_t m_data_size;
    const char* m_data;
};

class BPTree
{
public:
    explicit BPTree(std::string filename, size_t node_size);
    ~BPTree();

    bool Insert(const std::string& key, const void* value, size_t size);

    void Traverse(Node* node);
    void TraverseLeaf(Node* leaf_node);
    void TraverseIndex(Node* index_node);

    std::optional<Data> Search(const std::string& key) const;
    void DeleteIndexNode(Node* node);
    Node* GetRoot();

private:
    enum class PageType : uint8_t
    {
        kMeta = 1,
        kLeaf = 2,
        kInternal = 3,
    };

    struct MetaPage
    {
        uint32_t m_magic;
        uint32_t m_version;
        uint32_t m_page_size;
        uint32_t m_reserved;
        uint64_t m_node_size;
        uint64_t m_root_page_id;
        uint64_t m_next_page_id;
    };

    struct PendingChildren
    {
        Node* m_node;
        std::vector<uint64_t> m_child_page_ids;
    };

    Node* CreateNode(bool is_leaf);
    Node* GetNode(uint64_t page_id) const;
    void MarkDirty(Node* node);
    void InsertIntoLeaf(Node* leaf, const std::string& key, const std::string& value);
    Node* SplitLeafNode(Node* leaf);
    bool InsertInternal(const std::string& key, Node* cursor, Node* child);
    Node* SplitInternalNode(Node* node, std::string& promoted_key);
    std::pair<Node*, Node*> FindLeaf(const std::string& key) const;
    void AddRecord(Node* cur, const std::string& key, const void* value, size_t size);
    Node* DeleteNode(Node* node);
    void DeleteAllNodes();

    bool LoadFromDisk();
    bool FlushDirtyPages();
    bool LoadMetaPage(std::istream& in, MetaPage& meta);
    bool WriteMetaPage(std::ostream& out);
    Node* LoadNodePage(std::istream& in, uint64_t page_id, std::vector<PendingChildren>& pending_children);
    bool WriteNodePage(std::ostream& out, const Node* node);
    bool OpenStorage(std::fstream& io);
    void WriteUint32(char* buffer, size_t& offset, uint32_t value);
    void WriteUint64(char* buffer, size_t& offset, uint64_t value);
    void WriteString(char* buffer, size_t& offset, const std::string& value);
    uint32_t ReadUint32(const char* buffer, size_t& offset) const;
    uint64_t ReadUint64(const char* buffer, size_t& offset) const;
    std::string ReadString(const char* buffer, size_t& offset) const;

    static constexpr uint32_t kFileMagic = 0x42505431;  // BPT1
    static constexpr uint32_t kFileVersion = 1;
    static constexpr uint32_t kPageSize = 4096;

    std::string m_file;
    size_t m_record_max_size;
    Node* m_root;
    uint64_t m_next_page_id;
    std::unordered_map<uint64_t, Node*> m_nodes;
    std::unordered_set<uint64_t> m_dirty_pages;
    bool m_meta_dirty;
};

#endif
