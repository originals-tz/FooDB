#include "bptree.h"

#include <cstring>
#include <utility>

#include "macro.h"
#include "trace.h"

BPTree::BPTree(std::string filename, size_t record_max_size)
    : m_file(std::move(filename))
    , m_record_max_size(record_max_size)
    , m_root(nullptr)
{
    assert(m_record_max_size >= 2);
}

bool BPTree::Insert(const std::string& key, const void* value, size_t size)
{
    Require(key.size(), false, Trace("Insert: key is empty."));
    // 如果根节点为空，那么新建根节点
    if (!m_root)
    {
        m_root = new Node(true, m_record_max_size);
        m_root->GetLeaf()->AddRecord(key.c_str(), value, size);
        return true;
    }
    //寻找合适的叶子节点存储数据
    auto [cursor, parent] = FindLeaf(key);
    //如果叶子节点未满。那么直接存放
    if (cursor->GetSize() < m_record_max_size)
    {
        AddRecord(cursor, key, value, size);
        return true;
    }
    // 如果叶子节点已满，那么将其分裂
    Node* new_leaf_node = SplitLeafNode(cursor, key.c_str(), value, size);
    Leaf* new_leaf = new_leaf_node->GetLeaf();

    //分裂后，cursor是旧节点，new_leaf是新节点
    //如果存在父节点，那么将new_left放到parent下
    if (parent)
    {
        return InsertInternal(new_leaf->GetRecord(0)->GetKey(), parent, new_leaf_node);
    }
    //如果不存在，那么新建一个root节点
    Node* new_root = new Node(false, m_record_max_size);
    new_root->GetIndex()->m_nodes[0].m_key = new_leaf->GetRecord(0)->GetKey();
    new_root->GetIndex()->m_next[0] = cursor;
    new_root->GetIndex()->m_next[1] = new_leaf_node;
    new_root->GetIndex()->m_cur_index = 1;
    m_root = new_root;
    return true;
}

Node* BPTree::SplitLeafNode(Node* cursor, const char* key, const void* value, size_t size)
{
    // add a new leaf
    Leaf* old_leaf = cursor->GetLeaf();
    Node* new_leaf_node = new Node(true, m_record_max_size);
    Leaf* new_leaf = new_leaf_node->GetLeaf();

    // split the leaf node
    // 获取新key的位置但不将其存储
    size_t i = cursor->FindPos(key);
    size_t border = (m_record_max_size + 1) / 2;
    // 旧节点存放 max+1 /2的数据，如果是max==2， 那么旧的节点存放1个数据，新节点存放两个数据
    for (size_t x = border; x < m_record_max_size; x++)
    {
        new_leaf->AddRecord(old_leaf->GetRecord(x));
    }
    old_leaf->Resize(border);
    // 计算分裂后新key的位置
    size_t pos = i < border ? i : i - border;
    // 判断新key应该位于哪个节点上
    Leaf* tmp = i < border ? old_leaf : new_leaf;
    // 插入新key
    tmp->Insert(pos, key, value, size);
    new_leaf->m_next_leaf = old_leaf->m_next_leaf;
    old_leaf->m_next_leaf = new_leaf_node;
    return new_leaf_node;
}

std::pair<Node*, Node*> BPTree::FindLeaf(const std::string& key)
{
    assert(m_root && "root is nullptr");
    Node* cursor = m_root;
    Node* parent = nullptr;
    // 寻找叶子节点以及其父节点
    while (!cursor->m_is_leaf)
    {
        parent = cursor;

        // 当前节点为索引节点,该循环为了寻找下一个节点
        for (size_t i = 0; i < cursor->GetSize(); i++)
        {
            // 如果当前节点小于node[i].key,由于size(next) = size(node) + 1, next[i] < node[i], 那么next[i]为下一个节点
            if (cursor->Compare(i, key.c_str()) < 0)
            {
                cursor = cursor->GetIndex()->m_next[i];
                break;
            }
            // 如果当前节点大于全部的node[k].key, 那么去next的最后一个节点
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
    assert(cursor && "cursor node is nullptr");
    size_t pos = cursor->FindPos(key.c_str());
    Leaf* leaf = cursor->GetLeaf();
    leaf->Insert(pos, key.c_str(), value, size);
}

bool BPTree::InsertInternal(const std::string& key, Node* cursor, Node* child)
{
    //分裂索引节点, 索引节点的node和next数组长度不一致，因此，需要对index=0的情况进行判断，其余逻辑和分裂叶子节点一致
    Require(key.size(), false, Trace("InsertInternal: key is empty."));
    Require(cursor, false, Trace("InsertInternal: insert into a null node."));
    Require(child, false, Trace("InsertInternal: insert a null node."));
    Trace("insert internal node");
    if (cursor->GetSize() < m_record_max_size)
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
        index_node->m_cur_index++;
        return true;
    }
    Node* new_internal_node = new Node(false, m_record_max_size);
    IndexNode* new_index = new_internal_node->GetIndex();
    IndexNode* old_index = cursor->GetIndex();
    size_t new_pos = cursor->FindPos(key.c_str());
    size_t border = (m_record_max_size + 1) / 2;
    for (size_t index = m_record_max_size - border, k = 1; index >= 0; index--, k++)
    {
        if (index == 0)
        {
            new_index->m_next[0] = (border == new_pos) ?
                                   child :
                                   old_index->m_next[m_record_max_size + 1 - k];
            break;
        }

        if ((index + border) == new_pos)
        {
            new_index->m_nodes[index - 1].m_key = key;
            new_index->m_next[index] = child;
            new_index->m_cur_index++;
            k--;
            continue;
        }
        new_index->m_nodes[index - 1].m_key = old_index->m_nodes[m_record_max_size - k].m_key;
        new_index->m_next[index] = old_index->m_next[m_record_max_size + 1 - k];
        new_index->m_cur_index++;
    }
    old_index->m_cur_index = border;

    for (size_t j = border; j >= new_pos; j--)
    {
        if (j == new_pos)
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

    Node* new_root = new Node(false, m_record_max_size);
    new_root->GetIndex()->m_nodes[0].m_key = cursor->GetIndex()->m_nodes[cursor->GetSize()].m_key;
    new_root->GetIndex()->m_next[0] = cursor;
    new_root->GetIndex()->m_next[1] = new_internal_node;
    new_root->GetIndex()->m_cur_index = 1;
    m_root = new_root;

    return true;
}

Node* BPTree::Delete(Node* node)
{
    Require(node, nullptr, Trace("Delete: try to delete empty node!"));
    if (node->m_is_leaf) { free(node->GetLeaf()); }
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
        node = nullptr;
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
    assert(leaf_node && leaf_node->m_is_leaf && "nullptr or isn't leaf");
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
    assert(index_node && !index_node->m_is_leaf && "nullptr or isn't index");
    IndexNode* index = index_node->GetIndex();
    Trace("traverse index, size=", index->m_cur_index);
    for (size_t i = 0; i <= index->m_cur_index; i++)
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
    if (key.empty())
    {
        return data;
    }

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
