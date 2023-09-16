#ifndef LAND15_COMMON_DELETER_PTR_H_
#define LAND15_COMMON_DELETER_PTR_H_

#include <functional>
#include <memory>

// A convenience alias for creating unique_ptr's with cutom deleters.
// Use it like this:
//
// deleter_ptr<FILE> file(fopen("file.txt", "r"), [](FILE* f){ fclose(f); });
//
//
namespace land15 {
namespace common {

template <class T>
using deleter_ptr = std::unique_ptr<T, std::function<void(T*)>>;

}  // namespace common
}  // namespace land15

#endif  // LAND15_COMMON_DELETER_PTR_H_
