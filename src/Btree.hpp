#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

template <typename Key, size_t N>
class BTree;

template <typename Key, size_t N>
struct Node final {
  private:
    using keys_t = std::vector<Key>;
    using sons_t = std::vector<Node *>;

    friend BTree<Key, N>;

    keys_t m_keys;
    sons_t m_sons;
    size_t m_counter = 0; // sizeof subtree
    Node *m_parent = nullptr;

  public:
    Node() = default;
    Node(const Node &) = delete;
    Node(Node &&) = delete;

    Node(const keys_t &keys, const sons_t &sons, size_t size, Node *parent = nullptr)
        : m_keys(keys), m_sons(sons), m_counter(size), m_parent(parent) {}

    Node(keys_t &&other_keys, sons_t &&other_sons, size_t size) noexcept
        : m_keys(std::move(other_keys)), m_sons(std::move(other_sons)), m_counter(size) {}

    Node &operator=(const Node &rhs) = delete;
    Node &operator=(Node &&rhs) = delete;

    ~Node() {
        for (Node *son : m_sons) {
            delete son;
        }
    }

    // returns number of keys in subtree with this node as root
    size_t lower_count(const Key &key) const {
        auto result = std::lower_bound(m_keys.begin(), m_keys.end(), key);
        size_t index = result - m_keys.begin();
        size_t counter = m_keys.size() - index;

        if (m_sons.empty()) {
            return counter;
        }
        for (int i = index + 1; i < m_sons.size(); ++i) {
            counter += m_sons[i]->count();
        }

        if (result == m_keys.end() || (result != m_keys.end() && *result != key)) {
            counter += m_sons[index]->lower_count(key);
        }
        return counter;
    }
    size_t upper_count(const Key &key) const {
        auto result = std::upper_bound(m_keys.begin(), m_keys.end(), key);
        size_t index = result - m_keys.begin();
        size_t counter = index;

        if (m_sons.empty()) {
            return counter;
        }
        for (int i = 0; i < index; ++i) {
            counter += m_sons[i]->count();
        }
        if (result == m_keys.end() || (result != m_keys.end() && *result != key)) {
            counter += m_sons[index]->upper_count(key);
        }
        return counter;
    }

    size_t count() const { return m_counter; }

    size_t distance(const Key &begin, const Key &end) const {
        auto first = std::lower_bound(m_keys.begin(), m_keys.end(), begin);
        auto second = std::lower_bound(m_keys.begin(), m_keys.end(), end);

        size_t counter = second - first + 1;
        size_t left_bound = first - m_keys.begin();
        size_t right_bound = second - m_keys.begin();
        if (m_sons.empty()) {
            if (second == m_keys.end() || *second != end) {
                counter -= 1;
            }
            return counter;
        }

        if (first == m_keys.end()) {
            return m_sons.back()->distance(begin, end);
        }

        if (first == second) {
            if (*first != begin && *second != end) {
                return m_sons[left_bound]->distance(begin, end);
            }
            if (*first == begin && *second == end) {
                return counter;
            }
        }
        if (*first != begin) {
            counter += m_sons[left_bound]->lower_count(begin);
        }
        if (second == m_keys.end() || *second != end) {
            counter += m_sons[right_bound]->upper_count(end);
            right_bound -= 1;
            counter -= 1;
        }
        for (int i = left_bound + 1; i < right_bound + 1; ++i) {
            counter += m_sons[i]->count();
        }
        return counter;
    }

    BTree<Key, N>::const_iterator find(const Key &key) const {
        auto result = std::lower_bound(m_keys.begin(), m_keys.end(), key);
        if (result != m_keys.end()) {
            if (*result == key) {
                return {this, result - m_keys.begin()};
            } else if (!m_sons.empty()) {
                return m_sons[result - m_keys.begin()]->find(key);
            }
            return {};
        }
        if (!m_sons.empty()) {
            return m_sons.back()->find(key);
        }
        return {};
    }

    BTree<Key, N>::iterator find(const Key &key) { return std::as_const(*this).find(key); }

    bool insert(const Key &key) {
        auto result = std::lower_bound(m_keys.begin(), m_keys.end(), key);
        size_t index = result - m_keys.begin();
        m_counter += 1; // update node counter

        if (m_sons.size() > index) {
            if (m_sons[index]->m_keys.size() == 2 * N - 1) {
                split(index);
            }
            if (index == m_keys.size()) {
                return m_sons.back()->insert(key);
            }

            if (m_keys[index] >= key) {
                return m_sons[index]->insert(key);
            }
            return m_sons[index + 1]->insert(key);
        }
        if (result == m_keys.end() || *result != key) {
            m_keys.insert(result, key);
            return true;
        }
        m_counter -= 1;
        return false;
    }

    bool erase(const Key &key) {
        auto result = std::lower_bound(m_keys.begin(), m_keys.end(), key);
        size_t index = result - m_keys.begin();
        m_counter -= 1;

        if (m_sons.empty()) {
            if (result != m_keys.end() && *result != key) {
                m_counter += 1;
                return false;
            }
            // leaf node
            m_keys.erase(result);
            return true;
        }

        if (result != m_keys.end() && *result == key) {
            // take key from mostleft son
            if (m_sons[index]->m_keys.size() > N - 1) {
                *result = max(m_sons[index + 1]);
                return m_sons[index]->erase(*result);
            }
            // take key from mostright son
            if (m_sons[index + 1]->m_keys.size() > N - 1) {
                *result = min(m_sons[index]);
                return m_sons[index + 1]->erase(*result);
            }
            erase_helper(index, index + 1);
            return m_sons[index]->erase(key);
        }

        // delete key from son node
        if (m_sons[index]->m_keys.size() == N - 1) {
            if (index < m_sons.size() - 1 && m_sons[index + 1]->m_keys.size() == N) {
                // take key from right son
                m_sons[index]->m_keys.push_back(m_keys[index]);

                if (!m_sons[index]->m_sons.empty()) {
                    m_sons[index]->m_sons.push_back(m_sons[index + 1]->m_sons.front());
                    m_sons[index + 1]->m_sons.erase(m_sons.begin());
                }

                m_keys[index] = m_sons[index + 1]->m_keys.front();
                m_sons[index + 1]->m_keys.erase(m_sons[index + 1]->m_keys.begin());

                m_sons[index + 1]->m_counter -= 1;
                m_sons[index]->m_counter += 1;

            } else if (index > 0 && m_sons[index - 1]->m_keys.size() == N) {
                // take key from left son
                m_sons[index]->m_keys.insert(m_sons[index]->m_keys.begin(), m_keys[index]);

                if (!m_sons[index]->m_sons.empty()) {
                    m_sons[index]->m_sons.insert(m_sons[index]->m_sons.begin(),
                                                 m_sons[index - 1]->m_sons.back());

                    m_sons[index - 1]->m_sons.pop_back();
                }

                m_keys[index] = m_sons[index - 1]->m_keys.back();
                m_sons[index - 1]->m_keys.pop_back();

                m_sons[index - 1]->m_counter -= 1;
                m_sons[index]->m_counter += 1;
            } else {
                // merge with right brother
                if (index < m_sons.size() - 1) {
                    erase_helper(index, index + 1);
                    return m_sons[index]->erase(key);
                }
                // merge with left brother
                erase_helper(index - 1, index);
                return m_sons[index - 1]->erase(key);
            }
        }
        return m_sons[index]->erase(key);
    }

    template <typename CharT>
    void dump(std::basic_ostream<CharT> &out) const {
        out << "\tnode" << this << " [shape=Mrecord, label=\"{";
        out << " size: " << m_counter << " | { ";

        for (size_t i = 0; i < m_keys.size() - 1; ++i) {
            out << m_keys[i] << " | ";
        }
        out << m_keys.back() << " }}\"]\n";
        if (!m_sons.empty()) {
            for (const Node *son : m_sons) {
                out << "\tnode" << this << " -> node" << son << "[weight=100]\n";
                son->dump(out);
            }
        }
        out << "\tnode" << this << " -> node" << m_parent << "[style=dashed]\n";
    }

  private:
    Node *split_self(size_t begin, size_t end) {
        keys_t node_keys(m_keys.begin() + begin, m_keys.begin() + end);
        sons_t node_sons;
        size_t node_counter = node_keys.size();

        if (m_sons.size() == 2 * N) {
            for (int i = 0; i < end - begin + 1; ++i) {
                node_sons.push_back(m_sons[i + begin]);
                node_counter += m_sons[i + begin]->count();
            }
        }

        Node *new_node = new Node(std::move(node_keys), std::move(node_sons), node_counter);

        for (const auto &son : new_node->m_sons) {
            son->m_parent = new_node;
        }
        return new_node;
    }

    // split son node
    void split(size_t son_index) {
        Node *son = m_sons[son_index];
        m_keys.insert(m_keys.begin() + son_index, son->m_keys[N - 1]);
        // create two nodes, delete old node

        Node *left = son->split_self(0, N - 1);
        Node *right = son->split_self(N, 2 * N - 1);

        right->m_parent = this;
        left->m_parent = this;

        m_sons[son_index] = left;
        m_sons.push_back(right);

        son->m_sons.clear();
        delete son;
    }
    Key max(const Node *node) {
        while (!node->m_sons.empty()) {
            node = node->m_sons.back();
        }
        return node->m_keys.back();
    }

    Key min(const Node *node) {
        while (!node->m_sons.empty()) {
            node = node->m_sons.front();
        }
        return node->m_keys.front();
    }

    void delete_son(size_t index) {
        m_sons[index]->m_sons.clear();
        delete m_sons[index];
    }

    void erase_helper(size_t left, size_t right) {
        Node *left_son = m_sons[left];
        Node *right_son = m_sons[right];

        left_son->m_keys.push_back(m_keys[left]);
        left_son->m_counter += right_son->m_counter + 1;

        left_son->m_keys.insert(left_son->m_keys.end(), right_son->m_keys.begin(),
                                right_son->m_keys.end());
        left_son->m_sons.insert(left_son->m_sons.end(), right_son->m_sons.begin(),
                                right_son->m_sons.end());

        delete_son(right);

        m_keys.erase(m_keys.begin() + left);
        m_sons.erase(m_sons.begin() + right);
    }
};

template <typename CharT, typename Key, size_t N>
std::basic_ostream<CharT> &operator<<(std::basic_ostream<CharT> &out, const Node<Key, N> &node) {
    dump(node);
    return out;
}

template <typename Key, size_t N>
class BTree final {
  private:
    using node_p = Node<Key, N> *;
    using value_type = Key;

    node_p m_root = nullptr;

    node_p deep_copy(node_p other, node_p parent) {
        node_p new_node = new Node<Key, N>(other->m_keys, {}, other->m_counter, parent);

        if (!other->m_sons.empty()) {
            for (const node_p son : other->m_sons) {
                new_node->m_sons.push_back(deep_copy(son, new_node));
            }
        }
        return new_node;
    }

  public:
    BTree() = default;
    BTree(std::initializer_list<Key> list) : m_root(new Node<Key, N>) {
        for (auto elem : list) {
            insert(elem);
        }
    }

    BTree(BTree<Key, N> &&other) : m_root(std::exchange(other.m_root, nullptr)) {}

    BTree(const BTree &other) : m_root(deep_copy(other.m_root, nullptr)) {}

    BTree &operator=(const BTree &other) {
        if (this == std::addressof(other)) {
            return *this;
        }
        BTree temp(other);
        std::swap(temp.m_root, m_root);

        return *this;
    }
    BTree &operator=(BTree &&other) {
        if (this == std::addressof(other)) {
            return *this;
        }
        BTree temp{other};

        std::swap(m_root, temp.m_root);
        return *this;
    }

    ~BTree() { delete m_root; }

    bool insert(const Key &key) {
        if (!m_root) {
            m_root = new Node<Key, N>({key}, {}, 1);
            return true;
        }
        if (m_root->m_keys.size() == 2 * N - 1) {
            node_p left_root = m_root->split_self(0, N - 1);
            node_p right_root = m_root->split_self(N, 2 * N - 1);
            node_p new_root =
                new Node<Key, N>({m_root->m_keys[N - 1]}, {left_root, right_root}, m_root->count());

            left_root->m_parent = new_root;
            right_root->m_parent = new_root;

            m_root->m_sons.clear();
            delete m_root;

            m_root = new_root;
        }
        return m_root->insert(key);
    }

    bool erase(const Key &key) {
        if (!m_root) {
            return false;
        }
        if (m_root->m_keys.size() == 1 && m_root->m_keys.front() == key) {
            m_root->erase_helper(0, 1);
            node_p old_root = m_root;
            m_root = m_root->m_sons.front();
            old_root->m_sons.pop_back();
            delete old_root;
        }
        return m_root->erase(key);
    }

    size_t distance(const Key &begin, const Key &end) const {
        if (end <= begin) {
            return 0;
        }
        return m_root->distance(begin, end);
    }

    template <typename CharT>
    friend std::basic_ostream<CharT> &operator<<(std::basic_ostream<CharT> &out,
                                                 const BTree<Key, N> &tree) {
        out << "digraph G {\n";
        if (tree.m_root) {
            tree.m_root->dump(out);
        } else {
            out << "\tnullptr\n";
        }
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
        using const_node_pointer = const Node<Key, N> *;

      private:
        const_node_pointer node;
        ssize_t position;

      public:
        base_iterator(const_node_pointer node, ssize_t pos) : node(node), position(pos) {}
        base_iterator() : node(nullptr), position(0) {}

        const_reference operator*() const { return node->m_keys[position]; }
        const_pointer operator->() const { return &node->m_keys[position]; }
        base_iterator &operator++() {
            position += 1;
            if (position < node->m_sons.size() || position < node->m_keys.size()) {
                while (!node->m_sons.empty()) {
                    node = node->m_sons[position];
                    position = 0;
                }
                return *this;
            }
            position = node->m_keys.size();
            while (position == node->m_keys.size() && node->m_parent) {
                // TODO rewrite to std::upper_bound
#if 0
                auto temp = node->keys[position];
                node = node->parent;
                position = 
                    std::lower_bound(node->keys.begin(), node->keys.end(), temp) -
                    node->keys.begin()
#else
                auto temp = node;
                node = node->m_parent;
                position = std::find(node->m_sons.begin(), node->m_sons.end(), temp) -
                           node->m_sons.begin();
#endif
            }

            return *this;
        }
        base_iterator operator++(int) {
            base_iterator temp = *this;
            ++(*this);
            return temp;
        }

        // TODO implement
        base_iterator &operator--() {}

        base_iterator operator--(int) {
            base_iterator temp = *this;
            --(*this);
            return temp;
        };

        friend bool operator==(const base_iterator &lhs, const base_iterator &rhs) {
            return (lhs.node == rhs.node) && (lhs.position == rhs.position);
        }
        friend bool operator!=(const base_iterator &lhs, const base_iterator &rhs) {
            return !(lhs == rhs);
        }
    };

    static_assert(std::bidirectional_iterator<base_iterator>);

  public:
    using const_iterator = base_iterator;
    using iterator = base_iterator;

    const_iterator find(const Key &key) const {
        auto result = m_root->find(key);
        return (result == const_iterator() ? cend() : result);
    }

    iterator find(const Key &key) { return std::as_const(*this).find(key); }

    const_iterator cbegin() const {
        node_p begin = m_root;
        if (begin) {
            while (begin->m_sons.size()) {
                begin = begin->m_sons.front();
            }
        }
        return const_iterator(begin, 0);
    }
    const_iterator cend() const {
        return (m_root ? const_iterator(m_root, m_root->m_keys.size()) : const_iterator());
    }

    iterator begin() { return std::as_const(*this).cbegin(); }
    iterator end() { return std::as_const(*this).cend(); }
};
