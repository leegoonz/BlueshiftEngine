// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

/*
-------------------------------------------------------------------------------

    Array

    Templated dynamic array

    Array<T> is one of engine's generic container classes. It stores its items 
    in adjacent memory locations and provides fast index-based access.

    Array<T> does not allocate memory until the first item is added. Likewise 
    std::vector, Array<T> does reallocate and copy memory if it exceed the 
    capacity. To avoid reallocation, you can use Reserve() to ensure a certain 
    capacity before you really need it. And you can avoid frequent reallocation 
    by modifying granularity.

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

/// Templated dynamic array
template <typename T>
class Array {
public:
    static constexpr int DefaultGranularity = 16;

    using CmpFunc = int(const T *, const T *);
    using NewFunc = T();

    /// Constructs empty array with granularity.
    Array(int newGranularity = DefaultGranularity);
    /// Constructs from another array.
    Array(const Array<T> &array);
    /// Aggregates initialization constructor.
    Array(const std::initializer_list<T> &array);
    /// Destructs.
    ~Array();

                    /// Returns true if the array has count 0; otherwise returns false.
    bool            IsEmpty() const { return count == 0; }

                    /// Returns Number of elements.
    int             Count() const { return count; }

                    /// Returns the maximum number of items that can be stored in the array without forcing a reallocation.
    int             Capacity() const { return capacity; }

                    /// Sets the new granularity.
    void            SetGranularity(int newGranularity);

                    /// Gets the current granularity.
    int             GetGranularity() const { return granularity; }

                    /// Returns total size of allocated memory.
    size_t          Allocated() const { return capacity * sizeof(T); }

                    /// Returns total size of allocated memory including size of this type.
    size_t          Size() const { return sizeof(Array<T>) + Allocated(); }

                    /// Returns actual size of the used elements.
    size_t          MemoryUsed() const { return count * sizeof(*elements); }

                    /// Returns a pointer to the elements stored in the array.
                    /// The pointer can be used to access and modify the items in the array.
    T *             Ptr() { return elements; }

                    /// Returns a const pointer to the data stored in the array.
                    /// The pointer can be used to access the items in the array.
    const T *       Ptr() const { return elements; }

                    /// Returns the item at 'index' position as a constant reference.
                    /// 'index' must be a valid index position in the array (i.e., 0 <= index < Count()).
    const T &       operator[](int index) const;

                    /// Returns the item at 'index' position as a modifiable reference.
                    /// 'index' must be a valid index position in the array (i.e., 0 <= index < Count()).
    T &             operator[](int index);

                    /// Compares with another array.
    bool            operator==(const Array<T> &rhs) const;

                    /// Assigns from another array, replacing its current contents.
    Array<T> &      operator=(const Array<T> &rhs);

                    /// Removes all the elements from the array.
                    /// This also released the memory used by the array.
    void            Clear();

                    /// Deletes each elements. 
                    /// This is valid operation only for pointer typed array.
                    /// param @clear call clear function.
    void            DeleteContents(bool clear);

                    /// Releases any memory not required to store the items.
    void            Squeeze();

                    // elements 개수 설정, 필요하다면 resize 한다.
    void            SetCount(int newCount, bool forceResize = true);

                    // 주어진 개수의 elements 를 재할당 없이 사용할 수 있도록 보장한다.
    void            Reserve(int newCapacity);

                    // 주어진 개수의 elements 를 재할당 없이 사용할 수 있도록 보장하고, 추가된 elements 들은 초기값 세팅
    void            Reserve(int newCapacity, const T &initialValue);

                    // 주어진 개수의 elements 를 재할당 없이 사용할 수 있도록 보장하고, 추가된 elements 들은 할당자를 이용해서 new 초기화
                    /// This is valid operation only for pointer typed array.
    void            ReserveAlloc(int newCapacity, NewFunc *allocator);

                    /// Sets the capacity of this array.
                    /// Occurs reallocation and copy memory if capicity changed.
    void            Resize(int newCapacity);
    void            Resize(int newCapacity, int newGranularity);

                    /// Appends new element and returns reference of it.
    T &             Alloc();

                    /// Inserts 'value' at index position 'index'.
                    /// Returns index of the inserted element.
    template <typename CompatibleT>
    int             Insert(CompatibleT &&value, int index = 0);

                    /// Inserts the items of the array 'array' at index position 'index'.
                    /// Returns index of the last inserted element.
    int             InsertList(const Array<T> &array, int index = 0);

                    /// Appends 'value' at the end of the array.
                    /// Returns index of the last appended element.
    template <typename CompatibleT, typename... Rest>
    int             Append(CompatibleT &&value, Rest&&... rest);
    int             Append() { return count - 1; }

                    /// Appends the items of the array 'array' to this array.
                    /// Returns index of the last appended element.
    int             AppendList(const Array<T> &array);

                    /// Appends the unique value 'value'.
                    /// Nothing happens if value 'value' is in the array already.
                    /// Returns index of the element.
    template <typename CompatibleT>
    int             AddUnique(CompatibleT &&value);

                    /// Returns the index position of the first occurrence of 'value' in the array, searching forward from index position 'from'.
                    /// Returns -1 if no item matched.
    template <typename CompatibleT>
    int             FindIndex(CompatibleT &&value, int from = 0) const;
                    /// Returns the index position of the first matching element for which predicate 'finder' returns true in the array, searching forward from index position 'from'.
                    /// Returns -1 if no item matched.
    template <typename Functor>
    int             FindIndexIf(Functor &&finder, int from = 0);

                    /// Returns the element pointer of the first occurence of 'value' in the array, searching forward from index position 'from'.
                    /// Returns nullptr if no item matched.
    template <typename CompatibleT>
    T *             Find(CompatibleT &&value, int from = 0) const;
                    /// Returns the element pointer of the first matching element for which predicate 'finder' returns true in the array, searching forward from index position 'from'.
                    /// Returns nullptr if no item matched.
    template <typename Functor>
    T *             FindIf(Functor &&finder, int from = 0);

                    /// Returns the index position of the first occurence of nullptr.
                    /// This is valid operation only for pointer typed array.
    int             FindNull() const;

                    /// Returns the index position of the first occurence of non-nullptr.
                    /// This is valid operation only for pointer typed array.
    int             FindNotNull() const;

                    /// Calculate the index using valid element pointer.
                    /// This is valid operation only for pointer typed array.
                    // 메모리 포인터끼리 빼는 연산이므로 index 가 항상 array 에 존재하는 것은 아니다. 
                    // 범위를 넘어가는 index 일 경우 debug 모드에서는 assert 에러가 발생한다.
    int             IndexOf(const T *value) const;
                    
                    /// Removes the element at index position 'index'.
                    // 제거되고 이후의 element 들이 빈공간을 메꾸기 위해 앞으로 이동된다.
                    /// Returns false if failed to remove.
    bool            RemoveIndex(int index);

                    /// Removes the element at index position 'index' fast.
                    // 제거될 element 이후의 전체 element 들을 이동하는 대신, 삭제된 element 는 마지막 element 로 대치된다.
                    // 따라서 element 의 순서가 바뀐다.
                    /// Returns false if failed to remove.
    bool            RemoveIndexFast(int index);

                    /// Removes the first element that compares equal to 'value' from the array.
                    /// Returns whether an element was, in fact, removed.
                    // 제거되고 이후의 element 들이 빈공간을 메꾸기 위해 앞으로 이동된다.
    template <typename CompatibleT>
    bool            Remove(CompatibleT &&value);

                    /// Removes all elements that compare equal to 'value' from the array.
                    /// Returns the number of elements removed.
    template <typename CompatibleT>
    bool            RemoveAll(CompatibleT &&value);

                    /// Removes the element at index position 'index' and returns it.
    T               TakeAt(int index);

                    /// Removes the first item in the array and returns it.
    T               TakeFirst() { return TakeAt(0); }

                    /// Removes the last item in the array and returns it.
    T               TakeLast() { return TakeAt(count - 1); }

                    /// Sorts using predicate 'compare'
    template <typename Functor> 
    void            Sort(Functor &&compare);
    void            Sort() { Sort(std::less<T>()); }

                    /// Sorts sub-section using predicate 'compare'.
    template <typename Functor> 
    void            SortSubSection(int startIndex, int endIndex, Functor &&compare);
    void            SortSubSection(int startIndex, int endIndex) { SortSubSection(startIndex, endIndex, std::less<T>()); }

                    /// Swaps array 'array' with this array.
    void            Swap(Array<T> &array);

private:
    int             count;          ///< Number of elements in use
    int             capacity;       ///< Size of elements allocated for
    int             granularity;    ///< Allocation granularity
    T *             elements;       ///< Elements pointer
};

template <typename T>
BE_INLINE Array<T>::Array(int newGranularity) {
    assert(newGranularity > 0);

    elements = nullptr;
    granularity = newGranularity;

    Clear();
}

template <typename T>
BE_INLINE Array<T>::Array(const Array<T> &array) {
    elements = nullptr;
    *this = array;
}

template <typename T>
BE_INLINE Array<T>::Array(const std::initializer_list<T> &array) : Array() {
    Reserve(array.size());

    for (auto it = array.begin(); it != array.end(); ++it) {
        Append(*it);
    }
}

template <typename T>
BE_INLINE Array<T>::~Array() {
    Clear();
}

template <typename T>
BE_INLINE void Array<T>::SetGranularity(int newGranularity) {
    assert(newGranularity > 0);
    granularity = newGranularity;

    if (elements) {
        // granularity 에 맞게 resize
        int newCapacity = count + granularity - 1;
        newCapacity -= newCapacity % granularity;
        if (newCapacity != capacity) {
            Resize(newCapacity);
        }
    }
}

template <typename T>
BE_INLINE void Array<T>::Squeeze() {
    if (elements) {
        if (count > 0) {
            Resize(count);
        } else {
            Clear();
        }
    }
}

template <typename T>
BE_INLINE void Array<T>::SetCount(int newCount, bool forceResize) {
    assert(newCount >= 0);
    if (forceResize || newCount > capacity) {
        Resize(newCount);
    }

    count = newCount;
}

template <typename T>
BE_INLINE void Array<T>::Reserve(int newCapacity) {
    if (newCapacity > capacity) {
        if (granularity == 0) { // hack to fix memset
            granularity = DefaultGranularity;
        }

        newCapacity += granularity - 1;
        newCapacity -= newCapacity % granularity;
        Resize(newCapacity);
    }
}

template <typename T>
BE_INLINE void Array<T>::Reserve(int newCapacity, const T &initialValue) {
    if (newCapacity > capacity) {
        if (granularity == 0) { // hack to fix memset
            granularity = DefaultGranularity;
        }

        newCapacity += granularity - 1;
        newCapacity -= newCapacity % granularity;
        count = capacity;
        Resize(newCapacity);

        for (int i = count; i < newCapacity; i++) {
            elements[i] = initialValue;
        }
    }
}

template <typename T>
BE_INLINE void Array<T>::ReserveAlloc(int newCapacity, NewFunc *allocator) {
    if (newCapacity > capacity) {
        if (granularity == 0) { // hack to fix memset
            granularity = DefaultGranularity;
        }

        newCapacity += granularity - 1;
        newCapacity -= newCapacity % granularity;
        count = capacity;
        Resize(newCapacity);

        for (int i = count; i < newCapacity; i++) {
            elements[i] = (*allocator)();
        }
    }
}

template <typename T>
BE_INLINE void Array<T>::Resize(int newCapacity) {
    assert(newCapacity >= 0);

    if (newCapacity <= 0) {
        Clear();
        return;
    }

    if (newCapacity == capacity) {
        // capacity 가 바뀌지 않았다면 그냥 return
        return;
    }

    T *temp = elements;
    capacity = newCapacity;
    if (capacity < count) {
        count = capacity;
    }

    elements = new T[capacity];
    for (int i = 0; i < count; i++) {
        elements[i] = temp[i];
    }

    if (temp) {
        delete [] temp;
    }
}

template <typename T>
BE_INLINE void Array<T>::Resize(int newCapacity, int newGranularity) {
    assert(newCapacity >= 0);

    assert(newGranularity > 0);
    granularity = newGranularity;

    if (newCapacity <= 0) {
        Clear();
        return;
    }

    T *temp = elements;
    capacity = newCapacity;
    if (capacity < count) {
        count = capacity;
    }

    elements = new T[capacity];
    for (int i = 0; i < count; i++) {
        elements[i] = temp[i];
    }

    if (temp) {
        delete [] temp;
    }
}

template <typename T>
BE_INLINE Array<T> &Array<T>::operator=(const Array<T> &rhs) {
    Clear();

    count       = rhs.count;
    capacity    = rhs.capacity;
    granularity = rhs.granularity;

    if (capacity) {
        elements = new T[capacity];
        for (int i = 0; i < count; i++) {
            elements[i] = rhs.elements[i];
        }
    }

    return *this;
}

template <typename T>
BE_INLINE const T &Array<T>::operator[](int index) const {
    assert(index >= 0);
    assert(index < count);

    return elements[index];
}

template <typename T>
BE_INLINE T &Array<T>::operator[](int index) {
    assert(index >= 0);
    assert(index < count);

    return elements[index];
}

template <typename T>
BE_INLINE bool Array<T>::operator==(const Array<T> &rhs) const {
    if (count != rhs.count) {
        return false;
    }

    for (int i = 0; i < count; i++) {
        if (elements[i] != rhs.elements[i]) {
            return false;
        }
    }

    return true;
}

template <typename T>
BE_INLINE T &Array<T>::Alloc() {
    if (!elements) {
        Resize(granularity);
    }

    if (count == capacity) {
        Resize(capacity + granularity);
    }

    return elements[count++];
}

template <typename T>
template <typename CompatibleT>
BE_INLINE int Array<T>::Insert(CompatibleT &&value, int index) {
    assert(index >= 0 && index <= count);

    if (!elements) {
        Resize(granularity);
    }

    if (count == capacity) {
        if (granularity == 0) { // hack to fix memset
            granularity = DefaultGranularity;
        }

        int newCapacity = capacity + granularity;
        Resize(newCapacity - newCapacity % granularity);
    }

    for (int i = count; i > index; --i) {
        elements[i] = elements[i - 1];
    }
    elements[index] = std::forward<CompatibleT>(value);
    count++;

    return index;
}

template <typename T>
BE_INLINE int Array<T>::InsertList(const Array<T> &array, int index) {
    assert(index >= 0 && index <= count);

    if (!elements) {
        Resize(granularity);
    }

    if (count + array.count > capacity) {
        if (granularity == 0) { // hack to fix memset
            granularity = DefaultGranularity;
        }

        int newCapacity = capacity + array.count + granularity - 1;
        Resize(newCapacity - newCapacity % granularity);
    }

    for (int i = count - 1; i >= index; --i) {
        elements[i + array.count] = elements[i];
    }

    for (int i = 0; i < array.count; i++) {
        elements[index + i] = array.elements[i];
    }

    count += array.count;

    return index + array.count - 1;
}

template <typename T>
template <typename CompatibleT, typename... Rest>
BE_INLINE int Array<T>::Append(CompatibleT &&value, Rest&&... rest) {
    Insert(std::forward<CompatibleT>(value), count);
    return Append(std::forward<Rest>(rest)...);
}

template <typename T>
BE_INLINE int Array<T>::AppendList(const Array<T> &array) {
    return InsertList(array, count);
}

template <typename T>
template <typename CompatibleT>
BE_INLINE int Array<T>::AddUnique(CompatibleT &&value) {
    int index = FindIndex(value);
    if (index < 0) {
        index = Append(std::forward<CompatibleT>(value));
    }

    return index;
}

template <typename T>
template <typename CompatibleT>
BE_INLINE int Array<T>::FindIndex(CompatibleT &&value, int from) const {
    for (int i = from; i < count; i++) {
        if (elements[i] == std::forward<CompatibleT>(value)) {
            return i;
        }
    }

    return -1;
}

template <typename T>
template <typename Functor>
BE_INLINE int Array<T>::FindIndexIf(Functor &&finder, int from) {
    T *e = std::find_if(elements + from, elements + count, std::forward<Functor>(finder));
    if (e) {
        return (e - elements) / sizeof(T);
    }
    return -1;
}

template <typename T>
template <typename CompatibleT>
BE_INLINE T *Array<T>::Find(CompatibleT &&value, int from) const {
    int i = FindIndex(std::forward<CompatibleT>(value), from);
    if (i >= 0) {
        return &elements[i];
    }

    return nullptr;
}

template <typename T>
template <typename Functor>
BE_INLINE T *Array<T>::FindIf(Functor &&finder, int from) {
    return std::find_if(elements + from, elements + count, std::forward<Functor>(finder));
}

template <typename T>
BE_INLINE int Array<T>::FindNull() const {
    for (int i = 0; i < count; i++) {
        if (elements[i] == nullptr) {
            return i;
        }
    }

    return -1;
}

template <typename T>
BE_INLINE int Array<T>::FindNotNull() const {
    for (int i = 0; i < count; i++) {
        if (elements[i] != nullptr) {
            return i;
        }
    }

    return -1;
}

template <typename T>
BE_INLINE int Array<T>::IndexOf(const T *objPtr) const {
    int index = (int)(objPtr - elements);

    assert(index >= 0);
    assert(index < count);

    return index;
}

template <typename T>
BE_INLINE bool Array<T>::RemoveIndex(int index) {
    assert(elements != nullptr);
    assert(index >= 0);
    assert(index < count);

    if (index < 0 || index >= count) {
        return false;
    }

    count--;
    for (int i = index; i < count; i++) {
        elements[i] = elements[i + 1];
    }

    return true;
}

template <typename T>
BE_INLINE bool Array<T>::RemoveIndexFast(int index) {
    if ((index < 0) || (index >= count)) {
        return false;
    }

    count--;
    if (index != count) {
        elements[index] = elements[count];
    }

    return true;
}

template <typename T>
template <typename CompatibleT>
BE_INLINE bool Array<T>::Remove(CompatibleT &&value) {
    int index = FindIndex(std::forward<CompatibleT>(value));
    if (index >= 0) {
        return RemoveIndex(index);
    }
    
    return false;
}

template <typename T>
template <typename CompatibleT>
BE_INLINE bool Array<T>::RemoveAll(CompatibleT &&value) {
    int removed = 0;
    int index = FindIndex(value, 0);

    while (index >= 0) {
        RemoveIndex(index);
        removed++;

        index = FindIndex(value, index);
    }

    return removed;
}

template <typename T>
BE_INLINE T Array<T>::TakeAt(int index) {
    assert(elements != nullptr);
    assert(index >= 0);
    assert(index < count);

    if (index < 0 || index >= count) {
        return T();
    }

    T value = (*this)[index];
    RemoveIndex(index);
    return value;
}

template <typename T> 
BE_INLINE void Array<T>::Clear() {
    if (elements) {
        delete [] elements;
    }

    elements = nullptr;
    count = 0;
    capacity = 0;
}

template <typename T>
BE_INLINE void Array<T>::DeleteContents(bool clear) {
    for (int i = 0; i < count; i++) {
        delete elements[i];
        elements[i] = nullptr;
    }

    if (clear) {
        Clear();
    } else {
        memset(elements, 0, capacity * sizeof(T));
    }
}

template <typename T>
template <typename Functor>
BE_INLINE void Array<T>::Sort(Functor &&compare) {
    std::sort(elements, elements + count, std::forward<Functor>(compare));
}

template <typename T> 
template <typename Functor>
BE_INLINE void Array<T>::SortSubSection(int startIndex, int endIndex, Functor &&compare) {
    if (!elements) {
        return;
    }

    if (startIndex < 0) {
        startIndex = 0;
    }

    if (endIndex >= count) {
        endIndex = count - 1;
    }

    if (startIndex >= endIndex) {
        return;
    }

    std::sort(&elements[startIndex], &elements[endIndex], std::forward<Functor>(compare));
}

template <typename T>
BE_INLINE void Array<T>::Swap(Array<T> &array) {
    BE1::Swap(count, array.count);
    BE1::Swap(capacity, array.capacity);
    BE1::Swap(granularity, array.granularity);
    BE1::Swap(elements, array.elements);
}

BE_NAMESPACE_END
