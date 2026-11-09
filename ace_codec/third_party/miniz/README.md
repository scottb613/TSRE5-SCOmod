# miniz

Deflate and inflate (zlib subset) and ZIP reading, used by TSRE's
compressed SIMIS files, ACE textures, terrain files, elevation and OSM
data; wrapped for archives by `src/mzip/MZip`.

- Version: 2.2.0
- Source: https://github.com/richgel999/miniz, release `2.2.0`
  (`miniz-2.2.0.zip`, SHA-256
  `e4aa5078999c7f7466fa6b8f9848e39ddfff9a4bafc50215764aebe1f13b3841`)
- Licence: MIT, see `LICENSE` (from the release); the source's comments
  also carry its older public domain (Unlicense) statement.

Not unchanged: `miniz.h` is the release's `miniz.h` and `miniz.c` in one
file, changed by TSRE (compared ignoring whitespace; the file was also
reformatted):

- `MINIZ_EXPORT` is `inline`, so the implementation can be included by
  many sources; those that want only the declarations define
  `MINIZ_HEADER_FILE_ONLY` first.
- `<stdio.h>` included at the top.
- Little-endian reads (`MZ_READ_LE16`, `MZ_READ_LE32`) through `memcpy`,
  without the unaligned pointer casts (undefined behaviour), for the
  bounded ACE inflater and the other readers.
- `fopen` and `freopen` instead of `fopen_s` and `freopen_s` in the MSVC
  and 64-bit MinGW branch.
- The 32-bit MinGW and Tiny C branches' file macros call `nowide::fopen`,
  `nowide::freopen` and `nowide::remove`, which TSRE does not define;
  TSRE never builds those branches.
- One comment's "library" reads "KZip".

Include it as `<miniz/miniz.h>`. To update, take a release's `miniz.h` and
`miniz.c`, redo the changes above and update this file.
