// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <rcore/randomhash.hh>
#include <random>
#include <array>

using namespace Rapicorn;

/** SeedSeqFE256 provides a fixed-entropy seed sequence with good avalanche properties.
 * This class provides a replacement for std::seed_seq that avoids problems with bias,
 * performs better in empirical statistical tests, and executes faster in
 * normal-sized use cases.
 * The implementation is based on randutils::seed_seq_fe256 from Melissa E. O'Neill,
 * see: http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html
 */
class SeedSeqFE256 {
  std::array<uint32_t, 8> mixer_;
  static constexpr uint32_t INIT_A = 0x43b0d7e5;
  static constexpr uint32_t MULT_A = 0x931e8875;
  static constexpr uint32_t INIT_B = 0x8b51f9dd;
  static constexpr uint32_t MULT_B = 0x58f38ded;
  static constexpr uint32_t MIX_MULT_L = 0xca01f9dd;
  static constexpr uint32_t MIX_MULT_R = 0x4973f715;
  static constexpr uint32_t XSHIFT = sizeof (uint32_t) * 8 / 2;
public:
  SeedSeqFE256 ()           {}
  template<typename T>
  SeedSeqFE256 (std::initializer_list<T> init)
  {
    seed (init.begin(), init.end());
  }
  template<typename InputIter>
  SeedSeqFE256 (InputIter begin, InputIter end)
  {
    seed (begin, end);
  }
  template<typename InputIter> void
  seed (InputIter begin, InputIter end)
  {
    const uint32_t *beginp = &*begin;
    const uint32_t *endp = &*end;
    mix_input (beginp, endp);
  }
  template<typename RandomAccessIterator> void
  generate (RandomAccessIterator dest_begin, RandomAccessIterator dest_end) const
  {
    uint32_t *beginp = &*dest_begin;
    uint32_t *endp = &*dest_end;
    generate_output (beginp, endp);
  }
private:
  void
  mix_input (const uint32_t *begin, const uint32_t *end)
  {
    // based on http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html
    // Copyright (C) 2015 Melissa E. O'Neill, The MIT License (MIT)
    auto hash_const = INIT_A;
    auto hash = [&] (uint32_t value) {
      value ^= hash_const;
      hash_const *= MULT_A;
      value *= hash_const;
      value ^= value >> XSHIFT;
      return value;
    };
    auto mix = [] (uint32_t x, uint32_t y) {
      uint32_t result = MIX_MULT_L * x - MIX_MULT_R * y;
      result ^= result >> XSHIFT;
      return result;
    };
    const uint32_t *current = begin;
    for (auto &elem : mixer_)
      {
        if (current != end)
          elem = hash (*current++);
        else
          elem = hash (0U);
      }
    for (auto &src : mixer_)
      for (auto &dest : mixer_)
        if (&src != &dest)
          dest = mix (dest, hash (src));
    for (; current != end; ++current)
      for (auto &dest : mixer_)
        dest = mix (dest, hash (*current));
  }
  void
  generate_output (uint32_t *dest_begin, uint32_t *dest_end) const
  {
    // based on http://www.pcg-random.org/posts/developing-a-seed_seq-alternative.html
    // Copyright (C) 2015 Melissa E. O'Neill, The MIT License (MIT)
    auto src_begin  = mixer_.begin();
    auto src_end    = mixer_.end();
    auto src        = src_begin;
    auto hash_const = INIT_B;
    for (auto dest = dest_begin; dest != dest_end; ++dest)
      {
        auto dataval = *src;
        if (++src == src_end)
          src = src_begin;
        dataval ^= hash_const;
        hash_const *= MULT_B;
        dataval *= hash_const;
        dataval ^= dataval >> XSHIFT;
        *dest = dataval;
      }
  }
};

// == EntropyTests ==
struct EntropyTests : Entropy {
  void
  test (char *arg)
  {
    if      (strcmp (arg, "--entropy") == 0)
      {
        printout ("%016x%016x%016x%016x%016x%016x%016x%016x\n",
                  Entropy::get_seed(), Entropy::get_seed(), Entropy::get_seed(), Entropy::get_seed(),
                  Entropy::get_seed(), Entropy::get_seed(), Entropy::get_seed(), Entropy::get_seed());
        exit (0);
      }
    else if (strcmp (arg, "--system-entropy") == 0)
      {
        KeccakPRNG pool;
        Entropy::system_entropy (pool);
        printout ("%016x%016x%016x%016x%016x%016x%016x%016x\n", pool(), pool(), pool(), pool(), pool(), pool(), pool(), pool());
        exit (0);
      }
    else if (strcmp (arg, "--runtime-entropy") == 0)
      {
        KeccakPRNG pool;
        Entropy::runtime_entropy (pool);
        printout ("%016x%016x%016x%016x%016x%016x%016x%016x\n", pool(), pool(), pool(), pool(), pool(), pool(), pool(), pool());
        exit (0);
      }
  }
};

static void
test_entropy()
{
  const uint64_t seed1 = Entropy::get_seed();
  const uint64_t seed2 = Entropy::get_seed();
  TASSERT (seed1 != seed2);
  Entropy e;
  KeccakPRNG k1, k2 (e);
  TASSERT (k1 != k2);
}
REGISTER_TEST ("RandomHash/Entropy", test_entropy);

static void
test_random_numbers()
{
  TASSERT (random_int64() != random_int64());
  TASSERT (random_float() != random_float());
  TASSERT (random_irange (-999999999999999999, +999999999999999999) != random_irange (-999999999999999999, +999999999999999999));
  TASSERT (random_frange (-999999999999999999, +999999999999999999) != random_frange (-999999999999999999, +999999999999999999));
  for (size_t i = 0; i < 999999; i++)
    {
      const uint64_t ri = random_irange (989617512, 9876547656);
      TASSERT (ri >= 989617512 && ri < 9876547656);
      const double rf = random_frange (989617512, 9876547656);
      TASSERT (rf >= 989617512 && rf < 9876547656);
    }
  TASSERT (isnan (random_frange (NAN, 1)));
  TASSERT (isnan (random_frange (0, NAN)));
#if 0 // example penalty paid in random_int64()
  size_t i, j = 0;
  for (i = 0; i < 100; i++)
    {
      uint64_t r = k2();
      j += r > 9223372036854775808ULL;
      printout (" rand64: %d: 0x%016x\n", r > 9223372036854775808ULL, r);
    }
  printout (" rand64: fail: %d/%d -> %f%%\n", j, i, j * 100.0 / i);
  for (size_t i = 0; i < 100; i++)
    {
      // printout (" rand: %+d\n", random_irange (-5, 5));
      // printout (" rand: %f\n", random_float());
      printout (" rand: %g\n", random_frange (1000000000000000, 1000000000000000.912345678)-1000000000000000);
    }
#endif
}
REGISTER_TEST ("RandomHash/Random Numbers", test_random_numbers);

template<class Gen>
struct GeneratorBench64 {
  enum {
    N_RUNS = 1000,
    BLOCK_NUMS = 128,
  };
  Gen gen_;
  GeneratorBench64() : gen_() {}
  void
  operator() ()
  {
    uint64_t sum = 0;
    for (size_t j = 0; j < N_RUNS; j++)
      {
        for (size_t i = 0; i < BLOCK_NUMS; i++)
          sum ^= gen_();
      }
    TASSERT ((sum & 0xffffffff) != 0 && sum >> 32 != 0);
  }
  size_t
  bytes_per_run()
  {
    return sizeof (uint64_t) * BLOCK_NUMS * N_RUNS;
  }
};
struct Gen_lrand48 { uint64_t operator() () { return uint64_t (lrand48()) << 32 | lrand48(); } };
struct Gen_minstd  {
  std::minstd_rand minstd;
  uint64_t operator() () { return uint64_t (minstd()) << 32 | minstd(); }
};

struct Gen_Pcg32 {
  Pcg32Rng pcg1, pcg2;
  Gen_Pcg32() : pcg1 (0x853c49e6748fea9bULL, 1), pcg2 (0x853c49e6748fea9bULL, 2) {}
  uint64_t operator() () { return (uint64_t (pcg1.random()) << 32) | pcg2.random(); }
};

static void
random_hash_benchmarks()
{
  Test::Timer timer (0.5); // maximum seconds
  GeneratorBench64<std::mt19937_64> mb; // core-i7: 1415.3MB/s
  double bench_time = timer.benchmark (mb);
  TPASS ("mt19937_64 # size=%-4zd timing: fastest=%fs prng=%.1fMB/s\n", sizeof (mb), bench_time, mb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<Gen_minstd> sb;      // core-i7:  763.7MB/s
  bench_time = timer.benchmark (sb);
  TPASS ("minstd     # size=%-4zd timing: fastest=%fs prng=%.1fMB/s\n", sizeof (sb), bench_time, sb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<Gen_lrand48> lb;     // core-i7:  654.8MB/s
  bench_time = timer.benchmark (lb);
  TPASS ("lrand48()  # size=%-4zd timing: fastest=%fs prng=%.1fMB/s\n", sizeof (lb), bench_time, lb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<KeccakPRNG> kb;      // core-i7:  185.3MB/s
  bench_time = timer.benchmark (kb);
  TPASS ("KeccakPRNG # size=%-4zd timing: fastest=%fs prng=%.1fMB/s\n", sizeof (kb), bench_time, kb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<Gen_Pcg32> pb;
  bench_time = timer.benchmark (pb);
  TPASS ("Pcg32Rng   # size=%-4zd timing: fastest=%fs prng=%.1fMB/s\n", sizeof (pb), bench_time, pb.bytes_per_run() / bench_time / 1048576.);
}
REGISTER_TEST ("RandomHash/~ Benchmarks", random_hash_benchmarks);

static void
test_pcg32()
{
  Pcg32Rng pcg (0x853c49e6748fea9b, 0xda3e39cb94b95bdb);
  static const uint32_t ref[] = {
    0x486fa52f, 0x6c1825ef, 0xbc7dfd25, 0x2e39be5d, 0xbab9d529, 0x3db767df, 0x8a57b4e5, 0x62cb137d,
    0xb121455c, 0x5d82c6c9, 0x161abd58, 0x011d98b6, 0x82bcfc78, 0x41b769d5, 0x77519400, 0x18bb198e,
    0x3a8b2057, 0xf1512960, 0x9a750b7f, 0x04681633, 0x9b6a5b63, 0x5938d377, 0x29cd44fb, 0xc3721bf6,
    0x20cc9794, 0xe8d41401, 0x500009b9, 0xfe598389, 0xdeebbb9c, 0x4b2a51e7, 0x4ae4140d, 0xa64e4fa2,
    0x2dc6a4cc, 0xd01c11ad, 0xedbc9f1b, 0x35a3c1df, 0x4449116e, 0x2239fdf7, 0x7000690f, 0x4f60058e,
    0x1d20a167, 0xd9edfc2c, 0x8e8902f2, 0xcfed8ff2, 0x05ecda19, 0x228ee89b, 0x8f4b6611, 0x4502516a,
    0x75af390c, 0xc57e17ef, 0x7bce8d57, 0x95a0243a, 0x44756680, 0x41355c24, 0x6a90eb2c, 0x737a859a,
    0x09484943, 0xc203b087, 0xd74db10b, 0x58581181, 0x578164f4, 0x2f2e074a, 0x633016d3, 0x4e8ce966,
    0xffd9615f, 0xd2e3c4e6, 0x06d284d4, 0x26d4992f, 0xcf9e3b7b, 0xeaa253fe, 0xc3ceae63, 0x861fa55b,
    0x7b0903f6, 0x97942419, 0x17816c14, 0xc2ddae2c, 0x24649959, 0xe119bab4, 0xd5e799ee, 0x6f026406,
    0xa76379a9, 0xfb11a942, 0xad1bc66d, 0x417a71be, 0x2419f1d4, 0x2e147b0f, 0xab978e73, 0x0d99f55a,
    0xd4451092, 0x61289548, 0x98ef28b8, 0x13a103e4, 0x8c287e23, 0xa6b55d2f, 0x0188aced, 0xbef16c94,
    0xae63a19e, 0x681c6636, 0xc2420ad9, 0xcea8838b, 0x6b4702ea, 0xe07aa9ce, 0x0a84bb38, 0x5b854da5,
    0x9f1a7d0e, 0xaa3322ee, 0x7bf1e47d, 0xfa7628a9, 0x1fc2c457, 0x647eec8d, 0x9806ac98, 0x60f7ccd2,
    0x65a5f357, 0x66bf120c, 0xebb03252, 0x4ab8342c, 0x0815863a, 0x5e231280, 0xd8239bf8, 0xf07552a8,
    0x9de7423f, 0x1b638c20, 0x9cc1906d, 0xebf2f75a, 0xb0ae1c23, 0xba701ce1, 0x13e17960, 0x7e8152c4,
  };
  for (size_t i = 0; i < ARRAY_SIZE (ref); i++)
    TCMP (ref[i], ==, pcg.random());
}
REGISTER_TEST ("RandomHash/Pcg32Rng", test_pcg32);

static void
test_seeder32()
{
  uint32_t ref[337] = { 0, };
  constexpr uint32_t LENGTH = 17;
  SeedSeqFE256 seeder (&ref[0], &ref[ARRAY_SIZE (ref)]);
  // simple non-zero + mixing check
  uint32_t seed1[LENGTH] = { 0, };
  seeder.generate (&seed1[0], &seed1[LENGTH]);
  for (uint i = 0; i < LENGTH; i++)
    {
      assert (seed1[i] != 0);
      assert (seed1[i] != seed1[(i + 1) % LENGTH]);
    }
  // check avalange of single bit
  ref[5] = 0x00100000;
  seeder.seed (&ref[0], &ref[ARRAY_SIZE (ref)]);
  uint32_t seed2[LENGTH] = { 0, };
  seeder.generate (&seed2[0], &seed2[LENGTH]);
  for (uint i = 0; i < LENGTH; i++)
    {
      assert (seed2[i] != 0);
      assert (seed2[i] != seed1[i]);
      assert (seed2[i] != seed2[(i + 1) % LENGTH]);
    }
}
REGISTER_TEST ("RandomHash/SeedSeqFE256", test_seeder32);

static void
test_keccak_prng()
{
  KeccakPRNG krandom1;
  String digest;
  for (size_t i = 0; i < 6; i++)
    {
      const uint64_t r = krandom1();
      const uint32_t h = r >> 32, l = r;
      digest += string_format ("%02x%02x%02x%02x%02x%02x%02x%02x",
                               l & 0xff, (l>>8) & 0xff, (l>>16) & 0xff, l>>24, h & 0xff, (h>>8) & 0xff, (h>>16) & 0xff, h>>24);
    }
  // printf ("KeccakPRNG: %s\n", digest.c_str());
  TCMP (digest, ==, "c336e57d8674ec52528a79e41c5e4ec9b31aa24c07cdf0fc8c6e8d88529f583b37a389883d2362639f8cc042abe980e0"); // 24 rounds

  std::stringstream kss;
  kss << krandom1;
  KeccakPRNG krandom2;
  TASSERT (krandom1 != krandom2 && !(krandom1 == krandom2));
  kss >> krandom2;
  TASSERT (krandom1 == krandom2 && !(krandom1 != krandom2));
  TASSERT (krandom1() == krandom2() && krandom1() == krandom2() && krandom1() == krandom2() && krandom1() == krandom2());
  krandom1();
  TASSERT (krandom1 != krandom2 && krandom1() != krandom2());
  krandom2();
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom1.discard (0);
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom1.discard (777);
  TASSERT (krandom1 != krandom2 && krandom1() != krandom2());
  for (size_t i = 0; i < 777; i++)
    krandom2();
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom1();
  krandom2.discard (1);
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom2.forget();
  TASSERT (krandom1 != krandom2);
  krandom1.seed (0x11007700affe0101);
  krandom2.seed (0x11007700affe0101);
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  const uint64_t one = 1;
  krandom1.seed (one);          // seed with 0x1 directly
  std::seed_seq seq { 0x01 };   // seed_seq generates "random" bits from its input
  krandom2.seed (seq);
  TASSERT (krandom1 != krandom2);
  krandom2.seed (&one, 1);      // seed with array containing just 0x1
  TASSERT (krandom1 == krandom2 && krandom1() == krandom2());
  krandom2.seed (krandom1);     // uses krandom1.generate
  TASSERT (krandom1 != krandom2 && krandom1() != krandom2());
}
REGISTER_TEST ("RandomHash/KeccakPRNG", test_keccak_prng);


int
main (int   argc,
      char *argv[])
{
  if (argc >= 2)        // Entropy tests that need to be carried out before core_init
    EntropyTests().test (argv[1]);

  init_core_test (__PRETTY_FILE__, &argc, argv);

  return Test::run();
}
