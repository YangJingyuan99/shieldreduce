/**
 * @file enclaveBase.h
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief define the interface of enclave base
 * @version 0.1
 * @date 2023-12-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */


#ifndef ENCLAVE_BASE_H
#define ENCLAVE_BASE_H

#include "ecallOffline.h"
#include "commonEnclave.h"
#include "ecallEnc.h"
#include "ecallStorage.h"
#include "ecallLz4.h"

#define ENABLE_SEALING 1
static const double SEC_TO_USEC = 1000 * 1000;

class EnclaveClient;

class EnclaveBase {
    protected:
        string myName_ = "EnclaveBase";

        // storage core pointer
        EcallStorageCore* storageCoreObj_;

        // crypto obj inside the enclave 
        EcallCrypto* cryptoObj_;

        // for the limitation 
        uint64_t maxSegmentChunkNum_ = 0;

        /**
         * @brief identify whether it is the end of a segment
         * 
         * @param chunkHashVal the input chunk hash
         * @param chunkSize the input chunk size
         * @param segment the reference to current segment
         * @return true is the end
         * @return false is not the end 
         */
        bool IsEndOfSegment(uint32_t chunkHashVal, uint32_t chunkSize, Segment_t* segment);

        /**
         * @brief convert hash to a value
         * 
         * @param inputHash the input chunk hash
         * @return uint32_t the returned value
         */
        uint32_t ConvertHashToValue(const uint8_t* inputHash);

        /**
         * @brief update the file recipe
         * 
         * @param chunkAddrStr the chunk address string
         * @param inRecipe the in-enclave recipe buffer
         * @param upOutSGX the upload out-enclave var
         */
        void UpdateFileRecipe(string& chunkAddrStr, Recipe_t* inRecipe, UpOutSGX_t* upOutSGX, uint8_t *FP);

        /**
         * @brief process an unique chunk
         * 
         * @param chunkAddr the chunk address
         * @param chunkBuffer the chunk buffer
         * @param chunkSize the chunk size
         * @param upOutSGX the upload out-enclave var
         */
        void ProcessUniqueChunk(RecipeEntry_t* chunkAddr, uint8_t* chunkBuffer, 
            uint32_t chunkSize, UpOutSGX_t* upOutSGX);

        void ProcessSheUniqueChunk(RecipeEntry_t* chunkAddr, uint8_t* chunkBuffer, uint32_t chunkSize, UpOutSGX_t* upOutSGX, uint8_t* chunksf, uint8_t* chunkfp);

        /**
         * @brief update the index store
         * 
         * @param key the key of the k-v pair 
         * @param buffer the data buffer 
         * @param bufferSize the size of the buffer
         * @return true success
         * @return false fail
         */
        bool UpdateIndexStore(const string& key, const char* buffer, 
            size_t bufferSize);

        bool UpdateIndexSF(const string& key, const char* buffer, 
            size_t bufferSize);

        /**
         * @brief read the information from the index store
         * 
         * @param key key 
         * @param value value
         * @param upOutSGX the upload out-enclave var
         * @return true 
         * @return false 
         */
        bool ReadIndexStore(const string& key, string& value,
            UpOutSGX_t* upOutSGX);

        /**
         * @brief reset the value of current segment
         * 
         * @param sgxClient the to the current client
         */
        void ResetCurrentSegment(EnclaveClient* sgxClient);

        /**
         * @brief Get the Time Differ object
         * 
         * @param sTime the start time
         * @param eTime the end time
         * @return double the diff of time
         */
        double GetTimeDiffer(uint64_t sTime, uint64_t eTime);

    public:
        OFFLineBackward* offlinebackOBj_;
        bool Forward_Flag = false;
        bool Offline_Flag = true;

        // for statistic 
        uint64_t _logicalChunkNum = 0;
        uint64_t _logicalDataSize = 0;
        uint64_t _uniqueChunkNum = 0;
        uint64_t _uniqueDataSize = 0;
        long long _baseChunkNum = 0;
        uint64_t _baseDataSize = 0;
        uint64_t _deltaChunkNum = 0;
        uint64_t _deltaDataSize = 0;
        long long _lz4SaveSize = 0;
        uint64_t _DeltaSaveSize = 0;
        uint64_t _Online_DeltaSaveSize = 0;
        uint64_t _Inline_Ocall = 0;
        uint64_t _Inline_FPOcall = 0;
        uint64_t _Inline_SFOcall = 0;
        uint64_t _Inline_LocalOcall = 0;
        uint64_t _Inline_LoadOcall = 0;
        uint64_t _Inline_DeltaOcall = 0;
        uint64_t _Inline_RecipeOcall = 0;
        uint64_t _compressedDataSize = 0;
        uint64_t _onlineCompress_size = 0;
        uint64_t _onlineBackupSize = 0;

#if(EDR_BREAKDOWN == 1)
    uint64_t _startTime;
    uint64_t _endTime;
    uint64_t _dataTransTime = 0;
    uint64_t _dataTransCount= 0;
    uint64_t _fingerprintTime = 0;
    uint64_t _fingerprintCount = 0;
    uint64_t _freqTime = 0;
    uint64_t _freqCount = 0;
    uint64_t _superfeatureTime = 0;
    uint64_t _superfeatureCount = 0;
    uint64_t _firstDedupTime = 0;
    uint64_t _firstDedupCount = 0;
    uint64_t _secondDedupTime = 0;
    uint64_t _secondDedupCount = 0;
    uint64_t _localcheckerTime = 0;
    uint64_t _localcheckerCount = 0;
    uint64_t _deltacompressTime = 0;
    uint64_t _deltacompressCount = 0;
    uint64_t _lz4compressTime = 0;
    uint64_t _lz4compressCount = 0;
    uint64_t _encryptTime = 0;
    uint64_t _encryptCount = 0;
    uint64_t _testOCallTime = 0;
    uint64_t _testOCallCount = 0;
#endif

        /**
         * @brief Construct a new EnclaveBase object
         * 
         */
        EnclaveBase();

        /**
         * @brief Destroy the Enclave Base object
         * 
         */
        virtual ~EnclaveBase();

        /**
         * @brief process one batch
         * 
         * @param recvChunkBuf the recv chunk buffer
         * @param upOutSGX the pointer to enclave-related var 
         */
        virtual void ProcessOneBatch(SendMsgBuffer_t* recvChunkBuf, 
            UpOutSGX_t* upOutSGX) = 0;

        /**
         * @brief process the tailed batch when received the end of the recipe flag
         * 
         * @param upOutSGX the pointer to enclave-related var
         */
        virtual void ProcessTailBatch(UpOutSGX_t* upOutSGX) = 0;


        virtual void ProcessOffline(SendMsgBuffer_t* recvChunkBuf,UpOutSGX_t* upOutSGX) = 0;

        
};
#endif