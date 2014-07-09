// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
// Author: 2014, Tim Janik, see http://testbit.org/keccak
#ifndef __RAPICORN_RANDOMHASH_HH__
#define __RAPICORN_RANDOMHASH_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

namespace Lib { // Namespace for implementation internals

/// The Keccak-f[1600] Permutation, see the Keccak specification @cite Keccak11.
class KeccakF1600 {
  union {
    uint64_t            A[25];
    // __MMX__: __m64   V[25];
  } __attribute__ ((__aligned__ (16)));
public:
  uint64_t&     operator[]  (int      index)       { return A[index]; }
  uint64_t      operator[]  (int      index) const { return A[index]; }
  void          permute     (uint32_t n_rounds);
};

} // Lib

/** KeccakPRNG - A KeccakF1600 based cryptographically secure pseudo-random number generator.
 * The permutation steps are derived from the Keccak specification @cite Keccak11.
 * For further details about this implementation, see also: http://testbit.org/keccak
 */
class KeccakPRNG {
  Lib::KeccakF1600    state_;
  const size_t        prng_bit_rate_;
  size_t              opos_;
  void                permute1600();
public:
  /// Integral type of the KeccakPRNG generator results.
  typedef uint64_t    result_type;
  /// Amount of 64 bit random numbers per generated block.
  inline size_t       n_nums() const            { return prng_bit_rate_ / 64; }
  /// Amount of bits used to store hidden random number generator state.
  inline size_t       bit_capacity() const      { return 1600 - prng_bit_rate_; }
  /*dtor*/           ~KeccakPRNG  ();
  /// Create a Keccak PRNG with one 64 bit @a seed_value.
  explicit
  KeccakPRNG (uint64_t seed_value = 1) : prng_bit_rate_ (1024), opos_ (n_nums())
  {
    // static_assert (prng_bit_rate_ <= 1536 && prng_bit_rate_ % 64 == 0, "KeccakPRNG bit rate is invalid");
    std::fill (&state_[0], &state_[25], 0);
    seed (seed_value);
  }
  /// Create a Keccak PRNG, seeded from a @a seed sequence.
  template<class SeedSeq> explicit
  KeccakPRNG (SeedSeq &seed_sequence) : prng_bit_rate_ (1024), opos_ (n_nums())
  {
    // static_assert (prng_bit_rate_ <= 1536 && prng_bit_rate_ % 64 == 0, "KeccakPRNG bit rate is invalid");
    std::fill (&state_[0], &state_[25], 0);
    seed (seed_sequence);
  }
  void forget   ();
  void xor_seed (const uint64_t *seeds, size_t n_seeds);
  /// Reinitialize the generator state using a 64 bit @a seed_value.
  void seed     (uint64_t seed_value = 1)               { seed (&seed_value, 1); }
  /// Reinitialize the generator state using a nuber of 64 bit @a seeds.
  void
  seed (const uint64_t *seeds, size_t n_seeds)
  {
    std::fill (&state_[0], &state_[25], 0);
    xor_seed (seeds, n_seeds);
  }
  /// Reinitialize the generator state from a @a seed_sequence.
  template<class SeedSeq> void
  seed (SeedSeq &seed_sequence)
  {
    uint32_t u32[50];                   // fill 50 * 32 = 1600 state bits
    seed_sequence.generate (&u32[0], &u32[50]);
    uint64_t u64[25];
    for (size_t i = 0; i < 25; i++)     // Keccak bit order: 1) LSB 2) MSB
      u64[i] = u32[i * 2] | (uint64_t (u32[i * 2 + 1]) << 32);
    seed (u64, 25);
  }
  /// Generate a new 64 bit random number.
  /// A new block permutation is carried out every n_nums() calls, see also xor_seed().
  result_type
  operator() ()
  {
    if (opos_ >= n_nums())
      permute1600();
    return state_[opos_++];
  }
  /// Compare two generators for state equality.
  friend bool
  operator== (const KeccakPRNG &lhs, const KeccakPRNG &rhs)
  {
    for (size_t i = 0; i < 25; i++)
      if (lhs.state_[i] != rhs.state_[i])
        return false;
    return lhs.opos_ == rhs.opos_ && lhs.prng_bit_rate_ == rhs.prng_bit_rate_;
  }
  /// Compare two generators for state inequality.
  friend bool
  operator!= (const KeccakPRNG &lhs, const KeccakPRNG &rhs)
  {
    return !(lhs == rhs);
  }
  /// Minimum of the result type, for uint64_t that is 0.
  result_type
  min() const
  {
    return std::numeric_limits<result_type>::min(); // 0
  }
  /// Maximum of the result type, for uint64_t that is 18446744073709551615.
  result_type
  max() const
  {
    return std::numeric_limits<result_type>::max(); // 18446744073709551615
  }
  /// Serialize generator state into an OStream.
  template<typename CharT, typename Traits>
  friend std::basic_ostream<CharT, Traits>&
  operator<< (std::basic_ostream<CharT, Traits> &os, const KeccakPRNG &self)
  {
    typedef typename std::basic_ostream<CharT, Traits>::ios_base IOS;
    const typename IOS::fmtflags saved_flags = os.flags();
    os.flags (IOS::dec | IOS::fixed | IOS::left);
    const CharT space = os.widen (' ');
    const CharT saved_fill = os.fill();
    os.fill (space);
    os << self.opos_;
    for (size_t i = 0; i < 25; i++)
      os << space << self.state_[i];
    os.flags (saved_flags);
    os.fill (saved_fill);
    return os;
  }
  /// Deserialize generator state from an IStream.
  template<typename CharT, typename Traits>
  friend std::basic_istream<CharT, Traits>&
  operator>> (std::basic_istream<CharT, Traits> &is, KeccakPRNG &self)
  {
    typedef typename std::basic_istream<CharT, Traits>::ios_base IOS;
    const typename IOS::fmtflags saved_flags = is.flags();
    is.flags (IOS::dec | IOS::skipws);
    is >> self.opos_;
    self.opos_ = std::min (self.n_nums(), self.opos_);
    for (size_t i = 0; i < 25; i++)
      is >> self.state_[i];
    is.flags (saved_flags);
    return is;
  }
};

} // Rapicorn

#endif /* __RAPICORN_RANDOMHASH_HH__ */
