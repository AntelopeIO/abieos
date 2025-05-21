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

   size_type num_blocks() const {
      assert(m_bits.size() == calc_num_blocks(m_num_bits));
      return m_bits.size();
   }

   void resize(size_type num_bits) {
      m_bits.resize(calc_num_blocks(num_bits), 0);
      m_num_bits = num_bits;
      zero_unused_bits();
   }

   void set(size_type pos) {
      assert(pos < m_num_bits);
      m_bits[block_index(pos)] |= bit_mask(pos);
   }

   void clear(size_type pos) {
      assert(pos < m_num_bits);
      m_bits[block_index(pos)] &= ~bit_mask(pos);
   }

   bool test(size_type pos) const {
      return (*this)[pos];
   }

   bool operator[](size_type pos) const {
      assert(pos < m_num_bits);
      return !!(m_bits[block_index(pos)] & bit_mask(pos));
   }

   void flip(size_type pos) {
      assert(pos < m_num_bits);
      if (test(pos))
         clear(pos);
      else
         set(pos);
   }

   void flip() {
      for (auto& byte : m_bits)
         byte = ~byte;
      zero_unused_bits();
   }

   bool all() const {
      auto sz = size();
      for (size_t i=0; i<sz; ++i)
         if (!test(i))
            return false;
      return true;
   }

   bool none() const {
      for (auto& byte : m_bits)
         if (byte)
            return false;
      return true;
   }

   void zero_all_bits() {
      for (auto& byte : m_bits)
         byte = 0;
   }

   bitset operator|=(const bitset& o) {
      assert(size() == o.size());
      for (size_t i=0; i<m_bits.size(); ++i)
         m_bits[i] |= o.m_bits[i];
      return *this;
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

   friend auto operator<(const bitset& a, const bitset& b) {
      return std::tuple(a.m_num_bits, a.m_bits) < std::tuple(b.m_num_bits, b.m_bits);
   }

   friend bool operator==(const bitset& a, const bitset& b) {
      return std::tuple(a.m_num_bits, a.m_bits) == std::tuple(b.m_num_bits, b.m_bits);
   }

   uint8_t& byte(size_t i) {
      assert(i < m_bits.size());
      return m_bits[i];
   }

   const uint8_t& byte(size_t i) const {
      assert(i < m_bits.size());
      return m_bits[i];
   }

   std::string to_string() const {
      std::string res;
      res.resize(size());
      size_t idx = 0;
      for (auto i = size(); i-- > 0;)
         res[idx++] = (*this)[i] ? '1' : '0';
      return res;
   }

   static bitset from_string(std::string_view s) {
      bitset bs;
      auto   num_bits = s.size();
      bs.resize(num_bits);

      for (size_t i = 0; i < num_bits; ++i) {
         switch (s[i]) {
         case '0':
            break; // nothing to do, all bits initially 0
         case '1':
            bs.set(num_bits - i - 1); // high bitset indexes come first in the JSON representation
            break;
         default:
            throw std::invalid_argument( "unexpected character in bitset string representation" );
            break;
         }
      }
      assert(bs.unused_bits_zeroed());
      return bs;
   }

private:
   size_type   m_num_bits{0}; // members order would matter if we used defaulted `operator<=>` (must match `to_key` below).
   buffer_type m_bits;        // must be after `m_num_bits`
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
// The bitset is represented as a sequence of 0 and 1 characters in decreasing bit order,
// so that first character represents bit N and the last character bit 0.
// The number of 0 and 1 characters defines the size of the bitset.
// Any non-0/1 characters in the string result in failure.
//
// ex: "110001011" -> 0x09 0x8b 0x01
// ----------------------------------------------------------------------------------------
template <typename S>
void from_json(bitset& obj, S& stream) {
   auto str = stream.get_string();
   obj = bitset::from_string(str);
}

template <typename S>
void to_json(const bitset& obj, S& stream) {
   to_json(obj.to_string(), stream);
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