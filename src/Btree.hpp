#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stack>
#include <vector>

// TODO remove cout messages

template <typename Key, size_t N>
class BTree;

template <typename Key, size_t N>
struct Node {
  private:
    using keys_t = std::vector<Key>;
    using sons_t = std::vector<Node *>;

    friend BTree<Key, N>;

    template <typename T, size_t U>
    friend std::ofstream &operator<<(std::ofstream &out, const Node<T, U> &node);

    keys_t keys;
    sons_t sons;
    size_t counter; // sizeof subtree
    Node *parent = nullptr;

  public:
    Node() : counter(0) {}
    Node(const keys_t &keys, const sons_t &sons, size_t size)
        : keys(keys), sons(sons), counter(size) {}

    Node(keys_t &&other_keys, sons_t &&other_sons, size_t size)
        : keys(other_keys), sons(other_sons), counter(size) {}

    ~Node() {
        for (Node *son : sons) {
            delete son;
        }
    }

    // returns number of keys in subtree with this node as root
    size_t lower_count(const Key &key) const {
        auto result = std::lower_bound(keys.begin(), keys.end(), key);
        size_t index = result - keys.begin();
        size_t counter = keys.size() - index;

        if (sons.empty()) {
            return counter;
        }
        for (int i = index + 1; i < sons.size(); ++i) {
            counter += sons[i]->count();
        }

        if (result == keys.end() || (result != keys.end() && *result != key)) {
            counter += sons[index]->lower_count(key);
        }
        return counter;
    }
    size_t upper_count(const Key &key) const {
        auto result = std::upper_bound(keys.begin(), keys.end(), key);
        size_t index = result - keys.begin();
        size_t counter = index;

        if (sons.empty()) {
            return counter;
        }
        for (int i = 0; i < index; ++i) {
            counter += sons[i]->count();
        }
        if (result == keys.end() || (result != keys.end() && *result != key)) {
            counter += sons[index]->upper_count(key);
        }
        return counter;
    }

    size_t count() const { return counter; }

    size_t distance(const Key &begin, const Key &end) const {
        auto first = std::lower_bound(keys.begin(), keys.end(), begin);
        auto second = std::lower_bound(keys.begin(), keys.end(), end);

        size_t counter = second - first + 1;
        size_t left_bound = first - keys.begin();
        size_t right_bound = second - keys.begin();
        if (sons.empty()) {
            return counter;
        }

        if (first == keys.end()) {
            return sons.back()->distance(begin, end);
        }

        if (first == second) {
            if (*first != begin && *second != end) {
                return sons[left_bound]->distance(begin, end);
            }
            if (*first == begin && *second == end) {
                return counter;
            }
        }
        if (*first != begin) {
            counter += sons[left_bound]->lower_count(begin);
        }
        if (second == keys.end() || *second != end) {
            counter += sons[right_bound]->upper_count(end);
            right_bound -= 1;
            counter -= 1;
        }
        for (int i = left_bound + 1; i < right_bound + 1; ++i) {
            counter += sons[i]->count();
        }
        return counter;
    }

    BTree<Key, N>::const_iterator find(const Key &key) const {
        auto result = std::lower_bound(keys.begin(), keys.end(), key);
        if (result != keys.end()) {
            if (*result == key) {
                return {this, result - keys.begin()};
            } else if (!sons.empty()) {
                return sons[result - keys.begin()]->find(key);
            }
            return {};
        }
        if (!sons.empty()) {
            return sons.back()->find(key);
        }
        return {};
    }

    BTree<Key, N>::iterator find(const Key &key) {
        auto result = std::lower_bound(keys.begin(), keys.end(), key);
        if (result != keys.end()) {
            if (*result == key) {
                return {this, result - keys.begin()};
            } else if (!sons.empty()) {
                return sons[result - keys.begin()]->find(key);
            }
            return {};
        }
        if (!sons.empty()) {
            return sons.back()->find(key);
        }
        return {};
    }

    bool insert(const Key &key) {
        auto result = std::lower_bound(keys.begin(), keys.end(), key);
        size_t index = result - keys.begin();
        counter += 1; // update node counter

        if (sons.size() > index) {
            if (sons[index]->keys.size() == 2 * N - 1) {
                split(index);
            }
            if (index == keys.size()) {
                return sons.back()->insert(key);
            }

            if (keys[index] >= key) {
                return sons[index]->insert(key);
            }
            return sons[index + 1]->insert(key);
        }
        keys.insert(result, key);
        return true;
    }

  private:
    Key max(const Node *node) {
        while (!node->sons.empty()) {
            node = node->sons.back();
        }
        return node->keys.back();
    }

    Key min(const Node *node) {
        while (!node->sons.empty()) {
            node = node->sons.front();
        }
        return node->keys.front();
    }

    void delete_son(size_t index) {
        sons[index]->sons.clear();
        delete sons[index];
    }

    void erase_helper(size_t left, size_t right) {
        sons[left]->keys.push_back(keys[left]);
        sons[left]->counter += sons[right]->counter + 1;

        std::cout << "pushing key from right to left\n";
        for (const auto &key : sons[right]->keys) {
            sons[left]->keys.push_back(key);
        }
        std::cout << "pushing son_ptr from right to left\n";
        for (const auto &son_ptr : sons[right]->sons) {
            sons[left]->sons.push_back(son_ptr);
        }

        delete_son(right);

        std::cout << "erasing right son\n";

        keys.erase(keys.begin() + left);
        sons.erase(sons.begin() + right);
    }

  public:
    bool erase(const Key &key) {
        std::cout << "erasing..\n";
        auto result = std::lower_bound(keys.begin(), keys.end(), key);
        size_t index = result - keys.begin();
        counter -= 1;

        if (sons.empty()) {
            if (result != keys.end() && *result != key) {
                counter += 1;
                return false;
            }
            // leaf node
            std::cout << "erasing from leaf\n";
            keys.erase(result);
            return true;
        }

        if (result != keys.end() && *result == key) {
            // take key from mostleft son
            if (sons[index]->keys.size() > N - 1) {
                std::cout << "take key from mostleft son\n";
                *result = max(sons[index + 1]);
                return sons[index]->erase(*result);
            }
            // take key from mostright son
            if (sons[index + 1]->keys.size() > N - 1) {
                std::cout << "take key from mostright son\n";
                *result = min(sons[index]);
                return sons[index + 1]->erase(*result);
            }
            std::cout << "move keys from right son to left\n";
            erase_helper(index, index + 1);
            return sons[index]->erase(key);
        }

        // delete key from son node
        std::cout << "delete from son\n";
        if (sons[index]->keys.size() == N - 1) {
            if (index < sons.size() - 1 && sons[index + 1]->keys.size() == N) {
                // take key from right son
                std::cout << "take key from right son\n";

                sons[index]->keys.push_back(keys[index]);

                if (!sons[index]->sons.empty()) {
                    sons[index]->sons.push_back(sons[index + 1]->sons.front());
                    sons[index + 1]->sons.erase(sons.begin());
                }

                keys[index] = sons[index + 1]->keys.front();
                sons[index + 1]->keys.erase(sons[index + 1]->keys.begin());

                sons[index + 1]->counter -= 1;
                sons[index]->counter += 1;

            } else if (index > 0 && sons[index - 1]->keys.size() == N) {
                // take key from left son
                std::cout << "take key from left son\n";
                sons[index]->keys.insert(sons[index]->keys.begin(), keys[index]);

                if (!sons[index]->sons.empty()) {
                    sons[index]->sons.insert(sons[index]->sons.begin(),
                                             sons[index - 1]->sons.back());

                    sons[index - 1]->sons.pop_back();
                }

                keys[index] = sons[index - 1]->keys.back();
                sons[index - 1]->keys.pop_back();

                sons[index - 1]->counter -= 1;
                sons[index]->counter += 1;
            } else {
                // merge with right brother
                if (index < sons.size() - 1) {
                    erase_helper(index, index + 1);
                    return sons[index]->erase(key);
                }
                // merge with left brother
                erase_helper(index - 1, index);
                return sons[index - 1]->erase(key);
            }
        }
        return sons[index]->erase(key);
    }

    Node *split_self(size_t begin, size_t end) {
        keys_t node_keys(keys.begin() + begin, keys.begin() + end);
        sons_t node_sons;
        size_t node_counter = node_keys.size();

        if (sons.size() == 2 * N) {
            for (int i = 0; i < end - begin + 1; ++i) {
                node_sons.push_back(sons[i + begin]);
                node_counter += sons[i + begin]->counter;
            }
        }

        Node *new_node = new Node(std::move(node_keys), std::move(node_sons), node_counter);

        for (const auto &son : new_node->sons) {
            son->parent = new_node;
        }
        return new_node;
    }

    // split son node
    void split(size_t son_index) {
        Node *son = sons[son_index];
        keys.insert(keys.begin() + son_index, son->keys[N - 1]);
        // create two nodes, delete old node

        Node *left = son->split_self(0, N - 1);
        Node *right = son->split_self(N, 2 * N - 1);

        right->parent = this;
        left->parent = this;

        sons[son_index] = left;
        sons.push_back(right);

        son->sons.clear();
        delete son;
    }
};

template <typename Key, size_t N>
std::ofstream &operator<<(std::ofstream &out, const Node<Key, N> &node) {
    out << "\tnode" << &node << " [shape=Mrecord, label=\"{";
    out << " size: " << node.counter << " | { ";

    for (size_t i = 0; i < node.keys.size() - 1; ++i) {
        out << node.keys[i] << " | ";
    }
    out << node.keys.back() << " }}\"]\n";
    if (!node.sons.empty()) {
        for (const Node<Key, N> *son : node.sons) {
            out << "\tnode" << &node << " -> node" << son << "[weight=100]\n";
            out << *son;
        }
    }
    out << "\tnode" << &node << " -> node" << node.parent << "[style=dashed]\n";
    return out;
}

template <typename Key, size_t N>
class BTree {
  private:
    using node_p = Node<Key, N> *;
    using value_type = Key;

    node_p root;

  public:
    BTree() : root(nullptr) {}
    BTree(std::initializer_list<Key> list) : root(new Node<Key, N>) {
        for (auto elem : list) {
            root->insert(elem);
        }
    }
    BTree(const BTree<Key, N> &other) = delete;

    ~BTree() { delete root; }

    bool insert(const Key &key) {
        if (!root) {
            root = new Node<Key, N>({key}, {}, 1);
            return true;
        }
        if (root->keys.size() == 2 * N - 1) {
            node_p left_root = root->split_self(0, N - 1);
            node_p right_root = root->split_self(N, 2 * N - 1);
            node_p new_root =
                new Node<Key, N>({root->keys[N - 1]}, {left_root, right_root}, root->counter);

            left_root->parent = new_root;
            right_root->parent = new_root;

            root->sons.clear();
            delete root;

            root = new_root;
        }
        return root->insert(key);
    }

    bool erase(const Key &key) {
        if (!root) {
            return false;
        }
        if (root->keys.size() == 1 && root->keys.front() == key) {
            root->erase_helper(0, 1);
            node_p old_root = root;
            root = root->sons.front();
            old_root->sons.pop_back();
            delete old_root;
        }
        return root->erase(key);
    }

    size_t distance(const Key &begin, const Key &end) const { return root->distance(begin, end); }

    friend std::ofstream &operator<<(std::ofstream &out, const BTree<Key, N> &tree) {
        out << "digraph G {\n";
        out << *tree.root;
        out << "}\n";
        return out;
    }

  private:
    struct base_iterator {
      public:
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = BTree::value_type;
        using const_pointer = const value_type *;
        using const_reference = const value_type &;
        using node_pointer = BTree::node_p;

      private:
        friend BTree;

        node_pointer node;
        size_t position;

      public:
        base_iterator(node_pointer node, size_t pos) : node(node), position(pos) {}
        base_iterator() : node(nullptr), position(0) {}

        const_reference operator*() const { return node->keys[position]; }
        const_pointer operator->() const { return &node->keys[position]; }
        base_iterator &operator++() {
            position += 1;
            if (position < node->sons.size() || position < node->keys.size()) {
                while (!node->sons.empty()) {
                    node = node->sons[position];
                    position = 0;
                }
                return *this;
            }
            position = node->keys.size();
            while (position == node->keys.size() && node->parent) {
                // TODO rewrite to std::upper_bound
                /*
                    auto temp = node->keys[position];
                    node = node->parent;
                    position = 
                        std::upper_bound(node->keys.begin(), node->keys.end(), temp) -
                        node->keys.begin()

                */
                auto temp = node;
                node = node->parent;
                position =
                    std::find(node->sons.begin(), node->sons.end(), temp) - node->sons.begin();
            }

            return *this;
        }
        base_iterator operator++(int) {
            base_iterator temp = *this;
            ++(*this);
            return temp;
        }

        base_iterator &operator--(){};
        base_iterator operator--(int){};

        friend bool operator==(const base_iterator &lhs, const base_iterator &rhs) {
            return (lhs.node == rhs.node) && (lhs.position == rhs.position);
        }
        friend bool operator!=(const base_iterator &lhs, const base_iterator &rhs) {
            return (lhs.node != rhs.node) || (lhs.position != rhs.position);
        }
    };

    static_assert(std::bidirectional_iterator<base_iterator>);

  public:
    using const_iterator = base_iterator;
    using iterator = base_iterator;

    iterator find(const Key &key) {
        auto result = root->find(key);
        return (result == iterator() ? end() : result);
    }
    const_iterator find(const Key &key) const {
        auto result = root->find(key);
        return (result == const_iterator() ? cend() : result);
    }

    iterator begin() {
        node_p begin = root;
        while (begin->sons.size()) {
            begin = begin->sons.front();
        }
        return iterator(begin, 0);
    }
    iterator end() { return iterator(root, root->keys.size()); }

    const_iterator cbegin() const {
        node_p begin = root;
        while (begin->sons.size()) {
            begin = begin->sons.front();
        }
        return const_iterator(begin, 0);
    }
    const_iterator cend() const { return const_iterator(root, root->keys.size()); }
};
