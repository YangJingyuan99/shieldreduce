/**
 * @file ecallClient.cc
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief implement enclave client class
 * @version 0.1
 * @date 2023-07-07
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/ecallClient.h"

/**
 * @brief Construct a new Enclave Client object
 * 
 * @param clientID client ID
 * @param indexType index type
 * @param optType the operation type (upload / download)
 */
EnclaveClient::EnclaveClient(uint32_t clientID, int indexType, int optType) {
    // store the parameters
    _clientID = clientID;
    indexType_ = indexType;
    optType_ = optType;

    // init the ctx
    _cipherCtx = EVP_CIPHER_CTX_new();
    _mdCtx = EVP_MD_CTX_new();

    // get a random iv
    sgx_read_rand(_iv, CRYPTO_BLOCK_SIZE); 

    // init the buffer according to the 
    switch (optType_) {
        case UPLOAD_OPT: {
            this->InitUploadBuffer();
            break;
        }
        case DOWNLOAD_OPT: {
            this->InitRestoreBuffer();
            break;
        }
        default: {
            Ocall_SGX_Exit_Error("wrong init operation type");
        }
    }
}

/**
 * @brief Destroy the Enclave Client object
 * 
 */
EnclaveClient::~EnclaveClient() {
    switch (optType_) {
        case UPLOAD_OPT: {
            this->DestroyUploadBuffer();
            break;
        }
        case DOWNLOAD_OPT: {
            this->DestroyRestoreBuffer();
            break;
        }
        default: {
            Ocall_SGX_Exit_Error("EnclaveClient: wrong destroy operation type");
        }
    }
    EVP_CIPHER_CTX_free(_cipherCtx);
    EVP_MD_CTX_free(_mdCtx);
}

/**
 * @brief init the buffer used in the upload
 * 
 */
void EnclaveClient::InitUploadBuffer() {
    _recvBuffer = (uint8_t*) malloc(Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    _inRecipe.entryFpList = (uint8_t*) malloc(Enclave::sendRecipeBatchSize_ *
        CHUNK_HASH_SIZE);
    _inRecipe.recipeNum = 0;

    _inQueryBase = (InQueryEntry_t*) malloc(Enclave::sendChunkBatchSize_ * 
        sizeof(InQueryEntry_t));
    //_localIndex.reserve(Enclave::sendChunkBatchSize_);
    _inContainer.buf = (uint8_t*) malloc(MAX_CONTAINER_SIZE * sizeof(uint8_t));
    _inContainer.curSize = 0;
    _deltainContainer.buf = (uint8_t*) malloc(MAX_CONTAINER_SIZE * sizeof(uint8_t));
    _deltainContainer.curSize = 0;

    BaseLink_ = (DeltaRecord *)malloc(
        sizeof(DeltaRecord) * (MAX_CHUNK_SIZE / STRMIN + 50));
    cutEdelta_ = (int*)malloc(1024 * sizeof(int));
    encBaseBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE * 2);
    decBaseBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE * 2);
    plainBaseBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE * 2);
    ivBuffer_ = (uint8_t*)malloc(CRYPTO_BLOCK_SIZE * 2);
    deltaBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE * 2);
    psHTable2_ = (htable *)malloc(sizeof(htable));
    psHTable2_->init(0, 8, 16 * 1024);

    // for edelta
    BaseLinkOffline_ = (DeltaRecord *)malloc(
        sizeof(DeltaRecord) * (MAX_CHUNK_SIZE / STRMIN + 50));
    cutEdeltaOffline_ = (int*)malloc(1024 * sizeof(int));
    psHTable2Offline_ = (htable *)malloc(sizeof(htable));
    psHTable2Offline_->init(0, 8, 16 * 1024);

    // for easy update
    oldRecipe_ = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t));
    newRecipe_ = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t));
    deltaRecipe_ = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t));
    oldBasechunkSf_ = (uint8_t*)malloc(3*CHUNK_HASH_SIZE);
    newBasechunkSf_ = (uint8_t*)malloc(3*CHUNK_HASH_SIZE);
    
    offline_plainOldUniqueBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);

    // for GetNew_deltachunk()
    offline_tmpUniqueBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_plainNewDeltaChunkBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_newDeltaChunkEnc_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_oldDeltaChunkDec_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_oldDeltaChunkEnc_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);

    // for merge container and update cold container
    offline_mergeNewContainer_ = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    offline_mergeRecipeEnc_ = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t));
    offline_mergeRecipeDec_ = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t));

    // for offline process delta compression
    offline_oldChunkDecrypt_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_newChunkDecrypt_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_oldChunkDecompression_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_newChunkDecompression_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_encOldChunkBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);
    offline_encNewChunkBuffer_ = (uint8_t*)malloc(MAX_CHUNK_SIZE);

    // for offline delta chunk
    offline_deltaSFBuffer_ = (uint8_t*)malloc(3 * CHUNK_HASH_SIZE);
    offline_deltaIVBuffer_ = (uint8_t*)malloc(CRYPTO_BLOCK_SIZE);
    offline_oldIVBuffer_ = (uint8_t*)malloc(CRYPTO_BLOCK_SIZE);
    offline_newIVBuffer_ = (uint8_t*)malloc(CRYPTO_BLOCK_SIZE);
    offline_deltaFPBuffer_ = (uint8_t*)malloc(CHUNK_HASH_SIZE);
    offline_outRecipeBuffer_ = (uint8_t*)malloc(sizeof(RecipeEntry_t));

    return ;
}

/**
 * @brief destroy the buffer used in the upload
 * 
 */
void EnclaveClient::DestroyUploadBuffer() {
    free(_recvBuffer);
    free(_inRecipe.entryFpList);
    free(_inQueryBase);
    free(_inContainer.buf);
    free(_deltainContainer.buf);
    free(BaseLink_); 
    free(cutEdelta_);
    free(encBaseBuffer_);
    free(decBaseBuffer_);
    free(plainBaseBuffer_);
    free(ivBuffer_);
    free(deltaBuffer_);

    free(oldRecipe_);
    free(newRecipe_);
    free(deltaRecipe_);
    free(oldBasechunkSf_);
    free(newBasechunkSf_);
    free(offline_encOldChunkBuffer_);
    free(offline_encNewChunkBuffer_);
    free(offline_plainNewDeltaChunkBuffer_);

    free(offline_newDeltaChunkEnc_);
    free(offline_plainOldUniqueBuffer_);
    free(offline_oldDeltaChunkDec_);
    free(offline_oldDeltaChunkEnc_);

    free(offline_tmpUniqueBuffer_);
    // free(offline_coldNewContainer_);

    free(offline_mergeNewContainer_);
    free(offline_mergeRecipeEnc_);
    free(offline_mergeRecipeDec_);

    free(offline_oldChunkDecrypt_);
    free(offline_newChunkDecrypt_);
    free(offline_oldChunkDecompression_);
    free(offline_newChunkDecompression_);

    free(offline_deltaSFBuffer_);
    free(offline_deltaIVBuffer_);
    free(offline_oldIVBuffer_);
    free(offline_newIVBuffer_);
    free(offline_deltaFPBuffer_);
    free(offline_outRecipeBuffer_);
    

    if (psHTable2_) 
    {
        free(psHTable2_->table);
        free(psHTable2_);
    }

    if (psHTable2Offline_) 
    {
        free(psHTable2Offline_->table);
        free(psHTable2Offline_);
    }
    return ;
}

/**
 * @brief init the buffer used in the restore
 * 
 */
void EnclaveClient::InitRestoreBuffer() {
    // to store restore chunk
    _restoreChunkBuffer.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) +
        Enclave::sendChunkBatchSize_ * sizeof(Chunk_t));
    _restoreChunkBuffer.header = (NetworkHead_t*) _restoreChunkBuffer.sendBuffer;
    _restoreChunkBuffer.header->clientID = _clientID;
    _restoreChunkBuffer.header->currentItemNum = 0;
    _restoreChunkBuffer.header->dataSize = 0;
    _restoreChunkBuffer.dataBuffer = _restoreChunkBuffer.sendBuffer + sizeof(NetworkHead_t);

    // for recipe
    _plainRecipeBuffer = (uint8_t*) malloc(Enclave::sendRecipeBatchSize_ *
        sizeof(RecipeEntry_t));
    _enclaveRecipeBuffer.reserve(Enclave::sendRecipeBatchSize_);
    return ;
}

/**
 * @brief destroy the buffer used in the restore
 * 
 */
void EnclaveClient::DestroyRestoreBuffer() {
    free(_plainRecipeBuffer);
    free(_restoreChunkBuffer.sendBuffer);
    return ;
}

/**
 * @brief Set the Master Key object
 * 
 * @param encryptedSecret input encrypted secret
 * @param secretSize the input secret size
 */
void EnclaveClient::SetMasterKey(uint8_t* encryptedSecret, size_t secretSize) {
    EcallCrypto* crypto = new EcallCrypto(CIPHER_TYPE, HASH_TYPE);
    crypto->SessionKeyDec(_cipherCtx, encryptedSecret, secretSize,
        _sessionKey, _masterKey);
    delete crypto;
    return ;
}