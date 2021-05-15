#ifndef DS_H
#define DS_H

#include <stdlib.h>

template <typename T>
class DynamicArray {
    public:
    size_t capacity;
    size_t length;
    T *data;
    DynamicArray(size_t capacity) : length(0), capacity(capacity) {
        this->data = (T*) malloc(sizeof(T) * capacity);
    }

    DynamicArray() : length(0), capacity(0) {
        this->data = (T*) malloc(sizeof(T) * this->capacity);
    }

    void IncreaseCapacity(size_t new_capacity) {
        this->data = (T*) realloc(this->data, sizeof(T) * new_capacity);
        this->capacity = new_capacity;
    }

    void Resize(size_t new_length) {
        this->length = new_length;
        if (this->length > this->capacity) {
            this->IncreaseCapacity(this->length);
        }
    }

    void Push(T elem) {
        if (this->length >= this->capacity) {
            this->IncreaseCapacity((this->capacity * 2) + 1); // Add +1 here incase capacity is 0
        }

        this->data[this->length] = elem;
        *(this->data + this->length) = elem;
        this->length++;
    }

    void Put(T elem, size_t index) {
        this->data[index] = elem;
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

    T* End() {
        return &this->data[this->length];
    }

    void Clear() {
        this->length = 0;
    }

    void Free() {
        free(this->data);
    }
};

class String {
    public:
    DynamicArray<char> chars;
    String();
    String(char *c_str) : chars(DynamicArray<char>(10)) {
        char *iter = c_str;
        while (*iter) {
            this->chars.Push(*iter);
            iter++;
        }
    };

    // CStr places a null pointer past the end of the array but doesn't increase the size of the
    // underlying array. So if an element is pushed to the array after getting a pointer to the c
    // string, it will overwrite the null pointer
    char* CStr() {
        if (this->chars.length >= this->chars.capacity) {
            this->chars.IncreaseCapacity(this->chars.capacity + 1);
        }

        this->chars.Put(NULL, this->chars.length);
        return this->chars.data;
    }

    void Free() {
        this->chars.Free();
    }
};

#endif