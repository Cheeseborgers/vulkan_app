#pragma once
/**
 * @file small_vector.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module
 *
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <iterator>
#include <memory>

namespace gouda {

/**
 * @brief Growth policy that doubles the capacity each time.
 */
struct GrowthPolicyDouble {
    static size_t next(size_t current) { return current ? current * 2 : 1; }
};

/**
 * @brief Growth policy that increases capacity by 1.5x.
 */
struct GrowthPolicyOnePointFive {
    static size_t next(size_t current) { return current < 2 ? 2 : static_cast<size_t>(current * 1.5); }
};

/**
 * @brief Growth policy that adds one to the capacity each time.
 */
struct GrowthPolicyAddOne {
    static size_t next(size_t current) { return current + 1; }
};

/**
 * @brief SmallVector is a dynamic array optimized for small sizes.
 *        It uses a statically allocated buffer for the first N elements
 *        and falls back to heap allocation beyond that.
 * @tparam T Element type
 * @tparam N Stack capacity
 * @tparam GrowthPolicy Policy used to determine growth behavior
 */
template <typename T, size_t N, typename GrowthPolicy = GrowthPolicyOnePointFive>
class SmallVector {
public:
    /**
     * @brief Default constructor initializes with empty vector using stack storage.
     */
    SmallVector() : m_data(stack_data()), m_size(0), m_capacity(N) {}

    /**
     * @brief Destructor clears the vector and frees any heap memory if used.
     */
    ~SmallVector()
    {
        clear();
        if (m_data != stack_data()) {
            ::operator delete[](m_data);
        }
    }

    /**
     * @brief Move constructor transfers ownership from another SmallVector.
     * @param other The SmallVector to move from.
     */
    SmallVector(SmallVector &&other) noexcept : m_data(other.m_data), m_size(other.m_size), m_capacity(other.m_capacity)
    {
        if (m_data != other.stack_data()) {
            other.m_data = nullptr;
        }
        other.m_size = 0;
        other.m_capacity = 0;
    }

    /**
     * @brief Move assignment operator transfers ownership and cleans up current data.
     * @param other The SmallVector to move from.
     * @return Reference to this SmallVector.
     */
    SmallVector &operator=(SmallVector &&other) noexcept
    {
        if (this != &other) {
            clear();
            if (m_data != stack_data()) {
                ::operator delete[](m_data);
            }
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            if (m_data != other.stack_data()) {
                other.m_data = nullptr;
            }
            other.m_size = 0;
            other.m_capacity = 0;
        }
        return *this;
    }

    /**
     * @brief Copy constructor duplicates the contents of another SmallVector.
     * @param other The SmallVector to copy from.
     */
    SmallVector(const SmallVector &other) : m_data(stack_data()), m_size(other.m_size), m_capacity(other.m_capacity)
    {
        if (other.m_data != other.stack_data()) {
            m_data = static_cast<T *>(::operator new[](sizeof(T) * m_capacity));
        }
        std::uninitialized_copy(other.begin(), other.end(), m_data);
    }

    /**
     * @brief Copy assignment operator via copy-and-swap idiom.
     * @param other The SmallVector to copy from.
     * @return Reference to this SmallVector.
     */
    SmallVector &operator=(const SmallVector &other)
    {
        if (this != &other) {
            SmallVector temp(other);
            swap(temp);
        }
        return *this;
    }

    /**
     * @brief Swaps this vector with another.
     * @param other The SmallVector to swap with.
     */
    void swap(SmallVector &other) noexcept
    {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);
    }

    /**
     * @brief Adds a copy of the value to the end of the vector.
     * @param value The value to add.
     */
    void push_back(const T &value)
    {
        if (m_size == m_capacity)
            grow();
        new (m_data + m_size++) T(value);
    }

    /**
     * @brief Adds a moved value to the end of the vector.
     * @param value The value to add.
     */
    void push_back(T &&value)
    {
        if (m_size == m_capacity)
            grow();
        new (m_data + m_size++) T(std::move(value));
    }

    /**
     * @brief Constructs an element in place at the end.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to T's constructor.
     * @return Reference to the constructed element.
     */
    template <typename... Args>
    T &emplace_back(Args &&...args)
    {
        if (m_size == m_capacity)
            grow();
        T *ptr = new (m_data + m_size++) T(std::forward<Args>(args)...);
        return *ptr;
    }

    /**
     * @brief Ensures that the vector has at least the specified capacity.
     * @param new_cap New minimum capacity.
     */
    void reserve(size_t new_cap)
    {
        if (new_cap <= m_capacity)
            return;

        T *new_data = static_cast<T *>(::operator new[](sizeof(T) * new_cap));
        for (size_t i = 0; i < m_size; ++i) {
            new (new_data + i) T(std::move_if_noexcept(m_data[i]));
            m_data[i].~T();
        }

        if (m_data != stack_data()) {
            ::operator delete[](m_data);
        }

        m_data = new_data;
        m_capacity = new_cap;
    }

    /**
     * @brief Resizes the vector to the specified size.
     *        Constructs or destroys elements as needed.
     * @param new_size New size of the vector.
     */
    void resize(size_t new_size)
    {
        if (new_size < m_size) {
            for (size_t i = new_size; i < m_size; ++i)
                m_data[i].~T();
        }
        else if (new_size > m_size) {
            reserve(new_size);
            for (size_t i = m_size; i < new_size; ++i)
                new (m_data + i) T();
        }
        m_size = new_size;
    }

    /**
     * @brief Reduces capacity to match current size.
     */
    void shrink_to_fit()
    {
        if (m_capacity > m_size) {
            T *new_data = static_cast<T *>(::operator new[](sizeof(T) * m_size));
            for (size_t i = 0; i < m_size; ++i) {
                new (new_data + i) T(std::move(m_data[i]));
                m_data[i].~T();
            }
            if (m_data != stack_data()) {
                ::operator delete[](m_data);
            }
            m_data = new_data;
            m_capacity = m_size;
        }
    }

    /**
     * @brief Provides access to an element by index.
     * @param i Index of the element.
     * @return Reference to the element.
     */
    T &operator[](size_t i) { return m_data[i]; }

    /**
     * @brief Provides read-only access to an element by index.
     * @param i Index of the element.
     * @return Const reference to the element.
     */
    const T &operator[](size_t i) const { return m_data[i]; }

    /**
     * @brief Equality comparison operator.
     * @param other The SmallVector to compare with.
     * @return True if both vectors are equal, false otherwise.
     */
    bool operator==(const SmallVector &other) const
    {
        return m_size == other.m_size && std::equal(begin(), end(), other.begin());
    }

    /**
     * @brief Inequality comparison operator.
     * @param other The SmallVector to compare with.
     * @return True if vectors are not equal, false otherwise.
     */
    bool operator!=(const SmallVector &other) const { return !(*this == other); }

    /**
     * @brief Returns current number of elements.
     * @return Current size of the vector.
     */
    constexpr size_t size() const noexcept { return m_size; }

    /**
     * @brief Returns current capacity.
     * @return Current capacity of the vector.
     */
    constexpr size_t capacity() const noexcept { return m_capacity; }

    /**
     * @brief Checks whether the vector is empty.
     * @return True if empty, false otherwise.
     */
    bool empty() const noexcept { return m_size == 0; }

    /**
     * @brief Destroys all elements and resets size to zero.
     */
    void clear()
    {
        for (size_t i = 0; i < m_size; ++i) {
            m_data[i].~T();
        }
        m_size = 0;
    }

    /**
     * @brief Provides direct access to the underlying data array.
     * @return Pointer to the internal data array.
     */
    T *data() noexcept { return m_data; }

    /**
     * @brief Provides read-only access to the underlying data array.
     * @return Pointer to the internal data array.
     */
    const T *data() const noexcept { return m_data; }

    // Iterator support
    using iterator = T *;
    using const_iterator = const T *;

    iterator begin() { return m_data; }
    iterator end() { return m_data + m_size; }

    const_iterator begin() const { return m_data; }
    const_iterator end() const { return m_data + m_size; }
    const_iterator cbegin() const { return m_data; }
    const_iterator cend() const { return m_data + m_size; }

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }

    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

    using value_type = T;
    using size_type = size_t;
    using reference = T &;
    using const_reference = const T &;
    using pointer = T *;
    using const_pointer = const T *;

    /**
     * @brief Inserts an element at a specified position.
     * @param pos The position at which to insert the element.
     * @param value The value to insert.
     * @return Iterator to the newly inserted element.
     */
    iterator insert(const_iterator pos, const T &value)
    {
        // Calculate the index to insert at
        size_t index = pos - begin();

        // Make sure we have enough capacity to insert the element
        if (m_size == m_capacity) {
            grow(); // Grow the vector if necessary
        }

        // Shift elements to the right starting from the last element
        for (size_t i = m_size; i > index; --i) {
            new (m_data + i) T(std::move(m_data[i - 1])); // Move the element to the right
            m_data[i - 1].~T();                           // Destroy the old element
        }

        // Insert the new element
        new (m_data + index) T(value);
        ++m_size; // Increase the size of the vector

        return m_data + index; // Return the iterator to the inserted element
    }

    /**
     * @brief Inserts an element at a specified position (move version).
     * @param pos The position at which to insert the element.
     * @param value The value to insert (rvalue).
     * @return Iterator to the newly inserted element.
     */
    iterator insert(const_iterator pos, T &&value)
    {
        // Calculate the index to insert at
        size_t index = pos - begin();

        // Make sure we have enough capacity to insert the element
        if (m_size == m_capacity) {
            grow(); // Grow the vector if necessary
        }

        // Shift elements to the right starting from the last element
        for (size_t i = m_size; i > index; --i) {
            new (m_data + i) T(std::move(m_data[i - 1])); // Move the element to the right
            m_data[i - 1].~T();                           // Destroy the old element
        }

        // Insert the new element
        new (m_data + index) T(std::move(value));
        ++m_size; // Increase the size of the vector

        return m_data + index; // Return the iterator to the inserted element
    }

    /**
     * @brief Inserts a range of elements at the specified position.
     * @param pos The position to insert the elements.
     * @param first Iterator to the first element to insert.
     * @param last Iterator to the last element to insert.
     * @return Iterator to the first inserted element.
     */
    template <typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        size_t index = pos - begin();              // Calculate the index to insert at
        size_t count = std::distance(first, last); // Number of elements to insert

        if (m_size + count > m_capacity) {
            grow(); // Ensure enough capacity
        }

        // Shift elements to the right starting from the last element
        for (size_t i = m_size + count - 1; i >= index + count; --i) {
            new (m_data + i) T(std::move(m_data[i - count])); // Move element to the right
            m_data[i - count].~T();                           // Destroy the old element
        }

        // Insert the range
        size_t i = 0;
        for (; first != last; ++first, ++i) {
            new (m_data + index + i) T(*first); // Insert each element in the range
        }

        m_size += count; // Increase size
        return m_data + index;
    }

private:
    /**
     * @brief Grows the capacity using the selected GrowthPolicy.
     */
    void grow()
    {
        size_t new_capacity = GrowthPolicy::next(m_capacity);
        T *new_data = static_cast<T *>(::operator new[](new_capacity * sizeof(T)));
        for (size_t i = 0; i < m_size; ++i) {
            new (new_data + i) T(std::move_if_noexcept(m_data[i]));
            m_data[i].~T();
        }
        if (m_data != stack_data()) {
            ::operator delete[](m_data);
        }
        m_data = new_data;
        m_capacity = new_capacity;
    }

    /**
     * @brief Returns pointer to the start of the internal stack buffer.
     * @return Pointer to stack data.
     */
    T *stack_data() { return reinterpret_cast<T *>(m_stack_buffer); }

    size_t m_size;
    size_t m_capacity;
    T *m_data;
    alignas(T) char m_stack_buffer[sizeof(T) * N];
};

} // namespace gouda
