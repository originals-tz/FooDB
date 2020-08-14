#include "bptree.h"

#include <cstring>

#include "util/macro.h"
#include "util/trace.h"


Record::Record()
    : m_key(nullptr)
    , m_key_size(0)
    , m_data(nullptr)
    , m_data_size(0)
{
}

void Record::Copy(const Record* record)
{
    strncpy(m_key, record->m_key, record->m_key_size);
    memcpy(m_data, record->m_data, record->m_data_size);
}

void Record::Init(size_t key_size, size_t data_size, char* raw)
{
    AssertRequire(key_size > 0 && data_size > 0 && raw, "record : init size <= 0 or nullptr");

    Record* record = reinterpret_cast<Record*>(raw);
    record->m_key_size = key_size;
    record->m_data_size = data_size;

    record->m_key = raw + sizeof(Record);
    record->m_data = raw + sizeof(Record) + key_size;
}

void Record::SetKey(const std::string& key)
{
    AssertRequire(m_key && m_key_size, "record not init");
    AssertRequire(key.size() + 1 <= m_key_size, "key size is too big");

    strncpy(m_key, key.c_str(), key.size() + 1);
}

void Record::SetData(const void* data, size_t size)
{
    AssertRequire(m_data && m_data_size, "record not init");
    AssertRequire(size <= m_data_size, "data size is too big");

    memcpy(m_data, data, size);
}

char* Record::GetKey()
{
    return m_key;
}

char* Record::GetRawData()
{
    return m_data;
}

Data Record::GetData()
{
    AssertRequire(m_data && m_data_size, "record not init");
    Data data;
    data.m_data = m_data;
    data.m_data_size = m_data_size;
    return data;
}

size_t Record::GetKeySize()
{
    return m_key_size;
}

size_t Record::GetDataSize()
{
    return m_data_size;
}

Node::Node(bool is_leaf)
    : m_is_leaf(is_leaf)
    , m_leaf(nullptr)
    , m_index(nullptr)
{
    if (m_is_leaf)
    {
        m_leaf = Leaf::Alloc(MAX, 10, 10);
    }
    else
    {
        m_index = new IndexNode();
    }
}

Leaf::Leaf()
    : m_record_size(0)
    , m_record_count(0)
    , m_cur_count(0)
    , m_next_leaf(nullptr)
{
}

Leaf* Leaf::Alloc(size_t record_size, size_t key_size, size_t data_size)
{
    AssertRequire(record_size && key_size && data_size, "param error");

    size_t raw_size = sizeof(Leaf) + (sizeof(Record) + key_size + data_size) * record_size;
    void* raw = malloc(raw_size);
    bzero(raw, raw_size);

    Leaf* leaf = new (raw) Leaf();
    Require(leaf != nullptr, nullptr, Trace("malloc error"));
    InitLeaf(leaf, record_size, key_size, data_size);
    return leaf;
}

void Leaf::Free(Leaf* leaf)
{
    if (leaf)
    {
        free(leaf);
    }
}

void Leaf::InitLeaf(Leaf* leaf, size_t record_count, size_t key_size, size_t data_size)
{
    AssertRequire(leaf && record_count && key_size && data_size, "leaf init : param error");
    leaf->m_record_count = record_count;
    leaf->m_record_size = key_size + data_size + sizeof(Record);
    leaf->m_next_leaf = nullptr;

    char* record_buffer = leaf->m_records;
    for (size_t i = 0; i < record_count; i++)
    {
        Record::Init(key_size, data_size, record_buffer);
        record_buffer += leaf->m_record_size;
    }
}

bool Leaf::AddRecord(const char* key, const void* data, size_t data_size)
{
    Require(IsFull() == false, false, Trace("is full"));
    GetRecord(m_cur_count)->SetKey(key);
    GetRecord(m_cur_count)->SetData(data, data_size);
    m_cur_count++;
    return true;
}

bool Leaf::AddRecord(Record* record)
{
    AddRecord(record->GetKey(), record->GetRawData(), record->GetDataSize());
}

bool Leaf::IsFull()
{
    return m_cur_count == m_record_count;
}

Record* Leaf::GetRecord(size_t index)
{
    Require(index < m_record_count, nullptr, Trace("Leaf GetRecord : out of index"));
    return reinterpret_cast<Record*>(m_records + index * m_record_size);
};

size_t Leaf::Size()
{
    return m_record_count;
}

Record* Leaf::Insert(size_t index, const char* key, const void* data, size_t data_size)
{
    AssertRequire(index < m_record_count, "out of index");
    AssertRequire(key, "key is nullptr");
    Require(IsFull() == false, nullptr, Trace("leaf is full, fail to insert"));

    if (index >= m_cur_count)
    {
        m_cur_count++;
        GetRecord(index)->SetKey(key);
        GetRecord(index)->SetData(data, data_size);
        return GetRecord(index);
    }

    for (size_t new_end = m_cur_count; new_end > index; new_end--)
    {
        Record* new_record = GetRecord(new_end);
        Record* old_record = GetRecord(new_end - 1);
        new_record->Copy(old_record);
    }
    GetRecord(index)->SetKey(key);
    GetRecord(index)->SetData(data, data_size);
    m_cur_count++;
    return GetRecord(index);
}

void Leaf::Resize(size_t size)
{
    Require(size < m_record_count, , );
    m_cur_count = size;
}

size_t Node::GetSize()
{
    if (m_is_leaf)
    {
        assert(m_is_leaf && m_leaf);
    }
    else
    {
        assert(m_index);
    }
    return m_is_leaf ? m_leaf->m_cur_count : m_index->m_cur_count;
};

int Node::Compare(size_t i, const char* key)
{
    AssertRequire(key, "key is nullptr");
    const char* record_key = m_is_leaf ? GetLeaf()->GetRecord(i)->GetKey() : GetIndex()->m_nodes[i].m_key.c_str();
    return std::strcmp(key, record_key);
}

Leaf* Node::GetLeaf()
{
    return m_leaf;
}

IndexNode* Node::GetIndex()
{
    return m_index;
}

size_t Node::FindPos(const char* key)
{
    AssertRequire(key, "key is nullptr");
    size_t i = 0;
    for (; i < GetSize() && Compare(i, key) > 0; i++)
        ;
    Trace(key, " pos =", i);
    return i;
}

BPTree::BPTree(const std::string& filename)
    : m_file(filename)
    , m_root(nullptr)
{
}

bool BPTree::Insert(const std::string& key, const void* value, size_t size)
{
    Require(key.size(), false, Trace("Insert: key is empty."));
    if (!m_root)
    {
        m_root = new Node(true);
        m_root->GetLeaf()->AddRecord(key.c_str(), value, size);
        return true;
    }
    Node *cursor = nullptr, *parent = nullptr;
    std::tie(cursor, parent) = FindLeaf(key);

    if (cursor->GetSize() < MAX)
    {
        AddRecord(cursor, key, value, size);
        return true;
    }
    Node* new_leaf_node = SplitLeafNode(cursor, key.c_str(), value, size);
    Leaf* new_leaf = new_leaf_node->GetLeaf();

    if (cursor != m_root)
    {
        return InsertInternal(new_leaf->GetRecord(0)->GetKey(), parent, new_leaf_node);
    }
    Node* new_root = new Node(false);
    new_root->GetIndex()->m_nodes[0].m_key = new_leaf->GetRecord(0)->GetKey();
    new_root->GetIndex()->m_next[0] = cursor;
    new_root->GetIndex()->m_next[1] = new_leaf_node;
    new_root->GetIndex()->m_cur_count = 1;
    m_root = new_root;
    return true;
}

Node* BPTree::SplitLeafNode(Node* cursor, const char* key, const void* value, size_t size)
{
    // add a new leaf
    Leaf* old_leaf = cursor->GetLeaf();
    Node* new_leaf_node = new Node(true);
    Leaf* new_leaf = new_leaf_node->GetLeaf();

    // split the leaf node
    size_t i = cursor->FindPos(key);
    size_t border = (MAX + 1) / 2;
    for (size_t x = border; x < MAX; x++)
    {
        new_leaf->AddRecord(old_leaf->GetRecord(x));
    }
    old_leaf->Resize(border);
    {
        size_t pos = i < border ? i : i - border;
        Leaf* tmp = i < border ? old_leaf : new_leaf;
        tmp->Insert(pos, key, value, size);
    }
    new_leaf->m_next_leaf = old_leaf->m_next_leaf;
    old_leaf->m_next_leaf = new_leaf_node;
    return new_leaf_node;
}

std::pair<Node*, Node*> BPTree::FindLeaf(const std::string& key)
{
    AssertRequire(m_root, "root is nullptr");
    Node* cursor = m_root;
    Node* parent = nullptr;
    while (cursor->m_is_leaf == false)
    {
        parent = cursor;
        for (size_t i = 0; i < cursor->GetSize(); i++)
        {
            if (cursor->Compare(i, key.c_str()) < 0)
            {
                cursor = cursor->GetIndex()->m_next[i];
                break;
            }
            if (i == cursor->GetSize() - 1)
            {
                cursor = cursor->GetIndex()->m_next[i + 1];
                break;
            }
        }
    }
    return {cursor, parent};
}

void BPTree::AddRecord(Node* cursor, const std::string& key, const void* value, size_t size)
{
    AssertRequire(cursor, "cursor node is nullptr");
    size_t pos = cursor->FindPos(key.c_str());
    Leaf* leaf = cursor->GetLeaf();
    leaf->Insert(pos, key.c_str(), value, size);
}

bool BPTree::InsertInternal(const std::string& key, Node* cursor, Node* child)
{
    Require(key.size(), false, Trace("InsertInternal: key is empty."));
    Require(cursor, false, Trace("InsertInternal: insert into a null node."));
    Require(child, false, Trace("InsertInternal: insert a null node."));
    Trace("insert internal node");
    if (cursor->GetSize() < MAX)
    {
        size_t i = cursor->FindPos(key.c_str());
        IndexNode* index_node = cursor->GetIndex();
        for (size_t j = cursor->GetSize(); j > i; j--)
        {
            index_node->m_nodes[j] = index_node->m_nodes[j - 1];
            index_node->m_next[j + 1] = index_node->m_next[j];
        }
        index_node->m_nodes[i].m_key = key;
        index_node->m_next[i + 1] = child;
        index_node->m_cur_count++;
        return true;
    }
    Node* new_internal_node = new Node(false);
    IndexNode* new_index = new_internal_node->GetIndex();
    IndexNode* old_index = cursor->GetIndex();
    size_t i = cursor->FindPos(key.c_str());
    size_t border = (MAX + 1) / 2;
    for (size_t count = MAX - border, k = 1; count >= 0; count--, k++)
    {
        if (MAX <= 1)
        {
            break;
        }
        if (count == 0)
        {
            if (border == i)
            {
                new_index->m_next[0] = child;
            }
            else
            {
                new_index->m_next[0] = old_index->m_next[MAX + 1 - k];
            }
            break;
        }
        if ((count + border) == i)
        {
            new_index->m_nodes[count - 1].m_key = key;
            new_index->m_next[count] = child;
            new_index->m_cur_count++;
            k--;
            continue;
        }
        new_index->m_nodes[count - 1].m_key = old_index->m_nodes[MAX - k].m_key;
        new_index->m_next[count] = old_index->m_next[MAX + 1 - k];
        new_index->m_cur_count++;
    }
    old_index->m_cur_count = border;
    for (int j = border; j >= i; j--)
    {
        if (j == i)
        {
            old_index->m_nodes[j].m_key = key;
            old_index->m_next[j + 1] = child;
            break;
        }
        old_index->m_nodes[j].m_key = old_index->m_nodes[j - 1].m_key;
        old_index->m_next[j + 1] = old_index->m_next[j];
    }

    if (cursor != m_root)
    {
        return InsertInternal(cursor->GetIndex()->m_nodes[cursor->GetSize()].m_key, FindParent(m_root, cursor), new_internal_node);
    }

    Node* new_root = new Node(false);
    new_root->GetIndex()->m_nodes[0].m_key = cursor->GetIndex()->m_nodes[cursor->GetSize()].m_key;
    new_root->GetIndex()->m_next[0] = cursor;
    new_root->GetIndex()->m_next[1] = new_internal_node;
    new_root->GetIndex()->m_cur_count = 1;
    m_root = new_root;

    return true;
}

Node* BPTree::Delete(Node* node)
{
    Require(node, nullptr, Trace("Delete: try to delete empty node!"));
    if (node->m_is_leaf)
    {
        free(node->GetLeaf());
    }
    else
    {
        for (size_t i = 0; i <= node->GetSize(); i++)
        {
            if (node->GetIndex()->m_next[i] == nullptr)
            {
                continue;
            }
            delete Delete(node->GetIndex()->m_next[i]);
            node->GetIndex()->m_next[i] = nullptr;
        }
        delete node->GetIndex();
        node->m_index = nullptr;
    }
    if (node == m_root)
    {
        delete node;
        m_root = nullptr;
    }
    return node;
}

Node* BPTree::GetRoot()
{
    return m_root;
}

BPTree::~BPTree()
{
    Delete(m_root);
}

void BPTree::Traverse(Node* node)
{
    Require(m_root, , Trace("Tree is empty."));
    Require(node, , Trace("Traverse: traverse from an empty node."));
    node->m_is_leaf ? TraverseLeaf(node) : TraverseIndex(node);
}

void BPTree::TraverseLeaf(Node* leaf_node)
{
    AssertRequire(leaf_node && leaf_node->m_is_leaf, "nullptr or isn't leaf");
    Leaf* leaf = leaf_node->GetLeaf();
    Trace("leaf node, size = ", leaf->m_cur_count);
    for (size_t i = 0; i < leaf->m_cur_count; i++)
    {
        Data data = leaf->GetRecord(i)->GetData();
        std::string str_data(data.m_data, data.m_data_size);
        Trace(str_data);
    }
}

void BPTree::TraverseIndex(Node* index_node)
{
    AssertRequire(index_node && !index_node->m_is_leaf, "nullptr or isn't index");
    IndexNode* index = index_node->GetIndex();
    Trace("traverse index, size=", index->m_cur_count);
    for (size_t i = 0; i <= index->m_cur_count; i++)
    {
        Traverse(index->m_next[i]);
    }
}

Node* BPTree::FindParent(Node* cursor, Node* child)
{
    Require(cursor, nullptr, Trace("FindParent: Find a child from a null node."));
    Require(child, nullptr, Trace("FindParent: Want find a null node's parent."));
    Node* parent;
    if (cursor->m_is_leaf || (cursor->GetIndex()->m_next[0]->m_is_leaf))
    {
        return nullptr;
    }
    for (size_t i = 0; i < cursor->GetSize() + 1; i++)
    {
        if (cursor->GetIndex()->m_next[i] == child)
        {
            parent = cursor;
            return parent;
        }

        parent = FindParent(cursor->GetIndex()->m_next[i], child);
        if (parent != nullptr)
        {
            return parent;
        }
    }
    return parent;
}

Data BPTree::Search(const std::string& key)
{
    Data data;
    data.m_data = nullptr;
    data.m_data_size = 0;
    Require(key.size(), data, "Search: key is empty");
    if (m_root != nullptr)
    {
        Node* cursor = m_root;
        while (!cursor->m_is_leaf)
        {
            for (size_t i = 0; i < cursor->GetSize(); i++)
            {
                if (cursor->Compare(i, key.c_str()) < 0)
                {
                    cursor = cursor->GetIndex()->m_next[i];
                    break;
                }
                if (i == cursor->GetSize() - 1)
                {
                    cursor = cursor->GetIndex()->m_next[i + 1];
                    break;
                }
            }
        }
        for (size_t i = 0; i < cursor->GetSize(); i++)
        {
            if (cursor->GetLeaf()->GetRecord(i)->GetKey() == key)
            {
                Trace("found");
                data = cursor->GetLeaf()->GetRecord(i)->GetData();
            }
        }
    }
    return data;
}
