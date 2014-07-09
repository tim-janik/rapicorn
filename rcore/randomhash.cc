// Licensed GNU LGPL v3 or later: http://www.gnu.org/licenses/lgpl.html
// Author: 2014, Tim Janik, see http://testbit.org/keccak
#include "randomhash.hh"

namespace Rapicorn {

// == Lib::KeccakF1600 ==
static constexpr const uint8_t KECCAK_RHO_OFFSETS[25] = { 0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43,
                                                          25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14 };
static constexpr const uint64_t KECCAK_ROUND_CONSTANTS[255] = {
  1, 32898, 0x800000000000808a, 0x8000000080008000, 32907, 0x80000001, 0x8000000080008081, 0x8000000000008009, 138, 136, 0x80008009,
  0x8000000a, 0x8000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003, 0x8000000000008002, 0x8000000000000080, 32778,
  0x800000008000000a, 0x8000000080008081, 0x8000000000008080, 0x80000001, 0x8000000080008008, 0x8000000080008082, 0x800000008000800a,
  0x8000000000000003, 0x8000000080000009, 0x8000000000008082, 32777, 0x8000000000000080, 32899, 0x8000000000000081, 1, 32779,
  0x8000000080008001, 128, 0x8000000000008000, 0x8000000080008001, 9, 0x800000008000808b, 129, 0x8000000000000082, 0x8000008b,
  0x8000000080008009, 0x8000000080000000, 0x80000080, 0x80008003, 0x8000000080008082, 0x8000000080008083, 0x8000000080000088, 32905,
  32777, 0x8000000000000009, 0x80008008, 0x80008001, 0x800000000000008a, 0x800000000000000b, 137, 0x80000002, 0x800000000000800b,
  0x8000800b, 32907, 0x80000088, 0x800000000000800a, 0x80000089, 0x8000000000000001, 0x8000000000008088, 0x8000000000000081, 136,
  0x80008080, 129, 0x800000000000000b, 0, 137, 0x8000008b, 0x8000000080008080, 0x800000000000008b, 0x8000000000008000,
  0x8000000080008088, 0x80000082, 11, 0x800000000000000a, 32898, 0x8000000000008003, 0x800000000000808b, 0x800000008000000b,
  0x800000008000008a, 0x80000081, 0x80000081, 0x80000008, 131, 0x8000000080008003, 0x80008088, 0x8000000080000088, 32768, 0x80008082,
  0x80008089, 0x8000000080008083, 0x8000000080000001, 0x80008002, 0x8000000080000089, 130, 0x8000000080000008, 0x8000000000000089,
  0x8000000080000008, 0x8000000000000000, 0x8000000000000083, 0x80008080, 8, 0x8000000080000080, 0x8000000080008080,
  0x8000000000000002, 0x800000008000808b, 8, 0x8000000080000009, 0x800000000000800b, 0x80008082, 0x80008000, 0x8000000000008008, 32897,
  0x8000000080008089, 0x80008089, 0x800000008000800a, 0x800000000000008a, 0x8000000000000082, 0x80000002, 0x8000000000008082, 32896,
  0x800000008000000b, 0x8000000080000003, 10, 0x8000000000008001, 0x8000000080000083, 0x8000000000008083, 139, 32778,
  0x8000000080000083, 0x800000000000800a, 0x80000000, 0x800000008000008a, 0x80000008, 10, 0x8000000000008088, 0x8000000000000008,
  0x80000003, 0x8000000000000000, 0x800000000000000a, 32779, 0x8000000080008088, 0x8000000b, 0x80000080, 0x8000808a,
  0x8000000000008009, 3, 0x80000003, 0x8000000000000089, 0x8000000080000081, 0x800000008000008b, 0x80008003, 0x800000008000800b,
  0x8000000000008008, 32776, 0x8000000000008002, 0x8000000000000009, 0x80008081, 32906, 0x8000800a, 128, 0x8000000000008089,
  0x800000000000808a, 0x8000000080008089, 0x80008000, 0x8000000000008081, 0x8000800a, 9, 0x8000000080008002, 0x8000000a, 0x80008002,
  0x8000000080000000, 0x80000009, 32904, 2, 0x80008008, 0x80008088, 0x8000000080000001, 0x8000808b, 0x8000000000000002,
  0x8000000080008002, 0x80000083, 32905, 32896, 0x8000000080000082, 0x8000000000000088, 0x800000008000808a, 32906, 0x80008083,
  0x8000000b, 0x80000009, 32769, 0x80000089, 0x8000000000000088, 0x8000000080008003, 0x80008001, 0x8000000000000003,
  0x8000000080000080, 0x8000000080008009, 0x8000000080000089, 11, 0x8000000000000083, 0x80008009, 0x80000083, 32768, 0x8000800b, 32770,
  3, 0x8000008a, 0x8000000080000002, 32769, 0x80000000, 0x8000000080000003, 131, 0x800000008000808a, 32771, 32776, 0x800000000000808b,
  0x8000000080000082, 0x8000000000000001, 0x8000000000008001, 0x800000008000000a, 0x8000000080008008, 0x800000008000800b,
  0x8000000000008081, 0x80008083, 0x80000082, 130, 0x8000000080000081, 0x8000000080000002, 32904, 139, 32899, 0x8000000000000008,
  0x8000008a, 0x800000008000008b, 0x8000808a, 0x8000000000008080, 0x80000088, 0x8000000000008083, 2, 0x80008081, 32771, 32897,
  0x8000000080008000, 32770, 138,
};

/// Rotate left for uint64_t.
static inline constexpr uint64_t
bit_rotate64 (uint64_t bits, unsigned int offset)
{
  // bitwise rotate-left pattern recognized by gcc & clang iff 64==sizeof (bits)
  return (bits << offset) | (bits >> (64 - offset));
}

/// The Keccak-f[1600] permutation for up to 254 rounds, see @cite Keccak11.
void
Lib::KeccakF1600::permute (const uint32_t n_rounds)
{
  assert (n_rounds < 255); // adjust the KECCAK_ROUND_CONSTANTS access to lift this assertion
  // Keccak forward rounds
  for (size_t round_index = 0; round_index < n_rounds; round_index++)
    {
      // theta
      uint64_t C[5];
      for (size_t x = 0; x < 5; x++)
        {
          C[x] = A[x];
          for (size_t y = 1; y < 5; y++)
            C[x] ^= A[x + 5 * y];
        }
      for (size_t x = 0; x < 5; x++)
        {
          const uint64_t D = C[(5 + x - 1) % 5] ^ bit_rotate64 (C[(x + 1) % 5], 1);
          for (size_t y = 0; y < 5; y++)
            A[x + 5 * y] ^= D;
        }
      // rho
      for (size_t y = 0; y < 25; y += 5)
        {
          uint64_t *const plane = &A[y];
          for (size_t x = 0; x < 5; x++)
            plane[x] = bit_rotate64 (plane[x], KECCAK_RHO_OFFSETS[x + y]);
        }
      // pi
      const uint64_t a[25] = { A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[8], A[9], A[10], A[11], A[12],
                               A[13], A[14], A[15], A[16], A[17], A[18], A[19], A[20], A[21], A[22], A[23], A[24] };
      for (size_t y = 0; y < 5; y++)
        for (size_t x = 0; x < 5; x++)
          {
            const size_t X = (0 * x + 1 * y) % 5;
            const size_t Y = (2 * x + 3 * y) % 5;
            A[X + 5 * Y] = a[x + 5 * y];
          }
      // chi
      for (size_t y = 0; y < 25; y += 5)
        {
          uint64_t *const plane = &A[y];
          for (size_t x = 0; x < 5; x++)
            C[x] = plane[x] ^ (~plane[(x + 1) % 5] & plane[(x + 2) % 5]);
          for (size_t x = 0; x < 5; x++)
            plane[x] = C[x];
        }
      // iota
      A[0 + 5 * 0] ^= KECCAK_ROUND_CONSTANTS[round_index]; // round_index needs %255 for n_rounds>=255
    }
}

// == KeccakPRNG ==

/// Keccak permutation for 1600 bits and 24 rounds, see @cite Keccak11.
void
KeccakPRNG::permute1600()
{
  state_.permute (24);
  opos_ = 0;    // fresh outputs available
}

/** Discard 2^256 bits of the current generator state.
 * This makes it practically infeasible to guess previous generator states or
 * deduce generated values from the past.
 * Use this for forward security @cite Security03 of generated security tokens like session keys.
 */
void
KeccakPRNG::forget()
{
  std::fill (&state_[0], &state_[256 / 64], 0);
  permute1600();
}

/** Incorporate @a seed_values into the current generator state.
 * A block permutation to advance the generator state is carried out per n_nums() seed values.
 * After calling this function, generating the next n_nums() random values will not need to
 * block for a new permutation.
 */
void
KeccakPRNG::xor_seed (const uint64_t *seeds, size_t n_seeds)
{
  size_t i = 0;
  while (n_seeds)
    {
      if (i > 0)        // not first seed block
        permute1600();  // prepare for next seed block
      const size_t count = std::min (n_nums(), n_seeds);
      for (i = 0; i < count; i++)
        state_[i] ^= seeds[i];
      seeds += count;
      n_seeds -= count;
    }
  if (i < 25)           // apply Simple padding: pad10*
    state_[i] ^= 0x1;   // 1: payload boundary bit for Keccak Sponge compliant padding
  permute1600();        // integrate seed
}

/// The destructor resets the generator state to avoid leaving memory trails.
KeccakPRNG::~KeccakPRNG()
{
  // forget all state and leave no trails
  std::fill (&state_[0], &state_[25], 0xaffeaffeaffeaffe);
  forget();
  opos_ = n_nums();
}

// == SHAKE_Base - primitive for SHA3 hashing ==
template<size_t HASHBITS, uint8_t DOMAINBITS>
class SHAKE_Base {
  Lib::KeccakF1600      state_;
  const size_t          rate_;
  size_t                ipos_;
  bool                  feeding_mode_;
protected:
  /// Add stream data up to block size into Keccak state via XOR.
  size_t
  xor_state (size_t offset, const uint8_t *input, size_t n_in)
  {
    assert (offset + n_in <= byte_rate());
    size_t i;
    for (i = offset; i < offset + n_in; i++)
      state_.byte (i) ^= input[i - offset];
    return i - offset;
  }
  /** Pad stream from offset to block boundary into Keccak state via XOR.
   * The @a trail argument must contain the termination bit, optionally preceeded by
   * additional (LSB) bits for domain separation. A permutation is carried out if the
   * trailing padding bits do not fit into the remaining block length.
   */
  void
  absorb_padding (size_t offset, uint8_t trail = 0x01)
  {
    assert (offset < byte_rate());              // offset is first byte following payload
    assert (trail != 0x00);
    // MultiRatePadding
    state_.byte (offset) ^= trail;              // 1: payload boundary bit for MultiRatePadding (and SimplePadding)
    // state_.bits[i..last] ^= 0x0;             // 0*: bitrate filler for MultiRatePadding (and SimplePadding)
    const size_t lastbyte = byte_rate() - 1;    // last bitrate byte, may coincide with offset
    if (offset == lastbyte && trail >= 0x80)
      state_.permute (24);                      // prepare new block to append last bit
    state_.byte (lastbyte) ^= 0x80;             // 1: last bitrate bit; required by MultiRatePadding
  }
  SHAKE_Base (size_t rate) : rate_ (rate), ipos_ (0), feeding_mode_ (true)
  {
    std::fill (&state_[0], &state_[25], 0);
  }
public:
  size_t
  byte_rate() const
  {
    return rate_ / 8;
  }
  void
  reset()
  {
    std::fill (&state_[0], &state_[25], 0);
    ipos_ = 0;
    feeding_mode_ = true;
  }
  void
  update (const uint8_t *data, size_t length)
  {
    assert (feeding_mode_ == true);
    while (length)
      {
        const size_t count = min (byte_rate() - ipos_, length);
        xor_state (ipos_, data, count);
        ipos_ += count;
        data += count;
        length -= count;
        if (ipos_ >= byte_rate())
          {
            state_.permute (24);
            ipos_ = 0;
          }
      }
  }
  size_t
  get_hash (uint8_t hashvalue[HASHBITS / 8])
  {
    if (feeding_mode_)
      {
        const uint8_t shake_delimiter = DOMAINBITS;
        absorb_padding (ipos_, shake_delimiter);
        ipos_ = ~size_t (0);
        state_.permute (24);
        feeding_mode_ = false;
      }
    const size_t count = HASHBITS / 8;
    for (size_t i = 0; i < count; i++)
      hashvalue[i] = state_.byte (i);
    return count;
  }
};

// == SHA3_512 ==
struct SHA3_512::State : SHAKE_Base<512, 0x06> {
  State() : SHAKE_Base (576) {}
};

SHA3_512::SHA3_512 () :
  state_ (new State())
{}

SHA3_512::~SHA3_512 ()
{
  delete state_;
}

void
SHA3_512::update (const uint8_t *data, size_t length)
{
  state_->update (data, length);
}

void
SHA3_512::digest (uint8_t hashvalue[64])
{
  state_->get_hash (hashvalue);
}

void
SHA3_512::reset ()
{
  state_->reset();
}

void
sha3_512_hash (const void *data, size_t data_length, uint8_t hashvalue[64])
{
  SHA3_512 context;
  context.update ((const uint8_t*) data, data_length);
  context.digest (hashvalue);
}

} // Rapicorn
