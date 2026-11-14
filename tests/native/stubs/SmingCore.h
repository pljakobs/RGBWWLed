#pragma once
#include <sdk.h>
#include <algorithm>
#include <string>
#include <sstream>
#include "Wiring/WConstants.h"
#include "Wiring/WHashMap.h"
using uint32 = uint32_t;
inline uint32_t millis() { return Mock::now / 1000; }

class String : public std::string {
public:
    using std::string::string;
    String(const std::string& value) : std::string(value) {}
    float toFloat() const { return std::stof(*this); }
    int toInt() const { return std::stoi(*this); }
};

template <typename T> class Vector : public std::vector<T> {
public:
    unsigned count() const { return this->size(); }
    void addElement(const T& value) { this->push_back(value); }
    bool contains(const T& value) const {
        return std::find(this->begin(), this->end(), value) != this->end();
    }
};

inline void splitString(const String& input, char delimiter, Vector<String>& output)
{
    std::stringstream stream(input);
    std::string token;
    while(std::getline(stream, token, delimiter)) output.addElement(token);
}
