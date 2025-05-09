/**
 * @file encOCall.cpp
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief implement the OCALLs of EncOCALL 
 * @version 0.1
 * @date 2023-10-02
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../include/storeOCall.h"


#define Local_Batch 100
#define Cold_Batch 1

namespace OutEnclave {
    // for upload
    StorageCore* storageCoreObj_ = NULL;
    AbsDatabase* indexStoreObj_ = NULL;
    DataWriter* dataWriterObj_ = NULL;
    ofstream outSealedFile_;
    ifstream inSealedFile_;
    uint8_t* tmpOcallcontainer;

    // for restore
    EnclaveRecvDecoder* enclaveRecvDecoderObj_ = NULL;
    string myName_ = "OCall";

    // for lock
    pthread_rwlock_t outIdxLck_;

    pthread_rwlock_t outLogLck_;

};

using namespace OutEnclave;

/**
 * @brief setup the ocall var
 * 
 * @param dataWriterObj the pointer to the data writer
 * @param indexStoreObj the pointer to the index
 * @param storageCoreObj the pointer to the storageCoreObj
 * @param enclaveDecoderObj the pointer to the enclave recvDecoder
 */
void OutEnclave::Init(DataWriter* dataWriterObj,
    AbsDatabase* indexStoreObj,
    StorageCore* storageCoreObj,
    EnclaveRecvDecoder* enclaveRecvDecoderObj) {
    dataWriterObj_ = dataWriterObj;
    indexStoreObj_ = indexStoreObj;
    storageCoreObj_ = storageCoreObj;
    enclaveRecvDecoderObj_ = enclaveRecvDecoderObj;
    tmpOcallcontainer = (uint8_t*)malloc(MAX_CONTAINER_SIZE);

    // init the lck
    pthread_rwlock_init(&outIdxLck_, NULL);
    return ;
}

/**
 * @brief destroy the ocall var
 * 
 */
void OutEnclave::Destroy() {
    // destroy the lck
    free(tmpOcallcontainer);
    pthread_rwlock_destroy(&outIdxLck_);
    return ;
}

/**
 * @brief persist the buffer to file 
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_UpdateFileRecipe(void* outClient) {
    ClientVar *outClientPtr = (ClientVar *)outClient;
    Recipe_t *outRecipe = &outClientPtr->_outRecipe;
    storageCoreObj_->UpdateRecipeToFile(outRecipe->entryFpList,
                                        outRecipe->recipeNum, outClientPtr->_recipeWriteHandler);
    outRecipe->recipeNum = 0;
    return;
}

/**
 * @brief exit the enclave with error message
 * 
 * @param error_msg the error message
 */
void Ocall_SGX_Exit_Error(const char* error_msg) {
    tool::Logging(myName_.c_str(), "%s\n", error_msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief dump the inside container to the outside buffer
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_WriteContainer(void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
#if (MULTI_CLIENT == 1) 
    outClientPtr->_curContainer.deltaFlag = false;
    dataWriterObj_->SaveToFile(outClientPtr->_curContainer);
#else
    outClientPtr->_curContainer.deltaFlag = false;
    outClientPtr->_inputMQ->Push(outClientPtr->_curContainer);
#endif
    // reset current container
    tool::CreateUUID(outClientPtr->_curContainer.containerID,
        CONTAINER_ID_LENGTH,con_times);
    con_times++;
    outClientPtr->_curContainer.currentSize = 0;
    return ;
}

/**
 * @brief dump the inside deltacontainer to the outside buffer
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_WriteDeltaContainer(void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
#if (MULTI_CLIENT == 1) 
    outClientPtr->_curDeltaContainer.deltaFlag = true;
    dataWriterObj_->SaveToFile(outClientPtr->_curDeltaContainer);
#else
    outClientPtr->_curDeltaContainer.deltaFlag = true;
    outClientPtr->_inputMQ->Push(outClientPtr->_curDeltaContainer);
#endif
    // reset current container
    tool::CreateUUID(outClientPtr->_curDeltaContainer.containerID,
        CONTAINER_ID_LENGTH,con_times);
    con_times++;
    outClientPtr->_curDeltaContainer.currentSize = 0;
    return ;
}

/**
 * @brief printf interface for Ocall
 * 
 * @param str input string 
 */
void Ocall_Printf(const char* str) {
    fprintf(stderr, "**Enclave**: %s", str);
}

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
    const uint8_t* buffer, size_t bufferSize) {
    // tool::Logging(myName_.c_str(), "inmerge, insert key: %s.\n", key);
    *ret = indexStoreObj_->InsertBothBuffer(key, keySize, (char*)buffer, bufferSize);
    return ;
}


void Ocall_UpdateIndexStoreSF(bool* ret, const char* key, size_t keySize, 
    const uint8_t* buffer, size_t bufferSize) {
    *ret = indexStoreObj_->InsertSF(key,keySize,(char*)buffer,bufferSize,0);
    return ;
}

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
    uint8_t** retVal, size_t* expectedRetValSize, void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    *ret = indexStoreObj_->QueryBuffer(key, keySize, 
        outClientPtr->_tmpQueryBufferStr);
    (*retVal) = (uint8_t*)&outClientPtr->_tmpQueryBufferStr[0];
    (*expectedRetValSize) = outClientPtr->_tmpQueryBufferStr.size();
    return ;
}

/**
 * @brief get current time from the outside
 * 
 * @return long current time (usec)
 */
void Ocall_GetCurrentTime(uint64_t* retTime) {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    *retTime = currentTime.tv_sec * SEC_2_US + currentTime.tv_usec;
    return ;
}

/**
 * @brief init the file output stream
 *
 * @param ret the return result 
 * @param sealedFileName the sealed file name
 */
void Ocall_InitWriteSealedFile(bool* ret, const char* sealedFileName) {
    if (outSealedFile_.is_open()) {
        tool::Logging(myName_.c_str(), "sealed file is already opened: %s\n", sealedFileName);
        *ret = false;
        return ;
    }

    outSealedFile_.open(sealedFileName, ios_base::trunc | ios_base::binary);
    if (!outSealedFile_.is_open()) {
        tool::Logging(myName_.c_str(), "cannot open the sealed file.\n");
        *ret = false;
        return ;
    }
    *ret = true;
    return ;
}

/**
 * @brief write the data to the disk file
 * 
 * @param sealedFileName the name of the sealed file
 * @param sealedDataBuffer sealed data buffer
 * @param sealedDataSize sealed data size
 * @return true success
 * @return false fail
 */
void Ocall_WriteSealedData(const char* sealedFileName, uint8_t* sealedDataBuffer, size_t sealedDataSize) {
    outSealedFile_.write((char*)sealedDataBuffer, sealedDataSize);
    return ;
}

/**
 * @brief close the file output stream
 * 
 * @param sealedFileName the sealed file name
 */
void Ocall_CloseWriteSealedFile(const char* sealedFileName) {
    if (outSealedFile_.is_open()) {
        outSealedFile_.close();
        outSealedFile_.clear();
    }
    return ;
}

/**
 * @brief Init the unseal file stream 
 * 
 * @param fileSize the file size
 * @param sealedFileName the sealed file name
 * @return uint32_t the file size
 */
void Ocall_InitReadSealedFile(size_t* fileSize, const char* sealedFileName) {
    // return OutEnclave::ocallHandlerObj_->InitReadSealedFile(sealedFileName);
    string fileName(sealedFileName);
    tool::Logging(myName_.c_str(), "print the file name: %s\n", fileName.c_str());
    inSealedFile_.open(fileName, ios_base::binary);

    if (!inSealedFile_.is_open()) {
        tool::Logging(myName_.c_str(), "sealed file does not exist.\n");
        *fileSize = 0;
        return ;
    }

    size_t beginSize = inSealedFile_.tellg();
    inSealedFile_.seekg(0, ios_base::end);
    *fileSize = inSealedFile_.tellg();
    *fileSize = *fileSize - beginSize;

    // reset
    inSealedFile_.clear();
    inSealedFile_.seekg(0, ios_base::beg);

    return ;
}

/**
 * @brief close the file input stream
 * 
 * @param sealedFileName the sealed file name
 */
void Ocall_CloseReadSealedFile(const char* sealedFileName) {
    if (inSealedFile_.is_open()) {
        inSealedFile_.close();
    }
    return ;
}

/**
 * @brief read the sealed data from the file
 * 
 * @param sealedFileName the sealed file
 * @param dataBuffer the data buffer
 * @param sealedDataSize the size of sealed data
 */
void Ocall_ReadSealedData(const char* sealedFileName, uint8_t* dataBuffer, 
    uint32_t sealedDataSize) {
    inSealedFile_.read((char*)dataBuffer, sealedDataSize);
    return ;
}

/**
 * @brief Print the content of the buffer
 * 
 * @param buffer the input buffer
 * @param len the length in byte
 */
void Ocall_PrintfBinary(const uint8_t* buffer, size_t len) {
    fprintf(stderr, "**Enclave**: ");
    for (size_t i = 0; i < len; i++) {
        fprintf(stderr, "%02x", buffer[i]);
    }
    fprintf(stderr, "\n");
    return ;
}

/**
 * @brief Get the required container from the outside application
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_GetReqContainers(void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    enclaveRecvDecoderObj_->GetReqContainers(outClientPtr);
    return ;
}

/**
 * @brief send the restore chunks to the client
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_SendRestoreData(void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    enclaveRecvDecoderObj_->SendBatchChunks(
        &outClientPtr->_sendChunkBuf,
        outClientPtr->_clientSSL);
    return ;
}

/**
 * @brief query the outside deduplication index 
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_QueryOutIndex(void* outClient) {
#if (MULTI_CLIENT == 1)
    pthread_rwlock_rdlock(&outIdxLck_);
#endif
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tmpChunkAddress;
    tmpChunkAddress.resize(sizeof(RecipeEntry_t), 0);
    bool queryResult;
    for (size_t i = 0; i < outQuery->queryNum; i++) {
        // check the outside index
        queryResult = indexStoreObj_->QueryBuffer((char*)entry->chunkHash, 
            CHUNK_HASH_SIZE, tmpChunkAddress);
        if (queryResult) {
            // this chunk is duplicate in the outside index
            // store the query result in the buffer 
            entry->dedupFlag = DUPLICATE;
            memcpy(&entry->chunkAddr, &tmpChunkAddress[0], sizeof(RecipeEntry_t));
        } else {
            entry->dedupFlag = UNIQUE;
        }
        entry++; 
    }
#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outIdxLck_);
#endif
    return ;
}

/**
 * @brief update the outside deduplication index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_UpdateOutIndex(void* outClient) {
#if (MULTI_CLIENT == 1) 
    pthread_rwlock_wrlock(&outIdxLck_);
#endif
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;


    string tmpChunkAddress;
    tmpChunkAddress.resize(sizeof(RecipeEntry_t), 0);


    for (size_t i = 0; i < outQuery->queryNum; i++) {
        // update the outside index
        if (entry->dedupFlag == UNIQUE) {
            // this is unique for the outside index, update the outside index
            indexStoreObj_->InsertBothBuffer((char*)entry->chunkHash, CHUNK_HASH_SIZE,
                (char*)&entry->chunkAddr, sizeof(RecipeEntry_t));

                //tool::PrintBinaryArray((uint8_t*)entry->chunkHash,CHUNK_HASH_SIZE);

                if(entry->deltaFlag == NO_DELTA && entry->offlineFlag == 0){
                    indexStoreObj_->InsertSF((char*)entry->superfeature,CHUNK_HASH_SIZE*3,(char*)&entry->chunkHash,CHUNK_HASH_SIZE,0);
                }
        } 
        entry++;
    }
#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outIdxLck_);
#endif
    return ;
}

/**
 * @brief generate the UUID
 * 
 * @param id the uuid buffer
 * @param len the id len
 */
void Ocall_CreateUUID(uint8_t* id, size_t len) {
    tool::CreateUUID((char*)id, len,con_times);
    con_times++;
    return ;
}

void Ocall_ReadIndexStoreBatch(bool* ret, const char* key, size_t keySize,
    uint8_t** retVal, size_t* expectedRetValSize, void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    size_t recipeNum = keySize / CHUNK_HASH_SIZE;
    //tool::Logging("DEBUG", "Inside recipeNum is %d\n", recipeNum);
    outClientPtr->_tmpBatchQueryBufferStr.resize(0,recipeNum*sizeof(RecipeEntry_t));
    string value;
    for(size_t i = 0; i < recipeNum; i++)
    {
        value.resize(sizeof(RecipeEntry_t), 0);
        *ret=indexStoreObj_->QueryBuffer(key , CHUNK_HASH_SIZE, value);
        if(!(*ret)){
            tool::PrintBinaryArray((uint8_t*)key,CHUNK_HASH_SIZE);
            tool::Logging("DEBUG","Not find!!!\n");
        }
        outClientPtr->_tmpBatchQueryBufferStr += value;
        //memcpy(outClientPtr->_tmpBatchQueryBufferStr + i * sizeof(RecipeEntry_t), (uint8_t*)&value[0], sizeof(RecipeEntry_t));
        key += CHUNK_HASH_SIZE;
    }
    //*retVal = outClientPtr->_tmpBatchQueryBufferStr;
    (*retVal) = (uint8_t*)&outClientPtr->_tmpBatchQueryBufferStr[0]; 
    (*expectedRetValSize) = outClientPtr->_tmpBatchQueryBufferStr.size();   
    return ;
}

void Ocall_QueryBaseIndex(void* outClient) {
#if (MULTI_CLIENT == 1)
    pthread_rwlock_rdlock(&outIdxLck_);
#endif
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_baseoutQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tmpChunkAddress;
    tmpChunkAddress.resize(sizeof(RecipeEntry_t), 0);
    bool queryResult;
    for (size_t i = 0; i < outQuery->queryNum; i++) {
        // check the outside index
       // fprintf(stderr, "base chunk hash 3: %02x\n", entry->chunkHash[10]);
        queryResult = indexStoreObj_->QueryBuffer((char*)entry->chunkHash, 
            CHUNK_HASH_SIZE, tmpChunkAddress);
        if (queryResult) {
            // this chunk is duplicate in the outside index
            // store the query result in the buffer 
            //fprintf(stderr, "find out!\n");
            //fprintf(stderr, "address:%s\n",tmpChunkAddress.c_str());
            memcpy(&entry->chunkAddr, &tmpChunkAddress[0], sizeof(RecipeEntry_t));


        } else {
            fprintf(stderr, "no find out:%d!\n",i);
            for(int i = 0;i<CHUNK_HASH_SIZE;i++){
                fprintf(stderr, "%02x\n",entry->chunkHash[i]);
            }
        }
        entry++; 
    }
#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outIdxLck_);
#endif
    return ;
}


void Ocall_FreeContainer(void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_baseoutQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tmpbaseChunkAddress;
    tmpbaseChunkAddress.resize(sizeof(RecipeEntry_t), 0);
    for(size_t i = 0;i < outQuery->queryNum;i++){
        free(entry->containerbuffer);
        entry++;
    }
    return ;
}


void Ocall_QueryOutBasechunk(void* outClient){
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tempbasehash;
    tempbasehash.resize(CHUNK_HASH_SIZE, 0);
    string tmpbaseChunkAddress;
    tmpbaseChunkAddress.resize(sizeof(RecipeEntry_t), 0);

    bool queryResult;

#if (MULTI_CLIENT == 1) 
    pthread_rwlock_rdlock(&outIdxLck_);
#endif   
    
    for (size_t i = 0; i < outQuery->queryNum; i++) {
        if(entry->dedupFlag == UNIQUE && entry->deltaFlag == NO_DELTA){
            //fprintf(stderr, "done55555\n"); 
            //tool::PrintBinaryArray((uint8_t*)&entry->superfeature[0],3*CHUNK_HASH_SIZE);
            queryResult = indexStoreObj_->QuerySF((char*)entry->superfeature, CHUNK_HASH_SIZE*3, tempbasehash);
            // fprintf(stderr, "done566\n"); 

            if(queryResult){
                memcpy(entry->chunkAddr.basechunkHash, (uint8_t*)&tempbasehash[0], CHUNK_HASH_SIZE);
                entry->deltaFlag = OUT_DELTA;
            }

         if(entry->deltaFlag == OUT_DELTA){
            queryResult = indexStoreObj_->QueryBuffer((char*)entry->chunkAddr.basechunkHash, CHUNK_HASH_SIZE, tmpbaseChunkAddress);
            if(queryResult){
            entry->deltaFlag = OUT_DELTA;
            memcpy(&entry->basechunkAddr,&tmpbaseChunkAddress[0],sizeof(RecipeEntry_t));
            }else{
            fprintf(stderr, "error!!! Basechunk no find out-recipe\n"); 

            }
        }
        }    
        entry++;
    }

#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outIdxLck_);
#endif

    return;

}

void Ocall_getRefContainer(void* outClient) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tmpbaseChunkAddress;


    tmpbaseChunkAddress.resize(sizeof(RecipeEntry_t), 0);
    for(size_t i = 0;i < outQuery->currNum;i++){
        entry++;
    }
    if(entry->deltaFlag == OUT_DELTA){
    string tmpContainerIDStr;
    uint8_t* tempcontainerbuf;
    //tempcontainerbuf = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    tmpContainerIDStr.assign((char*)entry->basechunkAddr.containerName, CONTAINER_ID_LENGTH);
   // fprintf(stderr, "containerid:%s\n",tmpContainerIDStr.c_str());
/*     if(indexStoreObj_->QueryVersion((char*)entry->basechunkAddr.containerName,CONTAINER_ID_LENGTH,2)){
      //  fprintf(stderr, "CONTAINER IS OKKK\n");
    }else{
        //fprintf(stderr, "CONTAINER IS not OKKK-------------------------------------------------------------------------------------------------------\n");
        entry->chunkAddr.deltaFalg = DELAY_DELTA;
        return; 
    }  */

    ifstream containerIn;
    string containerNamePrefix_ = "Base-Containers/";
    string containerNameTail_ = config.GetContainerSuffix();
    
    string readFileNameStr = containerNamePrefix_ + tmpContainerIDStr + containerNameTail_;
  //  fprintf(stderr, "%s\n",readFileNameStr.c_str());

    containerIn.open(readFileNameStr, ifstream::in | ifstream::binary);
    if (!containerIn.is_open()) {
           //fprintf(stderr, "container have not store on disk1: %s\n",tmpContainerIDStr.c_str());
            entry->deltaFlag = NO_DELTA;
            //free(tempcontainerbuf);
            return;
    }

    containerIn.seekg(0, ios_base::end);
    int readSize = containerIn.tellg();
    containerIn.seekg(0, ios_base::beg);
    int containerSize = 0;
  //  fprintf(stderr, "container size:%d\n",readSize);

    containerSize = readSize;
        // read compression data
    containerIn.read((char*)tmpOcallcontainer, containerSize);
     if (containerIn.gcount() != containerSize) {
            fprintf(stderr, "not match\n");
            exit(EXIT_FAILURE);
    } 
    containerIn.close();

    entry->containerbuffer = tmpOcallcontainer;
    entry->containersize = containerSize;
    entry->deltaFlag = DELTA;

    return;
    }

    if(entry->deltaFlag == DELTA){
    string tmpContainerIDStr;
    uint8_t* tempcontainerbuf;
    //tempcontainerbuf = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    tmpContainerIDStr.assign((char*)entry->basechunkAddr.containerName, CONTAINER_ID_LENGTH);
    //fprintf(stderr, "delay_containerid:%s\n",tmpContainerIDStr.c_str());
    ifstream containerIn;
    string containerNamePrefix_ = "Base-Containers/";
    string containerNameTail_ = config.GetContainerSuffix();
    
    string readFileNameStr = containerNamePrefix_ + tmpContainerIDStr + containerNameTail_;
  //  fprintf(stderr, "%s\n",readFileNameStr.c_str());

    containerIn.open(readFileNameStr, ifstream::in | ifstream::binary);
    if (!containerIn.is_open()) {
            fprintf(stderr, "container have not store on disk :%s\n",tmpContainerIDStr.c_str());
            entry->deltaFlag = NO_DELTA;
           // free(tempcontainerbuf);
            return;
    }
    containerIn.seekg(0, ios_base::end);
    int readSize = containerIn.tellg();
    containerIn.seekg(0, ios_base::beg);
    int containerSize = 0;
   // fprintf(stderr, "delay_container size:%d\n",readSize);

    containerSize = readSize;
        // read compression data
    containerIn.read((char*)tmpOcallcontainer, containerSize);
     if (containerIn.gcount() != containerSize) {
            fprintf(stderr, "not match\n");
            exit(EXIT_FAILURE);
    } 
    containerIn.close();

    entry->containerbuffer = tmpOcallcontainer;
    entry->containersize = containerSize;



    entry->deltaFlag  = DELTA;

    return;
    }

    return ;
}

void Ocall_UpdateDeltaIndex(void* outClient, size_t chunkNum)
{
    ClientVar* outClientPtr = (ClientVar*)outClient;
    uint8_t* tmpBuffer = outClientPtr->_process_buffer;
#if (MULTI_CLIENT == 1) 
    pthread_rwlock_wrlock(&outLogLck_);
#endif 
    // tool::Logging("DEBUG", "chunkNum: %lu\n", chunkNum);
    uint32_t offset = 0;
    string baseChunkHash;
    string deltaChunkHash;
    for (int i = 0; i < chunkNum; i++) {
        baseChunkHash.assign((char*)tmpBuffer + offset, CHUNK_HASH_SIZE);
        offset += CHUNK_HASH_SIZE;
        deltaChunkHash.assign((char*)tmpBuffer + offset, CHUNK_HASH_SIZE);
        offset += CHUNK_HASH_SIZE;
        // tool::Logging("DEBUG", "base chunk hash: %s, delta chunk hash: %s\n", baseChunkHash.c_str(), deltaChunkHash.c_str());
        indexStoreObj_->InsertDeltaIndex(baseChunkHash, deltaChunkHash);
    }

#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outLogLck_);
#endif

    return ;
}

void Ocall_QueryDeltaIndex(void* outClient)
 {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    uint8_t* tmpBuffer = outClientPtr->_process_buffer;
    string oldBaseChunkFp;
    oldBaseChunkFp.assign((char*)tmpBuffer, CHUNK_HASH_SIZE);
    vector<string> result;
    bool queryResult = indexStoreObj_->QueryDeltaIndex(oldBaseChunkFp, result);
    if(queryResult)
    {
         for(size_t i = 0; i < result.size(); i++)
         {
             memcpy(tmpBuffer + (i + 1) * CHUNK_HASH_SIZE, (uint8_t*)&result[i][0], CHUNK_HASH_SIZE);
         } 
            outClientPtr->_deltaInfo.QueryNum = result.size();
    }else
    {
            outClientPtr->_deltaInfo.QueryNum = 0;
            //tool::Logging("DEBUG", "not find in delta index\n");
    }
    return ;

 }

int bugSkip = 0;
int local_delta = 0;

void Ocall_LocalInsert(void* outClient, size_t chunkNum){
    string oldbasechunkhash;
    string newbasechunkhash;
    ClientVar* outClientPtr = (ClientVar*)outClient;
    uint8_t* buffer = outClientPtr->_test_buffer;
#if (MULTI_CLIENT == 1) 
    pthread_rwlock_wrlock(&outLogLck_);
#endif  
    uint32_t offset = 0;
    for (int i = 0; i < chunkNum; i++)
    {
        oldbasechunkhash.assign((char*)buffer + offset, CHUNK_HASH_SIZE);
        offset += CHUNK_HASH_SIZE;
        newbasechunkhash.assign((char*)buffer + offset, CHUNK_HASH_SIZE);
        offset += CHUNK_HASH_SIZE;
        auto it = indexStoreObj_->_local_tmp_map.find(oldbasechunkhash);
        if(it != indexStoreObj_->_local_tmp_map.end())
        {

            //tool::Logging("debug","bugSkip is %d\n", bugSkip);
            indexStoreObj_->skipMap[oldbasechunkhash].push_back(it->second);
            bugSkip++;
        }
        indexStoreObj_->_local_tmp_map[oldbasechunkhash] = newbasechunkhash;
        local_delta++;
        // tool::Logging("DEBUG", "old chunk hash: %s, new chunk hash: %s\n", oldbasechunkhash.c_str(), newbasechunkhash.c_str());
    }
    


#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outLogLck_);
#endif
    return;
}

void Ocall_Localrevise(void* outClient){
    ClientVar* outClientPtr = (ClientVar*)outClient;
    pair<string,string> tmppair;

    unordered_map<string, string> local_greedy_basemap;


    for (unordered_map<string, string>::iterator it = indexStoreObj_->_local_tmp_map.begin(); it != indexStoreObj_->_local_tmp_map.end(); it++)
        {
            tmppair = {it->first,it->second};
            local_greedy_basemap[it->first] = it->second;
            // TODO insert
            auto it2 = indexStoreObj_->skipMap.find(it->first);
            if(it2 != indexStoreObj_->skipMap.end())
            {
                for(size_t i = 0; i < indexStoreObj_->skipMap[it->first].size();i++)
                {
                    pair<string, string> tmppair2;
                    tmppair2 = {indexStoreObj_->skipMap[it->first][i], it->second};
                    local_greedy_basemap[indexStoreObj_->skipMap[it->first][i]] = it->second;
                }
            }
        }
    //fprintf(stderr, "map_size %d\n",outClientPtr->_local_map.size()); 

    vector<pair<string,int>> Tmp_pair;

    for(unordered_map<string, string>::iterator it = local_greedy_basemap.begin(); it != local_greedy_basemap.end(); it++){
        int deltachunk_num = 0;
        auto delta_it = indexStoreObj_->delta_index.find(it->first);
        if(delta_it != indexStoreObj_->delta_index.end()){
            deltachunk_num = indexStoreObj_->delta_index[it->first].size();
        }else{
            deltachunk_num = 0;
        }
        Tmp_pair.push_back({it->first,deltachunk_num});
    }

    sort(Tmp_pair.begin(),Tmp_pair.end(),dataWriterObj_->myGreedyCompare);

    for(int i = 0;i < Tmp_pair.size();i++){

        //fprintf(stderr,"%d ",Tmp_pair[i].second);
        string oldbasechunkhash = Tmp_pair[i].first;
        string newbasechunkhash = local_greedy_basemap[Tmp_pair[i].first];
        indexStoreObj_->_local_map.push_back({oldbasechunkhash,newbasechunkhash});
    }

    indexStoreObj_->_local_tmp_map.clear();
    indexStoreObj_->skipMap.clear();

    tool::Logging("debug","bugSkip is %d\n", bugSkip);
    tool::Logging("debug","local_delta is %d\n", local_delta);
}


void Ocall_GetLocal( void* outClient){
    ClientVar* outClientPtr = (ClientVar*)outClient;
    uint8_t* buffer = outClientPtr->_test_buffer;
    size_t offset = 0;
    string old_basechunkhash;
    string new_basechunkhash;
    uint32_t num;

    if(indexStoreObj_->_local_map.size() > Local_Batch){
        num = Local_Batch;
        memcpy(buffer,&num,sizeof(uint32_t));
        offset += sizeof(uint32_t);
       
        for(int i = 0;i < Local_Batch;i++){
            old_basechunkhash=indexStoreObj_->_local_map.back().first;
            memcpy(buffer+offset,&old_basechunkhash[0],CHUNK_HASH_SIZE);
            offset += CHUNK_HASH_SIZE;
            new_basechunkhash= indexStoreObj_->_local_map.back().second;
            memcpy(buffer+offset,&new_basechunkhash[0],CHUNK_HASH_SIZE);
            offset += CHUNK_HASH_SIZE;
            indexStoreObj_->_local_map.pop_back();
        }
        //fprintf(stderr, "map_size %d\n",outClientPtr->_outbackward.backwardnum); 
        return;
    }else{
        num = indexStoreObj_->_local_map.size();
        memcpy(buffer,&num,sizeof(uint32_t));
        offset += sizeof(uint32_t);

        for(int i = 0;i < num;i++){
            old_basechunkhash=indexStoreObj_->_local_map.back().first;
            memcpy(buffer+offset,&old_basechunkhash[0],CHUNK_HASH_SIZE);
            offset += CHUNK_HASH_SIZE;
            new_basechunkhash= indexStoreObj_->_local_map.back().second;
            memcpy(buffer+offset,&new_basechunkhash[0],CHUNK_HASH_SIZE);
            offset += CHUNK_HASH_SIZE;
            indexStoreObj_->_local_map.pop_back();
        }


        return;
    }
}

void Ocall_OneRecipe(void* outClient) {
#if (MULTI_CLIENT == 1) 
    pthread_rwlock_rdlock(&outIdxLck_);
#endif

    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;

    string tmpChunkAddress;
    tmpChunkAddress.resize(sizeof(RecipeEntry_t), 0);
    bool queryresult;

    queryresult = indexStoreObj_->QueryBuffer((char*)entry->chunkHash, CHUNK_HASH_SIZE, tmpChunkAddress);
    if(queryresult){
        memcpy(&entry->chunkAddr,&tmpChunkAddress[0],sizeof(RecipeEntry_t));
    }else{
        fprintf(stderr, "do not find recipe\n");
    }

#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outIdxLck_);
#endif

    return;
}

void Ocall_OneContainer(void* outClient){
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tmpContainerIDStr;
    tmpContainerIDStr.resize(CONTAINER_ID_LENGTH,0);
    tmpContainerIDStr.assign((char*)entry->chunkAddr.containerName,CONTAINER_ID_LENGTH);
    ifstream containerIn;
    string containerNamePrefix_ = "Base-Containers/";
    string containerNameTail_ = config.GetContainerSuffix();
    string readFileNameStr = containerNamePrefix_ + tmpContainerIDStr + containerNameTail_;
    containerIn.open(readFileNameStr, ifstream::in | ifstream::binary);
        if (!containerIn.is_open()) {
            fprintf(stderr, "there is something wrong\n");
            fprintf(stderr, "Base chunk container: %s\n",readFileNameStr.c_str());
            entry->offlineFlag = false;
            return;
    }

    containerIn.seekg(0, ios_base::end);
    int readSize = containerIn.tellg();
    containerIn.seekg(0, ios_base::beg);
    int containerSize = 0;
    containerSize = readSize;
    containerIn.read((char*)tmpOcallcontainer, containerSize);
    if (containerIn.gcount() != containerSize) {
        fprintf(stderr, "not match\n");
        exit(EXIT_FAILURE);
    }
    containerIn.close();
    entry->containerbuffer = tmpOcallcontainer;
    entry->offlineFlag = true;
    return;
}

void Ocall_OFFline_updateIndex(void* outClient,size_t keySize){

#if (MULTI_CLIENT == 1) 
    pthread_rwlock_wrlock(&outIdxLck_);
#endif

    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;

    string tmpbaseChunkAddress;
    tmpbaseChunkAddress.resize(sizeof(RecipeEntry_t), 0);
    bool queryResult;

    queryResult = indexStoreObj_->QueryBuffer((char*)entry->chunkHash, CHUNK_HASH_SIZE, tmpbaseChunkAddress);

    if(!queryResult){
        fprintf(stderr, "Ocall_OFFline_updateIndex error\n");
    }


    if(keySize == 0){
        indexStoreObj_->InsertSF((char*)entry->superfeature,CHUNK_HASH_SIZE*3,(char*)&entry->chunkHash,CHUNK_HASH_SIZE,2);
    }else{
        indexStoreObj_->InsertSF((char*)entry->superfeature,CHUNK_HASH_SIZE*3,(char*)&entry->chunkHash,CHUNK_HASH_SIZE,1);
    }

#if (MULTI_CLIENT == 1)
    pthread_rwlock_unlock(&outIdxLck_);
#endif

    return;
}

void Ocall_SavehotContainer(const char* containerID, uint8_t* containerBody, size_t currentSize)
{
    FILE* containerFile = NULL;
    tool::Logging("DEBUG", "in Ocall save hot container, containerid is %s\n", containerID);


#if(CONTAINER_SEPARATE == 1)
    string containerNamePrefix_ = "Delta-Containers/";
#endif

#if(CONTAINER_SEPARATE == 0)
    string containerNamePrefix_ = "Base-Containers/";
#endif


    string containerNameTail_ = config.GetContainerSuffix();
    string fileName(containerID, CONTAINER_ID_LENGTH);
    string fileFullName = containerNamePrefix_ + fileName + containerNameTail_;
    containerFile = fopen(fileFullName.c_str(), "wb");
    if(!containerFile)
    {
        exit(EXIT_FAILURE);
    }

    fwrite((char*)containerBody, currentSize, 1, containerFile);
    fclose(containerFile);
    return ;
}

void Ocall_SaveColdContainer(const char* containerID, uint8_t* containerBody, size_t currentSize, bool* delta_flag)
{
    FILE* containerFile = NULL;
    string containerNamePrefix_;
    //tool::Logging("CCLOD", "in ocall save cold container. containerid is %s\n", containerID);
    if(*delta_flag)
    {
        containerNamePrefix_ = "Delta-Containers/";
    } else
    {
        containerNamePrefix_ = "Base-Containers/";
    }
    string containerNameTail_ = config.GetContainerSuffix();
    string fileName;
    fileName.assign(containerID, CONTAINER_ID_LENGTH);
    string fileFullName = containerNamePrefix_ + fileName + containerNameTail_;
    //tool::Logging("DEBUG", "fullFileName is  %s\n", fileFullName.c_str());
    containerFile = fopen(fileFullName.c_str(), "wb");
    if(!containerFile)
    {
        exit(EXIT_FAILURE);
    }
    fwrite((char*)containerBody, currentSize, 1, containerFile);
    fclose(containerFile);
    return ;
}

void Ocall_ColdInsert(void* outClient){
    string ColdContainerID;
    uint32_t offset;
    uint32_t length;
    pair<uint32_t,uint32_t> tmppair;
    ClientVar* outClientPtr = (ClientVar*)outClient;
    uint8_t* buffer = outClientPtr->_out_buffer;
    ColdContainerID.assign((char*)buffer,CONTAINER_ID_LENGTH);
    buffer += CONTAINER_ID_LENGTH;
    memcpy(&offset,buffer,sizeof(uint32_t));
    buffer += sizeof(uint32_t);
    memcpy(&length,buffer,sizeof(uint32_t));
    tmppair = {offset,length};
    indexStoreObj_->_cold_map[ColdContainerID].push_back(tmppair);
    auto it = indexStoreObj_->_cold_container_set.find(ColdContainerID);
    if(it==indexStoreObj_->_cold_container_set.end()){
        indexStoreObj_->_cold_container_set.insert(ColdContainerID);
        //fprintf(stderr, "Insert Container %s\n",ColdContainerID.c_str());
    }
    //fprintf(stderr, "Cold_map_size %d\n",indexStoreObj_->_cold_container_set.size()); 
    return;
}


void Ocall_GetCold( void* outClient){
    ClientVar* outClientPtr = (ClientVar*)outClient;
    uint8_t* buffer = outClientPtr->_out_buffer;
    size_t offset = 0;
    string Cold_ContainerID;
    string new_basechunkhash;
    uint32_t Cold_Containernum;
    vector<pair<uint32_t,uint32_t>> tmpCold_vector;

    if(indexStoreObj_->_cold_container_vector.size() > Cold_Batch){
        Cold_Containernum = Cold_Batch;
        memcpy(buffer+offset,&Cold_Containernum,sizeof(uint32_t));
        offset += sizeof(uint32_t);
       
        for(int i = 0;i < Cold_Batch;i++){
            Cold_ContainerID=indexStoreObj_->_cold_container_vector.back();
            memcpy(buffer+offset,&Cold_ContainerID[0],CONTAINER_ID_LENGTH);
            offset += CONTAINER_ID_LENGTH;
            uint32_t Cold_num = indexStoreObj_->_cold_map[Cold_ContainerID].size();
            memcpy(buffer+offset,&Cold_num,sizeof(uint32_t));
            offset += sizeof(uint32_t);
            for(int i = 0;i < Cold_num;i++){
                uint32_t Cold_offset = indexStoreObj_->_cold_map[Cold_ContainerID].back().first;
                uint32_t Cold_length = indexStoreObj_->_cold_map[Cold_ContainerID].back().second;
                memcpy(buffer+offset,&Cold_offset,sizeof(uint32_t));
                offset += sizeof(uint32_t);
                memcpy(buffer+offset,&Cold_length,sizeof(uint32_t));
                offset += sizeof(uint32_t);
                indexStoreObj_->_cold_map[Cold_ContainerID].pop_back();
            }
            indexStoreObj_->_cold_map.erase(Cold_ContainerID);
            indexStoreObj_->_cold_container_vector.pop_back();
        }
        //fprintf(stderr, "map_size %d\n",outClientPtr->_outbackward.backwardnum); 
        return;
    }else{
        Cold_Containernum = indexStoreObj_->_cold_container_vector.size();
        memcpy(buffer+offset,&Cold_Containernum,sizeof(uint32_t));
        offset += sizeof(uint32_t);
       
        for(int i = 0;i < Cold_Containernum;i++){
            Cold_ContainerID=indexStoreObj_->_cold_container_vector.back();
            memcpy(buffer+offset,&Cold_ContainerID[0],CONTAINER_ID_LENGTH);
            offset += CONTAINER_ID_LENGTH;
            uint32_t Cold_num = indexStoreObj_->_cold_map[Cold_ContainerID].size();
            memcpy(buffer+offset,&Cold_num,sizeof(uint32_t));
            offset += sizeof(uint32_t);
            for(int i = 0;i < Cold_num;i++){
                uint32_t Cold_offset = indexStoreObj_->_cold_map[Cold_ContainerID].back().first;
                uint32_t Cold_length = indexStoreObj_->_cold_map[Cold_ContainerID].back().second;
                memcpy(buffer+offset,&Cold_offset,sizeof(uint32_t));
                offset += sizeof(uint32_t);
                memcpy(buffer+offset,&Cold_length,sizeof(uint32_t));
                offset += sizeof(uint32_t);
                indexStoreObj_->_cold_map[Cold_ContainerID].pop_back();
            }
            indexStoreObj_->_cold_map.erase(Cold_ContainerID);
            indexStoreObj_->_cold_container_vector.pop_back();
        }
        return;
    }
}

void Ocall_Coldrevise(void* outClient){
    ClientVar* outClientPtr = (ClientVar*)outClient;
    pair<string,string> tmppair;

    for(auto a : indexStoreObj_->_cold_container_set){
        indexStoreObj_->_cold_container_vector.push_back(a);
    }
    //fprintf(stderr, "_cold_container_set_size %d\n",indexStoreObj_->_cold_container_set.size());
    //fprintf(stderr, "old vector map_size %d\n",indexStoreObj_->_cold_container_vector.size()); 
    indexStoreObj_->_cold_container_set.clear();
    return;
}

void Ocall_OneDeltaContainer(void* outClient)
{
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tmpContainerIDStr;
    tmpContainerIDStr.resize(CONTAINER_ID_LENGTH,0);
    tmpContainerIDStr.assign((char*)entry->chunkAddr.containerName,CONTAINER_ID_LENGTH);
    ifstream containerIn;
    string containerNamePrefix_ = "Delta-Containers/";
    string containerNameTail_ = config.GetContainerSuffix();
    string readFileNameStr = containerNamePrefix_ + tmpContainerIDStr + containerNameTail_;
    containerIn.open(readFileNameStr, ifstream::in | ifstream::binary);
        if (!containerIn.is_open()) {
            fprintf(stderr, "there is something wrong\n");
            fprintf(stderr, "Delta chunk container: %s\n",readFileNameStr.c_str());
            return;
    }

    containerIn.seekg(0, ios_base::end);
    int readSize = containerIn.tellg();
    containerIn.seekg(0, ios_base::beg);
    int containerSize = 0;
    containerSize = readSize;
    containerIn.read((char*)tmpOcallcontainer, containerSize);
    if (containerIn.gcount() != containerSize) {
        fprintf(stderr, "not match\n");
        fprintf(stderr, "Delta chunk container: %s\n",readFileNameStr.c_str());
        exit(EXIT_FAILURE);
    }
    containerIn.close();
    entry->containerbuffer = tmpOcallcontainer;
    return;
}

void Ocall_OneColdContainer(void* outClient, bool* deltaFlag) {
    ClientVar* outClientPtr = (ClientVar*)outClient;
    OutQuery_t* outQuery = &outClientPtr->_outQuery;
    OutQueryEntry_t* entry = outQuery->outQueryBase;
    string tmpContainerIDStr;
    tmpContainerIDStr.resize(CONTAINER_ID_LENGTH,0);
    tmpContainerIDStr.assign((char*)entry->chunkAddr.containerName,CONTAINER_ID_LENGTH);
    //tool::Logging("Ocall_OneColdContainer", "cold container id is %s\n", tmpContainerIDStr.c_str());
    ifstream containerIn;
    string containerNamePrefix_ = "Delta-Containers/";
    string containerNameTail_ = config.GetContainerSuffix();
    string readFileNameStr = containerNamePrefix_ + tmpContainerIDStr + containerNameTail_;

    *deltaFlag = 1;
    containerIn.open(readFileNameStr, ifstream::in | ifstream::binary);
    if (!containerIn.is_open()) {
        readFileNameStr = "Base-Containers/" + tmpContainerIDStr + containerNameTail_;
        containerIn.open(readFileNameStr, ifstream::in | ifstream::binary);
        *deltaFlag = 0;
        if(!containerIn.is_open())
        {
            return ;
        }
    }

    containerIn.seekg(0, ios_base::end);
    int readSize = containerIn.tellg();
    containerIn.seekg(0, ios_base::beg);
    int containerSize = 0;
    containerSize = readSize;
    containerIn.read((char*)tmpOcallcontainer, containerSize);
    if (containerIn.gcount() != containerSize) {
        fprintf(stderr, "not match\n");
        exit(EXIT_FAILURE);
    }
    containerIn.close();
    entry->containerbuffer = tmpOcallcontainer;
    entry->containersize = containerSize;
    return;
}

void Ocall_CleanLocalIndex(){
    indexStoreObj_->_local_map.clear();
}

void Ocall_GetMergeContainer(void* outClient)
{
    ClientVar* outClientPtr = (ClientVar*)outClient;
    // outClientPtr->_mergeContainerBuffer = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    // namespace fs = std::filesystem;
    std::vector<std::filesystem::path> baseContainerList_;
    std::string basePath = "Base-Containers/";
    // std::string deltaPath = "Delta-Containers/";
 
    // get container name and sort
    try 
    {
        for (const auto& entry : std::filesystem::directory_iterator(basePath)) {
            if (std::filesystem::is_regular_file(entry.status())) {
                baseContainerList_.push_back(entry.path());
            }
        }

        // for (const auto& entry : fs::directory_iterator(deltaPath)) {
        //     if (fs::is_regular_file(entry.status())) {
        //         outClientPtr->deltaContainerList_.push_back(entry.path());
        //     }
        // }
 
        std::sort(baseContainerList_.begin(), baseContainerList_.end());
        // std::sort(outClientPtr->deltaContainerList_.begin(), outClientPtr->deltaContainerList_.end());
    } 
    catch (const std::filesystem::filesystem_error& e) 
    {
        std::cerr << e.what() << std::endl;
    }

    uint32_t containerSize1;
    uint32_t containerSize2;
    // for base container
    for (int i = 1; i < baseContainerList_.size(); i++)
    {
        std::ifstream container1(baseContainerList_[i-1], std::ifstream::ate | std::ifstream::binary);
        std::ifstream container2(baseContainerList_[i], std::ifstream::ate | std::ifstream::binary);

        if (container1 && container2) {
            containerSize1 = container1.tellg();
            container1.close();
            containerSize2 = container2.tellg();
            container2.close();
            // tool::Logging(myName_.c_str(), "size1: %u, size2: %u.\n", containerSize1, containerSize2); 
        }
        else {
            tool::Logging(myName_.c_str(), "in merge container, can not open container.\n"); 
        }

        if (containerSize1 + containerSize2 < MAX_CONTAINER_SIZE - 1)
        {
            // tool::Logging(myName_.c_str(), "add base pair.\n"); 
            string containerName1 = baseContainerList_[i-1];
            string containerName2 = baseContainerList_[i];
            pair<string, string> pairTmp(containerName1, containerName2);
            outClientPtr->baseMergePair.push_back(pairTmp);
            i++;
        }
    }

    // containerSize1 = 0;
    // containerSize2 = 0;
    // // for delta container
    // for (int i = 1; i < outClientPtr->deltaContainerList_.size(); i++)
    // {
    //     std::ifstream container1(outClientPtr->deltaContainerList_[i-1], std::ifstream::ate | std::ifstream::binary);
    //     std::ifstream container2(outClientPtr->deltaContainerList_[i], std::ifstream::ate | std::ifstream::binary);

    //     if (container1 && container2) {
    //         containerSize1 = container1.tellg();
    //         container1.close();
    //         containerSize2 = container2.tellg();
    //         container2.close();
    //         tool::Logging(myName_.c_str(), "size1: %u, size2: %u.\n", containerSize1, containerSize2); 
    //     }
    //     else {
    //         tool::Logging(myName_.c_str(), "in merge container, can not open container.\n"); 
    //     }

    //     if (containerSize1 + containerSize2 < MAX_CONTAINER_SIZE)
    //     {
    //         tool::Logging(myName_.c_str(), "add delta pair.\n"); 
    //         string containerName1 = outClientPtr->deltaContainerList_[i-1];
    //         string containerName2 = outClientPtr->deltaContainerList_[i];
    //         pair<string, string> pairTmp(containerName1, containerName2);
    //         outClientPtr->deltaMergePair.push_back(pairTmp);
    //         i++;
    //     }
    // }

    if (outClientPtr->baseMergePair.size() != 0)
    {
        tool::Logging(myName_.c_str(), "have small containers, wait for merge.\n"); 
        outClientPtr->_upOutSGX.jobDoneFlag = NOT_DONE;
    }
    else
    {
        tool::Logging(myName_.c_str(), "no container need to be merged.\n"); 
        outClientPtr->_upOutSGX.jobDoneFlag = IS_DONE;
    }
    baseContainerList_.clear();
    return ;
}

void Ocall_CleanMerge(void* outClient)
{
    ClientVar* outClientPtr = (ClientVar*)outClient;
    
    outClientPtr->_upOutSGX.jobDoneFlag = IS_DONE;
    outClientPtr->baseMergePair.clear();
    outClientPtr->_upOutSGX.outQuery->outQueryBase->chunkAddr.offset = 0;
    return ;
}

// merge second container to first container
void Ocall_GetMergePair(void* outClient, uint8_t* containerID, uint32_t* size)
{
    ClientVar* outClientPtr = (ClientVar*)outClient;

    tool::Logging(myName_.c_str(), "merge remain pairs: %u.\n", outClientPtr->baseMergePair.size()); 

    if (outClientPtr->baseMergePair.size() != 0)
    {
        // get name
        string containerName1 = outClientPtr->baseMergePair[0].first.substr(16, CONTAINER_ID_LENGTH);
        string containerName2 = outClientPtr->baseMergePair[0].second.substr(16, CONTAINER_ID_LENGTH);
        tool::Logging(myName_.c_str(), "1st container name: %s.\n", containerName1.c_str()); 
        tool::Logging(myName_.c_str(), "2nd container name: %s.\n", containerName2.c_str()); 
        OutQueryEntry_t* entry = outClientPtr->_upOutSGX.outQuery->outQueryBase;
        
        memcpy(containerID, containerName1.c_str(), CONTAINER_ID_LENGTH);
    
        // get 1st container size
        std::ifstream container1(outClientPtr->baseMergePair[0].first.c_str(), std::ifstream::ate | std::ifstream::binary);
        uint32_t containerSize1 = 0;
        if (container1) {
            containerSize1 = container1.tellg();
            container1.close();
        }
        *size = containerSize1;

        // get 2nd container
        ifstream containerIn;
        // tool::Logging(myName_.c_str(), "2nd container path: %s.\n", outClientPtr->baseMergePair[0].second.c_str()); 
        containerIn.open(outClientPtr->baseMergePair[0].second.c_str(), ifstream::in | ifstream::binary);
        containerIn.seekg(0, ios_base::end);
        int readSize = containerIn.tellg();
        containerIn.seekg(0, ios_base::beg);
        int containerSize = 0;
        containerSize = readSize;
        containerIn.read((char*)outClientPtr->_mergeContianer.body, containerSize);
        // tool::Logging(myName_.c_str(), "out container: %d %d.\n", (int)outClientPtr->_mergeContainerBuffer[0],
        //     (int)outClientPtr->_mergeContainerBuffer[1]); 
        // tool::Logging(myName_.c_str(), "read %u bytes from 2nd containrt.\n", containerSize); 
        if (containerIn.gcount() != containerSize) {
            tool::Logging(myName_.c_str(), "container size: %u, read size: %u.\n", containerIn.gcount(), containerSize); 
            fprintf(stderr, "not match\n");
            exit(EXIT_FAILURE);
        }
        containerIn.close();
        entry->containersize = containerSize;
    }
    else
    {
        outClientPtr->_upOutSGX.jobDoneFlag = IS_DONE;
    }
    return ;
}

void Ocall_MergeContent(void* outClient, uint8_t* containerBody, size_t currentSize)
{
    ClientVar* outClientPtr = (ClientVar*)outClient;
    string fileName = outClientPtr->baseMergePair[0].first;
    
    std::ofstream file(fileName.c_str(), std::ios::app);
    file.write((char*)containerBody, currentSize);
    // tool::Logging(myName_.c_str(), "write size: %u, write success.\n", currentSize);
    file.close();

    if (unlink(outClientPtr->baseMergePair[0].second.c_str()) == 0) {
        tool::Logging(myName_.c_str(), "delete: %s success.\n", outClientPtr->baseMergePair[0].second.c_str());
    } 
    else {
        tool::Logging(myName_.c_str(), "delete: %s faled.\n", outClientPtr->baseMergePair[0].second.c_str());
    }

    outClientPtr->baseMergePair.erase(outClientPtr->baseMergePair.begin());

    return ;
}