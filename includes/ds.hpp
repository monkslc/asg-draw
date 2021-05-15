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

    DynamicArray() {
        DynamicArray(0);
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
};

#endif