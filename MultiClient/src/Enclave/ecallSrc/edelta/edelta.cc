#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "edelta.h"
#include "util.h"

/* flag=0 for 'D', 1 for 'S' */
void set_flag(void *record, uint32_t flag) {
  uint32_t *flag_length = (uint32_t *)record;
  if (flag == 0) {
    (*flag_length) &= ~(uint32_t)0 >> 1;
  } else {
    (*flag_length) |= (uint32_t)1 << 31;
  }
}

/* return 0 if flag=0, >0(not 1) if flag=1 */
u_int32_t get_flag(void *record) {
  u_int32_t *flag_length = (u_int32_t *)record;
  return (*flag_length) & (u_int32_t)1 << 31;
}

void set_length(void *record, uint32_t length) {
  uint32_t *flag_length = (uint32_t *)record;
  uint32_t musk = (*flag_length) & (uint32_t)1 << 31;
  *flag_length = length | musk;
}

uint32_t get_length(void *record) {
  uint32_t *flag_length = (uint32_t *)record;
  return (*flag_length) & ~(uint32_t)0 >> 1;
}

int Chunking_v3(unsigned char *data, int len, int num_of_chunks,
                DeltaRecord *subChunkLink) {
  int i = 0, *cut;
  /* cut is the chunking points in the stream */
  cut = (int *)malloc((num_of_chunks + 1) * sizeof(int));
  int numBytes =
      rolling_gear_v3(data, len, num_of_chunks, cut); //分割给定快的总字节数

  while (i < num_of_chunks) {
    int chunkLen = cut[i + 1] - cut[i];
    subChunkLink[i].nLength = chunkLen;
    subChunkLink[i].nOffset = cut[i]; /**/
    subChunkLink[i].DupFlag = 0;
    subChunkLink[i].nHash = weakHash(data + cut[i], chunkLen);
    //	SpookyHash::Hash64(data+ cut[i], chunkLen, 0x1af1);
    i++;
  }
  free(cut);
  return numBytes;
}

int EDeltaEncode(uint8_t *newBuf, uint32_t newSize, uint8_t *baseBuf,
                 uint32_t baseSize, uint8_t *deltaBuf, uint32_t *deltaSize) {
  /* detect the head and tail of one chunk */
  uint32_t beg = 0, end = 0, begSize = 0, endSize = 0;
  float matchsum = 0;
  float match = 0;
  while (begSize + 7 < baseSize && begSize + 7 < newSize) {
    if (*(uint64_t *)(baseBuf + begSize) == *(uint64_t *)(newBuf + begSize)) {
      begSize += 8;
    } else
      break;
  }
  while (begSize < baseSize && begSize < newSize) {
    if (baseBuf[begSize] == newBuf[begSize]) {
      begSize++;
    } else
      break;
  }

  if (begSize > 16)
    beg = 1;
  else
    begSize = 0;

  while (endSize + 7 < baseSize && endSize + 7 < newSize) {
    if (*(uint64_t *)(baseBuf + baseSize - endSize - 8) ==
        *(uint64_t *)(newBuf + newSize - endSize - 8)) {
      endSize += 8;
    } else
      break;
  }
  while (endSize < baseSize && endSize < newSize) {
    if (baseBuf[baseSize - endSize - 1] == newBuf[newSize - endSize - 1]) {
      endSize++;
    } else
      break;
  }

  if (begSize + endSize > newSize)
    endSize = newSize - begSize;

  if (endSize > 16)
    end = 1;
  else
    endSize = 0;
  /* end of detect */

  if (begSize + endSize >= baseSize) {
    DeltaUnit1 record1;
    DeltaUnit2 record2;
    uint32_t deltaLen = 0;
    if (beg) {
      set_flag(&record1, 0);
      record1.nOffset = 0;
      set_length(&record1, begSize);
      memcpy(deltaBuf + deltaLen, &record1, sizeof(DeltaUnit1));
      deltaLen += sizeof(DeltaUnit1);
    }
    if (newSize - begSize - endSize > 0) {
      set_flag(&record2, 1);
      set_length(&record2, newSize - begSize - endSize);
      memcpy(deltaBuf + deltaLen, &record2, sizeof(DeltaUnit2));
      deltaLen += sizeof(DeltaUnit2);
      memcpy(deltaBuf + deltaLen, newBuf + begSize, get_length(&record2));
      deltaLen += get_length(&record2);
    }
    if (end) {
      set_flag(&record1, 0);
      record1.nOffset = baseSize - endSize;
      set_length(&record1, endSize);
      memcpy(deltaBuf + deltaLen, &record1, sizeof(DeltaUnit1));
      deltaLen += sizeof(DeltaUnit1);
    }

    *deltaSize = deltaLen;
    return deltaLen;
  }

  uint32_t deltaLen = 0;
  uint32_t cursor_base = begSize;
  uint32_t cursor_input = begSize;
  uint32_t cursor_input1 = 0;
  uint32_t cursor_input2 = 0;
  uint32_t input_last_chunk_beg = begSize;
  uint32_t inputPos = begSize;
  uint32_t length;
  uint64_t hash;
  DeltaRecord *psDupSubCnk = NULL;
  DeltaUnit1 record1{0, 0};
  DeltaUnit2 record2{0};
  set_flag(&record1, 0);
  set_flag(&record2, 1);
  int flag = 0; /* to represent the last record in the deltaBuf,
       1 for DeltaUnit1, 2 for DeltaUnit2 */

  int numBase = 0;  /* the total number of chunks that the base has chunked */
  int numBytes = 0; /* the number of bytes that the base chunks once */
  DeltaRecord *BaseLink = (DeltaRecord *)malloc(
      sizeof(DeltaRecord) * ((baseSize - begSize - endSize) / STRMIN + 50));

  int offset = (char *)&BaseLink[0].psNextSubCnk - (char *)&BaseLink[0];
  htable *psHTable = (htable *)malloc(sizeof(htable));
  // psHTable->init(offset, 8, baseSize/16);
#if POST_DEDUP
  psHTable->init(offset, 8, (baseSize - begSize - endSize) * 2 / (STRAVG + 1));
#else
  psHTable->init(offset, 8, 8 * 1024);
#endif

  // int chunk_length;
  int flag_chunk = 1; // to tell if basefile has been chunked to the end
  int numBytes_old;
  int numBytes_accu = 0; // accumulated numBytes in one turn of chunking the
                         // base
  int probe_match; // to tell which chunk for probing matches the some base
  int flag_handle_probe; // to tell whether the probe chunks need to be handled

  if (beg) {
    record1.nOffset = 0;
    set_length(&record1, begSize);
    memcpy(deltaBuf + deltaLen, &record1, sizeof(DeltaUnit1));
    deltaLen += sizeof(DeltaUnit1);
    flag = 1;
  }

#define BASE_BEGIN 5
#define BASE_EXPAND 3
#define BASE_STEP 3
#define INPUT_TRY 5

/* if deltaLen > newSize * RECOMPRESS_THRESHOLD in the first round,we'll
 * go back to greedy.
 */
#define GO_BACK_TO_GREEDY 0

#define RECOMPRESS_THRESHOLD 0.2

  DeltaRecord InputLink[INPUT_TRY];
  // int test=0;

  while (inputPos < newSize - endSize) {
    if (flag_chunk) {
      //		if( cursor_input - input_last_chunk_beg >= numBytes_accu
      //* 0.8 ){
      if ((cursor_input / (float)newSize) + 0.5 >=
          (cursor_base / (float)baseSize)) {
        numBytes_old = numBytes_accu;
        numBytes_accu = 0;

        /* chunk a few chunks in the input first */
        // Chunking_v3(newBuf+cursor_input, newSize-endSize-cursor_input,
        // INPUT_TRY, InputLink);
        flag_handle_probe = 1;
        probe_match = INPUT_TRY;
        int chunk_number = BASE_BEGIN;
        for (int i = 0; i < BASE_STEP; i++) {
          numBytes = Chunking_v3(
              baseBuf + cursor_base, baseSize - endSize - cursor_base,
              chunk_number,
              BaseLink + numBase); //一个分块base的循环找到 match的就可以跳出

          for (int j = 0; j < chunk_number; j++) {
            if (BaseLink[numBase + j].nLength == 0) {
              flag_chunk = 0;
              break;
            }
            BaseLink[numBase + j].nOffset += cursor_base;
            psHTable->insert((unsigned char *)&BaseLink[numBase + j].nHash,
                             &BaseLink[numBase + j]);
          }

          cursor_base += numBytes;
          numBase += chunk_number;
          numBytes_accu += numBytes;

          chunk_number *= BASE_EXPAND;
          if (i == 0) {
            cursor_input1 = cursor_input;
            for (int j = 0; j < INPUT_TRY; j++) {
              cursor_input2 = cursor_input1;
              cursor_input1 = chunk_gear(newBuf + cursor_input2,
                                         newSize - cursor_input2 - endSize) +
                              cursor_input2;
              InputLink[j].nLength = cursor_input1 - cursor_input2;
              InputLink[j].nHash =
                  weakHash(newBuf + cursor_input2, InputLink[j].nLength);
              if ((psDupSubCnk = (DeltaRecord *)psHTable->lookup(
                       (unsigned char *)&(InputLink[j].nHash)))) {
                probe_match = j;
                goto lets_break;
              }
            }
          } else {
            for (int j = 0; j < INPUT_TRY; j++) {
              if ((psDupSubCnk = (DeltaRecord *)psHTable->lookup(
                       (unsigned char *)&(InputLink[j].nHash)))) {
                //printf("find INPUT_TRY: %d BASE_STEP: %d" 
								//	" cursor_input: %d round of chunk: %d\n",
                //	j,i,cursor_input,test);
                probe_match = j;
                goto lets_break;
              }
            }
          }

          if (flag_chunk == 0)
          lets_break:
            break;
        }

        input_last_chunk_beg =
            (cursor_input > (input_last_chunk_beg + numBytes_old)
                 ? cursor_input
                 : (input_last_chunk_beg + numBytes_old));
        // test++;
      }
    }

    // to handle the chunks in input file for probing
    if (flag_handle_probe) {
      for (int i = 0; i < INPUT_TRY; i++) {
        matchsum++;
        length = InputLink[i].nLength;
        cursor_input = length + inputPos;

        if (i == probe_match) {
          flag_handle_probe = 0;
          goto match;
        } else {
          if (flag == 2) { //把不match的块弄过去
            /* continuous unique chunks only add unique bytes into the deltaBuf,
             * but not change the last DeltaUnit2, so the DeltaUnit2 should be
             * overwritten when flag=1 or at the end of the loop.
             */
            memcpy(deltaBuf + deltaLen, newBuf + inputPos, length);
            deltaLen += length;
            set_length(&record2, get_length(&record2) + length);
          } else {
            set_length(&record2, length);

            memcpy(deltaBuf + deltaLen, &record2, sizeof(DeltaUnit2));
            deltaLen += sizeof(DeltaUnit2);

            memcpy(deltaBuf + deltaLen, newBuf + inputPos, length);
            deltaLen += length;

            flag = 2;
          }

          inputPos = cursor_input;
        }
      }

      flag_handle_probe = 0;
    }

    cursor_input =
        chunk_gear(newBuf + inputPos, newSize - inputPos - endSize) + inputPos;
    matchsum++;
    length = cursor_input - inputPos;
    hash = weakHash(newBuf + inputPos, length);

    /* lookup */
    if ((psDupSubCnk =
             (DeltaRecord *)psHTable->lookup((unsigned char *)&hash))) {
    // printf("inputPos: %d length: %d\n", inputPos, length);
    match:
      if (length == psDupSubCnk->nLength &&
          memcmp(newBuf + inputPos, baseBuf + psDupSubCnk->nOffset, length) ==
              0) {
        //	printf("match:%d\n",length);
        match++;
        if (flag == 2) {
          /* continuous unique chunks only add unique bytes into the deltaBuf,
           * but not change the last DeltaUnit2, so the DeltaUnit2 should be
           * overwritten when flag=1 or at the end of the loop.
           */
          memcpy(deltaBuf + deltaLen - get_length(&record2) -
                     sizeof(DeltaUnit2),
                 &record2, sizeof(DeltaUnit2));
        }

        // greedily detect forward
        int j = 0;

        while (psDupSubCnk->nOffset + length + j + 7 < baseSize - endSize &&
               cursor_input + j + 7 < newSize - endSize) {
          if (*(uint64_t *)(baseBuf + psDupSubCnk->nOffset + length + j) ==
              *(uint64_t *)(newBuf + cursor_input + j)) {
            j += 8;
          } else
            break;
        }
        while (psDupSubCnk->nOffset + length + j < baseSize - endSize &&
               cursor_input + j < newSize - endSize) {
          if (baseBuf[psDupSubCnk->nOffset + length + j] ==
              newBuf[cursor_input + j]) {
            j++;
          } else
            break;
        }

        cursor_input += j;
        if (psDupSubCnk->nOffset + length + j > cursor_base)
          cursor_base = psDupSubCnk->nOffset + length + j;

        set_length(&record1, cursor_input - inputPos);
        record1.nOffset = psDupSubCnk->nOffset;

        /* detect backward */
        uint32_t k = 0;
        if (flag == 2) {
          while (k + 1 <= psDupSubCnk->nOffset &&
                 k + 1 <= get_length(&record2)) {
            if (baseBuf[psDupSubCnk->nOffset - (k + 1)] ==
                newBuf[inputPos - (k + 1)])
              k++;
            else
              break;
          }
        }
        if (k > 0) {
          deltaLen -= get_length(&record2);
          deltaLen -= sizeof(DeltaUnit2);

          set_length(&record2, get_length(&record2) - k);

          if (get_length(&record2) > 0) {
            memcpy(deltaBuf + deltaLen, &record2, sizeof(DeltaUnit2));
            deltaLen += sizeof(DeltaUnit2);
            deltaLen += get_length(&record2);
          }

          set_length(&record1, get_length(&record1) + k);
          record1.nOffset -= k;
        }

        memcpy(deltaBuf + deltaLen, &record1, sizeof(DeltaUnit1));
        deltaLen += sizeof(DeltaUnit1);
        flag = 1;
      } else {
        // printf("Spooky Hash Error!!!!!!!!!!!!!!!!!!\n");
        goto handle_hash_error;
      }
    } else {
    handle_hash_error:
      //	printf("unmatch:%d\n",length);
      if (flag == 2) {
        /* continuous unique chunks only add unique bytes into the deltaBuf,
         * but not change the last DeltaUnit2, so the DeltaUnit2 should be
         * overwritten when flag=1 or at the end of the loop.
         */
        memcpy(deltaBuf + deltaLen, newBuf + inputPos, length);
        deltaLen += length;
        set_length(&record2, get_length(&record2) + length);
      } else {
        set_length(&record2, length);

        memcpy(deltaBuf + deltaLen, &record2, sizeof(DeltaUnit2));
        deltaLen += sizeof(DeltaUnit2);

        memcpy(deltaBuf + deltaLen, newBuf + inputPos, length);
        deltaLen += length;

        flag = 2;
      }
    }

    inputPos = cursor_input;
    // printf("cursor_input:%d\n",inputPos);
  }

  if (flag == 2) {
    /* continuous unique chunks only add unique bytes into the deltaBuf,
     * but not change the last DeltaUnit2, so the DeltaUnit2 should be
     * overwritten when flag=1 or at the end of the loop.
     */
    memcpy(deltaBuf + deltaLen - get_length(&record2) - sizeof(DeltaUnit2),
           &record2, sizeof(DeltaUnit2));
  }

  if (end) {
    record1.nOffset = baseSize - endSize;
    set_length(&record1, endSize);
    memcpy(deltaBuf + deltaLen, &record1, sizeof(DeltaUnit1));
    deltaLen += sizeof(DeltaUnit1);
  }

  if (psHTable) {
    free(psHTable->table);
    free(psHTable);
  }

  free(BaseLink);

  *deltaSize = deltaLen;
  return deltaLen;
}

int EDeltaDecode(uint8_t *deltaBuf, uint32_t deltaSize, uint8_t *baseBuf,
                 uint32_t baseSize, uint8_t *outBuf, uint32_t *outSize) {

  uint32_t dataLength = 0, readLength = 0;
  int matchnum = 0;
  // int matchlength = 0;
  // int unmatchlength = 0;
  int unmatchnum = 0;
  while (1) {
    u_int32_t flag = get_flag(deltaBuf + readLength);

    if (flag == 0) {
      matchnum++;
      DeltaUnit1 record;
      memcpy(&record, deltaBuf + readLength, sizeof(DeltaUnit1));
      readLength += sizeof(DeltaUnit1);
      // matchlength += get_length(&record);
      memcpy(outBuf + dataLength, baseBuf + record.nOffset,
             get_length(&record));

      dataLength += get_length(&record);
    } else {
      unmatchnum++;
      DeltaUnit2 record;
      memcpy(&record, deltaBuf + readLength, sizeof(DeltaUnit2));
      readLength += sizeof(DeltaUnit2);
      // unmatchlength += get_length(&record);
      memcpy(outBuf + dataLength, deltaBuf + readLength, get_length(&record));

      readLength += get_length(&record);
      dataLength += get_length(&record);
    }

    if (readLength >= deltaSize) {
      break;
    }
  }
  *outSize = dataLength;
  return dataLength;
}