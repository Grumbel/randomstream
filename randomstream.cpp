// randomstream - stream of pseudo random numbers
// Copyright (C) 2014-2016 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <array>
#include <assert.h>
#include <errno.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <random>
#include <thread>
#include <mutex>
#include <condition_variable>

constexpr size_t BUFFERSIZE = 1024 * 1024 / sizeof(uint64_t);

class RndGenerator
{
public:
  using result_type = uint64_t;
  inline result_type min() const { return 0; }
  inline result_type max() const { return std::numeric_limits<result_type>::max(); }

public:
  virtual ~RndGenerator() {}
  virtual uint64_t operator()() = 0;
  virtual bool is_const() { return false; }
};

// 530MiB/s on Intel Core Duo E6300 1.86Ghz
class XORShift96 : public RndGenerator
{
private:
  uint64_t x;
  uint64_t y = 362436069;
  uint64_t z = 521288629;

public:
  XORShift96(uint64_t seed = 123456789) :
    x(seed)
  {}

  inline uint64_t operator()()
  {
    uint64_t t;

    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    return z;
  }
};

// 450MiB/s on Intel Core Duo E6300 1.86Ghz
class XORShift64 : public RndGenerator
{
private:
  uint64_t x;

public:
  XORShift64(uint64_t seed = 123456789) :
    x(seed)
  {}

  inline uint64_t operator()()
  {
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;

    return x * UINT64_C(2685821657736338717);
  }
};

class ConstGenerator : public RndGenerator
{
private:
  uint64_t x;

public:
  ConstGenerator(uint64_t seed = 0) :
    x(seed)
  {}

  inline uint64_t operator()()
  {
    return x;
  }

  bool is_const()
  {
    return true;
  }
};

void print_help(int argc, char** argv)
{
  std::cout <<
    "Usage: " << argv[0] << " [OPTION]...\n"
    "\n"
    "Options:\n"
    "  -h, --help              Display this help text\n"
    "  --version               Display version number\n"
    "  -a, --algorithm ALG     Generate random numbers with ALG (default: xorshift96)\n"
    "  -A, --ascii             Limit output to printable ASCII characters\n"
    "  -s, --seed SEED         Use SEED as uint64 seed value, \n"
    "                          'time' for time of day seed (default: 0)\n"
    "\n"
    "Algorithms:\n"
    "  xorshift96   XORShift96 Algorithm\n"
    "  xorshift64   XORSHIFT64 Algorithm\n"
    "  zero         Output 0s\n"
    "  const        Output the seed value repeatedly\n";
  std::cout << std::flush;
}

void print_version(int argc, char** argv)
{
  std::cout << argv[0] << " v0.1.0" << std::endl;
}

enum class AlgorithmType {
  XORSHIFT64,
  XORSHIFT96,
  ZERO,
  CONST
};

std::unique_ptr<RndGenerator> create_rnd(AlgorithmType type, uint64_t seed)
{
  switch(type)
  {
    case AlgorithmType::XORSHIFT96:
      return std::make_unique<XORShift96>(seed);

    case AlgorithmType::XORSHIFT64:
      return std::make_unique<XORShift64>(seed);

    case AlgorithmType::ZERO:
      return std::make_unique<ConstGenerator>(0);

    case AlgorithmType::CONST:
      return std::make_unique<ConstGenerator>(seed);

    default:
      assert(false && "never reached");
      return {};
  }
}

std::vector<std::pair<const char*, AlgorithmType> > string_algorithmtype_map =
{
  { "xorshift64", AlgorithmType::XORSHIFT64 },
  { "xorshift96", AlgorithmType::XORSHIFT96 },
  { "zero",       AlgorithmType::ZERO },
  { "const",      AlgorithmType::CONST }
};

AlgorithmType string_to_algorithm(const char* text)
{
  auto it = std::find_if(string_algorithmtype_map.begin(),
                         string_algorithmtype_map.end(),
                         [text](auto const& p){
                           return (strcmp(p.first, text) == 0);
                         });

  if (it == string_algorithmtype_map.end())
  {
    std::cerr << "error: couldn't convert '" << text << "' to algorithm" << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  {
    return it->second;
  }
}

uint64_t time_seed()
{
  // using gettimeofday() instead of time(NULL) to get sub-second
  // seed changes
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec ^ tv.tv_usec);
}

class Options
{
public:
  Options() :
    algorithm(AlgorithmType::XORSHIFT96),
    seed(0),
    count(0),
    ascii(false)
  {}

  AlgorithmType algorithm;
  uint64_t seed;
  uint64_t count;
  bool ascii;
};

Options parse_args(int argc, char** argv)
{
  Options opts;

  for(int i = 1; i < argc; ++i)
  {
    auto opt = [&]{ return argv[i]; };
    auto arg = [&]{
      if (i + 1 >= argc)
      {
        std::cerr << "error: " << opt() << " requires an argument" << std::endl;
        exit(EXIT_FAILURE);
      }
      return argv[i + 1];
    };
    auto skip_arg = [&]{
      i += 1;
    };

    if (strcmp(opt(), "--help") == 0 ||
        strcmp(opt(), "-h") == 0)
    {
      print_help(argc, argv);
      exit(EXIT_SUCCESS);
    }
    else if (strcmp(opt(), "--version") == 0)
    {
      print_version(argc, argv);
      exit(EXIT_SUCCESS);
    }
    else if (strcmp(opt(), "--algorithm") == 0 ||
        strcmp(opt(), "-a") == 0)
    {
      opts.algorithm = string_to_algorithm(arg());
      skip_arg();
    }
    else if (strcmp(opt(), "--ascii") == 0 ||
             strcmp(opt(), "-A") == 0)
    {
      opts.ascii = true;
    }
    else if (strcmp(opt(), "--seed") == 0 ||
             strcmp(opt(), "-s") == 0)
    {
      if (strcmp(arg(), "time") == 0)
      {
        opts.seed = time_seed();
      }
      else
      {
        opts.seed = std::stoll(arg());
      }
      skip_arg();
    }
    else if (strcmp(opt(), "--count") == 0 ||
             strcmp(opt(), "-c") == 0)
    {
      opts.count = std::stoll(arg());
      skip_arg();
    }
    else
    {
      std::cerr << "Unknown option: " << argv[i] << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  return opts;
}

template<typename T>
class PseudoRng
{
public:
  using result_type = T;
  inline result_type min() const { return 0; }
  inline result_type max() const { return std::numeric_limits<result_type>::max(); }

public:
  inline PseudoRng(result_type value) :
    m_value(value)
  {}

  inline result_type operator()() { return m_value; }

private:
  result_type m_value;
};

inline uint64_t make_ascii(RndGenerator& rng)
{
  static std::uniform_int_distribution<uint64_t> ascii_distribution(32, 126);
  uint64_t rnd = rng();
  PseudoRng<uint64_t> prng1(rnd >> 0);
  PseudoRng<uint64_t> prng2(rnd >> 8);
  PseudoRng<uint64_t> prng3(rnd >> 16);
  PseudoRng<uint64_t> prng4(rnd >> 24);
  PseudoRng<uint64_t> prng5(rnd >> 32);
  PseudoRng<uint64_t> prng6(rnd >> 40);
  PseudoRng<uint64_t> prng7(rnd >> 48);
  PseudoRng<uint64_t> prng8(rnd >> 56);
  return ((ascii_distribution(prng1) <<  0) |
          (ascii_distribution(prng2) <<  8) |
          (ascii_distribution(prng3) << 16) |
          (ascii_distribution(prng4) << 24) |
          (ascii_distribution(prng5) << 32) |
          (ascii_distribution(prng6) << 40) |
          (ascii_distribution(prng7) << 48) |
          (ascii_distribution(prng8) << 56));
}

inline void rnd_ascii_fill_buffer(RndGenerator& rng, uint8_t* buffer, size_t len)
{
  size_t i = 0;
  while(true)
  {
    uint64_t rnd = rng();
    for(int j = 0; j < 8; ++j)
    {
      buffer[i] = static_cast<uint8_t>(rnd >> (8 * j)) & 0x7f;
      if (32 <= buffer[i] && buffer[i] < 127)
      {
        i += 1;
        if (!(i < len))
        {
          return;
        }
      }
    }
  }
}

inline void rnd_fill_buffer(RndGenerator& rng, uint64_t* buffer, size_t len)
{
  for(size_t i = 0; i < len; ++i)
  {
    buffer[i] = rng();
  }
}

int main(int argc, char** argv)
{
  Options opts = parse_args(argc, argv);

  // FIXME: this is a bit of a mess
  if (opts.count != 0)
  {
    auto rnd = create_rnd(opts.algorithm, opts.seed);

    std::vector<uint64_t> buffer(BUFFERSIZE);

    for(auto&& v : buffer)
    {
      v = (*rnd)();
    }

    uint64_t total = 0;
    bool quit = false;
    while(!quit && total != opts.count)
    {
      auto len = buffer.size() * sizeof(uint64_t);
      if (total + len > opts.count)
      {
        len = opts.count - total;
      }

      if (write(STDOUT_FILENO, buffer.data(), len) < 0)
      {
        perror("<stdout>");
        quit = true;
      }

      total += len;
    }
  }
  else
  {
    auto t_rnd = create_rnd(opts.algorithm, opts.seed);
    if (t_rnd->is_const())
    {
      auto rnd = create_rnd(opts.algorithm, opts.seed);
      std::vector<uint64_t> buffer(BUFFERSIZE);

      // fill it with initial values
      for(auto&& v : buffer)
      {
        if (opts.ascii)
        {
          v = make_ascii(*rnd);
        }
        else
        {
          v = (*rnd)();
        }
      }

      bool quit = false;
      while(!quit)
      {
        if (write(STDOUT_FILENO, buffer.data(), buffer.size() * sizeof(uint64_t)) < 0)
        {
          perror("<stdout>");
          quit = true;
        }
      }
    }
    else
    {
      auto num_threads = std::thread::hardware_concurrency();
      if (num_threads == 0)
        num_threads = 1;

      bool quit = false;
      std::vector<std::thread> threads;
      for(size_t t = 0; t < num_threads; ++t)
      {
        threads.emplace_back(
          [&]{
            std::vector<uint64_t> buffer1(BUFFERSIZE);
            std::vector<uint64_t> buffer2(BUFFERSIZE);

            std::vector<uint64_t>* write_buffer = &buffer1;
            std::vector<uint64_t>* read_buffer = &buffer2;

            auto rnd = create_rnd(opts.algorithm, opts.seed + t);

            std::mutex read_buffer_mutex;
            std::condition_variable read_buffer_ready_cv;
            bool read_buffer_ready = false;

            std::thread write_thread(
              [&]
              {
                while(!quit)
                {
                  { // wait for generator thread to be done
                    std::unique_lock<std::mutex> lk(read_buffer_mutex);
                    read_buffer_ready_cv.wait(lk, [&read_buffer_ready]{ return read_buffer_ready; });
                  }

                  if (write(STDOUT_FILENO, read_buffer->data(), read_buffer->size() * sizeof(uint64_t)) < 0)
                  {
                    perror("<stdout>");
                    quit = true;
                  }

                  {
                    std::unique_lock<std::mutex> lk(read_buffer_mutex);
                    read_buffer_ready = false;
                  }
                  read_buffer_ready_cv.notify_one();
                }
              });

            while(!quit)
            {
              if (opts.ascii)
              {
                rnd_ascii_fill_buffer(*rnd,
                                      reinterpret_cast<uint8_t*>(write_buffer->data()),
                                      write_buffer->size() * sizeof(uint64_t));
              }
              else
              {
                rnd_fill_buffer(*rnd, write_buffer->data(), write_buffer->size());
              }

              { // wait for write thread to be done
                std::unique_lock<std::mutex> lk(read_buffer_mutex);
                read_buffer_ready_cv.wait(lk, [&read_buffer_ready]{ return !read_buffer_ready; });
              }

              {
                std::unique_lock<std::mutex> lk(read_buffer_mutex);
                std::swap(read_buffer, write_buffer);
                read_buffer_ready = true;
              }
              read_buffer_ready_cv.notify_one();
            }

            write_thread.join();
          });
      }

      for(auto&& t : threads)
      {
        t.join();
      }
    }
  }

  return 0;
}

/* EOF */
