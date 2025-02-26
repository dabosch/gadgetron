#pragma once

#include <iostream>
#include <type_traits>

#include "hoNDArray.h"
#include "Types.h"
#include <boost/hana/adapt_struct.hpp>

namespace Gadgetron::Core::IO {

    template<class T>
    std::enable_if_t<is_trivially_copyable_v<T>> read(std::istream &stream, T &t);

    template<class T>
    std::enable_if_t<is_trivially_copyable_v<T>> read(std::istream &stream, T *data, size_t elements);

    template<class T>
    std::enable_if_t<!is_trivially_copyable_v<T>> read(std::istream &stream, T *data, size_t elements);

    template<class T>
    void read(std::istream &stream, Core::optional<T> &opt);

    template<class T>
    void read(std::istream &stream, std::vector<T> &vec);

    template<class T>
    void read(std::istream &stream, hoNDArray<T> &array);

    template<class T>
    std::enable_if_t<boost::hana::Struct<T>::value> read(std::istream &istream, T &x);

    template<class T>
    T read(std::istream &stream) {
        T val;
        IO::read(stream, val);
        return val;
    }

    template<class T = uint64_t >
    std::string read_string_from_stream(std::istream &stream);

    template<class T, class V = std::enable_if_t<is_trivially_copyable_v<T>>>
    void write(std::ostream &stream, const T &value);

    template<class T>
    void write(std::ostream &ostream, const Core::optional<T> &val);

    template<class T>
    void write(std::ostream &ostream, const std::vector<T> &val);

    template<class T>
    std::enable_if_t<boost::hana::Struct<T>::value, void> write(std::ostream &ostream, const T &x);

    template<class T>
    std::enable_if_t<is_trivially_copyable_v<T>>
    write(std::ostream &stream, const T *data, size_t number_of_elements);

    template<class T>
    std::enable_if_t<!is_trivially_copyable_v<T>>
    write(std::ostream &stream, const T *data, size_t number_of_elements);

    template<class T>
    void write(std::ostream &stream, const hoNDArray<T> &array);

    template<class T = uint64_t>
    void write_string_to_stream(std::ostream &stream, const std::string &str);
}

#include "primitives.hpp"