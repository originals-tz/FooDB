#include "bptree.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <utility>

#include <fmt/format.h>

BPTree::BPTree(std::string filename, size_t node_size)
    : m_file(std::move(filename))
    , m_record_max_size(node_size)
    , m_root(nullptr)
    , m_next_page_id(1)
    , m_meta_dirty(false)
{
    assert(m_record_max_size >= 2);
    LoadFromDisk();
}

BPTree::~BPTree()
{
    FlushDirtyPages();
    DeleteAllNodes();
}

bool BPTree::Insert(const std::string& key, const void* value, size_t size)
{
    assert(!key.empty() && "Insert: key is empty.");
    assert(value && "Insert: value is nullptr.");

    if (!m_root)
    {
        m_root = CreateNode(true);
        m_root->m_keys.emplace_back(key);
        m_root->m_values.emplace_back(static_cast<const char*>(value), size);
        MarkDirty(m_root);
        return FlushDirtyPages();
    }

    auto [cursor, parent] = FindLeaf(key);
    AddRecord(cursor, key, value, size);
    if (cursor->GetSize() <= m_record_max_size)
    {
        return FlushDirtyPages();
    }

    Node* new_leaf_node = SplitLeafNode(cursor);
    if (parent)
    {
        InsertInternal(new_leaf_node->m_keys.front(), parent, new_leaf_node);
    }
    else
    {
        Node* new_root = CreateNode(false);
        new_root->m_keys.push_back(new_leaf_node->m_keys.front());
        new_root->m_children.push_back(cursor);
        new_root->m_children.push_back(new_leaf_node);
        cursor->m_parent_page_id = new_root->m_page_id;
        new_leaf_node->m_parent_page_id = new_root->m_page_id;
        MarkDirty(cursor);
        MarkDirty(new_leaf_node);
        MarkDirty(new_root);
        m_root = new_root;
        m_meta_dirty = true;
    }

    return FlushDirtyPages();
}

void BPTree::Traverse(Node* node)
{
    assert(m_root && "Tree is empty.");
    assert(node && "Traverse: traverse from an empty node.");
    node->m_is_leaf ? TraverseLeaf(node) : TraverseIndex(node);
}

void BPTree::TraverseLeaf(Node* leaf_node)
{
    assert(leaf_node && leaf_node->m_is_leaf && "nullptr or isn't leaf");
    fmt::println("leaf node, size = {}", leaf_node->GetSize());
    for (size_t i = 0; i < leaf_node->m_values.size(); ++i)
    {
        fmt::println("{}", leaf_node->m_values[i]);
    }
}

void BPTree::TraverseIndex(Node* index_node)
{
    assert(index_node && !index_node->m_is_leaf && "nullptr or isn't index");
    fmt::println("traverse index, size={}", index_node->GetSize());
    for (Node* child : index_node->m_children)
    {
        Traverse(child);
    }
}

std::optional<Data> BPTree::Search(const std::string& key)
{
    std::optional<Data> data;
    if (key.empty() || !m_root)
    {
        return data;
    }

    auto [leaf, _] = FindLeaf(key);
    if (!leaf)
    {
        return data;
    }

    for (size_t i = 0; i < leaf->m_keys.size(); ++i)
    {
        if (leaf->m_keys[i] == key)
        {
            Data record;
            record.m_data_size = leaf->m_values[i].size();
            record.m_data = leaf->m_values[i].data();
            data = record;
            return data;
        }
    }
    return data;
}

void BPTree::DeleteIndexNode(Node* node)
{
    if (!node)
    {
        return;
    }
    DeleteNode(node);
}

Node* BPTree::GetRoot()
{
    return m_root;
}

Node* BPTree::CreateNode(bool is_leaf)
{
    Node* node = new Node(is_leaf, m_record_max_size, m_next_page_id++);
    m_nodes[node->m_page_id] = node;
    MarkDirty(node);
    m_meta_dirty = true;
    return node;
}

Node* BPTree::GetNode(uint64_t page_id) const
{
    if (page_id == 0)
    {
        return nullptr;
    }
    auto it = m_nodes.find(page_id);
    return it == m_nodes.end() ? nullptr : it->second;
}

void BPTree::MarkDirty(Node* node)
{
    assert(node && "MarkDirty: node is nullptr.");
    m_dirty_pages.insert(node->m_page_id);
}

void BPTree::InsertIntoLeaf(Node* leaf, const std::string& key, const std::string& value)
{
    const size_t pos = leaf->FindPos(key.c_str());
    if (pos < leaf->m_keys.size() && leaf->m_keys[pos] == key)
    {
        leaf->m_values[pos] = value;
        MarkDirty(leaf);
        return;
    }

    leaf->m_keys.insert(leaf->m_keys.begin() + static_cast<std::ptrdiff_t>(pos), key);
    leaf->m_values.insert(leaf->m_values.begin() + static_cast<std::ptrdiff_t>(pos), value);
    MarkDirty(leaf);
}

Node* BPTree::SplitLeafNode(Node* leaf)
{
    assert(leaf && leaf->m_is_leaf && "SplitLeafNode: invalid leaf.");
    Node* new_leaf = CreateNode(true);
    const size_t split_pos = (leaf->m_keys.size() + 1) / 2;

    new_leaf->m_keys.assign(leaf->m_keys.begin() + static_cast<std::ptrdiff_t>(split_pos), leaf->m_keys.end());
    new_leaf->m_values.assign(leaf->m_values.begin() + static_cast<std::ptrdiff_t>(split_pos), leaf->m_values.end());

    leaf->m_keys.erase(leaf->m_keys.begin() + static_cast<std::ptrdiff_t>(split_pos), leaf->m_keys.end());
    leaf->m_values.erase(leaf->m_values.begin() + static_cast<std::ptrdiff_t>(split_pos), leaf->m_values.end());

    new_leaf->m_next_leaf = leaf->m_next_leaf;
    leaf->m_next_leaf = new_leaf;
    new_leaf->m_parent_page_id = leaf->m_parent_page_id;

    MarkDirty(leaf);
    MarkDirty(new_leaf);
    return new_leaf;
}

bool BPTree::InsertInternal(const std::string& key, Node* cursor, Node* child)
{
    assert(cursor && !cursor->m_is_leaf && "InsertInternal: parent is invalid.");
    assert(child && "InsertInternal: child is nullptr.");

    size_t key_pos = 0;
    while (key_pos < cursor->m_keys.size() && key >= cursor->m_keys[key_pos])
    {
        ++key_pos;
    }

    const size_t child_pos = key_pos + 1;
    cursor->m_keys.insert(cursor->m_keys.begin() + static_cast<std::ptrdiff_t>(key_pos), key);
    cursor->m_children.insert(cursor->m_children.begin() + static_cast<std::ptrdiff_t>(child_pos), child);
    child->m_parent_page_id = cursor->m_page_id;
    MarkDirty(cursor);
    MarkDirty(child);

    if (cursor->GetSize() <= m_record_max_size)
    {
        return true;
    }

    std::string promoted_key;
    Node* new_internal_node = SplitInternalNode(cursor, promoted_key);
    if (cursor == m_root)
    {
        Node* new_root = CreateNode(false);
        new_root->m_keys.push_back(promoted_key);
        new_root->m_children.push_back(cursor);
        new_root->m_children.push_back(new_internal_node);
        cursor->m_parent_page_id = new_root->m_page_id;
        new_internal_node->m_parent_page_id = new_root->m_page_id;
        MarkDirty(cursor);
        MarkDirty(new_internal_node);
        MarkDirty(new_root);
        m_root = new_root;
        m_meta_dirty = true;
        return true;
    }

    Node* parent = GetNode(cursor->m_parent_page_id);
    return InsertInternal(promoted_key, parent, new_internal_node);
}

Node* BPTree::SplitInternalNode(Node* node, std::string& promoted_key)
{
    assert(node && !node->m_is_leaf && "SplitInternalNode: invalid node.");
    Node* new_internal = CreateNode(false);
    const size_t mid = node->m_keys.size() / 2;
    promoted_key = node->m_keys[mid];

    new_internal->m_keys.assign(node->m_keys.begin() + static_cast<std::ptrdiff_t>(mid + 1), node->m_keys.end());
    new_internal->m_children.assign(node->m_children.begin() + static_cast<std::ptrdiff_t>(mid + 1), node->m_children.end());
    for (Node* child : new_internal->m_children)
    {
        child->m_parent_page_id = new_internal->m_page_id;
        MarkDirty(child);
    }

    node->m_keys.erase(node->m_keys.begin() + static_cast<std::ptrdiff_t>(mid), node->m_keys.end());
    node->m_children.erase(node->m_children.begin() + static_cast<std::ptrdiff_t>(mid + 1), node->m_children.end());
    new_internal->m_parent_page_id = node->m_parent_page_id;

    MarkDirty(node);
    MarkDirty(new_internal);
    return new_internal;
}

std::pair<Node*, Node*> BPTree::FindLeaf(const std::string& key)
{
    assert(m_root && "root is nullptr");
    Node* cursor = m_root;
    Node* parent = nullptr;
    while (cursor && !cursor->m_is_leaf)
    {
        parent = cursor;
        size_t child_index = 0;
        while (child_index < cursor->m_keys.size() && key >= cursor->m_keys[child_index])
        {
            ++child_index;
        }
        cursor = cursor->m_children[child_index];
    }
    return {cursor, parent};
}

void BPTree::AddRecord(Node* cursor, const std::string& key, const void* value, size_t size)
{
    assert(cursor && cursor->m_is_leaf && "cursor node is invalid");
    InsertIntoLeaf(cursor, key, std::string(static_cast<const char*>(value), size));
}

Node* BPTree::DeleteNode(Node* node)
{
    assert(node && "Delete: try to delete empty node!");
    if (!node->m_is_leaf)
    {
        auto children = node->m_children;
        for (Node* child : children)
        {
            DeleteNode(child);
        }
        node->m_children.clear();
    }

    m_dirty_pages.erase(node->m_page_id);
    m_nodes.erase(node->m_page_id);
    if (node == m_root)
    {
        m_root = nullptr;
        m_meta_dirty = true;
    }
    delete node;
    return nullptr;
}

void BPTree::DeleteAllNodes()
{
    for (auto& [page_id, node] : m_nodes)
    {
        (void) page_id;
        delete node;
    }
    m_nodes.clear();
    m_dirty_pages.clear();
    m_root = nullptr;
}

bool BPTree::LoadFromDisk()
{
    std::ifstream in(m_file, std::ios::binary);
    if (!in.good())
    {
        return false;
    }

    MetaPage meta {};
    if (!LoadMetaPage(in, meta))
    {
        return false;
    }
    if (meta.m_magic != kFileMagic || meta.m_version != kFileVersion || meta.m_page_size != kPageSize)
    {
        return false;
    }
    if (meta.m_node_size != m_record_max_size)
    {
        throw std::runtime_error("bptree file node size mismatch");
    }

    m_next_page_id = meta.m_next_page_id;
    if (meta.m_root_page_id == 0)
    {
        return true;
    }

    std::vector<PendingChildren> pending_children;
    m_root = LoadNodePage(in, meta.m_root_page_id, pending_children);
    if (!m_root)
    {
        return false;
    }

    for (const PendingChildren& pending : pending_children)
    {
        pending.m_node->m_children.reserve(pending.m_child_page_ids.size());
        for (uint64_t child_page_id : pending.m_child_page_ids)
        {
            Node* child = GetNode(child_page_id);
            if (!child)
            {
                return false;
            }
            pending.m_node->m_children.push_back(child);
        }
    }
    return true;
}

bool BPTree::FlushDirtyPages()
{
    if (m_dirty_pages.empty() && !m_meta_dirty)
    {
        return true;
    }

    std::fstream io;
    if (!OpenStorage(io))
    {
        return false;
    }

    if (m_meta_dirty)
    {
        if (!WriteMetaPage(io))
        {
            return false;
        }
        m_meta_dirty = false;
    }

    std::vector<uint64_t> dirty_pages(m_dirty_pages.begin(), m_dirty_pages.end());
    std::sort(dirty_pages.begin(), dirty_pages.end());
    for (uint64_t page_id : dirty_pages)
    {
        Node* node = GetNode(page_id);
        if (!node)
        {
            continue;
        }
        if (!WriteNodePage(io, node))
        {
            return false;
        }
    }

    io.flush();
    if (!io.good())
    {
        return false;
    }
    m_dirty_pages.clear();
    return true;
}

bool BPTree::LoadMetaPage(std::istream& in, MetaPage& meta)
{
    std::array<char, kPageSize> buffer {};
    in.seekg(0, std::ios::beg);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if (!in.good())
    {
        return false;
    }

    size_t offset = 0;
    const PageType page_type = static_cast<PageType>(ReadUint32(buffer.data(), offset));
    if (page_type != PageType::kMeta)
    {
        return false;
    }

    meta.m_magic = ReadUint32(buffer.data(), offset);
    meta.m_version = ReadUint32(buffer.data(), offset);
    meta.m_page_size = ReadUint32(buffer.data(), offset);
    meta.m_reserved = ReadUint32(buffer.data(), offset);
    meta.m_node_size = ReadUint64(buffer.data(), offset);
    meta.m_root_page_id = ReadUint64(buffer.data(), offset);
    meta.m_next_page_id = ReadUint64(buffer.data(), offset);
    return true;
}

bool BPTree::WriteMetaPage(std::ostream& out)
{
    std::array<char, kPageSize> buffer {};
    size_t offset = 0;
    WriteUint32(buffer.data(), offset, static_cast<uint32_t>(PageType::kMeta));
    WriteUint32(buffer.data(), offset, kFileMagic);
    WriteUint32(buffer.data(), offset, kFileVersion);
    WriteUint32(buffer.data(), offset, kPageSize);
    WriteUint32(buffer.data(), offset, 0);
    WriteUint64(buffer.data(), offset, m_record_max_size);
    WriteUint64(buffer.data(), offset, m_root ? m_root->m_page_id : 0);
    WriteUint64(buffer.data(), offset, m_next_page_id);

    auto* io = dynamic_cast<std::fstream*>(&out);
    assert(io && "WriteMetaPage requires fstream.");
    io->seekp(0, std::ios::beg);
    io->write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    return io->good();
}

Node* BPTree::LoadNodePage(std::istream& in, uint64_t page_id, std::vector<PendingChildren>& pending_children)
{
    if (Node* existing = GetNode(page_id))
    {
        return existing;
    }

    std::array<char, kPageSize> buffer {};
    in.seekg(static_cast<std::streamoff>(page_id * kPageSize), std::ios::beg);
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    if (!in.good())
    {
        return nullptr;
    }

    size_t offset = 0;
    const PageType page_type = static_cast<PageType>(ReadUint32(buffer.data(), offset));
    if (page_type != PageType::kLeaf && page_type != PageType::kInternal)
    {
        return nullptr;
    }

    const bool is_leaf = page_type == PageType::kLeaf;
    const uint64_t stored_page_id = ReadUint64(buffer.data(), offset);
    if (stored_page_id != page_id)
    {
        return nullptr;
    }

    Node* node = new Node(is_leaf, m_record_max_size, page_id);
    node->m_parent_page_id = ReadUint64(buffer.data(), offset);
    const uint64_t next_leaf_page_id = ReadUint64(buffer.data(), offset);

    const uint32_t key_count = ReadUint32(buffer.data(), offset);
    node->m_keys.reserve(key_count);
    for (uint32_t key_index = 0; key_index < key_count; ++key_index)
    {
        node->m_keys.push_back(ReadString(buffer.data(), offset));
    }

    if (is_leaf)
    {
        node->m_values.reserve(key_count);
        for (uint32_t value_index = 0; value_index < key_count; ++value_index)
        {
            node->m_values.push_back(ReadString(buffer.data(), offset));
        }
    }
    else
    {
        PendingChildren pending {};
        pending.m_node = node;
        const uint32_t child_count = ReadUint32(buffer.data(), offset);
        pending.m_child_page_ids.reserve(child_count);
        for (uint32_t child_index = 0; child_index < child_count; ++child_index)
        {
            const uint64_t child_page_id = ReadUint64(buffer.data(), offset);
            pending.m_child_page_ids.push_back(child_page_id);
            if (!LoadNodePage(in, child_page_id, pending_children))
            {
                delete node;
                return nullptr;
            }
        }
        pending_children.push_back(std::move(pending));
    }

    m_nodes[page_id] = node;
    if (next_leaf_page_id != 0)
    {
        Node* next_leaf = LoadNodePage(in, next_leaf_page_id, pending_children);
        if (!next_leaf)
        {
            m_nodes.erase(page_id);
            delete node;
            return nullptr;
        }
        node->m_next_leaf = next_leaf;
    }
    return node;
}

bool BPTree::WriteNodePage(std::ostream& out, const Node* node)
{
    assert(node && "WriteNodePage: node is nullptr.");
    std::array<char, kPageSize> buffer {};
    size_t offset = 0;
    WriteUint32(buffer.data(), offset, static_cast<uint32_t>(node->m_is_leaf ? PageType::kLeaf : PageType::kInternal));
    WriteUint64(buffer.data(), offset, node->m_page_id);
    WriteUint64(buffer.data(), offset, node->m_parent_page_id);
    WriteUint64(buffer.data(), offset, node->m_next_leaf ? node->m_next_leaf->m_page_id : 0);
    WriteUint32(buffer.data(), offset, static_cast<uint32_t>(node->m_keys.size()));
    for (const std::string& key : node->m_keys)
    {
        WriteString(buffer.data(), offset, key);
    }

    if (node->m_is_leaf)
    {
        for (const std::string& value : node->m_values)
        {
            WriteString(buffer.data(), offset, value);
        }
    }
    else
    {
        WriteUint32(buffer.data(), offset, static_cast<uint32_t>(node->m_children.size()));
        for (Node* child : node->m_children)
        {
            WriteUint64(buffer.data(), offset, child->m_page_id);
        }
    }

    auto* io = dynamic_cast<std::fstream*>(&out);
    assert(io && "WriteNodePage requires fstream.");
    io->seekp(static_cast<std::streamoff>(node->m_page_id * kPageSize), std::ios::beg);
    io->write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    return io->good();
}

bool BPTree::OpenStorage(std::fstream& io)
{
    io.open(m_file, std::ios::in | std::ios::out | std::ios::binary);
    if (io.good())
    {
        return true;
    }

    std::ofstream create(m_file, std::ios::binary | std::ios::trunc);
    if (!create.good())
    {
        return false;
    }
    create.close();

    io.clear();
    io.open(m_file, std::ios::in | std::ios::out | std::ios::binary);
    return io.good();
}

void BPTree::WriteUint32(char* buffer, size_t& offset, uint32_t value)
{
    assert(offset + sizeof(value) <= kPageSize && "page buffer overflow");
    std::memcpy(buffer + offset, &value, sizeof(value));
    offset += sizeof(value);
}

void BPTree::WriteUint64(char* buffer, size_t& offset, uint64_t value)
{
    assert(offset + sizeof(value) <= kPageSize && "page buffer overflow");
    std::memcpy(buffer + offset, &value, sizeof(value));
    offset += sizeof(value);
}

void BPTree::WriteString(char* buffer, size_t& offset, const std::string& value)
{
    WriteUint32(buffer, offset, static_cast<uint32_t>(value.size()));
    assert(offset + value.size() <= kPageSize && "page buffer overflow");
    std::memcpy(buffer + offset, value.data(), value.size());
    offset += value.size();
}

uint32_t BPTree::ReadUint32(const char* buffer, size_t& offset) const
{
    uint32_t value = 0;
    std::memcpy(&value, buffer + offset, sizeof(value));
    offset += sizeof(value);
    return value;
}

uint64_t BPTree::ReadUint64(const char* buffer, size_t& offset) const
{
    uint64_t value = 0;
    std::memcpy(&value, buffer + offset, sizeof(value));
    offset += sizeof(value);
    return value;
}

std::string BPTree::ReadString(const char* buffer, size_t& offset) const
{
    const uint32_t size = ReadUint32(buffer, offset);
    std::string value(size, '\0');
    std::memcpy(value.data(), buffer + offset, size);
    offset += size;
    return value;
}
