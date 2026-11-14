#pragma once
#include <algorithm>
using std::min;
using std::max;
template <typename T, typename L, typename H> T constrain(T value, L low, H high)
{
    return std::max<T>(low, std::min<T>(high, value));
}
