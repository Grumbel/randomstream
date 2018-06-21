randomstream
============

A simple utility that will print pseudo random data to stdout, similar
to what can be accomplished with `/dev/urandom`, however
`randomstream` is around four times faster:

    $ randomstream
    'É5Û¼<ä>ºÚæÊú®¤ÌÑøH'F¬1ÄWÚ¥"8lì£7,ªËÆ%ç9t%¨¸...


Usage
-----

    $ randomstream --help
    Usage: randomstream [OPTION]...

    Options:
      -h, --help              Display this help text
      --version               Display version number
      -a, --algorithm ALG     Generate random numbers with ALG (default: xorshift96)
      -A, --ascii             Limit output to printable ASCII characters
      -s, --seed SEED         Use SEED as uint64 seed value,
                              'time' for time of day seed (default: 0)

    Algorithms:
      xorshift96   XORShift96 Algorithm
      xorshift64   XORSHIFT64 Algorithm
      zero         Output 0s
      const        Output the seed value repeatedly
