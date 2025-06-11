#pragma once
/**
 * @file small_vector.hpp
 * @author GoudaCheeseburgers
 * @date 2025-04-24
 * @brief Engine module
 *
 * @copyright
 * Copyright (c) 2025 GoudaCheeseburgers <https://github.com/Cheeseborgers>
 *
 * This file is part of the Gouda engine and licensed under the GNU Affero General Public License v3.0 or later.
 * See <https://www.gnu.org/licenses/> for more information.
 */
#include "debug/assert.hpp"
#include <algorithm> // For std::uninitialized_copy_n
#include <iterator>
#include <memory>
#include <new> // For std::launder

namespace gouda {

/**
 * @brief Growth policy that doubles the capacity each time.
 */
struct GrowthPolicyDouble {
    static size_t next(const size_t current) { return current ? current * 2 : 1; }
};

/**
 * @brief Growth policy that increases capacity by 1.5x.
 */
struct GrowthPolicyOnePointFive {
    static size_t next(const size_t current)
    {
        return current < 2 ? 2 : static_cast<size_t>(static_cast<double>(current) * 1.5);
    }
};

/**
 * @brief Growth policy that adds one to the capacity each time.
 */
struct GrowthPolicyAddOne {
    static size_t next(const size_t current) { return current + 1; }
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
     * @brief Initializes the SmallVector with elements from an initializer list.
     *
     * Constructs the vector using stack storage if the number of elements is small enough.
     * Reserves capacity upfront to avoid multiple reallocations.
     *
     * @param init The initializer list of elements to populate the vector with.
     */
    SmallVector(std::initializer_list<T> init) : m_data(stack_data()), m_size(0), m_capacity(N)
    {
        if (init.size() > N) {
            reserve(init.size()); // Use heap if needed
        }
        for (const auto &v : init) {
            new (m_data + m_size++) T(v);
        }
    }

    /**
     * @brief Constructs a SmallVector with count default-initialized elements.
     * @param count The number of elements to create.
     * @param alloc The allocator to use (default: Allocator()).
     * @throws Any exception thrown by T's default constructor or allocator.
     */
    explicit SmallVector(size_t count) : m_size(0), m_capacity(N), m_data(stack_data())
    {
        if (count > N) {
            reserve(count);
        }

        if constexpr (std::is_trivially_default_constructible_v<T>) {
            std::uninitialized_default_construct_n(m_data, count);
        }
        else {
            for (size_t i = 0; i < count; ++i) {
                std::construct_at(m_data + i);
            }
        }
        m_size = count;
    }

    /**
     * @brief Constructs a SmallVector with count copies of a given value.
     * @param count The number of elements.
     * @param value The value to copy into each element.
     */
    SmallVector(const size_t count, const T &value) : m_size(0), m_capacity(N), m_data(stack_data())
    {
        if (count > N) {
            reserve(count);
        }

        for (size_t i = 0; i < count; ++i) {
            std::construct_at(m_data + i, value);
        }
        m_size = count;
    }

    /**
     * @brief Destructor clears the vector and frees any heap memory if used.
     */
    ~SmallVector()
    {
        clear();
        if (m_data != stack_data()) {
            ::operator delete(m_data);
        }
    }

    /**
     * @brief Move constructor transfers ownership from another SmallVector.
     * @param other The SmallVector to move from.
     */
    SmallVector(SmallVector&& other) noexcept
        : m_size(0), m_capacity(N), m_data(stack_data()) {
        if (other.m_data == other.stack_data()) {
            // Copy stack data to our stack
            std::uninitialized_copy_n(other.m_data, other.m_size, m_data);
            m_size = other.m_size;
            // Clear other's elements
            for (size_t i = 0; i < other.m_size; ++i) other.m_data[i].~T();
            other.m_size = 0;
        } else {
            // Take heap data
            m_data = other.m_data;
            m_size = other.m_size;
            m_capacity = other.m_capacity;
            other.m_data = nullptr; // Prevent double-free
            other.m_size = 0;
            other.m_capacity = 0;
        }
    }

    /**
     * @brief Move assignment operator transfers ownership and cleans up current data.
     * @param other The SmallVector to move from.
     * @return Reference to this SmallVector.
     */
    SmallVector& operator=(SmallVector&& other) noexcept {
        if (this != &other) {
            clear();
            if (m_data != stack_data()) ::operator delete(m_data);
            m_data = stack_data();
            m_size = 0;
            m_capacity = N;
            if (other.m_data == other.stack_data()) {
                // Copy stack data to our stack
                std::uninitialized_copy_n(other.m_data, other.m_size, m_data);
                m_size = other.m_size;
                // Clear other's elements
                for (size_t i = 0; i < other.m_size; ++i) other.m_data[i].~T();
                other.m_size = 0;
            } else {
                // Take heap data
                m_data = other.m_data;
                m_size = other.m_size;
                m_capacity = other.m_capacity;
                other.m_data = nullptr; // Prevent double-free
                other.m_size = 0;
                other.m_capacity = 0;
            }
        }
        return *this;
    }

    /**
     * @brief Copy constructor duplicates the contents of another SmallVector.
     * @param other The SmallVector to copy from.
     */
    SmallVector(const SmallVector &other) : m_size(0), m_capacity(N), m_data(stack_data())
    {
        reserve(other.m_size);
        try {
            std::uninitialized_copy_n(other.m_data, other.m_size, m_data);
            m_size = other.m_size;
        }
        catch (...) {
            // Clean up partially constructed elements
            clear();
            if (m_data != stack_data()) {
                ::operator delete(m_data);
                m_data = stack_data();
            }
            throw;
        }
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
        if (m_size == m_capacity) {
            grow();
        }
        new (m_data + m_size++) T(value);
    }

    /**
     * @brief Adds a moved value to the end of the vector.
     * @param value The value to add.
     */
    void push_back(T &&value)
    {
        if (m_size == m_capacity) {
            grow();
        }
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
        if (m_size == m_capacity) {
            grow();
        }
        T *ptr = new (m_data + m_size++) T(std::forward<Args>(args)...);
        return *ptr;
    }

    /**
     * @brief Removes the last element from the vector.
     * @pre The vector must not be empty.
     */
    void pop_back()
    {
        if (m_size > 0) {
            // Call the destructor for the last element
            m_data[m_size - 1].~T();
            --m_size;
        }
    }

    /**
     * @brief Ensures that the vector has at least the specified capacity.
     * @param new_cap New minimum capacity.
     */
    void reserve(const size_t new_cap)
    {
        if (new_cap <= m_capacity) {
            return;
        }
        T *new_data = static_cast<T *>(::operator new(new_cap * sizeof(T)));
        try {
            for (size_t i = 0; i < m_size; ++i) {
                new (new_data + i) T(std::move_if_noexcept(m_data[i]));
                m_data[i].~T();
            }
        }
        catch (...) {
            for (size_t i = 0; i < m_size; ++i) {
                new_data[i].~T();
            }
            ::operator delete(new_data);
            throw;
        }
        if (m_data != stack_data()) {
            ::operator delete(m_data);
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
            for (size_t i = new_size; i < m_size; ++i) {
                m_data[i].~T();
            }
        }
        else if (new_size > m_size) {
            reserve(new_size);
            for (size_t i = m_size; i < new_size; ++i) {
                new (m_data + i) T();
            }
        }
        m_size = new_size;
    }

    /**
     * @brief Resizes the vector to the specified size and fills new elements with a given value.
     * @param new_size New size of the vector.
     * @param value The value to initialize new elements with.
     */
    void resize(size_t new_size, const T &value)
    {
        if (new_size < m_size) {
            for (size_t i = new_size; i < m_size; ++i) {
                m_data[i].~T();
            }
        }
        else if (new_size > m_size) {
            reserve(new_size);
            for (size_t i = m_size; i < new_size; ++i) {
                new (m_data + i) T(value);
            }
        }
        m_size = new_size;
    }

    /**
     * @brief Reduces capacity to match current size.
     */
    void shrink_to_fit()
    {
        if (m_capacity > m_size && m_data != stack_data()) {
            if (m_size <= N) {
                // Move back to stack
                T *new_data = stack_data();
                for (size_t i = 0; i < m_size; ++i) {
                    new (new_data + i) T(std::move(m_data[i]));
                    m_data[i].~T();
                }
                ::operator delete(m_data);
                m_data = new_data;
                m_capacity = N;
            }
            else {
                // Reallocate heap
                T *new_data = static_cast<T *>(::operator new(m_size * sizeof(T)));
                for (size_t i = 0; i < m_size; ++i) {
                    new (new_data + i) T(std::move(m_data[i]));
                    m_data[i].~T();
                }
                ::operator delete(m_data);
                m_data = new_data;
                m_capacity = m_size;
            }
        }
    }

    /**
     * @brief Provides access to an element by index.
     * @param i Index of the element.
     * @return Reference to the element.
     */
    T &operator[](size_t i) { return *std::launder(m_data + i); }

    /**
     * @brief Provides read-only access to an element by index.
     * @param i Index of the element.
     * @return Const reference to the element.
     */
    const T &operator[](size_t i) const { return *std::launder(m_data + i); }

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
    [[nodiscard]] constexpr size_t size() const noexcept { return m_size; }

    /**
     * @brief Returns current capacity.
     * @return Current capacity of the vector.
     */
    [[nodiscard]] constexpr size_t capacity() const noexcept { return m_capacity; }

    /**
     * @brief Checks whether the vector is empty.
     * @return True if empty, false otherwise.
     */
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }

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
     * @brief Accesses the first element.
     * @return Reference to the first element.
     * @pre The vector must not be empty.
     * @note constexpr if T is trivially copyable.
     * @note This function is noexcept.
     */
    constexpr T &front() noexcept
    {
        ASSERT(m_size > 0, "front on empty SmallVector");
        return *std::launder(m_data);
    }

    /**
     * @brief Accesses the first element (const).
     * @return Const reference to the first element.
     * @pre The vector must not be empty.
     * @note constexpr if T is trivially copyable.
     * @note This function is noexcept.
     */
    constexpr const T &front() const noexcept
    {
        ASSERT(m_size > 0, "front on empty SmallVector");
        return *std::launder(m_data);
    }

    /**
     * @brief Accesses the last element.
     * @return Reference to the last element.
     * @pre The vector must not be empty.
     * @note constexpr if T is trivially copyable.
     * @note This function is noexcept.
     */
    constexpr T &back() noexcept
    {
        ASSERT(m_size > 0, "back on empty SmallVector");
        return *std::launder(m_data + m_size - 1);
    }

    /**
     * @brief Accesses the last element (const).
     * @return Const reference to the last element.
     * @pre The vector must not be empty.
     * @note constexpr if T is trivially copyable.
     * @note This function is noexcept.
     */
    constexpr const T &back() const noexcept
    {
        ASSERT(m_size > 0, "back on empty SmallVector");
        return *std::launder(m_data + m_size - 1);
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
        size_t index = pos - begin();
        if (m_size == m_capacity) {
            grow();
        }
        for (size_t i = m_size; i > index; --i) {
            new (m_data + i) T(std::move(m_data[i - 1]));
            m_data[i - 1].~T();
        }
        new (m_data + index) T(value);
        ++m_size;
        return m_data + index;
    }

    /**
     * @brief Inserts an element at a specified position (move version).
     * @param pos The position at which to insert the element.
     * @param value The value to insert (rvalue).
     * @return Iterator to the newly inserted element.
     */
    iterator insert(const_iterator pos, T &&value)
    {
        size_t index = pos - begin();
        if (m_size == m_capacity) {
            grow();
        }
        for (size_t i = m_size; i > index; --i) {
            new (m_data + i) T(std::move(m_data[i - 1]));
            m_data[i - 1].~T();
        }
        new (m_data + index) T(std::move(value));
        ++m_size;
        return m_data + index;
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
        size_t index = pos - begin();
        const size_t count = std::distance(first, last);
        if (m_size + count > m_capacity) {
            const size_t new_capacity = GrowthPolicy::next(std::max(m_capacity, m_size + count));
            reserve(new_capacity);
        }
        for (size_t i = m_size + count - 1; i >= index + count; --i) {
            new (m_data + i) T(std::move(m_data[i - count]));
            m_data[i - count].~T();
        }
        size_t i = 0;
        for (; first != last; ++first, ++i) {
            new (m_data + index + i) T(*first);
        }
        m_size += count;
        return m_data + index;
    }

    /**
     * @brief Erases the element at the given position.
     * @param pos Iterator to the element to remove.
     * @return Iterator following the removed element.
     */
    iterator erase(const_iterator pos)
    {
        size_t index = pos - begin();
        m_data[index].~T(); // Destroy the element

        // Move elements to fill the gap
        for (size_t i = index; i < m_size - 1; ++i) {
            new (m_data + i) T(std::move(m_data[i + 1]));
            m_data[i + 1].~T();
        }

        --m_size;
        return m_data + index;
    }

    /**
     * @brief Erases elements in the range [first, last).
     * @param first Iterator to the first element to remove.
     * @param last Iterator to one past the last element to remove.
     * @return Iterator following the last removed element.
     */
    iterator erase(const_iterator first, const_iterator last)
    {
        size_t start = first - begin();
        const size_t end = last - begin();
        size_t count = end - start;

        // Destroy the range
        for (size_t i = start; i < end; ++i) {
            m_data[i].~T();
        }

        // Move elements to fill the gap
        for (size_t i = end; i < m_size; ++i) {
            new (m_data + i - count) T(std::move(m_data[i]));
            m_data[i].~T();
        }

        m_size -= count;
        return m_data + start;
    }

private:
    /**
     * @brief Grows the capacity using the selected GrowthPolicy.
     */
    void grow()
    {
        const size_t new_capacity = GrowthPolicy::next(m_capacity);
        reserve(new_capacity);
    }

    /**
     * @brief Returns pointer to the start of the internal stack buffer.
     * @return Pointer to stack data.
     */
    T *stack_data() noexcept { return std::launder(reinterpret_cast<T *>(m_stack_buffer)); }
    const T *stack_data() const noexcept { return std::launder(reinterpret_cast<const T *>(m_stack_buffer)); }

    size_t m_size;
    size_t m_capacity;
    T *m_data;
    alignas(T) char m_stack_buffer[sizeof(T) * N];
};

template <typename T>
using Vector = SmallVector<T, 0>;

} // namespace gouda