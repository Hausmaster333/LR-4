#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include "core/sequence.h"
#include "core/ienumerator.h"
#include "option.h"
#include "sequence.h"

template <class T>
class BinaryTree {
    private:
        struct TreeNode
        {
            T value;
            TreeNode* left;
            TreeNode* right;
            int height; // Для AVL

            TreeNode(const T& value) : value(value), left(nullptr), right(nullptr) {}
        };
        
        TreeNode* root;
        int count;

        TreeNode* insert_node(TreeNode* node, const T& value);
        TreeNode* find_node(TreeNode* node, const T& value) const;
        TreeNode* remove_node(TreeNode* node, const T& value);
        TreeNode* find_min(TreeNode* node) const;
        
        void destroy(TreeNode* node);
        TreeNode* copy_tree(const TreeNode* node) const;

        int get_height(TreeNode* node) const;
        int get_balance(TreeNode* node) const;
        void update_height(TreeNode* node);

        TreeNode* rotate_left(TreeNode* node);
        TreeNode* rotate_right(TreeNode* node);
        TreeNode* rebalance(TreeNode* node);

        void traverse_node(TreeNode* node, const std::string& other, Sequence<T>* out) const;

        TreeNode* map_node(TreeNode* node, T (*func)(const T&)) const;
        TreeNode* where_node(TreeNode* node, bool (*predicate)(const T&)) const;

        bool contains_subtree_node(TreeNode* hay, TreeNode* needle) const;
        bool trees_equal(TreeNode*a, TreeNode* b ) const;

        void save_node(TreeNode* node, const std::string& format, std::string& out) const;

    public:
        BinaryTree();
        BinaryTree(const BinaryTree<T>& other);
        
        BinaryTree<T>& operator=(const BinaryTree<T>& other);

        void insert(const T& value);
        bool contains(const T& value);
        void remove(const T& value);

        int get_count() const;
        bool is_empty() const;

        Sequence<T>* traverse(const std::string& order) const

        BinaryTree<T>* map(T (*func)(const T& elem)) const;
        BinaryTree<T>* where(bool (*predicate)(const T& elem)) const;
        T reduce(T (*func)(const T& first_elem, const T& second_elem), const T& initial_elem, const std::string& order) const;
        
        BinaryTree<T>* merge(const BinaryTree<T>* other) const;

        BinaryTree<T>* extract_subtree(const T& value) const;

        bool contains_subtree(const BinaryTree<T>* sub) const;

        std::string save_to_string(const std::string& format) const;
        
        static BinaryTree<T>* read_from_string(const std::string& text, const std::string& format);

        class Enumerator;

        IEnumerator<T>* get_enumerator() const;

        ~BinaryTree();
};

#include "trees/binary_tree.tpp"

#endif