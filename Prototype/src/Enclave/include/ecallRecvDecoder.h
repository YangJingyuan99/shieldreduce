/**
 * @file ecallRecvDecoder.h
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief define the interfaces of ecallRecvDecoder 
 * @version 0.1
 * @date 2023-03-01
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef ECALL_RECV_DECODER_H
#define ECALL_RECV_DECODER_H


#include "commonEnclave.h"
#include "ecallEnc.h"
#include "ecallLz4.h"



#include "../../../include/constVar.h"
#include "../../../include/chunkStructure.h"

#include "md5.h"
#include "util.h"
#include "xxhash.h"

extern "C"
{
#include "config.h"
#include "xdelta3.h"
}

// forward declaration
class EcallCrypto;

class EcallRecvDecoder {
    private:
        string myName_ = "EcallRecvDecoder";
        EcallCrypto* cryptoObj_;
        //InContainercache* InContainercache_;

        /**
         * @brief recover a chunk
         * 
         * @param chunkBuffer the chunk buffer
         * @param chunkSize the chunk size
         * @param restoreChunkBuf the restore chunk buffer
         * @param cipherCtx the pointer to the EVP cipher
         * 
         */
        void RecoverOneChunk(uint8_t* chunkBuffer, uint32_t chunkSize, 
            SendMsgBuffer_t* restoreChunkBuf, EVP_CIPHER_CTX* cipherCtx);
    public:
        int lz4_times = 0;
        uint64_t _restoretime = 0;
        uint64_t _deltarestoretime = 0;
        uint64_t _starttime;
        uint64_t _endtime;
        uint64_t _starttime1;
        uint64_t _endtime1;
        
   
        /**
         * @brief Construct a new EcallRecvDecoder object
         * 
         */
        EcallRecvDecoder();

        /**
         * @brief Destroy the Ecall Recv Decoder object
         * 
         */
        ~EcallRecvDecoder();

        /**
         * @brief process a batch of recipes and write chunk to the outside buffer
         * 
         * @param recipeBuffer the pointer to the recipe buffer
         * @param recipeNum the input recipe buffer
         * @param resOutSGX the pointer to the out-enclave var
         * 
         * @return size_t the size of the sended buffer
         */
        void ProcRecipeBatch(uint8_t* recipeBuffer, size_t recipeNum, 
            ResOutSGX_t* resOutSGX);

        /**
         * @brief process the tail batch of recipes
         * 
         * @param resOutSGX the pointer to the out-enclave var
         */
        void ProcRecipeTailBatch(ResOutSGX_t* resOutSGX);

        uint8_t* xd3_decode(const uint8_t *in, size_t in_size, const uint8_t *ref, size_t ref_size, size_t *res_size);

        int EDeltaDecode(uint8_t *deltaBuf, uint32_t deltaSize, uint8_t *baseBuf,
            uint32_t baseSize, uint8_t *outBuf, uint32_t *outSize);

        uint8_t *ed3_decode(uint8_t *in, size_t in_size, uint8_t *ref, size_t ref_size, size_t *res_size);

        u_int32_t get_flag(void *record);

        uint32_t get_length(void *record);

};

#endif