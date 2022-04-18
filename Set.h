#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <utility>

// Ordered set of elements with insert, erase, find and lower_bound methods, implemented with using AA-tree
template<typename ValueType>
class Set {
  private:
    // Vertex of the AA-tree
    struct Node {
        ValueType value;
        size_t level = BASIC_LEVEL;
        Node* parent = nullptr;
        Node* left_son = nullptr;
        Node* right_son = nullptr;

        static constexpr size_t BASIC_LEVEL = 1;

        Node(const ValueType& value)
            : value(value)
        {}
    };

  public:
    Set() = default;

    template<typename FirstIterator, typename LastIterator>
    Set(FirstIterator begin, LastIterator end) {
        while (begin != end) {
            Node* inserted_node = nullptr;
            tree_root_ = Insert(tree_root_, *begin, inserted_node);
            ++begin;
        }
    }

    Set(std::initializer_list<ValueType> elements) {
        for (const auto& value: elements) {
            Node* inserted_node = nullptr;
            tree_root_ = Insert(tree_root_, value, inserted_node);
        }
    }

    Set(const Set& s) {
        set_size_ = s.set_size_;
        tree_root_ = Copy(s.tree_root_);
    }

    Set(Set&& s) {
        std::swap(s.tree_root_, tree_root_);
        set_size_ = s.set_size_;
    }

    Set& operator=(const Set& s) {
        if (&s == this) {
            return *this;
        }
        Delete(tree_root_);
        set_size_ = s.set_size_;
        tree_root_ = Copy(s.tree_root_);
        return *this;
    }

    Set& operator=(Set&& s) {
        std::swap(tree_root_, s.tree_root_);
        set_size_ = s.set_size_;
        return *this;
    }

    // Iterator of the element of the set
    class iterator {
      public:
        iterator(const Set* iterator_owner, const Node* current_vertex)
            : iterator_owner(iterator_owner)
            , current_vertex(current_vertex)
        {}

        iterator(const iterator& it)
            : iterator_owner(it.iterator_owner)
            , current_vertex(it.current_vertex)
        {}

        iterator() : iterator_owner(nullptr), current_vertex(nullptr) {}

        iterator& operator=(const iterator& it) {
            iterator_owner = it.iterator_owner;
            current_vertex = it.current_vertex;
            return *this;
        }

        const ValueType& operator*() const {
            return current_vertex->value;
        }

        const ValueType* operator->() const {
            return &current_vertex->value;
        }

        // Finds iterator of the next element by value, complexity O(log n), average complexity O(1)
        iterator& operator++() {
            current_vertex = iterator_owner->Next(current_vertex);
            return *this;
        }

        iterator operator++(int) {
            iterator ans = *this;
            current_vertex = iterator_owner->Next(current_vertex);
            return ans;
        }

        // Finds iterator of the previous element by value, complexity O(log n), average complexity O(1)
        iterator& operator--() {
            if (current_vertex == nullptr) {
                current_vertex = iterator_owner->tree_root_;
                while (current_vertex->right_son != nullptr) {
                    current_vertex = current_vertex->right_son;
                }
                return *this;
            }
            current_vertex = iterator_owner->Prev(current_vertex);
            return *this;
        }

        iterator operator--(int) {
            iterator ans = *this;
            if (current_vertex == nullptr) {
                current_vertex = iterator_owner->tree_root_;
                while (current_vertex->right_son != nullptr) {
                    current_vertex = current_vertex->right_son;
                }
                return *this;
            }
            current_vertex = iterator_owner->Prev(current_vertex);
            return ans;

        }

        bool operator!=(const iterator& it) const {
            return it.current_vertex != current_vertex || it.iterator_owner != iterator_owner;
        }

        bool operator==(const iterator& it) const {
            return !(*this != it);
        }

      private:
        const Set* iterator_owner;
        const Node* current_vertex;
    };

    // If given value isn't in the set - inserts it, returns iterator of element with given value and boolean that
    // equals true if value was inserted, complexity O(log n)
    std::pair<iterator, bool> insert(const ValueType& value) {
        Node* inserted_vertex = nullptr;
        size_t previous_size = set_size_;
        tree_root_ = Insert(tree_root_, value, inserted_vertex);
        return {iterator(this, inserted_vertex), set_size_ == previous_size};
    }

    // If given value is in the set - erase it, returns amount of erased elements, complexity O(log n)
    size_t erase(const ValueType& value) {
        size_t previous_size = set_size_;
        tree_root_ = Erase(tree_root_, value);
        return previous_size - set_size_;
    }

    // If given value is in the set - returns iterator of the element with this value,
    // else - returns end(), complexity O(log n)
    iterator find(const ValueType& value) const {
        return iterator(this, Find(tree_root_, value));
    }

    // Returns iterator of the element with the smallest value not less, then given,
    // or end() if this element doesn't exist, complexity O(log n)
    iterator lower_bound(const ValueType& value) const {
        return iterator(this, LowerBound(tree_root_, value));
    }

    // Returns iterator of the first element of the set, or end() if set is empty(), complexity O(log n)
    iterator begin() const {
        Node* t = tree_root_;
        while (t != nullptr && t->left_son != nullptr) {
            t = t->left_son;
        }
        return iterator(this, t);
    }

    // Returns iterator of the end of the set, complexity O(1)
    iterator end() const {
        return iterator(this, nullptr);
    }

    // Returns amount of elements the set contains, complexity O(1)
    size_t size() const {
        return set_size_;
    }

    // Returns true if set is empty, or false if it isn't, complexity O(1)
    bool empty() const {
        return size() == EMPTY_SIZE;
    }

    ~Set() {
        Delete(tree_root_);
    }

    static constexpr size_t EMPTY_SIZE = 0;

  private:
    // Rotates the given vertex to balance level, according to the left son, complexity O(1)
    Node* Skew(Node* vertex) {
        if (vertex->left_son == nullptr || vertex->left_son->level != vertex->level) {
            return vertex;
        }
        Node* s = vertex->left_son;
        vertex->left_son = s->right_son;
        if (s->right_son != nullptr) {
            s->right_son->parent = vertex;
        }
        s->right_son = vertex;
        s->parent = vertex->parent;
        vertex->parent = s;
        return s;
    }

    // Rotates the given vertex to balance level, according to the right son, complexity O(1)
    Node* Split(Node* vertex) {
        if (vertex->right_son == nullptr || vertex->right_son->right_son == nullptr ||
            vertex->level != vertex->right_son->right_son->level) {
            return vertex;
        }
        Node* s = vertex->right_son;
        vertex->right_son = s->left_son;
        if (s->left_son != nullptr) {
            s->left_son->parent = vertex;
        }
        s->left_son = vertex;
        s->parent = vertex->parent;
        vertex->parent = s;
        ++s->level;
        return s;
    }

    // Insert value it the AA-tree, returns root of the modified tree and vertex with the given value,
    // complexity O(log n)
    Node* Insert(Node* t, const ValueType& value, Node*& inserted_vertex) {
        if (t == nullptr) {
            inserted_vertex = new Node(value);
            ++set_size_;
            return inserted_vertex;
        }
        if (value < t->value) {
            t->left_son = Insert(t->left_son, value, inserted_vertex);
            t->left_son->parent = t;
        } else if (t->value < value) {
            t->right_son = Insert(t->right_son, value, inserted_vertex);
            t->right_son->parent = t;
        } else {
            inserted_vertex = t;
            return t;
        }
        t = Skew(t);
        t = Split(t);
        return t;
    }

    // Balances level of given vertex, complexity O(1)
    void DecreaseLevel(Node* vertex) {
        size_t expected_level;
        if (vertex->left_son == nullptr || vertex->right_son == nullptr) {
            expected_level = 1;
        } else {
            expected_level = std::min(vertex->left_son->level, vertex->right_son->level) + 1;
        }
        if (vertex->level > expected_level) {
            vertex->level = expected_level;
            if (vertex->right_son != nullptr && vertex->right_son->level > expected_level) {
                vertex->right_son->level = expected_level;
            }
        }
    }

    // Returns son with the first value that is greater than the value of the give vertex, complexity O(log n)
    const Node* Successor(const Node* vertex) const {
        vertex = vertex->right_son;
        while (vertex->left_son != nullptr) {
            vertex = vertex->left_son;
        }
        return vertex;
    }

    // Returns son with the first value that is less than the value of the give vertex, complexity O(log n)
    const Node* Predecessor(const Node* vertex) const {
        vertex = vertex->left_son;
        while (vertex->right_son != nullptr) {
            vertex = vertex->right_son;
        }
        return vertex;
    }

    // Erases value from the AA-tree if it contains it, returns root of the modified tree, complexity O(log n)
    Node* Erase(Node* vertex, const ValueType& value) {
        if (vertex == nullptr) {
            return nullptr;
        }
        if (value < vertex->value) {
            vertex->left_son = Erase(vertex->left_son, value);
        } else if (vertex->value < value) {
            vertex->right_son = Erase(vertex->right_son, value);
        } else {
            if (vertex->left_son == nullptr && vertex->right_son == nullptr) {
                delete vertex;
                --set_size_;
                return nullptr;
            }
            if (vertex->left_son == nullptr) {
                const Node* s = Successor(vertex);
                vertex->value = s->value;
                vertex->right_son = Erase(vertex->right_son, s->value);
            } else {
                const Node* s = Predecessor(vertex);
                vertex->value = s->value;
                vertex->left_son = Erase(vertex->left_son, s->value);
            }
        }
        DecreaseLevel(vertex);
        if (vertex->right_son != nullptr) {
            vertex->right_son = Skew(vertex->right_son);
            if (vertex->right_son->right_son != nullptr) {
                vertex->right_son->right_son = Skew(vertex->right_son->right_son);
            }
        }
        vertex = Split(vertex);
        if (vertex->right_son != nullptr) {
            vertex->right_son = Split(vertex->right_son);
        }
        return vertex;
    }

    // Returns vertex with the first value that is greater than the value of the given vertex, complexity O(log n)
    const Node* Next(const Node* vertex) const {
        if (vertex->right_son != nullptr) {
            return Successor(vertex);
        }
        while (vertex->parent != nullptr && vertex->parent->left_son != vertex) {
            vertex = vertex->parent;
        }
        return vertex->parent;
    }

    // Returns vertex with the first value that is less than the value of the given vertex, complexity O(log n)
    const Node* Prev(const Node* vertex) const {
        if (vertex->left_son != nullptr) {
            return Predecessor(vertex);
        }
        while (vertex->parent != nullptr && vertex->parent->right_son != vertex) {
            vertex = vertex->parent;
        }
        return vertex->parent;
    }

    // Returns root of the copied version of the tree with the given root, complexity O(n)
    Node* Copy(Node* vertex) {
        if (vertex == nullptr) {
            return nullptr;
        }
        Node* copied_vertex = new Node(vertex->value);
        copied_vertex->level = vertex->level;
        copied_vertex->left_son = Copy(vertex->left_son);
        copied_vertex->right_son = Copy(vertex->right_son);
        if (copied_vertex->left_son != nullptr) {
            copied_vertex->left_son->parent = copied_vertex;
        }
        if (copied_vertex->right_son != nullptr) {
            copied_vertex->right_son->parent = copied_vertex;
        }
        return copied_vertex;
    }

    // Returns vertex with the given value if tree with the given root contains it, or nullptr if it isn't,
    // complexity O(log n)
    const Node* Find(const Node* vertex, const ValueType& value) const {
        const Node* ans = nullptr;
        while (vertex != nullptr) {
            if (value < vertex->value) {
                vertex = vertex->left_son;
            } else if (vertex->value < value) {
                vertex = vertex->right_son;
            } else {
                return vertex;
            }
        }
        return ans;
    }

    // Returns vertex in the tree with the given root with the first value that is not less than the given value,
    // or nullptr if given tree doesn't contain it, complexity O(log n)
    const Node* LowerBound(const Node* vertex, const ValueType& value) const {
        const Node* ans = nullptr;
        while (vertex != nullptr) {
            if (value < vertex->value) {
                ans = vertex;
                vertex = vertex->left_son;
            } else if (vertex->value < value) {
                vertex = vertex->right_son;
            } else {
                return vertex;
            }
        }
        return ans;
    }

    // Deletes all vertexes of the tree with the given root, complexity O(n)
    void Delete(Node* vertex) {
        if (vertex != nullptr) {
            Delete(vertex->left_son);
            Delete(vertex->right_son);
            delete vertex;
        }
    }

    Node* tree_root_ = nullptr;
    size_t set_size_ = EMPTY_SIZE;
};
