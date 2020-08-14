#ifndef _BPTREE_H_
#define _BPTREE_H_

#include <string>
#include <vector>

#define MAX 3

struct Header
{
    u_int32_t m_block_size;
    u_int32_t m_key_size;
    u_int32_t m_root_pos;
};

struct Data
{
    size_t m_data_size;
    const char* m_data;
};
class Record
{
public:
    /*
     * @brief base data of bptree
     */
    Record();

    void Copy(const Record* record);

    /*
     * @brief init the record
     * @param raw where record will be place
     */
    static void Init(size_t key_size, size_t data_size, char* raw);

    /*
     * @brief set the key for record
     */
    void SetKey(const std::string& key);

    /*
     * @brief set the record data
     */
    void SetData(const void* data, size_t size);

    /*
     * @brief get the key of record
     */
    char* GetKey();

    char* GetRawData();

    /*
     * @brief get the data of record
     */
    Data GetData();

    /*
     * @brief get key size
     */
    size_t GetKeySize();

    /*
     * @brief get data size
     */
    size_t GetDataSize();

private:
    char* m_key;
    size_t m_key_size;

    char* m_data;
    size_t m_data_size;
};

struct Node;

struct Leaf
{
    Leaf();
    /*
     * @brief allocate a leaf node
     * @return the leaf or nullptr
     */
    static Leaf* Alloc(size_t record_size, size_t key_size, size_t data_size);

    /*
     * @brief free the leaf node
     * @param leaf
     */
    static void Free(Leaf* leaf);

    /*
     * @brief init the leaf node
     * @param leaf the node which will be init
     * @param record_count the number of record
     * @param key_size size of key
     * @param data_size size of record data
     */
    static void InitLeaf(Leaf* leaf, size_t record_count, size_t key_size, size_t data_size);

    /*
     * @brief add a record in a leaf
     * @return if add record successfully
     */
    bool AddRecord(const char* key, const void* data, size_t data_size);

    bool AddRecord(Record* record);

    /*
     * @brief check if the leaf hasn't slot for new record
     * @return true if leaf hasn't  slot for new record
     */
    bool IsFull();

    /*
     * @brief get record from leaf
     * @return if has record in leaf[index], return pointer of record, else return nullptr
     */
    Record* GetRecord(size_t index);

    /*
     * @brief get the count of record
     * @return count of record
     */
    size_t Size();
    /*
     * @brief insert a record
     */
    Record* Insert(size_t index, const char* key, const void* data, size_t data_size);

    /**
     * @brief set m_cur_count = size
     **/
    void Resize(size_t size);

    size_t m_record_size;
    size_t m_record_count;
    size_t m_cur_count;
    /*
     * @param m_next_leaf is used for find the next leaf node
     * it will become easier to find a series number in a scope such as 1 - 5
     */
    Node* m_next_leaf;
    char m_records[];
};

struct IndexNodeData
{
    std::string m_key;
};

struct IndexNode
{
    size_t m_cur_count;
    IndexNodeData m_nodes[MAX + 1];
    /*
     * with 3 IndexNode, there should be 4 space for its child node
     */
    Node* m_next[MAX + 2];
};

struct Node
{
    bool m_is_leaf;

    IndexNode* m_index;

    Leaf* m_leaf;
    /*
     * @brief constructor of node
     * @param is_leaf leaf node or not
     */
    Node(bool is_leaf);
    /*
     * @brief get the size of data
     * @return size of data
     */
    size_t GetSize();
    /*
     * @brief compare the key with the key of data[i]
     * @return if key > data[i]'s key, return true
     */
    int Compare(size_t i, const char* key);
    /*
     * @brief get node's pointer as leaf node
     * @return leaf pointer
     */
    Leaf* GetLeaf();
    /*
     * @brief get node's pointer as index node
     * @return index pointer
     */
    IndexNode* GetIndex();

    size_t FindPos(const char* key);
};

class BPTree
{
public:
    /*
     * @brief constructor of bptree
     * @param filename of bptree
     */
    explicit BPTree(const std::string& filename);

    /*
     * @brief insert a leaf node
     * @return result of insert successful
     */
    bool Insert(const std::string& key, const void* value, size_t size);

    /*
     * @brief destructor of bptree
     */
    ~BPTree();

    /*
     * @brief traverse all tree
     * @param which node is the beginning
     */
    void Traverse(Node* node);
    void TraverseLeaf(Node* leaf_node);
    void TraverseIndex(Node* index_node);

    /*
     * @brief search one record by key
     * @param key key will be found
     * @return Data struct with record data in
     */
    Data Search(const std::string& key);

    /*
     * @brief delete children of node
     * @node node's children will delete
     * @return node
     */
    Node* Delete(Node* node);

    Node* GetRoot();

private:
    /*
     * @brief insert one node into a parent index node
     * @param key cursor's minimum key
     * @param cursor parent index node
     * @param child node should be insert
     * @return true for success
     */
    bool InsertInternal(const std::string& key, Node* cursor, Node* child);
    /*
     * @brief find child's parant from cursor
     * @param cursor child's ancestor
     * @param child the node will be found in tree
     */
    Node* FindParent(Node* cursor, Node* child);
    /*
     * @brief find a leaf node for key
     * @return <leaf, parent node of leaf>
     */
    std::pair<Node*, Node*> FindLeaf(const std::string& key);
    /*
     * @brief add a record in cur node
     */
    void AddRecord(Node* cur, const std::string& key, const void* value, size_t size);
    /**
     * @brief split the leaf node
     */
    Node* SplitLeafNode(Node* cur, const char* key, const void* value, size_t size);

    std::string m_file;
    Node* m_root;
};

#endif
