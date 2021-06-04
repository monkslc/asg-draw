#ifndef DS_H
#define DS_H

#include <functional>
#include <stdlib.h>

// TODO: Create documentation explaining how the allocators / data structures were built and how they should be used
// avoiding this for now because I'm not sure what's going to stick for them. I don't the implementation right now.
// Right now its a lot of generics which makes debugging compiler errors tough. Idk we'll play around with it and see.

class SysAllocator {
    public:
    SysAllocator(){};

    template <typename T>
    T* Alloc(size_t items) {
        return (T*) malloc(items * sizeof(T));
    }

    template <typename T>
    T* Realloc(T *data, size_t old_capacity, size_t new_capacity) {
        return (T*) realloc((void *)data, sizeof(T) * new_capacity);
    }

    void Free(void *data) {
        free(data);
    }
};

// Needs to be defined in one of the cpp files
extern SysAllocator global_allocator;

class LinearAllocator {
    public:
        void *start;
        size_t used;
        size_t capacity;
        LinearAllocator(size_t capacity) : used(0), capacity(capacity) {
            this->start = malloc(capacity);
        }

        template <typename T>
        T* Alloc(size_t items) {
            size_t bytes     = sizeof(T) * items;
            size_t alignment = alignof(T);

            // Alignment calculation stolen from
            // https://os.phil-opp.com/allocator-designs/#address-alignment
            size_t aligned_up = (this->used + alignment - 1) & ~(alignment - 1);
            size_t new_used = aligned_up + bytes;

            if(aligned_up - this->used) {
                printf("we lost %zu due to alignment\n", aligned_up - this->used);
            }

            if (new_used > this->capacity) {
              return NULL;
            }

            this->used = new_used;
            return (T*)((uint8_t *)this->start + aligned_up);
        }

        template <typename T>
        T* Realloc(void *data, size_t old_capacity, size_t new_capacity) {
            T* new_loc = this->Alloc(new_capacity);
            if (new_loc) {
                memcpy(new_loc, data, sizeof(T) * old_capacity);
            }

            return (T*)new_loc;
        }

        void Free(void *data) {
            // Do nothing and free all the memory ath the end using FreeAllocator
        }

        void FreeAllocator() {
            free(this->start);
        }
};

template <typename T, typename A>
class DynamicArrayEx {
    public:
    size_t capacity;
    size_t length;
    T *data;
    DynamicArrayEx(size_t capacity, A *allocator) : length(0), capacity(capacity) {
        this->data = allocator->template Alloc<T>(capacity);
    }

    DynamicArrayEx(A *allocator) : length(0), capacity(0) {
        this->data = allocator->template Alloc<T>(this->capacity);
    }

    DynamicArrayEx() : length(0), capacity(0), data(NULL) {};

    void IncreaseCapacity(size_t new_capacity, A *allocator) {
        this->data = (T*) allocator->template Realloc<T>(this->data, this->capacity, new_capacity);
        this->capacity = new_capacity;
    }

    void Resize(size_t new_length, A *allocator) {
        this->length = new_length;
        if (this->length > this->capacity) {
            this->IncreaseCapacity(this->length, allocator);
        }
    }

    void Push(T elem, A *allocator) {
        if (this->length >= this->capacity) {
            this->IncreaseCapacity((this->capacity * 2) + 1, allocator); // Add +1 here incase capacity is 0
        }

        this->data[this->length] = elem;
        *(this->data + this->length) = elem;
        this->length++;
    }

    void Put(T elem, size_t index) {
        this->data[index] = elem;
    }

    void Remove(T elem) {
       size_t last_index = this->length - 1;
       for (auto i=0; i<this->length; i++) {
           if (this->data[i] == elem) {
               if (i != last_index) {
                   this->data[i] = this->data[last_index];
               }

               this->length--;
               return;
           }
       }
    }

    void RemoveIndex(size_t index) {
        size_t last_index = this->length - 1;
        if (index < last_index) {
            this->data[index] = this->data[last_index];
        }

        this->length--;
    }

    T Get(size_t index) {
        return this->data[index];
    }

    T* GetPtr(size_t index) {
        return &this->data[index];
    }

    T Last() {
        return this->data[this->length - 1];
    }

    T* LastPtr() {
        return &this->data[this->length - 1];
    }

    T* Data() {
        return this->data;
    }

    T* End() {
        return &this->data[this->length];
    }

    size_t Length() {
        return this->length;
    }

    size_t Capacity() {
        return this->capacity;
    }


    void Clear() {
        this->length = 0;
    }

    DynamicArrayEx<T, A> Clone(A* allocator) {
        DynamicArrayEx<T, A> new_array = DynamicArrayEx<T, A>(this->capacity, allocator);
        new_array.length = this->length;

        std::memcpy(new_array.data, this->data, sizeof(T) * this->length);

        return new_array;
    }


    void Free(A *allocator) {
        allocator->Free(this->data);
    }

    void FreeAll(A *allocator) {
        for (auto i=0; i<this->length; i++) {
           T* elem = this->GetPtr(i);
           elem->Free();
        }

        this->Free(allocator);
    }

    void ReleaseAll() {
        for (auto i=0; i<this->length; i++) {
            T* elem = this->GetPtr(i);
            (*elem)->Release();
        }
    }

    bool operator==(DynamicArrayEx<T, A>& rhs) {
        if (this->Length() != rhs.Length()) {
            return false;
        }

        for (auto i=0; i<this->Length(); i++) {
            if (this->data[i] != rhs.data[i]) {
                return false;
            }
        }

        return true;
    }
};

template <typename T>
class DynamicArray {
    public:
    DynamicArrayEx<T, SysAllocator> array;
    DynamicArray(size_t capacity) : array(DynamicArrayEx<T, SysAllocator>(capacity, &global_allocator)) {};

    DynamicArray() : array(DynamicArrayEx<T, SysAllocator>()) {};
    DynamicArray(DynamicArrayEx<T, SysAllocator> array) : array(array) {};

    // Creates and array of size length with every entry set to 0
    static DynamicArray<T> Zeroed(size_t length) {
        auto array = DynamicArray<T>();
        array.array.length   = length;
        array.array.capacity = length;

        array.array.data = (T*) calloc(length, sizeof(T));

        return array;
    }

    void IncreaseCapacity(size_t new_capacity) {
        this->array.IncreaseCapacity(new_capacity, &global_allocator);
    }

    void Resize(size_t new_length) {
        this->array.Resize(new_length &global_allocator);
    }

    void Push(T elem) {
        this->array.Push(elem, &global_allocator);
    }

    void Put(T elem, size_t index) {
        this->array.Put(elem, index);
    }

    void Remove(T elem) {
        this->array.Remove(elem);
    }

    void RemoveIndex(size_t index) {
        this->array.RemoveIndex(index);
    }

    T Get(size_t index) {
        return this->array.Get(index);
    }

    T* GetPtr(size_t index) {
        return this->array.GetPtr(index);
    }

    T Last() {
        return this->array.Last();
    }

    T* LastPtr() {
        return this->array.LastPtr();
    }

    T* End() {
        return this->array.End();
    }

    T* Data() {
        return this->array.Data();
    }

    size_t Length() {
        return this->array.Length();
    }

    size_t Capacity() {
        return this->array.Capacity();
    }

    void Clear() {
        return this->array.Clear();
    }

    DynamicArray<T> Clone() {
        return DynamicArray<T>(this->array.Clone(&global_allocator));
    }

    void Free() {
        return this->array.Free(&global_allocator);
    }

    void FreeAll() {
        for (auto i=0; i<this->Length(); i++) {
           T* elem = this->GetPtr(i);
           elem->Free();
        }

        this->Free();
    }

    void ReleaseAll() {
        this->array.ReleaseAll();
    }

    bool operator==(DynamicArray<T>& rhs) {
        return this->array == rhs.array;
    }
};

constexpr size_t kDefaultPoolSize = 5;
class LinearAllocatorPool {
    public:
    DynamicArray<LinearAllocator> pool;
    size_t allocation_size;
    LinearAllocatorPool(size_t allocation_size) :
        allocation_size(allocation_size),
        pool(DynamicArray<LinearAllocator>(kDefaultPoolSize)) {

        this->pool.Push(LinearAllocator(this->allocation_size));
    };

    template <typename T>
    T* Alloc(size_t items) {
        T* data = this->pool.LastPtr()->template Alloc<T>(items);
        if (data) {
            return data;
        }

        size_t asked_for = sizeof(T) * items;

        // Most linear allocator pools shouldn't have to expand beyond the first pool. If we've reached this point
        // then the current pool couldn't hold the allocation. Log out that this is happening so pool sizes can be
        // adjusted if needed.
        printf("The current linear allocator couldn't fit an allocation of size: %zu. We have %zu pools of size %zu\n", asked_for, this->pool.Length(), this->allocation_size);

        // If the allocation being asked for is larger than our allocation size, allocate a larger pool than the
        // allocation size instead of failing. If this is hapenning we likely don't have a large enough pool size
        // so log it out.
        if (asked_for > allocation_size) {
            printf("Ruh roh the current allocation being asked for (%zu) is greater than our allocator size (%zu)\n", asked_for, this->allocation_size);
            this->pool.Push(LinearAllocator(asked_for));
        } else {
            this->pool.Push(LinearAllocator(this->allocation_size));
        }

        return this->template Alloc<T>(items);
    }

    template <typename T>
    void *Realloc(void *data, size_t old_capacity, size_t new_capacity) {
        T* new_loc = this->template Alloc<T>(new_capacity);
        memcpy(new_loc, data, sizeof(T) * old_capacity);

        return new_loc;
    }

    void Free(void *data) {
        // Do nothing and free all of the data at the end using FreeAllocator
    }

    void FreeAllocator() {
        for (size_t i=0; i<this->pool.Length(); i++) {
            this->pool.GetPtr(i)->FreeAllocator();
        }

        this->pool.Free();
    }
};

template<typename A>
class StringEx {
    public:
    DynamicArrayEx<char, A> chars;
    StringEx(A* allocator) : chars(DynamicArrayEx<char, A>(10, allocator)) {};
    StringEx(char *c_str, A* allocator) : chars(DynamicArrayEx<char, A>(10, allocator)) {
        char *iter = c_str;
        while (*iter) {
            this->chars.Push(*iter, allocator);
            iter++;
        }
    };

    char* Data() {
        return this->chars.Data();
    }

    char* End() {
        return this->chars.End();
    }

    // CStr places a null pointer past the end of the array but doesn't increase the size of the
    // underlying array. So if an element is pushed to the array after getting a pointer to the c
    // string, it will overwrite the null pointer
    char* CStr(A* allocator) {
        if (this->chars.Length() >= this->chars.Capacity()) {
            this->chars.IncreaseCapacity(this->chars.Capacity() + 1, allocator);
        }

        this->chars.Put(NULL, this->chars.Length());
        return this->chars.Data();
    }

    void Free(A* allocator) {
        this->chars.Free(allocator);
    }

    bool operator==(StringEx& rhs) {
        return this->chars == rhs.chars;
    }
};

class String {
    public:
    StringEx<SysAllocator> strex;
    String() : strex(StringEx<SysAllocator>(&global_allocator)) {};
    String(char *c_str) : strex(StringEx<SysAllocator>(c_str, &global_allocator)) {};

    char* Data() {
        return this->strex.Data();
    }

    char* End() {
        return this->strex.End();
    }

    char* CStr() {
        return this->strex.CStr(&global_allocator);
    }

    void Free() {
        return this->strex.Free(&global_allocator);
    }

    bool operator==(String& rhs) {
        return this->strex== rhs.strex;
    }
};

namespace std {
  template <typename A> struct hash<StringEx<A>> {
    // stolen from: http://www.cse.yorku.ca/~oz/hash.html
    size_t operator()(StringEx<A> &s) {
        unsigned long hash = 5381;
        int c;

        for (auto i=0; i<s.chars.Length(); i++) {
            c = s.chars.Get(i);
            hash = ((hash << 5) + hash) + c;
        }

        return hash;
    }
  };

  template <> struct hash<String> {
    size_t operator()(String &s) {
        return std::hash<StringEx<SysAllocator>>()(s.strex);
    }
  };
}

template<typename K, typename V>
class KeyValuePair {
    public:
    K key;
    V value;
    public:
    KeyValuePair(K key, V value) : key(key), value(value) {};
};

constexpr float kMaxLoad          = 0.85;
constexpr size_t kDefaultSlotSize = 5;
template <typename K, typename V, typename A>
class HashMapEx {
    public:
    DynamicArrayEx<KeyValuePair<K, V>, A> *data;
    size_t capacity;
    size_t length;

    HashMapEx(size_t capacity, A *allocator) : length(0), capacity(capacity) {
        this->data = allocator->template Alloc<DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>>(capacity);
        for (auto i=0; i<capacity; i++) {
            data[i] = DynamicArrayEx<KeyValuePair<K, V>, A>(kDefaultSlotSize, allocator);
        }
    }

    HashMapEx() {}

    // This is not quite right because if we needed to reference the data explicitly, for example in a sys
    // allocator we would need to pass it the data
    void Free(A *allocator) {
        for (auto i=0; i<this->capacity; i++) {
            this->data[i].Free(allocator);
        }

        allocator->Free(this->data);
    }

    void FreeKeys() {
        for (auto i=0; i<this->capacity; i++) {
            DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>* slot = &this->data[i];
            for (auto j=0; j<slot->Length(); j++) {
                slot->GetPtr(j)->key.Free();
            }
        }
    }

    void FreeValues() {
        for (auto i=0; i<this->capacity; i++) {
            DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>* slot = &this->data[i];
            for (auto j=0; j<slot->Length(); j++) {
                slot->GetPtr(j)->value.Free();
            }
        }
    }

    void FreeKeyValues() {
        for (auto i=0; i<this->capacity; i++) {
            DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>* slot = &this->data[i];
            for (auto j=0; j<slot->Length(); j++) {
                slot->GetPtr(j)->key.Free();
                slot->GetPtr(j)->value.Free();
            }
        }
    }

    V* Set(K key, V value, A *allocator) {
        size_t hashed_value = std::hash<K>()(key) % this->capacity;
        DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool> *hash_slot = &this->data[hashed_value];
        for (auto i=0; i<hash_slot->Length(); i++) {
            KeyValuePair<K, V> *entry = hash_slot->GetPtr(i);
            if (entry->key == key) {
                entry->value = value;
                return &entry->value;
            }
        }

        this->length++;
        hash_slot->Push(KeyValuePair<K, V>(key, value), allocator);

        if (length / capacity > 0.85) {
            this->Expand(allocator);
        }

        return &hash_slot->LastPtr()->value;
    }

    void Remove(K key) {
        size_t hashed_value = std::hash<K>()(key) % this->capacity;
        DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool> *hash_slot = &this->data[hashed_value];
        for (auto i=0; i<hash_slot->Length(); i++) {
            KeyValuePair<K, V> *entry = hash_slot->GetPtr(i);
            if (entry->key == key) {
                hash_slot->RemoveIndex(i);
                this->length--;
                return;
            }
        }
    }

    void Expand(A* allocator) {
        size_t new_capacity = this->capacity * 2;
        HashMapEx<K, V, A> new_map = HashMapEx<K, V, A>(new_capacity, allocator);
        for (auto i=0; i<this->capacity; i++) {
            DynamicArrayEx<KeyValuePair<K, V>, A>* slot = &this->data[i];
            for (auto j=0; j<slot->Length(); j++) {
                KeyValuePair<K, V>* entry = slot->GetPtr(j);
                new_map.Set(entry->key, entry->value, allocator);
            }
        }

        this->Free(allocator);
        *this = new_map;
    }

    // Returns null if no matching entry is found is not found
    V* GetPtr(K key) {
        size_t hashed_value = std::hash<K>()(key) % this->capacity;
        DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool> *hash_slot = &this->data[hashed_value];
        for (auto i=0; i<hash_slot->Length(); i++) {
            KeyValuePair<K, V> *entry = hash_slot->GetPtr(i);
            if (entry->key == key) {
                return &entry->value;
            }
        }

        return NULL;
    }

    // Attempts to get the entry that matches the "key" variable. If there is no matching entry call the default constructor for V
    // and add it to the map then return a pointer to that.
    V* GetPtrOrDefault(K key, A* allocator) {
        size_t hashed_value = std::hash<K>()(key) % this->capacity;
        DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool> *hash_slot = &this->data[hashed_value];
        for (auto i=0; i<hash_slot->Length(); i++) {
            KeyValuePair<K, V> *entry = hash_slot->GetPtr(i);
            if (entry->key == key) {
                return &entry->value;
            }
        }

        this->length++;
        KeyValuePair<K, V> default_entry = KeyValuePair<K, V>(key, V());
        hash_slot->Push(default_entry, allocator);
        return &hash_slot->LastPtr()->value;
    }

    size_t Length() {
        return this->length;
    }

    size_t Capacity() {
        return this->capacity;
    }

    DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>* Slot(size_t i) {
        return &this->data[i];
    }

    HashMapEx<K, V, A> Clone(A* allocator) {
        HashMapEx<K, V, A> new_map = HashMapEx<K, V, A>();
        new_map.capacity = this->capacity;
        new_map.length   = this->length;

        new_map.data = allocator->template Alloc<DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>>(new_map.capacity);
        std::memcpy(new_map.data, this->data, sizeof(DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>) * new_map.capacity);

        for (auto i=0; i<new_map.capacity; i++) {
            new_map.data[i] = this->Slot(i)->Clone(allocator);
        }

        return new_map;
    }
};

// TODO:
// Right now we have a problem with this map that we don't ever really take back the space used in it
// which could be fine but we have to use a new linear allocator when expanding, which we can't really do right now
// If the hashmap increases in size significantly, our linear allocator is way too small
template <typename K, typename V>
class HashMap {
    public:
    HashMapEx<K, V, LinearAllocatorPool> map;
    LinearAllocatorPool allocator;

    HashMap(size_t capacity) : allocator(LinearAllocatorPool(
            ((sizeof(DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>) * capacity) +
            (sizeof(KeyValuePair<K, V>) * kDefaultSlotSize * capacity))
        )) {
        size_t da_size = sizeof(DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>);
        size_t kv_size = sizeof(KeyValuePair<K, V>);
        this->map = HashMapEx<K, V, LinearAllocatorPool>(capacity, &this->allocator);
    };

    HashMap(HashMapEx<K, V, LinearAllocatorPool> map, LinearAllocatorPool allocator) : map(map), allocator(allocator) {};

    void Free() {
        this->map.Free(&this->allocator);
    }

    void FreeKeys() {
        this->map.FreeKeys();
    }

    void FreeValues() {
        this->map.FreeValues();
    }

    void FreeKeyValues() {
        for (auto i=0; i<this->Capacity(); i++) {
            DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>* slot = &this->map.data[i];
            for (auto j=0; j<slot->Length(); j++) {
                slot->GetPtr(j)->key.Free();
                slot->GetPtr(j)->value.Free();
            }
        }
    }

    V* Set(K key, V value) {
        return this->map.Set(key, value, &this->allocator);
    }

    void Remove(K key) {
        this->map.Remove(key);
    }

    V* GetPtr(K key) {
        return this->map.GetPtr(key);
    }

    V* GetPtrOrDefault(K key) {
        return this->map.GetPtrOrDefault(key, &this->allocator);
    }

    size_t Length() {
        return this->map.Length();
    }

    size_t Capacity() {
        return this->map.Capacity();
    }

    DynamicArrayEx<KeyValuePair<K, V>, LinearAllocatorPool>* Slot(size_t i) {
        return this->map.Slot(i);
    }

    HashMap<K, V> Clone() {
        return HashMap(this->map.Clone(&this->allocator), this->allocator);
    }
};

#endif