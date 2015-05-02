randomstream
============

A simple utility that will print pseudo random data to stdout:

    $ randomstream
    'É5Û¼<ä>ºÚæÊú®¤ÌÑøH'F¬1ÄWÚ¥"8lì£7,ªËÆ%ç9t%¨¸...

By default the seed will be initialized with data from
`gettimeofday()`, if an argument is provided it will be used as seed:

    $ randomstream 42
    -�|��Not~:�e�T�����? G����C 8�VBc��<R�P�b�BlA...
