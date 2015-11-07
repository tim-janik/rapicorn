// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include <rcore/testutils.hh>
#include <rcore/randomhash.hh>
#include <random>

using namespace Rapicorn;

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

static void
random_hash_benchmarks()
{
  Test::Timer timer (1); // 1 second maximum
  GeneratorBench64<std::mt19937_64> mb; // core-i7: 1415.3MB/s
  double bench_time = timer.benchmark (mb);
  TPASS ("mt19937_64 # timing: fastest=%fs prng=%.1fMB/s\n", bench_time, mb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<Gen_minstd> sb;      // core-i7:  763.7MB/s
  bench_time = timer.benchmark (sb);
  TPASS ("minstd     # timing: fastest=%fs prng=%.1fMB/s\n", bench_time, sb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<Gen_lrand48> lb;     // core-i7:  654.8MB/s
  bench_time = timer.benchmark (lb);
  TPASS ("lrand48()  # timing: fastest=%fs prng=%.1fMB/s\n", bench_time, lb.bytes_per_run() / bench_time / 1048576.);
  GeneratorBench64<KeccakPRNG> kb;      // core-i7:  185.3MB/s
  bench_time = timer.benchmark (kb);
  TPASS ("KeccakPRNG # timing: fastest=%fs prng=%.1fMB/s\n", bench_time, kb.bytes_per_run() / bench_time / 1048576.);
}
REGISTER_TEST ("RandomHash/~ Benchmarks", random_hash_benchmarks);

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
  TASSERT (digest == "c336e57d8674ec52528a79e41c5e4ec9b31aa24c07cdf0fc8c6e8d88529f583b37a389883d2362639f8cc042abe980e0");

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
