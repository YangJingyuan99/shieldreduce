/**
 * @file clientVar.cc
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief implement the interface of client var
 * @version 0.1
 * @date 2023-04-24
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/clientVar.h"

int con_times = 1;

/**
 * @brief Construct a new ClientVar object
 * 
 * @param clientID the client ID
 * @param clientSSL the client SSL
 * @param optType the operation type (upload / download)
 * @param recipePath the file recipe path
 */
ClientVar::ClientVar(uint32_t clientID, SSL* clientSSL, 
    int optType, string& recipePath) {
    // basic info
    _clientID = clientID;
    _clientSSL = clientSSL;
    optType_ = optType;
    recipePath_ = recipePath;
    myName_ = myName_ + "-" + to_string(_clientID);

    // config
    sendChunkBatchSize_ = config.GetSendChunkBatchSize();
    sendRecipeBatchSize_ = config.GetSendRecipeBatchSize();

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
            tool::Logging(myName_.c_str(), "wrong client opt type.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Destroy the Client Var object
 * 
 */
ClientVar::~ClientVar() {
    switch (optType_) {
        case UPLOAD_OPT: {
            this->DestroyUploadBuffer();
            break;
        }
        case DOWNLOAD_OPT: {
            this->DestroyRestoreBuffer();
            break;
        }
    }
}

/**
 * @brief init the upload buffer
 * 
 */
void ClientVar::InitUploadBuffer() {
    // assign a random id to the container
    tool::CreateUUID(_curContainer.containerID, CONTAINER_ID_LENGTH,con_times);
    _curContainer.currentSize = 0;
    con_times++;

    tool::CreateUUID(_curDeltaContainer.containerID, CONTAINER_ID_LENGTH,con_times);
    _curDeltaContainer.currentSize = 0;
    con_times++;

    // for querying outside index 
    _outQuery.outQueryBase = (OutQueryEntry_t*) malloc(sizeof(OutQueryEntry_t) * 
        sendChunkBatchSize_);
    _outQuery.queryNum = 0;
    _outQuery.currNum = 0;

    _process_buffer = (uint8_t*)malloc(100000 * CHUNK_HASH_SIZE);
    _out_buffer = (uint8_t*)malloc(4000*CHUNK_HASH_SIZE);
    _test_buffer = (uint8_t*)malloc(8000*CHUNK_HASH_SIZE);
    _deltaInfo.QueryNum = 0;

    // init the recv buffer
    _recvChunkBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * sizeof(Chunk_t));
    _recvChunkBuf.header = (NetworkHead_t*) _recvChunkBuf.sendBuffer;
    _recvChunkBuf.header->clientID = _clientID;
    _recvChunkBuf.header->dataSize = 0;
    _recvChunkBuf.dataBuffer = _recvChunkBuf.sendBuffer + sizeof(NetworkHead_t);

    // prepare the input MQ
#if (MULTI_CLIENT == 1)
    _inputMQ = new MessageQueue<Container_t>(1);
#else
    _inputMQ = new MessageQueue<Container_t>(CONTAINER_QUEUE_SIZE);
#endif
    // prepare the ciphertext recipe buffer
    _outRecipe.entryFpList = (uint8_t*) malloc(sendRecipeBatchSize_ * 
        CHUNK_HASH_SIZE);
    _outRecipe.recipeNum = 0;

    // build the param passed to the enclave
    _upOutSGX.curContainer = &_curContainer;
    _upOutSGX.curDeltaContainer = &_curDeltaContainer;
    _upOutSGX.outRecipe = &_outRecipe;
    _upOutSGX.outQuery = &_outQuery;
    _upOutSGX.outClient = this;

    _upOutSGX.process_buffer = _process_buffer;
    _upOutSGX.out_buffer = _out_buffer;
    _upOutSGX.test_buffer = _test_buffer;
    _upOutSGX.deltaInfo = &_deltaInfo;

    _upOutSGX.outcallcontainer = _outCallcontainer;

    // for offline
    // _mergeContainerBuffer = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    // _upOutSGX.mergeContainerBuffer = _mergeContainerBuffer;
    _upOutSGX.mergeContainer = &_mergeContianer;
    
    // init the file recipe
    _recipeWriteHandler.open(recipePath_, ios_base::trunc | ios_base::binary);
    if (!_recipeWriteHandler.is_open()) {
        tool::Logging(myName_.c_str(), "cannot init recipe file: %s\n",
            recipePath_.c_str());
        exit(EXIT_FAILURE);
    }
    FileRecipeHead_t virtualRecipeEnd;
    _recipeWriteHandler.write((char*)&virtualRecipeEnd, sizeof(FileRecipeHead_t));

    return ;
}

/**
 * @brief destroy the upload buffer
 * 
 */
void ClientVar::DestroyUploadBuffer() {
    if (_recipeWriteHandler.is_open()) {
        _recipeWriteHandler.close();
    }
    free(_outRecipe.entryFpList);
    free(_outQuery.outQueryBase);
    free(_recvChunkBuf.sendBuffer);
    free(_process_buffer);
    free(_out_buffer);
    free(_test_buffer);
    // free(_mergeContainerBuffer);
    delete _inputMQ;
    return ;
}

/**
 * @brief init the restore buffer
 * 
 */
void ClientVar::InitRestoreBuffer() {
    // init buffer    
    // wj: 修改为CHUNK_HASH_SIZE
    _readRecipeBuf = (uint8_t*) malloc(sendRecipeBatchSize_ * CHUNK_HASH_SIZE);
    _reqContainer.idBuffer = (uint8_t*) malloc(CONTAINER_CAPPING_VALUE * 
        CONTAINER_ID_LENGTH);
    _reqContainer.containerArray = (uint8_t**) malloc(CONTAINERARRAY_VALUE * 
        sizeof(uint8_t*));
    _reqContainer.idNum = 0;
    for (size_t i = 0; i < CONTAINERARRAY_VALUE; i++) {
        _reqContainer.containerArray[i] = (uint8_t*) malloc(sizeof(uint8_t) * 
            MAX_CONTAINER_SIZE);
    }


    //wrl-2020.12.21
    _baseoutQuery.outQueryBase = (OutQueryEntry_t*) malloc(sizeof(OutQueryEntry_t) * 
        5);
    _baseoutQuery.queryNum = 0;

    //wj: init one container buffer 
    _reqOneContainer.id = (uint8_t*) malloc(CONTAINER_ID_LENGTH);
    _reqOneContainer.container = (uint8_t*)malloc(MAX_CONTAINER_SIZE);


    // init the send buffer
    _sendChunkBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + 
        sendChunkBatchSize_ * sizeof(Chunk_t));
    _sendChunkBuf.header = (NetworkHead_t*) _sendChunkBuf.sendBuffer;
    _sendChunkBuf.header->clientID = _clientID;
    _sendChunkBuf.header->dataSize = 0;
    _sendChunkBuf.dataBuffer = _sendChunkBuf.sendBuffer + sizeof(NetworkHead_t);
    // _tmpBatchQueryBufferStr = (uint8_t*)malloc(2*sendRecipeBatchSize_*sizeof(RecipeEntry_t));
    // memset(_tmpBatchQueryBufferStr, 0 , 2*sendRecipeBatchSize_*sizeof(RecipeEntry_t));

    // init the container cache
    _containerCache = new ReadCache();

    // build the param passed to the enclave
    _resOutSGX.reqContainer = &_reqContainer;
    _resOutSGX.sendChunkBuf = &_sendChunkBuf;
    _resOutSGX.outClient = this;
    _resOutSGX.baseQuery = &_baseoutQuery;
    _resOutSGX.reqOneContainer = &_reqOneContainer;

    // init the recipe handler
    _recipeReadHandler.open(recipePath_, ios_base::in | ios_base::binary);
    if (!_recipeReadHandler.is_open()) {
        tool::Logging(myName_.c_str(), "cannot init the file recipe: %s.\n",
            recipePath_.c_str());
        exit(EXIT_FAILURE);
    }
    return ;
}

/**
 * @brief destroy the restore buffer
 * 
 */
void ClientVar::DestroyRestoreBuffer() {
    tool::Logging(myName_.c_str(), "DestroyRestoreBuffer start.\n");
    if (_recipeReadHandler.is_open()) {
        _recipeReadHandler.close();
    }
    tool::Logging(myName_.c_str(), "_recipeReadHandler.close() over.\n");
    free(_sendChunkBuf.sendBuffer);
    tool::Logging(myName_.c_str(), "free(_sendChunkBuf.sendBuffer); over.\n");
    free(_readRecipeBuf);
    tool::Logging(myName_.c_str(), "free(_readRecipeBuf); over.\n");
    free(_reqContainer.idBuffer);
    tool::Logging(myName_.c_str(), "free(_reqContainer.idBuffer); over.\n");
    for (size_t i = 0; i < CONTAINER_CAPPING_VALUE; i++) {
        free(_reqContainer.containerArray[i]);
    }
    tool::Logging(myName_.c_str(), "free(_reqContainer.containerArray[i]); over.\n");
    free(_reqContainer.containerArray);
    tool::Logging(myName_.c_str(), "free(_reqContainer.containerArray); over.\n");
    free(_baseoutQuery.outQueryBase);
    tool::Logging(myName_.c_str(), "free(_baseoutQuery.outQueryBase); over.\n");
    // wj: free query one container buffer
    free(_reqOneContainer.id);
    tool::Logging(myName_.c_str(), "free(_reqOneContainer.id); over.\n");
    free(_reqOneContainer.container);
    tool::Logging(myName_.c_str(), "free(_reqOneContainer.container); over.\n");
    //free(_tmpBatchQueryBufferStr);
    delete _containerCache;
    tool::Logging(myName_.c_str(), "delete _containerCache; over.\n");
    return ;
}