#ifndef UTILS_H
#define UTILS_H

char* FindChar(char *str, char ch);
char* SkipChar(char *str, char ch);
float RoundFloatingInput(float x);

template<typename T>
class Slice {
    T* data;
    size_t size;
    Slice(T* data, size_t size) : data(data), size(size) {};
};

#endif