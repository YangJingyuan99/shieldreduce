#ifndef STORE_OCALL_H 
#define STORE_OCALL_H 

#include "ocallUtil.h"
#include "../../../include/storageCore.h"
#include "../../../include/clientVar.h"
#include "../../../include/absDatabase.h"
#include "../../../include/dataWriter.h"
#include "../../../include/enclaveRecvDecoder.h"
#include <vector>
#include <filesystem>
#include <sys/stat.h>

#include "sgx_urts.h"

// forward declaration
class EnclaveRecvDecoder;

/**
 * @brief define the variable use in Ocall
 * 
 */
namespace OutEnclave {
    // ocall for upload
    extern StorageCore* storageCoreObj_;
    extern DataWriter* dataWriterObj_;
    extern AbsDatabase* indexStoreObj_;

    // ocall for restore
    extern EnclaveRecvDecoder* enclaveRecvDecoderObj_;
    extern string myName_;

    // for persistence
    extern ofstream outSealedFile_;
    extern ifstream inSealedFile_;

    // rw lock for index
    extern pthread_rwlock_t outIdxLck_;
    
    /**
     * @brief setup the ocall var
     * 
     * @param dataWriterObj the pointer to the data writer
     * @param indexStoreObj the pointer to the index
     * @param storageCoreObj the pointer to the storageCoreObj
     * @param enclaveDecoderObj the pointer to the enclave recvDecoder
     */
    void Init(DataWriter* dataWriterObj, AbsDatabase* indexStoreObj,
        StorageCore* storageCoreObj, EnclaveRecvDecoder* enclaveRecvDecoderObj);

    /**
     * @brief destroy the ocall var
     * 
     */
    void Destroy();
};

/**
 * @brief exit the enclave with error message
 * 
 * @param error_msg 
 */
void Ocall_SGX_Exit_Error(const char* error_msg);

/**
 * @brief dump the inside container to the outside buffer
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_WriteContainer(void* outClient);

/**
 * @brief printf interface for Ocall
 * 
 * @param str input string 
 */
void Ocall_Printf(const char* str);

/**
 * @brief persist the buffer to file 
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_UpdateFileRecipe(void* outClient);

/**
 * @brief update the outside index store 
 * 
 * @param ret return result
 * @param key pointer to the key
 * @param keySize the key size
 * @param buffer pointer to the buffer
 * @param bufferSize the buffer size
 */
void Ocall_UpdateIndexStoreBuffer(bool* ret, const char* key, size_t keySize,
    const uint8_t* buffer, size_t bufferSize);


void Ocall_UpdateIndexStoreSF(bool* ret, const char* key, size_t keySize,
    const uint8_t* buffer, size_t bufferSize);

/**
 * @brief read the outside index store
 *
 * @param ret return result 
 * @param key pointer to the key
 * @param keySize the key size
 * @param retVal pointer to the buffer <return>
 * @param expectedRetValSize the expected buffer size <return>
 * @param outClient the out-enclave client ptr
 */
void Ocall_ReadIndexStore(bool* ret, const char* key, size_t keySize,
    uint8_t** retVal, size_t* expectedRetValSize, void* outClient);

/**
 * @brief write the data to the disk file
 * 
 * @param sealedFileName the name of the sealed file
 * @param sealedDataBuffer sealed data buffer
 * @param sealedDataSize sealed data size
 */
void Ocall_WriteSealedData(const char* sealedFileName, uint8_t* sealedDataBuffer, size_t sealedDataSize);

/**
 * @brief init the file output stream
 *
 * @param ret the return result 
 * @param sealedFileName the sealed file name
 */
void Ocall_InitWriteSealedFile(bool* ret, const char* sealedFileName);

/**
 * @brief close the file output stream
 * 
 * @param sealedFileName the sealed file name
 */
void Ocall_CloseWriteSealedFile(const char* sealedFileName);


/**
 * @brief read the sealed data from the file
 * 
 * @param sealedFileName the sealed file
 * @param dataBuffer the data buffer
 * @param sealedDataSize the size of sealed data
 */
void Ocall_ReadSealedData(const char* sealedFileName, uint8_t* dataBuffer, uint32_t sealedDataSize);

/**
 * @brief get current time from the outside
 * 
 */
void Ocall_GetCurrentTime(uint64_t* retTime);

/**
 * @brief Init the unseal file stream 
 *
 * @param fileSize the return file size
 * @param sealedFileName the sealed file name
 */
void Ocall_InitReadSealedFile(size_t* fileSize, const char* sealedFileName);
/**
 * @brief close the file input stream
 * 
 * @param sealedFileName the sealed file name
 */
void Ocall_CloseReadSealedFile(const char* sealedFileName);

/**
 * @brief Print the content of the buffer
 * 
 * @param buffer the input buffer
 * @param len the length in byte
 */
void Ocall_PrintfBinary(const uint8_t* buffer, size_t len);

/**
 * @brief Get the required container from the outside application
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_GetReqContainers(void* outClient);

/**
 * @brief send the restore chunks to the client
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_SendRestoreData(void* outClient);

/**
 * @brief query the outside deduplication index 
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_QueryOutIndex(void* outClient);

/**
 * @brief update the outside deduplication index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_UpdateOutIndex(void* outClient);

/**
 * @brief generate the UUID
 * 
 * @param id the uuid buffer
 * @param len the id len
 */
void Ocall_CreateUUID(uint8_t* id, size_t len);

/**
 * @brief get the single target recipe
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_OneRecipe(void* outClient);

/**
 * @brief get the single target container
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_OneContainer(void* outClient);

/**
 * @brief get the single target delta container
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_OneDeltaContainer(void* outClient);

/**
 * @brief get the single target cold container
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_OneColdContainer(void* outClient,bool* deltaFlag);

/**
 * @brief update SF index in offline phase
 * 
 * @param outClient the out-enclave client ptr
 * @param keySize 
 */
void Ocall_OFFline_updateIndex(void* outClient,size_t keySize);

/**
 * @brief save hot container to disk
 * 
 * @param containerID the out-enclave client ptr
 * @param containerBody the pointer to container buffer
 * @param currentSize the container size
 */
void Ocall_SavehotContainer(const char* containerID, uint8_t* containerBody, size_t currentSize);

/**
 * @brief save cold container to disk
 * 
 * @param containerID the out-enclave client ptr
 * @param containerBody the pointer to container buffer
 * @param currentSize the container size
 */
void Ocall_SaveColdContainer(const char* containerID, uint8_t* containerBody, size_t currentSize, bool* delta_flag);

/**
 * @brief get the single target cold container
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_QueryOutBasechunk(void* outClient);

/**
 * @brief get local index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_GetLocal( void* outClient);

/**
 * @brief insert local index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_LocalInsert(void* outClient, size_t chunkNum);

/**
 * @brief sort local index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_Localrevise(void* outClient);

/**
 * @brief query delta index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_QueryDeltaIndex(void* outClient);

/**
 * @brief update delta index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_UpdateDeltaIndex(void* outClient, size_t chunkNum);

/**
 * @brief insert cold pair
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_ColdInsert(void* outClient);

/**
 * @brief get cold index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_GetCold( void* outClient);

/**
 * @brief sort cold index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_Coldrevise(void* outClient);

/**
 * @brief free cold index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_CleanLocalIndex();

void Ocall_GetMergeContainer(void* outClient);

void Ocall_CleanMerge(void* outClient);

void Ocall_GetMergePair(void* outClient, uint8_t* containerID, uint32_t* size);

void Ocall_MergeContent(void* outClient, uint8_t* containerBody, size_t currentSize);

#endif //  ENC_OCALL_H!