/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include "from_bin.hpp"
#include "from_json.hpp"
#include "to_bin.hpp"
#include "to_json.hpp"

namespace eosio {

// -------------------------------------------------------------------------------
//      see https://github.com/AntelopeIO/spring/wiki/ABI-1.3:-bitset-type
// -------------------------------------------------------------------------------


// stores a bitset in a std::vector<uint8_t>
//
// - bits 0-7 in first byte, 8-15 in second, ...
// - least significant bit of byte 0 is bit 0 of bitset.
// - unused bits must be zero.
// ---------------------------------------------------------------------------------
struct bitset {
   using buffer_type                         = std::vector<uint8_t>;
   using size_type                           = uint32_t;
   static constexpr size_type bits_per_block = 8;
   static constexpr size_type npos           = static_cast<size_type>(-1);

   static constexpr size_type calc_num_blocks(size_type num_bits) {
      return (num_bits + bits_per_block - 1) / bits_per_block;
   }

   static size_type block_index(size_type pos) noexcept { return pos / bits_per_block; }
   static uint8_t   bit_index(size_type pos)   noexcept { return static_cast<uint8_t>(pos % bits_per_block); }
   static uint8_t   bit_mask(size_type pos)    noexcept { return uint8_t(1) << bit_index(pos); }

   size_type size() const { return m_num_bits; }

   void resize(size_type num_bits) {
      m_bits.resize(calc_num_blocks(num_bits), 0);
      m_num_bits = num_bits;
      zero_unused_bits();
   }

   void set(size_type pos) {
      assert(pos < m_num_bits);
      m_bits[block_index(pos)] |= bit_mask(pos);
   }

   bool operator[](size_type pos) const {
      assert(pos < m_num_bits);
      return !!(m_bits[block_index(pos)] & bit_mask(pos));
   }

   void zero_all_bits() {
      for (auto& byte : m_bits)
         byte = 0;
   }

   void zero_unused_bits() {
      assert (m_bits.size() == calc_num_blocks(m_num_bits));

      // if != 0 this is the number of bits used in the last block
      const size_type extra_bits = bit_index(size());

      if (extra_bits != 0)
         m_bits.back() &= (uint8_t(1) << extra_bits) - 1;
   }

   bool unused_bits_zeroed() const {
      // if != 0 this is the number of bits used in the last block
      const size_type extra_bits = bit_index(size());
      return extra_bits == 0 || (m_bits.back() & ~((uint8_t(1) << extra_bits) - 1)) == 0;
   }

   friend auto operator<=>(const bitset& a, const bitset& b) {
      return std::tie(a.m_num_bits, a.m_bits) <=> std::tie(b.m_num_bits, b.m_bits);
   }

   friend bool operator==(const bitset& a, const bitset& b)  {
      return std::tie(a.m_num_bits, a.m_bits) == std::tie(b.m_num_bits, b.m_bits);
   }

   uint8_t& byte(size_t i) {
      assert(i < m_bits.size());
      return m_bits[i];
   }

   const uint8_t& byte(size_t i) const {
      assert(i < m_bits.size());
      return m_bits[i];
   }

private:
   buffer_type m_bits;
   size_type   m_num_bits{0};
};

constexpr const char* get_type_name(bitset*) { return "bitset"; }

// binary representation
// ---------------------
// The bitset first encodes the number of bits it contains as a varint, then encodes
// (size+8-1)/8 bytes into the stream. The first byte represents bits 0-7, the next 8-15,
// and so on; i.e. LSB first.
// Within a byte, the least significant bit stores the smaller bitset index.
// Unused bits should be written as 0.
//
// This matches the storage scheme of bitset above
// ---------------------------------------------------------------------------------------
template <typename S>
void from_bin(bitset& obj, S& stream) {
   uint32_t num_bits = 0;
   varuint32_from_bin(num_bits, stream);
   obj.resize(num_bits);
   if (num_bits > 0) {
      auto num_blocks = bitset::calc_num_blocks(obj.size());
      for (size_t i=0; i<num_blocks; ++i) 
         from_bin(obj.byte(i), stream);
      obj.zero_unused_bits();
      assert(obj.unused_bits_zeroed());
   }
}

template <typename S>
void to_bin(const bitset& obj, S& stream) {
   varuint32_to_bin(obj.size(), stream);
   if (obj.size() > 0) {
      auto num_blocks = bitset::calc_num_blocks(obj.size());
      assert(num_blocks >= 1);

      for (size_t i=0; i<num_blocks; ++i)
         to_bin(obj.byte(i), stream);
   }
}

// JSON representation
// -------------------
// The bitset is represented as a string starting with 0b and then a sequence of 0 and 1
// characters in decreasing bit order, so that first character represents bit N and the last
// character bit 0.
// The number of 0 and 1 characters defines the size of the bitset.
// Not starting with 0b or containing any non-0/1 characters in the string after the header
// result in failure.
//
// ex: "0b110001011" -> 0x09 0x8b 0x01
// ----------------------------------------------------------------------------------------
template <typename S>
void from_json(bitset& obj, S& stream) {
   auto str = stream.get_string();
   check(str.starts_with("0b"), convert_json_error(from_json_error::incorrect_bitset_prefix));
   auto num_chars = str.size();
   obj.zero_all_bits();                      // reset all existing bytes to 0
   obj.resize(num_chars - 2);        // `num_chars - 2` is number of bits. if size greater, new bytes will be 0 as well
   
   for (size_t i=2; i<num_chars; ++i) {
      switch(str[i]) {
      case '0':
         break;                      // nothing to do, all bits initially 0
      case '1':
         obj.set(num_chars - i - 1); // high bitset indexes come first in the JSON representation
         break;
      default:
         eosio::detail::assert_or_throw(convert_json_error(from_json_error::unexpected_character_in_bitset));
         break;
      }
   }
   assert(obj.unused_bits_zeroed());
}

template <typename S>
void to_json(const bitset& obj, S& stream) {
   stream.write("\"0b", 3);
   if (obj.size() > 0) {
      // write bits in decreasing order N to 0, high bitset indexes come first in the JSON representation
      for (auto i = obj.size(); i-- > 0 ;)
         stream.write(obj[i] ? '1' : '0');
   }
   stream.write('"');
}

template <typename S>
void to_key(const bitset& obj, S& stream) {
   assert(obj.unused_bits_zeroed());
   to_key(obj.size(), stream);
   auto num_blocks = bitset::calc_num_blocks(obj.size());
   for (size_t i=0; i<num_blocks; ++i)
      to_key_optional(&obj.byte(i), stream);
   to_key_optional((const uint8_t*)nullptr, stream);
}

}