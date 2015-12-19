// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
// Author: 2014, Tim Janik, see http://testbit.org/keccak
#ifndef __RAPICORN_RANDOMHASH_HH__
#define __RAPICORN_RANDOMHASH_HH__

#include <rcore/utilities.hh>

namespace Rapicorn {

/** SHA3_224 - 224 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 224 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_224 {
  /*dtor*/ ~SHA3_224    ();
  /*ctor*/  SHA3_224    ();         ///< Create context to calculate a 224 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[28]);                ///< Retrieve the resulting hash value.
  class     State;
private: State *state_;
};
/// Calculate 224 bit SHA3 digest from @a data, see also class SHA3_224.
void    sha3_224_hash   (const void *data, size_t data_length, uint8_t hashvalue[28]);

/** SHA3_256 - 256 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 256 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_256 {
  /*dtor*/ ~SHA3_256    ();
  /*ctor*/  SHA3_256    ();         ///< Create context to calculate a 256 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[32]);                ///< Retrieve the resulting hash value.
  class     State;
private: State *state_;
};
/// Calculate 256 bit SHA3 digest from @a data, see also class SHA3_256.
void    sha3_256_hash   (const void *data, size_t data_length, uint8_t hashvalue[32]);

/** SHA3_384 - 384 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 384 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_384 {
  /*dtor*/ ~SHA3_384    ();
  /*ctor*/  SHA3_384    ();         ///< Create context to calculate a 384 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[48]);                ///< Retrieve the resulting hash value.
  class     State;
private: State *state_;
};
/// Calculate 384 bit SHA3 digest from @a data, see also class SHA3_384.
void    sha3_384_hash   (const void *data, size_t data_length, uint8_t hashvalue[48]);

/** SHA3_512 - 512 Bit digest generation.
 * This class implements the SHA3 hash funtion to create 512 Bit digests, see FIPS 202 @cite Fips202 .
 */
struct SHA3_512 {
  /*dtor*/ ~SHA3_512    ();
  /*ctor*/  SHA3_512    ();         ///< Create context to calculate a 512 bit SHA3 hash digest.
  void      reset       ();         ///< Reset state to feed and retrieve a new hash value.
  void      update      (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      digest      (uint8_t hashvalue[64]);                ///< Retrieve the resulting hash value.
  class     State;
private: State *state_;
};
/// Calculate 512 bit SHA3 digest from @a data, see also class SHA3_512.
void    sha3_512_hash   (const void *data, size_t data_length, uint8_t hashvalue[64]);

/** SHAKE128 - 128 Bit extendable output digest generation.
 * This class implements the SHA3 extendable output hash funtion with 128 bit security strength, see FIPS 202 @cite Fips202 .
 */
struct SHAKE128 {
  /*dtor*/ ~SHAKE128        ();
  /*ctor*/  SHAKE128        ();         ///< Create context to calculate an unbounded SHAKE128 hash digest.
  void      reset           ();         ///< Reset state to feed and retrieve a new hash value.
  void      update          (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      squeeze_digest  (uint8_t *hashvalues, size_t n);        ///< Retrieve an arbitrary number of hash value bytes.
  class     State;
private: State *state_;
};
/// Calculate SHA3 extendable output digest for 128 bit security strength, see also class SHAKE128.
void    shake128_hash   (const void *data, size_t data_length, uint8_t *hashvalues, size_t n);

/** SHAKE256 - 256 Bit extendable output digest generation.
 * This class implements the SHA3 extendable output hash funtion with 256 bit security strength, see FIPS 202 @cite Fips202 .
 */
struct SHAKE256 {
  /*dtor*/ ~SHAKE256        ();
  /*ctor*/  SHAKE256        ();         ///< Create context to calculate an unbounded SHAKE256 hash digest.
  void      reset           ();         ///< Reset state to feed and retrieve a new hash value.
  void      update          (const uint8_t *data, size_t length);   ///< Feed data to be hashed.
  void      squeeze_digest  (uint8_t *hashvalues, size_t n);        ///< Retrieve an arbitrary number of hash value bytes.
  class     State;
private: State *state_;
};
/// Calculate SHA3 extendable output digest for 256 bit security strength, see also class SHAKE256.
void    shake256_hash   (const void *data, size_t data_length, uint8_t *hashvalues, size_t n);

namespace Lib { // Namespace for implementation internals

/// The Keccak-f[1600] Permutation, see the Keccak specification @cite Keccak11 .
class KeccakF1600 {
  union {
    uint64_t            A[25];
    uint8_t             bytes[200];
    // __MMX__: __m64   V[25];
  } __attribute__ ((__aligned__ (16)));
public:
  uint64_t&     operator[]  (int      index)       { return A[index]; }
  uint64_t      operator[]  (int      index) const { return A[index]; }
  void          permute     (uint32_t n_rounds);
  inline uint8_t&
  byte (size_t state_index)
  {
#if   __BYTE_ORDER == __LITTLE_ENDIAN
    return bytes[(state_index / 8) * 8 + (state_index % 8)];            // 8 == sizeof (uint64_t)
#elif __BYTE_ORDER == __BIG_ENDIAN
    return bytes[(state_index / 8) * 8 + (8 - 1 - (state_index % 8))];  // 8 == sizeof (uint64_t)
#else
#   error "Unknown __BYTE_ORDER"
#endif
  }
};

} // Lib

/** KeccakPRNG - A KeccakF1600 based cryptographically secure pseudo-random number generator.
 * The permutation steps are derived from the Keccak specification @cite Keccak11 .
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
  void discard  (unsigned long long count);
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
  /// Fill the range [begin, end) with random unsigned integer values.
  template<typename RandomAccessIterator> void
  generate (RandomAccessIterator begin, RandomAccessIterator end)
  {
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type Value;
    while (begin != end)
      {
        const uint64_t rbits = operator()();
        *begin++ = Value (rbits);
        if (sizeof (Value) <= 4 && begin != end)
          *begin++ = Value (rbits >> 32);
      }
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

/** Pcg32Rng is a permutating linear congruential PRNG.
 * At the core, this pseudo random number generator uses the well known
 * linear congruential generator:
 * 6364136223846793005 * accumulator + 1442695040888963407 mod 2^64.
 * See also TAOCP by D. E. Knuth, section 3.3.4, table 1, line 26.
 * For good statistical performance, the output function of the permuted congruential
 * generator family is used as described on http://www.pcg-random.org/.
 * Period length for this generator is 2^64, the specified seed @a offset
 * chooses the position of the genrator and the seed @a sequence parameter
 * can be used to choose from 2^63 distinct random sequences.
 */
class Pcg32Rng {
  uint64_t increment_;          // must be odd, allows for 2^63 distinct random sequences
  uint64_t accu_;               // can contain all 2^64 possible values
  static constexpr const uint64_t A = 6364136223846793005ULL; // from C. E. Hayness, see TAOCP by D. E. Knuth, 3.3.4, table 1, line 26.
  static inline constexpr uint32_t
  ror32 (const uint32_t bits, const uint32_t offset)
  {
    // bitwise rotate-right pattern recognized by gcc & clang iff 32==sizeof (bits)
    return (bits >> offset) | (bits << (32 - offset));
  }
  static inline constexpr uint32_t
  pcg_xsh_rr (const uint64_t input)
  {
    // Section 6.3.1. 32-bit Output, 64-bit State: PCG-XSH-RR
    // http://www.pcg-random.org/pdf/toms-oneill-pcg-family-v1.02.pdf
    return ror32 ((input ^ (input >> 18)) >> 27, input >> 59);
  }
public:
  /// Initialize and seed.
  explicit Pcg32Rng  (uint64_t offset = 11400714819323198485ULL, uint64_t sequence = 1442695040888963407ULL);
  /// Seed by seeking to position @a offset within stream @a sequence.
  void     seed      (uint64_t offset, uint64_t sequence = 1442695040888963407ULL);
  /// Generate uniformly distributed 32 bit pseudo random number.
  uint32_t
  random ()
  {
    const uint64_t lcgout = accu_;      // using the *last* state as ouput helps with CPU pipelining
    accu_ = A * accu_ + increment_;
    return pcg_xsh_rr (lcgout);         // PCG XOR-shift + random rotation
  }
};

} // Rapicorn

#endif /* __RAPICORN_RANDOMHASH_HH__ */
