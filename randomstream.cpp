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

constexpr size_t BUFFERSIZE = 4 * 1024 * 1024 / sizeof(uint64_t);

class RndGenerator
{
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
    "  -a, --algorithm ALG     Generate random numbers with ALG (default: xorshift96)\n"
    "  -s, --seed SEED         Use SEED as uint64 seed value (default: time)\n"
    "\n"
    "Algorithms:\n"
    "  xorshift96   XORShift96 Algorithm\n"
    "  xorshift64   XORSHIFT64 Algorithm\n"
    "  zero         Output 0s\n"
    "  const        Output the seed value repeatedly\n";
  std::cout << std::flush;
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
    seed(time_seed()),
    count(0)
  {}

  AlgorithmType algorithm;
  uint64_t seed;
  uint64_t count;
};

Options parse_args(int argc, char** argv)
{
  Options opts;

  for(int i = 1; i < argc; ++i)
  {
    auto opt = [&]{ return argv[i]; };
    auto arg = [&]{
      if (i + 1 > argc)
      {
        std::cerr << "error: " << opt() << " requires an argument" << std::endl;
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
    else if (strcmp(opt(), "--algorithm") == 0 ||
        strcmp(opt(), "-a") == 0)
    {
      opts.algorithm = string_to_algorithm(arg());
      skip_arg();
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
        std::cout << arg() << " -> " << opts.seed;
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

int main(int argc, char** argv)
{
  Options opts = parse_args(argc, argv);

  auto rnd = create_rnd(opts.algorithm, opts.seed);

  std::array<uint64_t, BUFFERSIZE> buffer;

  // FIXME: this is a bit of a mess
  if (opts.count != 0)
  {
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
    if (rnd->is_const())
    {
      // fill it with initial values
      for(auto&& v : buffer)
      {
        v = (*rnd)();
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
      bool quit = false;
      while(!quit)
      {
        for(auto&& v : buffer)
        {
          v = (*rnd)();
        }

        if (write(STDOUT_FILENO, buffer.data(), buffer.size() * sizeof(uint64_t)) < 0)
        {
          perror("<stdout>");
          quit = true;
        }
      }
    }
  }

  return 0;
}

/* EOF */
