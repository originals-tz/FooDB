#include "bptree.h"

#include <cstring>

#include "util/macro.h"
#include "util/trace.h"

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

    if (cursor->GetSize() < DataConf::GetInstance()->m_max_size)
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
    size_t border = (DataConf::GetInstance()->m_max_size + 1) / 2;
    for (size_t x = border; x < DataConf::GetInstance()->m_max_size; x++)
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
    if (cursor->GetSize() < DataConf::GetInstance()->m_max_size)
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
    size_t new_pos = cursor->FindPos(key.c_str());
    size_t border = (DataConf::GetInstance()->m_max_size + 1) / 2;
    for (size_t count = DataConf::GetInstance()->m_max_size - border, k = 1; count >= 0; count--, k++)
    {
        if (DataConf::GetInstance()->m_max_size <= 1)
        {
            break;
        }
        if (count == 0)
        {
            if (border == new_pos)
            {
                new_index->m_next[0] = child;
            }
            else
            {
                new_index->m_next[0] = old_index->m_next[DataConf::GetInstance()->m_max_size + 1 - k];
            }
            break;
        }
        if ((count + border) == new_pos)
        {
            new_index->m_nodes[count - 1].m_key = key;
            new_index->m_next[count] = child;
            new_index->m_cur_count++;
            k--;
            continue;
        }
        new_index->m_nodes[count - 1].m_key = old_index->m_nodes[DataConf::GetInstance()->m_max_size - k].m_key;
        new_index->m_next[count] = old_index->m_next[DataConf::GetInstance()->m_max_size + 1 - k];
        new_index->m_cur_count++;
    }
    old_index->m_cur_count = border;
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
