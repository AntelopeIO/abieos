#pragma once

#include <cmath>
#include <charconv>
#include "for_each_field.hpp"
#include "stream.hpp"
#include "types.hpp"
#include <limits>
#include <optional>
#include <rapidjson/encodings.h>
#include <variant>
#include <map>

namespace eosio {

inline constexpr char hex_digits[] = "0123456789ABCDEF";

// Adaptors for rapidjson
struct stream_adaptor {
   stream_adaptor(const char* src, int sz) {
      int chars = std::min(sz, 4);
      memcpy(buf, src, chars);
      memset(buf + chars, 0, 4 - chars);
   }
   void Put(char ch) {}
   char Take() { return buf[idx++]; }
   char buf[4];
   int  idx = 0;
};

// Replaces any invalid utf-8 bytes with ?
template <typename S>
void to_json(std::string_view sv, S& stream) {
   stream.write('"');
   auto begin = sv.begin();
   auto end   = sv.end();
   while (begin != end) {
      auto pos = begin;
      while (pos != end && *pos != '"' && *pos != '\\' && (unsigned char)(*pos) >= 32 && *pos != 127) ++pos;
      while (begin != pos) {
         stream_adaptor s2(begin, static_cast<std::size_t>(pos - begin));
         if (rapidjson::UTF8<>::Validate(s2, s2)) {
            stream.write(begin, s2.idx);
            begin += s2.idx;
         } else {
            ++begin;
            stream.write('?');
         }
      }
      if (begin != end) {
         if (*begin == '"') {
            stream.write("\\\"", 2);
         } else if (*begin == '\\') {
            stream.write("\\\\", 2);
         } else {
            stream.write("\\u00", 4);
            stream.write(hex_digits[(unsigned char)(*begin) >> 4]);
            stream.write(hex_digits[(unsigned char)(*begin) & 15]);
         }
         ++begin;
      }
   }
   stream.write('"');
}

template <typename S>
void to_json(const std::string& s, S& stream) {
   to_json(std::string_view{ s }, stream);
}

template <typename S>
void to_json(const char* s, S& stream) {
   to_json(std::string_view{ s }, stream);
}

/*
template <typename S>
result<void> to_json(const shared_memory<std::string_view>& s, S& stream) {
   return to_json(*s, stream);
}
*/

template <typename S>
void to_json(bool value, S& stream) {
   if (value)
      stream.write("true", 4);
   else
      stream.write("false", 5);
}

template <typename T>
struct make_unsigned : std::make_unsigned<T> {};

#ifndef ABIEOS_NO_INT128
// some standard library does not support std::make_unsigned<__int128> yet. 
template <>
struct make_unsigned<__int128> {
   using type = unsigned __int128;
};

template <>
struct make_unsigned<unsigned __int128> {
   using type = unsigned __int128;
};
#endif

template <typename T>
using make_unsigned_t = typename make_unsigned<T>::type;

template <typename T>
char* int_to_decimal(T value, char* buffer) {
   char* pos = buffer;
   auto uvalue = make_unsigned_t<T>(value);
   bool neg    = value < 0;
   if (neg)
      uvalue = -uvalue;
   
   do {
      *pos++ = '0' + (uvalue % 10);
      uvalue /= 10;
   } while (uvalue);

   if (neg)
      *pos++ = '-';
   std::reverse(buffer, pos);
   return pos;
}

template <typename T, typename S>
void int_to_json(T value, S& stream) {
   // For older versions of libstdc++ (g++ version 9 and below) std::numeric_limits<__int128>::digits10 
   // would return 0 when compiling with -std=c++17 flag
   constexpr int num_digits = sizeof(T) == 16 ? 38 : std::numeric_limits<T>::digits10;
   small_buffer<num_digits + 4> b;
   if (sizeof(T) > 4)
      *b.pos++ = '"';
   b.pos = int_to_decimal(value, b.pos);
   if (sizeof(T) > 4)
      *b.pos++ = '"';
   stream.write(b.data, b.pos - b.data);
}

template <typename S>
void fp_to_json(double value, S& stream) {
   // fpconv is not quite consistent with javascript for nans and infinities
   if (value == std::numeric_limits<double>::infinity()) {
      stream.write("\"Infinity\"", 10);
   } else if (value == -std::numeric_limits<double>::infinity()) {
      stream.write("\"-Infinity\"", 11);
   } else if (std::isnan(value)) {
      stream.write("\"NaN\"", 5);
   } else {
      std::array<char, 25> b;
      auto r = std::to_chars(b.data(), b.data() + b.size(), value, std::chars_format::fixed);
      if (r.ec != std::errc())
         r = std::to_chars(b.data(), b.data() + b.size(), value);
      check( r.ec == std::errc(), convert_stream_error(stream_error::float_error) );
      stream.write(b.data(), r.ptr - b.data());
   }
}

// clang-format off
template <typename S> void to_json(unsigned char value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(uint16_t value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(uint32_t value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(uint64_t value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(char value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(signed char value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(int16_t value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(int32_t value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(int64_t value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(double value, S& stream) { return fp_to_json(value, stream); }
template <typename S> void to_json(float value, S& stream) { return fp_to_json(value, stream); }

#ifndef ABIEOS_NO_INT128
template <typename S> void to_json(unsigned __int128 value, S& stream) { return int_to_json(value, stream); }
template <typename S> void to_json(__int128 value, S& stream) { return int_to_json(value, stream); }
#endif

// clang-format on

template <typename T, typename S>
void to_json(const std::vector<T>& obj, S& stream) {
   stream.write('[');
   bool first = true;
   for (auto& v : obj) {
      if (first) {
         increase_indent(stream);
      } else {
         stream.write(',');
      }
      write_newline(stream);
      first = false;
      to_json(v, stream);
   }
   if (!first) {
      decrease_indent(stream);
      write_newline(stream);
   }
   stream.write(']');
}

template <typename K, typename V, typename S>
void to_json(const std::map<K, V>& obj, S& stream) {
   stream.write('{');
   bool first = true;
   for (const auto& [k,v] : obj) {
      if (first) {
         increase_indent(stream);
      } else {
         stream.write(',');
      }
      write_newline(stream);
      first = false;
      to_json(k, stream);
      stream.write(':');
      to_json(v, stream);
   }
   if (!first) {
      decrease_indent(stream);
      write_newline(stream);
   }
   stream.write('}');
}

template <typename T, typename S>
void to_json(const std::optional<T>& obj, S& stream) {
   if (obj) {
      to_json(*obj, stream);
   } else {
      stream.write("null", 4);
   }
}

template <typename... T, typename S>
void to_json(const std::variant<T...>& obj, S& stream) {
   stream.write('[');
   increase_indent(stream);
   write_newline(stream);
   std::visit(
      [&](const auto& t) { to_json(get_type_name((std::decay_t<decltype(t)>*)nullptr), stream); }, obj);
   stream.write(',');
   write_newline(stream);
   std::visit([&](auto& x) { return to_json(x, stream); }, obj);
   decrease_indent(stream);
   write_newline(stream);
   stream.write(']');
}

   template<typename>
   struct is_std_optional : std::false_type {};

   template<typename T>
   struct is_std_optional<std::optional<T>> : std::true_type {
      using value_type = T;
   };

template <typename T, typename S>
void to_json(const T& t, S& stream) {
   bool         first = true;
   stream.write('{');
   eosio::for_each_field<T>([&](const char* name, auto&& member) {
       auto addfield = [&]() {
         if (first) {
            increase_indent(stream);
            first = false;
         } else {
            stream.write(',');
         }
         write_newline(stream);
         to_json(name, stream);
         write_colon(stream);
         to_json(member(&t), stream);
      };

      auto m = member(&t);
      using member_type = std::decay_t<decltype(m)>;
      if constexpr ( not is_std_optional<member_type>::value ) {
         addfield();
      } else {
         // !!! Skipping empty optionals breaks the tests, because
         // abi serialization can't handle it.
         if( !!m || true )
            addfield();
      }
   });
   if (!first) {
      decrease_indent(stream);
      write_newline(stream);
   }
   stream.write('}');
}

template <typename S>
void to_json_hex(const char* data, size_t size, S& stream) {
   stream.write('"');
   for (size_t i = 0; i < size; ++i) {
      unsigned char byte = data[i];
      stream.write(hex_digits[byte >> 4]);
      stream.write(hex_digits[byte & 15]);
   }
   stream.write('"');
}

#ifdef __eosio_cdt__

template <typename S> void to_json(long double value, S& stream) {
   return to_json_hex(reinterpret_cast<const char*>(&value), sizeof(long double), stream);
}

#endif

template <typename T>
std::string convert_to_json(const T& t) {
   size_stream ss;
   to_json(t, ss);
   std::string      result(ss.size, 0);
   fixed_buf_stream fbs(result.data(), result.size());
   to_json(t, fbs);
   check( fbs.pos == fbs.end, convert_stream_error(stream_error::underrun) );
   return result;
}

template <typename T>
std::string format_json(const T& t) {
   pretty_stream<size_stream> ss;
   to_json(t, ss);
   std::string                     result(ss.size, 0);
   pretty_stream<fixed_buf_stream> fbs(result.data(), result.size());
   to_json(t, fbs);
   check( fbs.pos == fbs.end, convert_stream_error(stream_error::underrun) );
   return result;
}

} // namespace eosio
