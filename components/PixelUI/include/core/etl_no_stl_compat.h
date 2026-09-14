#pragma once

#include <stddef.h>

// ETL 20.48.1 skips these declarations on Apple Clang when ETL_NO_STL is set,
// but still emits the corresponding structured-binding specializations.
#if defined(ETL_NO_STL) && defined(__APPLE__) && defined(__clang__)
namespace std {
inline namespace __1 {
template <typename T>
struct tuple_size;

template <size_t Index, typename T>
struct tuple_element;
} // namespace __1
} // namespace std
#endif
