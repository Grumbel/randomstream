// randomstream - stream of pseudo random numbers
// Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
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

#include <array>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

constexpr size_t BUFFERSIZE = 1024 * 1024 / sizeof(uint64_t);

class XORShift96
{
private:
  uint64_t x;
  uint64_t y = 362436069;
  uint64_t z = 521288629;

public:
  XORShift96(uint64_t seed = 123456789) :
    x(seed)
  {}

  uint64_t operator()()
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

class XORShift64
{
private:
  uint64_t x;

public:
  XORShift64(uint64_t seed = 123456789) :
    x(seed)
  {}

  uint64_t operator()()
  {
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;

    return x * UINT64_C(2685821657736338717);
  }
};

int main(int argc, char** argv)
{
  if (argc != 1)
  {
    puts("Usage: randomstream");
    return 1;
  }
  else
  {
    // 530MiB/s on Intel Core Duo E6300 1.86Ghz
    auto rnd = XORShift96(time(NULL));

    // 450MiB/s on Intel Core Duo E6300 1.86Ghz
    //auto rnd = XORShift64(time(NULL));

    std::array<uint64_t , BUFFERSIZE> buffer;

    bool quit = false;
    while(!quit)
    {
      for(auto&& v : buffer)
      {
        v = rnd();
      }

      if (write(STDOUT_FILENO, buffer.data(), buffer.size() * sizeof(uint64_t)) < 0)
      {
        perror("<stdout>");
        quit = true;
      }
    }

    return 0;
  }
}

/* EOF */
