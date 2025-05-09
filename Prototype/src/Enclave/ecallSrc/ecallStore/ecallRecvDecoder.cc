/**
 * @file ecallRecvDecoder.cc
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief implement the interface of enclave-based recv decoder
 * @version 0.1
 * @date 2023-03-02
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/ecallRecvDecoder.h"

int test2 = true;

int delta_num = 0;
int delta_rec_num = 0;
int total_datasize = 0;
int total_deltasize = 0;
int lz4_flag = 0;
int recipe_num = 0;

size_t delta_num_0 =0 ;
vector<string> Container_list;

/**
 * @brief Construct a new EcallRecvDecoder object
 * 
 */
EcallRecvDecoder::EcallRecvDecoder() {
    cryptoObj_ = new EcallCrypto(CIPHER_TYPE, HASH_TYPE);
    Enclave::Logging(myName_.c_str(), "init the RecvDecoder.\n");
}



/**
 * @brief Destroy the Ecall Recv Decoder object
 * 
 */
EcallRecvDecoder::~EcallRecvDecoder() {
   /*  Enclave::Logging(myName_.c_str(), "========EcallFreqIndex Info-Restore========\n");
    Enclave::Logging(myName_.c_str(), "total restore_time: %lu\n", _restoretime);
    Enclave::Logging(myName_.c_str(), "total delta restore_time: %lu\n", _deltarestoretime);
    Enclave::Logging(myName_.c_str(), "===================================\n"); */
    delete(cryptoObj_);
}

/**
 * @brief Use Xdelta3 method for delta decompression
 *
 * @param in the pointer of the delta chunk content
 * @param in_size the size of delta chunk
 * @param ref the pointer of the basechunk content
 * @param ref_size the size of basechunk
 * @param res_size the size of restore chunk
 * 
 * @return the pointer of the restore chunk
 */
uint8_t *EcallRecvDecoder::xd3_decode(const uint8_t *in, size_t in_size, const uint8_t *ref, size_t ref_size, size_t *res_size) //更改函数
{
    const auto max_buffer_size = (in_size + ref_size) * 2;
    uint8_t *buffer;
    buffer = (uint8_t *)malloc(max_buffer_size);
    size_t sz;
    xd3_decode_memory(in, in_size, ref, ref_size, buffer, &sz, max_buffer_size, 0);
    uint8_t *res;
    res = (uint8_t *)malloc(sz);
    *res_size = sz;
    memcpy(res, buffer, sz);
    free(buffer);
    return res;
}

uint8_t *EcallRecvDecoder::ed3_decode(uint8_t *in, size_t in_size, uint8_t *ref, size_t ref_size, size_t *res_size) //更改函数
{
    const auto max_buffer_size = (in_size + ref_size) * 2;
    uint8_t *buffer;
    buffer = (uint8_t *)malloc(max_buffer_size);
    size_t sz;
    uint32_t res32_;
    EDeltaDecode(in, in_size, ref, ref_size, buffer, &res32_);
    sz = res32_;
    uint8_t *res;
    res = (uint8_t *)malloc(sz);
    *res_size = sz;
    memcpy(res, buffer, sz);
    free(buffer);
    return res;
}

u_int32_t EcallRecvDecoder::get_flag(void *record) {
  u_int32_t *flag_length = (u_int32_t *)record;
  return (*flag_length) & (u_int32_t)1 << 31;
}

uint32_t EcallRecvDecoder::get_length(void *record) {
  uint32_t *flag_length = (uint32_t *)record;
  return (*flag_length) & ~(uint32_t)0 >> 1;
}

int EcallRecvDecoder::EDeltaDecode(uint8_t *deltaBuf, uint32_t deltaSize, uint8_t *baseBuf,
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


/**
 * @brief process a batch of recipes and write chunk to the outside buffer
 * 
 * @param recipeBuffer the pointer to the recipe buffer
 * @param recipeNum the input recipe buffer
 * @param resOutSGX the pointer to the out-enclave var
 * 
 * @return size_t the size of the sended buffer
 */
void EcallRecvDecoder::ProcRecipeBatch(uint8_t* recipeBuffer, size_t recipeNum, 
    ResOutSGX_t* resOutSGX) {
    // out-enclave info
    Ocall_GetCurrentTime(&_starttime);
    ReqContainer_t* reqContainer = (ReqContainer_t*)resOutSGX->reqContainer;
    uint8_t* idBuffer = reqContainer->idBuffer;
    uint8_t** containerArray = reqContainer->containerArray;
    SendMsgBuffer_t* sendChunkBuf = resOutSGX->sendChunkBuf;

    OutQueryEntry_t *baseQueryBase = resOutSGX->baseQuery->outQueryBase;
    OutQueryEntry_t *baseQueryEntry = baseQueryBase;
    uint32_t total_batch_size = 0;
    for(int i = 0;i<resOutSGX->baseQuery->queryNum;i++){
        baseQueryEntry++;
    }
    // wj: test get one container
    // ReqOneContainer_t * reqOneContainer =  (ReqOneContainer_t*)resOutSGX->reqOneContainer;
    // uint8_t* ContainerId = reqOneContainer->id;
 


    OutQueryEntry_t *tmpEntry;
    tmpEntry = (OutQueryEntry_t *)malloc(sizeof(OutQueryEntry_t));

    // in-enclave info
    EnclaveClient* sgxClient = (EnclaveClient*)resOutSGX->sgxClient;
    SendMsgBuffer_t* restoreChunkBuf = &sgxClient->_restoreChunkBuffer;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    uint8_t* sessionKey = sgxClient->_sessionKey;
    uint8_t* masterKey = sgxClient->_masterKey;

    string tmpContainerIDStr;
    string tmpBaseContainerIDStr;
    unordered_map<string, uint32_t> tmpContainerMap;
    tmpContainerMap.reserve(CONTAINER_CAPPING_VALUE);
    EnclaveRecipeEntry_t tmpEnclaveRecipeEntry;
    EnclaveRecipeEntry_t tmpEnclaveBaseRecipeEntry;

    // decrypt the recipe file
    // wj: 改为recipeNum * CHUNK_HASH_SIZE
    //Enclave::Logging("recvDecoder", "start restore\n");
    uint8_t* tmpFpBuffer = (uint8_t*)malloc(recipeNum * CHUNK_HASH_SIZE);
    cryptoObj_->DecryptWithKey(cipherCtx, recipeBuffer, recipeNum * CHUNK_HASH_SIZE,
                               masterKey, tmpFpBuffer);
    char *tmpRecipeEntryFp;
    tmpRecipeEntryFp = (char *)&tmpFpBuffer[0];
   

    // wj: batch query Fp Index
    bool status;
    uint8_t *fileRecipes;
    size_t expectedBufferSize = 0;
    // uint8_t* tmpCryptChunkHash = (uint8_t*)malloc(recipeNum * CHUNK_HASH_SIZE);
    // uint8_t* tmpEncFpValue = (uint8_t*)malloc(CHUNK_HASH_SIZE);
    string tmpCryptChunkHash;
    string tmpEncFpValue;

    uint8_t tmphash[CHUNK_HASH_SIZE];
  
    for(size_t i = 0; i < recipeNum; i++)
    {
        tmpEncFpValue.assign("0", CHUNK_HASH_SIZE);
        //Ocall_PrintfBinary((uint8_t*)tmpRecipeEntryFp,CHUNK_HASH_SIZE);
        cryptoObj_->IndexAESCMCEnc(cipherCtx, (uint8_t*)tmpRecipeEntryFp, CHUNK_HASH_SIZE, Enclave::indexQueryKey_, (uint8_t*)&tmpEncFpValue[0]);

        //Ocall_PrintfBinary((uint8_t*)&tmpEncFpValue[0],CHUNK_HASH_SIZE);
        tmpCryptChunkHash += tmpEncFpValue;
        tmpRecipeEntryFp += CHUNK_HASH_SIZE;
    }
    
   
    // Ocall batch query file recipes
    //Ocall_ReadIndexStoreBatch(&status, (char*)&tmpCryptChunkHash[0], recipeNum * CHUNK_HASH_SIZE, &fileRecipes, &expectedBufferSize, resOutSGX->outClient);
    Ocall_ReadIndexStoreBatch(&status, tmpCryptChunkHash.c_str(), recipeNum * CHUNK_HASH_SIZE, &fileRecipes, &expectedBufferSize, resOutSGX->outClient);

    //Enclave::Logging("DEBUG", "Ocall_recipenum is %d\n", expectedBufferSize/sizeof(RecipeEntry_t));
    //Enclave::Logging("DEBUG", "recipenum is %d\n", recipeNum);
    
    uint8_t* tmpEncValue = (uint8_t*)malloc(sizeof(RecipeEntry_t));
    uint8_t* tmpDecValue = (uint8_t*)malloc(sizeof(RecipeEntry_t));
    //Enclave::Logging("DEBUG", "recipeNum is %d\n", recipeNum);

    RecipeEntry_t* tmpRecipeEntry;
    //Enclave::Logging("DEBUG", "Outside recipeNum is %d\n", recipeNum);
    for (size_t i = 0; i < recipeNum; i++) {
        if(i%100 == 0){
        //Enclave::Logging(myName_.c_str(), "restore %d chunk\n",i);
        }
        //parse the recipe entry one-by-one
        // uint8_t* value;
        // Ocall_ReadIndexStore(&status, (char*)&tmpCryptChunkHash[i*CHUNK_HASH_SIZE], CHUNK_HASH_SIZE, &value, &expectedBufferSize, resOutSGX->outClient);
        
        memset(tmpEncValue, 0, sizeof(RecipeEntry_t));
        memset(tmpDecValue, 0, sizeof(RecipeEntry_t));
            
        memcpy(tmpEncValue, fileRecipes + i * sizeof(RecipeEntry_t), sizeof(RecipeEntry_t));
        //Enclave::Logging("debug", "decrypt3\n");
        cryptoObj_->AESCBCDec(cipherCtx, tmpEncValue, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, tmpDecValue);
        //Enclave::Logging("debug", "decrypt4\n");
        tmpRecipeEntry = (RecipeEntry_t*)tmpDecValue;

        tmpContainerIDStr.assign((char*)tmpRecipeEntry->containerName, CONTAINER_ID_LENGTH);

        

        tmpEnclaveRecipeEntry.offset = tmpRecipeEntry->offset;
        tmpEnclaveRecipeEntry.length = tmpRecipeEntry->length;
        tmpEnclaveRecipeEntry.deltaFlag = tmpRecipeEntry->deltaFlag;
        if(tmpEnclaveRecipeEntry.deltaFlag == DELTA){
            //if chunk is a delta chunk, save basechunk fp to basequeryentry(out-enclave)
            delta_num_0++;
            if(resOutSGX->baseQuery->queryNum <  5){
                memcpy(tmpEnclaveRecipeEntry.basechunkHash,tmpRecipeEntry->basechunkHash,CHUNK_HASH_SIZE);
                resOutSGX->baseQuery->queryNum++;
                //Enclave::Logging("debug", "squery num is %d and delta num is %d\n", resOutSGX->baseQuery->queryNum, delta_num_0);
                memcpy((uint8_t *)&baseQueryEntry->chunkHash,(uint8_t *)&tmpRecipeEntry->basechunkHash,CHUNK_HASH_SIZE);
                baseQueryEntry++;
                delta_rec_num++;
            } else if(resOutSGX->baseQuery->queryNum == 5)
            {
                Ocall_QueryBaseIndex(resOutSGX->outClient);
                baseQueryEntry = baseQueryBase;
                for(int i = 0;i<resOutSGX->baseQuery->queryNum;i++){
                    //decrypto basechunk recipe
                    //Enclave::Logging("debug", "decrypt0\n");
                    cryptoObj_->AESCBCDec(cipherCtx, (uint8_t *)&baseQueryEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t *)&tmpEntry->chunkAddr);
                    //Enclave::Logging("debug", "decrypt1\n");
                    tmpEnclaveBaseRecipeEntry.offset = tmpEntry->chunkAddr.offset;
                    tmpEnclaveBaseRecipeEntry.length = tmpEntry->chunkAddr.length;
                    tmpEnclaveBaseRecipeEntry.deltaFlag = tmpEntry->chunkAddr.deltaFlag;
                    tmpBaseContainerIDStr.assign((char*)tmpEntry->chunkAddr.containerName, CONTAINER_ID_LENGTH);
                    auto findResult = tmpContainerMap.find(tmpBaseContainerIDStr);
                    if(findResult == tmpContainerMap.end())
                    {
                        tmpEnclaveBaseRecipeEntry.containerID = reqContainer->idNum;
                        tmpContainerMap[tmpBaseContainerIDStr] = reqContainer->idNum;
                        memcpy(idBuffer + reqContainer->idNum * CONTAINER_ID_LENGTH, 
                                tmpBaseContainerIDStr.c_str(), CONTAINER_ID_LENGTH);
                        reqContainer->idNum++;
                    } else 
                    {
                        tmpEnclaveBaseRecipeEntry.containerID = findResult->second;
                    }
                    sgxClient->_baseRecipeBuffer.push_back(tmpEnclaveBaseRecipeEntry);
                    //Enclave::Logging("debug","a base chunk buffer size is %d\n", sgxClient->_baseRecipeBuffer.size());
                    baseQueryEntry++;
                }
                baseQueryEntry = baseQueryBase;
                //Enclave::Logging("debug", "aquery num is %d and delta num is %d\n", resOutSGX->baseQuery->queryNum, delta_num_0);
    
                memcpy(tmpEnclaveRecipeEntry.basechunkHash,tmpRecipeEntry->basechunkHash,CHUNK_HASH_SIZE);
                //Enclave::Logging("debug", "squery num is %d and delta num is %d\n", resOutSGX->baseQuery->queryNum, delta_num_0);
                memcpy((uint8_t *)&baseQueryEntry->chunkHash,(uint8_t *)&tmpRecipeEntry->basechunkHash,CHUNK_HASH_SIZE);
                baseQueryEntry++;
                delta_rec_num++;
                resOutSGX->baseQuery->queryNum = 1;
            }
            //Enclave::Logging("recvDecoder", "offset is %d\n", tmpEnclaveRecipeEntry.offset);
        }

        //1.ocall 2.baserecipe buffer 3. idnum++
        
        auto findResult = tmpContainerMap.find(tmpContainerIDStr);
        if (findResult == tmpContainerMap.end()) {
            // this is a unique container entry, it does not exist in current local index
            tmpEnclaveRecipeEntry.containerID = reqContainer->idNum;
            tmpContainerMap[tmpContainerIDStr] = reqContainer->idNum;
            
            memcpy(idBuffer + reqContainer->idNum * CONTAINER_ID_LENGTH, 
                tmpContainerIDStr.c_str(), CONTAINER_ID_LENGTH);
            reqContainer->idNum++;
        } else {
            // this is a duplicate container entry, using existing result.
            tmpEnclaveRecipeEntry.containerID = findResult->second;
        }

        sgxClient->_enclaveRecipeBuffer.push_back(tmpEnclaveRecipeEntry);
        
        Container_list.push_back(tmpContainerIDStr);

        // judge whether reach the capping value 
        //Enclave::Logging("debug", "id num is %d and query num is %d\n",reqContainer->idNum, resOutSGX->baseQuery->queryNum);
        if ((reqContainer->idNum + resOutSGX->baseQuery->queryNum ) >= CONTAINER_CAPPING_VALUE) {

            // start to let outside application to fetch the container data
            //Enclave::Logging("debug", "reach capping value\n");
            if(resOutSGX->baseQuery->queryNum != 0)
            {
                Ocall_QueryBaseIndex(resOutSGX->outClient);
                baseQueryEntry = baseQueryBase;
                //Enclave::Logging("debug", "query num is %d\n", resOutSGX->baseQuery->queryNum);
                for(int i = 0;i<resOutSGX->baseQuery->queryNum;i++){
                    //decrypto basechunk recipe
                    //Enclave::Logging("debug", "decrypt0\n");
                    cryptoObj_->AESCBCDec(cipherCtx, (uint8_t *)&baseQueryEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t *)&tmpEntry->chunkAddr);
                    //Enclave::Logging("debug", "decrypt1\n");
                    tmpEnclaveBaseRecipeEntry.offset = tmpEntry->chunkAddr.offset;
                    tmpEnclaveBaseRecipeEntry.length = tmpEntry->chunkAddr.length;
                    tmpEnclaveBaseRecipeEntry.deltaFlag = tmpEntry->chunkAddr.deltaFlag;
                    tmpBaseContainerIDStr.assign((char*)tmpEntry->chunkAddr.containerName, CONTAINER_ID_LENGTH);
                    auto findResult = tmpContainerMap.find(tmpBaseContainerIDStr);
                    if(findResult == tmpContainerMap.end())
                    {
                        tmpEnclaveBaseRecipeEntry.containerID = reqContainer->idNum;
                        tmpContainerMap[tmpBaseContainerIDStr] = reqContainer->idNum;
                        memcpy(idBuffer + reqContainer->idNum * CONTAINER_ID_LENGTH, 
                                tmpBaseContainerIDStr.c_str(), CONTAINER_ID_LENGTH);
                        reqContainer->idNum++;
                    } else 
                    {
                        tmpEnclaveBaseRecipeEntry.containerID = findResult->second;
                    }
                    sgxClient->_baseRecipeBuffer.push_back(tmpEnclaveBaseRecipeEntry);
                    //Enclave::Logging("debug","b base chunk buffer size is %d\n", sgxClient->_baseRecipeBuffer.size());
                    baseQueryEntry++;
                }
                baseQueryEntry = baseQueryBase;
                resOutSGX->baseQuery->queryNum = 0;
            }

            Ocall_GetReqContainers(resOutSGX->outClient);
            size_t bidx = 0;
            for (size_t idx = 0; idx < sgxClient->_enclaveRecipeBuffer.size(); idx++) {
                //Enclave::Logging("debug", "idx is %d\n", idx);
                uint32_t containerID = sgxClient->_enclaveRecipeBuffer[idx].containerID;
                uint32_t offset = sgxClient->_enclaveRecipeBuffer[idx].offset;
                uint32_t chunkSize = sgxClient->_enclaveRecipeBuffer[idx].length;
                uint8_t deltaflag = sgxClient->_enclaveRecipeBuffer[idx].deltaFlag;
                uint8_t* chunkBuffer = containerArray[containerID] + offset + sizeof(RecipeEntry_t) + 4*CHUNK_HASH_SIZE;
               // Enclave::Logging("debug", "start process delta\n");
                if(deltaflag == DELTA){
                    //Enclave::Logging("debug", "delta chunk\n");
                    Ocall_GetCurrentTime(&_starttime1);
                    //restore delta chunk
                    uint8_t* iv = chunkBuffer+chunkSize;
                    uint8_t* outputBuffer = restoreChunkBuf->dataBuffer + restoreChunkBuf->header->dataSize;
                    uint8_t decompressedChunk[MAX_CHUNK_SIZE];
                    //decrypto the delta chunk with iv_key
                    //Enclave::Logging("debug", "start decrypt delta chunk\n");
                    cryptoObj_->DecryptionWithKeyIV(cipherCtx, chunkBuffer, chunkSize, 
                        Enclave::enclaveKey_, decompressedChunk, iv);
                    //Enclave::Logging("debug", "decrypt delta chunk end\n");
                    
                    uint8_t decompressbaseChunk[MAX_CHUNK_SIZE];
                    //Enclave::Logging("debug","c base chunk buffer size is %d\n", sgxClient->_baseRecipeBuffer.size());
                    //Enclave::Logging("debug", "bidx is %d\n", bidx);
                    //uint8_t* basechunkBuffer = baseQueryEntry->containerbuffer+baseQueryEntry->chunkAddr.offset+sizeof(RecipeEntry_t) + 4*CHUNK_HASH_SIZE;
                    //Enclave::Logging("debug", "1\n");
                    uint32_t baseContainerId = sgxClient->_baseRecipeBuffer[bidx].containerID;
                    //Enclave::Logging("debug", "container id is %d\n", sgxClient->_baseRecipeBuffer[bidx].containerID);
                    //Enclave::Logging("debug", "2\n");
                    uint32_t baseChunkOffset = sgxClient->_baseRecipeBuffer[bidx].offset;
                    //Enclave::Logging("debug", "3\n");
                    uint32_t baseChunkLength = sgxClient->_baseRecipeBuffer[bidx].length;
                    //Enclave::Logging("debug", "4\n");
                    uint8_t* basechunkBuffer = containerArray[baseContainerId] + baseChunkOffset + sizeof(RecipeEntry_t)+4*CHUNK_HASH_SIZE;
                    //uint8_t* basechunkBuffer = containerArray[baseContainerId];
                    //Enclave::Logging("debug", "5\n");
                    iv = basechunkBuffer + baseChunkLength;
                    //Enclave::Logging("debug", "6\n");
                    bidx++;
                    //decrpto the base chunk with iv_key  
                    //Enclave::Logging("debug", "start decrypt base chunk\n");
                    cryptoObj_->DecryptionWithKeyIV(cipherCtx, basechunkBuffer, baseChunkLength, 
                        Enclave::enclaveKey_, decompressbaseChunk, iv);
                    //Enclave::Logging("debug", "decrypt base chunk end\n");
                    uint8_t lz4decompressbaseChunk[MAX_CHUNK_SIZE];
                    uint8_t *recchunk;
                    size_t recchunk_size;
                    //decompress basechunk with lz4
                    int decompressedSize = LZ4_decompress_safe((char*)decompressbaseChunk, (char*)lz4decompressbaseChunk,baseChunkLength, MAX_CHUNK_SIZE);
                    if(decompressedSize > 0){
                    //base chunk can decompress, use decompressed chunk as basechunk
                    //Enclave::Logging("debug", "xdelta decode start\n");
                    recchunk = ed3_decode((uint8_t*)&decompressedChunk, chunkSize,(uint8_t*)&lz4decompressbaseChunk, decompressedSize, &recchunk_size);
                    }else{
                    //base chunk can decompress, use it as basechunk
                    recchunk = ed3_decode((uint8_t*)&decompressedChunk, chunkSize,(uint8_t*)&decompressbaseChunk, baseChunkLength, &recchunk_size);
                    }
                    delta_num++;
                    //Enclave::Logging("debug", "xdelta decode end\n");
                    if(recchunk_size == 0){
                        Enclave::Logging(myName_.c_str(), "recchunk chunk size: %d\n", recchunk_size);
                        Enclave::Logging(myName_.c_str(), "offset: %d\n", offset);
                        Enclave::Logging(myName_.c_str(), "container id: %d\n", containerID);
                    }
                    //Enclave::Logging("debug", "xrestore succeed\n");


                    //
                    //Enclave::Logging(myName_.c_str(), "recchunk chunk num: %d\n", delta_num);
                    memcpy(outputBuffer, &recchunk_size, sizeof(uint32_t));
                    memcpy(outputBuffer + sizeof(uint32_t), recchunk, recchunk_size);
                    restoreChunkBuf->header->dataSize += sizeof(uint32_t) + recchunk_size;
                    total_batch_size += recchunk_size;
                    total_datasize = recchunk_size+total_datasize;
                    total_deltasize = recchunk_size+total_deltasize;
                    free(recchunk);
                    //Enclave::Logging("debug", "memcpy done\n");
             
                    restoreChunkBuf->header->currentItemNum++;
                    Ocall_GetCurrentTime(&_endtime1);
                    _deltarestoretime += _endtime1 - _starttime1;
                }else{
                    //restore unique chunk
                    this->RecoverOneChunk(chunkBuffer, chunkSize, restoreChunkBuf, cipherCtx);
                    if(lz4_flag == 1){
                        tmpContainerIDStr.assign((char*)idBuffer + containerID * CONTAINER_ID_LENGTH, CONTAINER_ID_LENGTH);
  
                        for(int i = 0;i<reqContainer->idNum;i++){
                            tmpContainerIDStr.assign((char*)idBuffer + i * CONTAINER_ID_LENGTH, CONTAINER_ID_LENGTH);

                        }
                    }
                    lz4_flag = 0;

                }

                recipe_num++;

                
                //Enclave::Logging("debug", "start sending data\n");
                if (restoreChunkBuf->header->currentItemNum % 
                    Enclave::sendChunkBatchSize_ == 0) {
                    cryptoObj_->SessionKeyEnc(cipherCtx, restoreChunkBuf->dataBuffer,
                        restoreChunkBuf->header->dataSize, sessionKey, sendChunkBuf->dataBuffer);
                    
                    // copy the header to the send buffer
                    restoreChunkBuf->header->messageType = SERVER_RESTORE_CHUNK;
                    memcpy(sendChunkBuf->header, restoreChunkBuf->header, sizeof(NetworkHead_t));
                    //Enclave::Logging("DEBUG", "processOneBatch buf size is %d\n", restoreChunkBuf->header->dataSize);
                    Ocall_SendRestoreData(resOutSGX->outClient);
                    restoreChunkBuf->header->dataSize = 0;
                    restoreChunkBuf->header->currentItemNum = 0;
                }
                //Enclave::Logging("debug", "sending data end\n");
            }
            

            // reset 
            Ocall_FreeContainer(resOutSGX->outClient);
            reqContainer->idNum = 0;
            //Enclave::Logging(myName_.c_str(), "a id Num is %d\n", reqContainer->idNum);
            tmpContainerMap.clear();
            sgxClient->_enclaveRecipeBuffer.clear();
            sgxClient->_baseRecipeBuffer.clear();
            baseQueryEntry = baseQueryBase;
            resOutSGX->baseQuery->queryNum = 0;
            Container_list.clear();
        }
    }
    free(tmpEntry);
    //Enclave::Logging(myName_.c_str(), "batch end\n");
    //Enclave::Logging(myName_.c_str(), "b id Num is %d\n", reqContainer->idNum);
    Ocall_GetCurrentTime(&_endtime);
    _restoretime += _endtime - _starttime;
    free(tmpFpBuffer);
    free(tmpDecValue);
    free(tmpEncValue);
   // free(fileRecipes);
    //free(tmpEncFpValue);
    //free(tmpCryptChunkHash);
    return ;
}

/**
 * @brief process the tail batch of recipes
 * 
 * @param resOutSGX the pointer to the out-enclave var
 */
void EcallRecvDecoder::ProcRecipeTailBatch(ResOutSGX_t* resOutSGX) {
    // out-enclave info
    ReqContainer_t* reqContainer = (ReqContainer_t*)resOutSGX->reqContainer;
    if(reqContainer == NULL)
        Enclave::Logging("DEBUG", "reqContainer is NULLPTR\n");
    uint8_t** containerArray = reqContainer->containerArray;
    SendMsgBuffer_t* sendChunkBuf = resOutSGX->sendChunkBuf;
    uint8_t* idBuffer = reqContainer->idBuffer;

    // uint32_t myCounter = 0;
    OutQueryEntry_t *baseQueryBase = resOutSGX->baseQuery->outQueryBase;
    unordered_map<string, uint32_t> tmpContainerMap;
    string tmpContainerIDStr;
    OutQueryEntry_t *baseQueryEntry = baseQueryBase;
    OutQueryEntry_t *tmpEntry;
    tmpEntry = (OutQueryEntry_t *)malloc(sizeof(OutQueryEntry_t));
    for(int i = 0;i<resOutSGX->baseQuery->queryNum;i++){
        baseQueryEntry++;   
    }
    // in-enclave info
    EnclaveRecipeEntry_t tmpEnclaveBaseRecipeEntry;
    string tmpBaseContainerIDStr;
    EnclaveClient* sgxClient = (EnclaveClient*)resOutSGX->sgxClient;
    SendMsgBuffer_t* restoreChunkBuf = &sgxClient->_restoreChunkBuffer;
    EVP_CIPHER_CTX* cipherCtx = sgxClient->_cipherCtx;
    uint8_t* sessionKey = sgxClient->_sessionKey;
    //Enclave::Logging(myName_.c_str(), "recipe end\n");
    if (sgxClient->_enclaveRecipeBuffer.size() != 0) {
        // start to let outside application to fetch the container data
       // Ocall_GetReqContainers(resOutSGX->outClient);
       //Enclave::Logging("DEBUG", "query num is %d\n", resOutSGX->baseQuery->queryNum);
        if (resOutSGX->baseQuery->queryNum != 0)
        {
            Ocall_QueryBaseIndex(resOutSGX->outClient);
        }
        baseQueryEntry = baseQueryBase;
        for(int i = 0;i<resOutSGX->baseQuery->queryNum;i++){
            cryptoObj_->AESCBCDec(cipherCtx, (uint8_t *)&baseQueryEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t *)&tmpEntry->chunkAddr);
            baseQueryEntry->chunkAddr = tmpEntry->chunkAddr;
            tmpEnclaveBaseRecipeEntry.offset = tmpEntry->chunkAddr.offset;
            tmpEnclaveBaseRecipeEntry.length = tmpEntry->chunkAddr.length;
            tmpEnclaveBaseRecipeEntry.deltaFlag = tmpEntry->chunkAddr.deltaFlag;
            //Enclave::Logging(myName_.c_str(), "Base container:%s\n",baseQueryEntry->chunkAddr.containerName);
            tmpBaseContainerIDStr.assign((char*)tmpEntry->chunkAddr.containerName, CONTAINER_ID_LENGTH);
            auto findResult = tmpContainerMap.find(tmpBaseContainerIDStr);
            if(findResult == tmpContainerMap.end())
            {
                tmpEnclaveBaseRecipeEntry.containerID = reqContainer->idNum;
                tmpContainerMap[tmpBaseContainerIDStr] = reqContainer->idNum;
                memcpy(idBuffer + reqContainer->idNum * CONTAINER_ID_LENGTH, 
                        tmpBaseContainerIDStr.c_str(), CONTAINER_ID_LENGTH);
                reqContainer->idNum++;
            } else 
            {
                tmpEnclaveBaseRecipeEntry.containerID = findResult->second;
            }
            sgxClient->_baseRecipeBuffer.push_back(tmpEnclaveBaseRecipeEntry);
            baseQueryEntry++;
        }
        baseQueryEntry = baseQueryBase;
        resOutSGX->baseQuery->queryNum = 0;
        Ocall_GetReqContainers(resOutSGX->outClient);
       
        baseQueryEntry = baseQueryBase;
        uint32_t remainChunkNum = sgxClient->_enclaveRecipeBuffer.size();
        bool endFlag = 0;
        size_t bidx = 0;
        for (size_t idx = 0; idx < sgxClient->_enclaveRecipeBuffer.size(); idx++) {
            uint32_t containerID = sgxClient->_enclaveRecipeBuffer[idx].containerID;
            uint32_t offset = sgxClient->_enclaveRecipeBuffer[idx].offset;
            uint32_t chunkSize = sgxClient->_enclaveRecipeBuffer[idx].length;
            uint8_t deltaflag = sgxClient->_enclaveRecipeBuffer[idx].deltaFlag;
            uint8_t* chunkBuffer = containerArray[containerID] + offset + sizeof(RecipeEntry_t) + 4*CHUNK_HASH_SIZE;
            if(deltaflag == DELTA){
                //restore delta chunk
                    uint8_t* iv = chunkBuffer+chunkSize;
                    uint8_t* outputBuffer = restoreChunkBuf->dataBuffer + restoreChunkBuf->header->dataSize;
                    uint8_t decompressedChunk[MAX_CHUNK_SIZE];
                    //decrypto the delta chunk with iv_key
                    cryptoObj_->DecryptionWithKeyIV(cipherCtx, chunkBuffer, chunkSize, 
                    Enclave::enclaveKey_, decompressedChunk, iv);
                    uint8_t decompressbaseChunk[MAX_CHUNK_SIZE];
                    uint32_t baseContainerId = sgxClient->_baseRecipeBuffer[bidx].containerID;
                    uint32_t baseChunkOffset = sgxClient->_baseRecipeBuffer[bidx].offset;
                    uint32_t baseChunkLength = sgxClient->_baseRecipeBuffer[bidx].length;
                    uint8_t* basechunkBuffer = containerArray[baseContainerId] + baseChunkOffset + sizeof(RecipeEntry_t)+4*CHUNK_HASH_SIZE;
                    iv = basechunkBuffer + baseChunkLength;
                    bidx++;
                    //decrpto the base chunk with iv_key
                    cryptoObj_->DecryptionWithKeyIV(cipherCtx, basechunkBuffer, baseChunkLength, 
                    Enclave::enclaveKey_, decompressbaseChunk, iv);
                    uint8_t lz4decompressbaseChunk[MAX_CHUNK_SIZE];
                    uint8_t *recchunk;
                    size_t recchunk_size;
                    //decompress basechunk with lz4
                    int decompressedSize = LZ4_decompress_safe((char*)decompressbaseChunk, (char*)lz4decompressbaseChunk, baseChunkLength, MAX_CHUNK_SIZE);
                    if(decompressedSize > 0){
                    //base chunk can decompress, use decompressed chunk as basechunk
                    recchunk = ed3_decode((uint8_t*)&decompressedChunk, chunkSize,(uint8_t*)&lz4decompressbaseChunk, decompressedSize, &recchunk_size);
                    }else{
                    //base chunk can decompress, use it as basechunk
                    recchunk = ed3_decode((uint8_t*)&decompressedChunk, chunkSize,(uint8_t*)&decompressbaseChunk, baseChunkLength, &recchunk_size);
                    }
                    //Enclave::Logging(myName_.c_str(), "recchunk chunk size2: %d\n", recchunk_size);
                    //Enclave::Logging(myName_.c_str(), "recchunk chunk num2: %d\n", delta_num);
                    memcpy(outputBuffer, &recchunk_size, sizeof(uint32_t));
                    memcpy(outputBuffer + sizeof(uint32_t), recchunk, recchunk_size);
                    restoreChunkBuf->header->dataSize += sizeof(uint32_t) + recchunk_size;
                    total_datasize = recchunk_size+total_datasize;
                    total_deltasize = recchunk_size+total_deltasize;
                    //Enclave::Logging("DEBUG", "total data size is %d\n", total_data_size);
                    free(recchunk);
                    restoreChunkBuf->header->currentItemNum++;
                    remainChunkNum--;
                }else{
                    this->RecoverOneChunk(chunkBuffer, chunkSize, restoreChunkBuf, 
                    cipherCtx);
                    remainChunkNum--;   
            }
            if (remainChunkNum == 0) {
                // this is the last batch of chunks;
                endFlag = 1;
            }
            if ((restoreChunkBuf->header->currentItemNum % 
                Enclave::sendChunkBatchSize_ == 0) || endFlag) {
                cryptoObj_->SessionKeyEnc(cipherCtx, restoreChunkBuf->dataBuffer,
                    restoreChunkBuf->header->dataSize, sessionKey, 
                    sendChunkBuf->dataBuffer);

                // copy the header to the send buffer
                if (endFlag == 1) {
                    restoreChunkBuf->header->messageType = SERVER_RESTORE_FINAL;
                } else {
                    restoreChunkBuf->header->messageType = SERVER_RESTORE_CHUNK;
                }
                memcpy(sendChunkBuf->header, restoreChunkBuf->header, sizeof(NetworkHead_t));
                Ocall_SendRestoreData(resOutSGX->outClient);
                //Enclave::Logging("DEBUG", "processTailBatch buf size is %d\n", restoreChunkBuf->header->dataSize);

                restoreChunkBuf->header->dataSize = 0;
                restoreChunkBuf->header->currentItemNum = 0;
            }
        }
    } else {
        cryptoObj_->SessionKeyEnc(cipherCtx, restoreChunkBuf->dataBuffer,
            restoreChunkBuf->header->dataSize, sessionKey,
            sendChunkBuf->dataBuffer);

        // copy the header to the send buffer
        restoreChunkBuf->header->messageType = SERVER_RESTORE_FINAL;
        memcpy(sendChunkBuf->header, restoreChunkBuf->header, sizeof(NetworkHead_t));
        Ocall_SendRestoreData(resOutSGX->outClient);
        
        restoreChunkBuf->header->currentItemNum = 0;
        restoreChunkBuf->header->dataSize = 0;
    }
    recipe_num = 0;
    free(tmpEntry);
    return ;
}

/**
 * @brief recover a chunk
 * 
 * @param chunkBuffer the chunk buffer
 * @param chunkSize the chunk size
 * @param restoreChunkBuf the restore chunk buffer
 * @param cipherCtx the pointer to the EVP cipher
 * 
 */
void EcallRecvDecoder::RecoverOneChunk(uint8_t* chunkBuffer, uint32_t chunkSize, 
    SendMsgBuffer_t* restoreChunkBuf, EVP_CIPHER_CTX* cipherCtx) {
    uint8_t* iv = chunkBuffer + chunkSize; 
    uint8_t* outputBuffer = restoreChunkBuf->dataBuffer + 
        restoreChunkBuf->header->dataSize;
    uint8_t decompressedChunk[MAX_CHUNK_SIZE];
    
    // first decrypt the chunk first
    cryptoObj_->DecryptionWithKeyIV(cipherCtx, chunkBuffer, chunkSize, 
        Enclave::enclaveKey_, decompressedChunk, iv); 

    // try to decompress the chunk
    int decompressedSize = LZ4_decompress_safe((char*)decompressedChunk, 
        (char*)(outputBuffer + sizeof(uint32_t)), chunkSize, MAX_CHUNK_SIZE);
    if (decompressedSize > 0) {
        lz4_times++;
        // it can do the decompression, write back the decompressed chunk size
        memcpy(outputBuffer, &decompressedSize, sizeof(uint32_t));
        restoreChunkBuf->header->dataSize += sizeof(uint32_t) + decompressedSize; 
        total_datasize = decompressedSize+total_datasize;
    } else {
        lz4_flag = 1;
        // it cannot do the decompression
        memcpy(outputBuffer, &chunkSize, sizeof(uint32_t));
        memcpy(outputBuffer + sizeof(uint32_t), decompressedChunk, chunkSize);
        restoreChunkBuf->header->dataSize += sizeof(uint32_t) + chunkSize;
        total_datasize = chunkSize+total_datasize;
    }
   

    restoreChunkBuf->header->currentItemNum++;
    return ;
}

