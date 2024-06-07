#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <ostream>
#include <sstream>
#include <string>

template <typename T>
concept StreamInsertable = requires(T a, std::ostream os) { os << a; };

template <typename Arg>
    requires StreamInsertable<Arg>
void _build_string(std::ostringstream& os, Arg&& arg) {
    os << arg;
}

template <typename Arg, typename... Args>
    requires StreamInsertable<Arg>
void _build_string(std::ostringstream& os, Arg&& arg, Args&&... args) {
    os << arg;
    _build_string(os, std::forward<Args>(args)...);
}

/**
 * Builds a string from the given arguments. Arguments must be stream
 * insertable.
 *
 * @tparam Args The types of the arguments.
 * @param args The arguments to build the string from.
 * @return The built string.
 */
template <typename... Args>
std::string build_string(Args&&... args) {
    std::ostringstream os;
    _build_string(os, std::forward<Args>(args)...);
    return os.str();
}

#endif  // UTILS_H
