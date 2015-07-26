#ifndef TYPES_H
#define TYPES_H

#include <memory>

template<typename T>
using SPtr = std::shared_ptr<T>;

template<typename T>
using UPtr = std::unique_ptr<T>;

template<typename T>
using WPtr = std::weak_ptr<T>;

#endif
