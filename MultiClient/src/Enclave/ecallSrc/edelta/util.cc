#include <cstdio>
#include <cstring>

#include "md5.h"
#include "util.h"
#include "xxhash.h"

uint64_t weakHash(unsigned char *buf, int len) {
  return XXH64(buf, len, 0x7fcaf1);
}

// jump by STRMIN bytes
/* different from rolling_gear_v2, where @n indicates the bytes left in @p that
 * can be chunked. And rolling_gear_v2 is abandoned.
 */
int rolling_gear_v3(unsigned char *p, int n, int num_of_chunks, int *cut) {
  uint32_t fingerprint = 0;
  int i = 0, count = 0;
  cut[count++] = 0;

  i += STRMIN + 1;

  int j = 0;
  while (j < num_of_chunks) {
    if (i < n) {
      fingerprint = (fingerprint << 1) + GEAR[p[i]];
      if (i <= (cut[count - 1] + STRMAX)) {
        if (!(fingerprint & STRAVG)) {
          /* this requires the last few bits of fingerprint to be all 0 */
          cut[count++] = i;
          i += STRMIN;
          fingerprint = 0;
          j++;
        }
      } else {
        cut[count++] = i;
        i += STRMIN;
        fingerprint = 0;
        j++;
      }
      i++;
    } else {
      while (j < num_of_chunks) {
        cut[count++] = n;
        j++;
      }

      break;
    }
  }

  return cut[count - 1];
}

// jump by STRMIN bytes
int chunk_gear(unsigned char *p, int n) {
  uint32_t fingerprint = 0;
  int i = STRMIN + 1;
  if (n <= STRMAX) {
    return n;
  }

  while (i <= n) {
    fingerprint = (fingerprint << 1) + GEAR[p[i]];
    if (i <= STRMAX) {
      if (!(fingerprint & STRAVG))
        return i;
    } else {
      return i;
    }
    i++;
  }
  return i;
}

#ifndef PREDEFINED_GEAR_MATRIX
uint32_t GEAR[256];

void PrintGearV2() {
  printf("uint32_t gear_matrix[256] = {\n");
  for (int i = 0; i < 255; ++i) {
    if (i % 3 == 0)
      printf("\n   ");
    printf("%0#8x, ", GEAR[i]);
  }
  printf("}\n");
}

void InitGearMatrix() {
  const uint32_t SymbolTypes = 256;
  const uint32_t MD5Length = 16;
  const int SeedLength = 32;

  char seed[SeedLength];
  for (int i = 0; i < SymbolTypes; i++) {
    for (int j = 0; j < SeedLength; j++) {
      seed[j] = i;
    }

    GEAR[i] = 0;
    char md5_result[MD5Length];
    md5_state_t md5_state;
    md5_init(&md5_state);
    md5_append(&md5_state, (md5_byte_t *)seed, SeedLength);
    md5_finish(&md5_state, (md5_byte_t *)md5_result);

    memcpy(&GEAR[i], md5_result, sizeof(uint32_t));
  }
  PrintGearV2();
}
#endif

uint32_t GEAR[256] = {
    0x4b8fbc70, 0x95a75be0, 0x7fa97617, 0xdb51a0d,  0x7c71d5b3, 0x97842403,
    0x87d60b89, 0x10c081cb, 0x176c2faf, 0xc7392648, 0x2f15cf70, 0x842062ac,
    0x7d19bc1b, 0xc9a22b6d, 0x29f65703, 0x54f0a470, 0x4913c078, 0x91dd2661,
    0xce401296, 0x1080796b, 0x3c6ba084, 0x291fd606, 0x1be96ae,  0x5e104df3,
    0x953a3946, 0xd59e7f77, 0x9f4734d9, 0xf095c27c, 0xe4448418, 0x47ca4676,
    0xe9131404, 0xc6965ee3, 0x2f46c05a, 0x8ba2c750, 0x5fb2fec6, 0xaae07db7,
    0xf96ad818, 0xaed74a8d, 0x3b73296d, 0x42b47c83, 0x3f14fa76, 0xe775583b,
    0x8583a649, 0xe347effd, 0x1814ed33, 0xd747a499, 0x1241b4c,  0xc2ba1de6,
    0x9e459ecd, 0xf2cb801a, 0xb1575ba0, 0x57230ea3, 0xfd6e88ca, 0x1d0501a2,
    0xf33ed508, 0x531f339c, 0xece73621, 0x2171ee60, 0xc545563a, 0x3b68b071,
    0x480657e4, 0x69b955c1, 0x954a24b1, 0xc458ee33, 0x48958571, 0xccdd1652,
    0xa67ad0f0, 0xa93e0890, 0x8255fe6,  0xc190ff58, 0x6d2966a,  0x1e3e1a20,
    0x1649fd23, 0xa5452efe, 0x247ce55,  0xf4220d84, 0xf817eb4b, 0xab0ff6bf,
    0xd98c80da, 0xbb660cad, 0xce39b4a0, 0xebfa6bfe, 0xc4baba1b, 0x5cfd0930,
    0x7cdba262, 0xab2c4cdb, 0xdd1858e7, 0x7aa35c54, 0x36c7793b, 0xc3298957,
    0x5212e006, 0x772a10b3, 0x4b412801, 0x6348ea44, 0x90b7a8c,  0xf3903817,
    0x55ec8d5,  0xd39bca5e, 0xa19e4f8b, 0xf8472700, 0xd792025e, 0xcfdc83e0,
    0x576880c7, 0xcbaa6558, 0x692f5350, 0x1400ee49, 0x3a89aa8b, 0x46873d99,
    0x63ecb853, 0x9860b122, 0x31685257, 0x32874e3d, 0x6b44e063, 0x87525cd0,
    0xf0a5025c, 0x6ccb4020, 0xf1d8849d, 0x414f66e7, 0x6afed5a4, 0x9a9befeb,
    0xd6e18fdc, 0x27129a4c, 0x7d6bcf28, 0x37fa944a, 0xe9b09b54, 0x9159093,
    0x4a31242,  0x28d0d7d2, 0x97e8d65a, 0xfcf68625, 0x1800702,  0x39dbe43f,
    0x35da0210, 0xa6c68d9,  0x68500f4e, 0xc8a5ab6e, 0x8b2b756a, 0xd5fb25bd,
    0xb0c0405d, 0xc453b18c, 0xe2e66ea4, 0xda553370, 0x7dbc5759, 0x886eeab,
    0x9fafd0e5, 0x4e430b,   0x8331c2e5, 0x5a5d06b4, 0xe43f7b9d, 0xf3ec3644,
    0x6a46d11,  0xcd41e503, 0xa18a2e19, 0xadde1ea1, 0x36f7488e, 0x455255bd,
    0x8075175e, 0x88d32d58, 0x9a0a25b0, 0x970e0c67, 0x77b009e,  0x12d5bf75,
    0xdc7fd18,  0xde77a2bb, 0x6ab176e0, 0xa6ab6c09, 0x6210efa8, 0xd947e049,
    0x5fb01605, 0xde4bd448, 0x5b2af7ba, 0xd2b49fe9, 0x1783f531, 0x66fffd39,
    0x59e8a9ba, 0xc2ee818b, 0x1f3b0f3c, 0x4cd0e620, 0xeaf278ba, 0x92045e3d,
    0x64e6ebca, 0x9b70573,  0x3aaf57a4, 0xe287e26,  0x54dcafbb, 0x8c25a8,
    0x99642381, 0x5c8de9f0, 0x51fda6c0, 0x910e0b6d, 0xf6328e12, 0xec5e5389,
    0x8ed195f,  0xe80f0583, 0xa7185a25, 0xa83c3268, 0xa4e79433, 0xe6def4b,
    0x77065b1f, 0x43678dc7, 0x7e506399, 0x88503f60, 0x62c4983d, 0x70ece12,
    0xe0d421a4, 0x71821295, 0x801310e8, 0x3bd7bf4b, 0xe42779be, 0xefee3449,
    0xd6aaaffc, 0xa5608bae, 0xf6e0b8ae, 0xdd79aa9f, 0x8bd6606d, 0x3b83977a,
    0x512a0c70, 0x2c6de88,  0x25f5ea97, 0x68ceb140, 0xdaa87097, 0x602b8497,
    0x7767467a, 0xfee216a8, 0x14512325, 0x142c4e04, 0x7bf5ac09, 0x384364d5,
    0xf58188a8, 0xa410598,  0x799223f,  0xc1b76b42, 0xd828d74b, 0x931ecbb1,
    0x41922ad3, 0x787fb489, 0xf41c1d00, 0x3dbc3f88, 0x12a70e39, 0x26ae363d,
    0x47f7274,  0x86385074, 0x2ffb7263, 0xb8e3de33, 0x9496a61,  0x92025809,
    0xbf8b296d, 0xf1a57003, 0xa8057fb6, 0x2ce2e565, 0x56d7a64a, 0xa6e30007,
    0xe0562996, 0xabec18bd, 0x6b8c68ed};