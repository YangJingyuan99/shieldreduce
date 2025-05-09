/**
 * @file serverOptThread.cc
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief server main thread
 * @version 0.1
 * @date 2023-07-11
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/serverOptThead.h"

/**
 * @brief Construct a new Server Opt Thread object
 * 
 * @param dataSecureChannel data security communication channel
 * @param fp2ChunkDB the index
 * @param eidSGX sgx enclave id
 * @param indexType 
 */

uint32_t restoreId = 0;
uint32_t backupid = 0;
double total_online_time = 0.;
double total_offline_time = 0.;
double total_backupsize = 0;
double total_keyExchangeTime = 0;

ServerOptThread::ServerOptThread(SSLConnection* dataSecureChannel, 
    AbsDatabase* fp2ChunkDB, sgx_enclave_id_t eidSGX, int indexType) {
    dataSecureChannel_ = dataSecureChannel;
    fp2ChunkDB_ = fp2ChunkDB;
    eidSGX_ = eidSGX;
    indexType_ = indexType;        

    // init the upload
    dataWriterObj_ = new DataWriter();
    storageCoreObj_ = new StorageCore();
    absIndexObj_ = new EnclaveIndex(fp2ChunkDB_, indexType_, eidSGX_);
    absIndexObj_->SetStorageCoreObj(storageCoreObj_);
    dataReceiverObj_ = new DataReceiver(absIndexObj_, dataSecureChannel_, eidSGX_);
    dataReceiverObj_->SetStorageCoreObj(storageCoreObj_);

    // init the restore
    recvDecoderObj_ = new EnclaveRecvDecoder(dataSecureChannel_, 
        eidSGX_);

    // init the RA 
    raUtil_ = new RAUtil(dataSecureChannel_);

    // init the out-enclave var
    OutEnclave::Init(dataWriterObj_, fp2ChunkDB_, storageCoreObj_,
        recvDecoderObj_);

    // for log file
    if (!tool::FileExist(logFileName_)) {
        // if the log file not exist, add the header
        logFile_.open(logFileName_, ios_base::out);
        logFile_ << "BackupID, "
        <<"OnlineSpeed(MB/s), "
        <<"OnlineTime(s), "
        <<"AverageOnlineTime(s), "
        <<"BackupSize, "
        <<"OnlineSize, "
        <<"OfflineSize, "
        <<"OfflineTime(s), "
        <<"AverageOfflineTime(s), "
        <<"OverallReductionRatio"
        <<endl;
    } else {
        // the log file exists
        logFile_.open(logFileName_, ios_base::app | ios_base::out);
    }
    
    // for restore file
    if (!tool::FileExist(restoreFileName_)) {
        // if the log file not exist, add the header
        restoreFile_.open(restoreFileName_, ios_base::out);
        restoreFile_ << "BackID" << " Process Time"<< endl;
    } else {
        // the log file exists
        restoreFile_.open(restoreFileName_, ios_base::app | ios_base::out);
    }

#if(EDR_BREAKDOWN == 1)

  if (!tool::FileExist(breakdownFileName_)) {
        // if the log file not exist, add the header
        breakdownFile_.open(breakdownFileName_, ios_base::out);
        breakdownFile_ <<"Backup ID,"<<"Backup Size(MB),"<<"dataTranTime(us/MB),"<<"fpTime(ms/MB),"<<"freqTime(ms/MB),"<<"firstDedupTime(ms/MB),"<<"secondDedupTime(ms/MB),"<<"total_dedupTime(ms/MB),"
        <<"sfTime(ms/MB),"<<"checkTime(ms/MB),"<<"lz4compressTime(ms/MB),"<<"deltacompressTime(ms/MB),"<<"encTime(ms/MB),"<<"Total size(MB)"<<endl;
    } else {
        // the log file exists
        breakdownFile_.open(breakdownFileName_, ios_base::app | ios_base::out);
    }

#endif


#if(INDEX_INFO == 1)

  if (!tool::FileExist(indexsizeFileName_)) {
        // if the log file not exist, add the header
        indexsizeFile_.open(indexsizeFileName_, ios_base::out);
        indexsizeFile_ <<"Backup ID, "
        << "FPindex, "
        << "FPindex (%), "
        << "SFindex, "
        << "SFindex (%), "
        << "Deltaindex, "
        << "Deltaindex (%)"
        << endl;
    } else {
        // the log file exists
        indexsizeFile_.open(indexsizeFileName_, ios_base::app | ios_base::out);
    }    

#endif

#if(CHUNK_INFO == 1)

  if (!tool::FileExist(chunkinfoFileName_)) {
        // if the log file not exist, add the header
        chunkinfoFile_.open(chunkinfoFileName_, ios_base::out);
        chunkinfoFile_ <<"Backup ID, "<<"Logical chunk num, "<<"Unique chunk num, "<<"Base chunk num, "<<"Delta chunk num, "<<"Logical chunk size, "<<"Unique chunk size, "<<"Base chunk size, "<<"Delta chunk size, "<<"LZ4_save size, "<<"Delta_save size, "<<endl;
    } else {
        // the log file exists
        chunkinfoFile_.open(chunkinfoFileName_, ios_base::app | ios_base::out);
    }   

#endif

#if(OFFLINE_INFO == 1)

  if (!tool::FileExist(offlineinfoFileName_)) {
        // if the log file not exist, add the header
        offlineinfoFile_.open(offlineinfoFileName_, ios_base::out);
        offlineinfoFile_ 
        << "Backup ID, "
        << "Online Delta_save, "
        << "Online Delta_save per OCall, "
        << "Offline Delta_save, "
        << "Offline delta time, "
        << "Offline dedelta time, "
        << "Offline delete time, "
        << "Offline Delta chunk num"
        <<endl;
    } else {
        // the log file exists
        offlineinfoFile_.open(offlineinfoFileName_, ios_base::app | ios_base::out);
    } 

#endif


#if(SGX_INFO == 1)

    if(!tool::FileExist(sgxinfoFileName_)) {
        // if the log file not exist, add the header
        sgxinfoFile_.open(sgxinfoFileName_, ios_base::out);
        sgxinfoFile_
        << "Backup ID, "
        << "Inline Ecall, "
        << "Index queries Ocall, "
        << "Index updates Ocall, "
        << "Data transfers Ocall, "
        << "Total Inline Ocall, "
        << "Offline Ecall, "
        << "Offline_Ocall, "
        << "_Inline_FPOcall, "
        << "_Inline_SFOcall, "
        << "_Inline_LocalOcall, "
        << "_Inline_LoadOcall, "
        << "_Inline_DeltaOcall, "
        << "_Inline_RecipeOcall, "
        << "_Inline_Write_ContainerOcall, "
        << "_inline_have_similar_chunk_num, "
        << "_inline_need_load_container_num, "
        << "_inlineDeltaChunkNum, "
        << endl;
    }else {
        // the log file exists
        sgxinfoFile_.open(sgxinfoFileName_, ios_base::app | ios_base::out);
    }

#endif


    tool::Logging(myName_.c_str(), "init the ServerOptThread.\n");
}   

/**
 * @brief Destroy the Server Opt Thread object
 * 
 */
ServerOptThread::~ServerOptThread() {
    OutEnclave::Destroy();
    delete dataWriterObj_;
    delete storageCoreObj_;
    delete absIndexObj_;
    delete dataReceiverObj_;
    delete recvDecoderObj_;
    delete raUtil_;

    for (auto it : clientLockIndex_) {
        delete it.second;
    }

    // destroy the variables inside the enclave
    Ecall_Destroy_Restore(eidSGX_);
    Ecall_Destroy_Upload(eidSGX_);
    logFile_.close();
    restoreFile_.close();

    fprintf(stderr, "========ServerOptThread Info========\n");
    fprintf(stderr, "total recv upload requests: %lu\n", totalUploadReqNum_);
    fprintf(stderr, "total recv download requests: %lu\n", totalRestoreReqNum_);
    fprintf(stderr, "====================================\n");
}

/**
 * @brief the main process
 * 
 * @param clientSSL the client ssl
 */
void ServerOptThread::Run(SSL* clientSSL) {
    boost::thread* thTmp;
    boost::thread_attributes attrs;
    attrs.set_stack_size(THREAD_STACK_SIZE);
    vector<boost::thread*> thList;
    EnclaveInfo_t enclaveInfo;

    SendMsgBuffer_t recvBuf;
    recvBuf.sendBuffer = (uint8_t*) malloc(sizeof(NetworkHead_t) + SESSION_KEY_BUFFER_SIZE);
    recvBuf.header = (NetworkHead_t*) recvBuf.sendBuffer;
    recvBuf.header->dataSize = 0;
    recvBuf.dataBuffer = recvBuf.sendBuffer + sizeof(NetworkHead_t);
    uint32_t recvSize = 0;

    tool::Logging(myName_.c_str(), "the main thread is running.\n");

#if (ENABLE_SGX_RA == 1)
    // check whether do remote attestation
    if (!dataSecureChannel_->ReceiveData(clientSSL, recvBuf.sendBuffer, recvSize)) {
        tool::Logging(myName_.c_str(), "recv RA decision fails.\n");
        exit(EXIT_FAILURE);
    }
    sgx_ra_context_t raCtx;
    switch (recvBuf.header->messageType) {
        case SGX_RA_NEED: {
            raUtil_->DoAttestation(eidSGX_, raCtx, clientSSL);
            if (!dataSecureChannel_->ReceiveData(clientSSL, recvBuf.sendBuffer, recvSize)) {
                tool::Logging(myName_.c_str(), "client closed socket connect, RA finish.\n");
                dataSecureChannel_->ClearAcceptedClientSd(clientSSL);
            }
            free(recvBuf.sendBuffer);
            return ;
        }
        case SGX_RA_NOT_NEED: {
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong RA request type.\n");
            exit(EXIT_FAILURE);
        }
    }

#else
    // wait the RA request
    if (!dataSecureChannel_->ReceiveData(clientSSL, recvBuf.sendBuffer, recvSize)) {
        tool::Logging(myName_.c_str(), "recv the RA request header fails.\n");
        exit(EXIT_FAILURE);
    } 
    switch (recvBuf.header->messageType) {
        case SGX_RA_NOT_SUPPORT: {
            // cannot perform RA
            if (!dataSecureChannel_->ReceiveData(clientSSL, recvBuf.sendBuffer, 
                recvSize)) {
                tool::Logging(myName_.c_str(), "client closed socket connect, RA not support.\n");
                dataSecureChannel_->ClearAcceptedClientSd(clientSSL);
            }
            free(recvBuf.sendBuffer);
            return;
        }
        case SGX_RA_NOT_NEED: {
            // does not need to perform RA
            break;   
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong RA request type.\n");
            exit(EXIT_FAILURE);
        }
    }
#endif

    // generate the session key
    if (!dataSecureChannel_->ReceiveData(clientSSL, recvBuf.sendBuffer, 
        recvSize)) {
        tool::Logging(myName_.c_str(), "recv the session key request error.\n");
        exit(EXIT_FAILURE);
    }
    if (recvBuf.header->messageType != SESSION_KEY_INIT) { 
        tool::Logging(myName_.c_str(), "recv the wrong session key init type.\n");
        exit(EXIT_FAILURE);
    }

    // check the client lock here (ensure exist only one client with the same client ID)
    uint32_t clientID = recvBuf.header->clientID;
    boost::mutex* tmpLock;
    {
        lock_guard<mutex> lock(clientLockSetLock_);
        auto clientLockRes = clientLockIndex_.find(clientID);
        if (clientLockRes != clientLockIndex_.end()) {
            // try to lock this mutex
            tmpLock = clientLockIndex_[clientID];
            tmpLock->lock();
        } else {
            // add a new lock to the current index
            tmpLock = new boost::mutex();
            clientLockIndex_[clientID] = tmpLock;
            tmpLock->lock();
        }
    }

    struct timeval sTime;
    struct timeval eTime;
    double keyExchangeTime = 0;
    gettimeofday(&sTime, NULL);
    Ecall_Session_Key_Exchange(eidSGX_, recvBuf.dataBuffer, clientID);
    gettimeofday(&eTime, NULL);
    keyExchangeTime = tool::GetTimeDiff(sTime, eTime);

    recvBuf.header->messageType = SESSION_KEY_REPLY;
    if (!dataSecureChannel_->SendData(clientSSL, recvBuf.sendBuffer,
        sizeof(NetworkHead_t) + SESSION_KEY_BUFFER_SIZE)) {
        tool::Logging(myName_.c_str(), "send the session key fails.\n");
        exit(EXIT_FAILURE);
    }
    if (!dataSecureChannel_->ReceiveData(clientSSL, recvBuf.sendBuffer, 
        recvSize)) {
        tool::Logging(myName_.c_str(), "recv the login message error.\n");
        exit(EXIT_FAILURE);
    }

    // ---- the main process ----
    int optType = 0;
    switch (recvBuf.header->messageType) {
        case CLIENT_LOGIN_UPLOAD: {
            optType = UPLOAD_OPT;
            tool::Logging(myName_.c_str(), "Upload Access\n");
            break;
        }
        case CLIENT_LOGIN_DOWNLOAD: {
            optType = DOWNLOAD_OPT;
            break;
        }
        case CLIENT_LOGIN_OFFLINE:  {
            optType = OFFLINE_OPT;
            tool::Logging(myName_.c_str(), "Offline Access\n");
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong client login type.\n");
            exit(EXIT_FAILURE);
        }
    }

    // check the file status
    // convert the file name hash to the file path
    char fileHashBuf[CHUNK_HASH_SIZE * 2 + 1];
    for (uint32_t i = 0; i < CHUNK_HASH_SIZE; i++) {
        sprintf(fileHashBuf + i * 2, "%02x", recvBuf.dataBuffer[i]);
    }
    string fileName;
    fileName.assign(fileHashBuf, CHUNK_HASH_SIZE * 2);
    string recipePath = config.GetRecipeRootPath() +
        fileName + config.GetRecipeSuffix();
    if (!this->CheckFileStatus(recipePath, optType)) {
        recvBuf.header->messageType = SERVER_FILE_NON_EXIST;
        if (!dataSecureChannel_->SendData(clientSSL, recvBuf.sendBuffer,
            sizeof(NetworkHead_t))) {
            tool::Logging(myName_.c_str(), "send the file not exist reply error.\n");
            exit(EXIT_FAILURE);
        }

        // wait the client to close the connection
        if (!dataSecureChannel_->ReceiveData(clientSSL, 
            recvBuf.sendBuffer, recvSize)) {
            tool::Logging(myName_.c_str(), "client close the socket connection.\n");
            dataSecureChannel_->ClearAcceptedClientSd(clientSSL);
        } else {
            tool::Logging(myName_.c_str(), "client does not close the connection.\n");
            exit(EXIT_FAILURE);
        }

        // clear the tmp variable
        free(recvBuf.sendBuffer);
        tmpLock->unlock();
        return ;
    } else {
        tool::Logging(myName_.c_str(), "file status check successfully.\n");
    }
    /// check done
    
    // init the vars for this client
    ClientVar* outClient;
    switch (optType) {
        case UPLOAD_OPT:
        case OFFLINE_OPT: {
            // update the req number
            totalUploadReqNum_++;
            tool::Logging(myName_.c_str(), "recv the upload request from client: %u\n",
                clientID);
            outClient = new ClientVar(clientID, clientSSL, UPLOAD_OPT, recipePath);
            Ecall_Init_Client(eidSGX_, clientID, indexType_, UPLOAD_OPT, 
                recvBuf.dataBuffer + CHUNK_HASH_SIZE, 
                &outClient->_upOutSGX.sgxClient);

            thTmp = new boost::thread(attrs, boost::bind(&DataReceiver::Run, dataReceiverObj_,
                outClient, &enclaveInfo));
            thList.push_back(thTmp); 
#if (MULTI_CLIENT == 0)
            thTmp = new boost::thread(attrs, boost::bind(&DataWriter::Run, dataWriterObj_,
                outClient->_inputMQ));
            thList.push_back(thTmp);
#endif
            // send the upload-response to the client
            recvBuf.header->messageType = SERVER_LOGIN_RESPONSE;
            if (!dataSecureChannel_->SendData(clientSSL, recvBuf.sendBuffer, 
                sizeof(NetworkHead_t))) {
                tool::Logging(myName_.c_str(), "send the upload-login response error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case DOWNLOAD_OPT: {
            // update the req number 
            totalRestoreReqNum_++;
            tool::Logging(myName_.c_str(), "recv the restore request from client: %u\n",
                clientID);
            outClient = new ClientVar(clientID, clientSSL, DOWNLOAD_OPT, recipePath);
            Ecall_Init_Client(eidSGX_, clientID, indexType_, DOWNLOAD_OPT, 
                recvBuf.dataBuffer + CHUNK_HASH_SIZE,
                &outClient->_resOutSGX.sgxClient);

            thTmp = new boost::thread(attrs, boost::bind(&EnclaveRecvDecoder::Run, recvDecoderObj_,
                outClient, &enclaveInfo));
            thList.push_back(thTmp);

            // send the restore-response to the client (include the file recipe header)
            recvBuf.header->messageType = SERVER_LOGIN_RESPONSE;
            outClient->_recipeReadHandler.read((char*)recvBuf.dataBuffer,
                sizeof(FileRecipeHead_t));
            if (!dataSecureChannel_->SendData(clientSSL, recvBuf.sendBuffer, 
                sizeof(NetworkHead_t) + sizeof(FileRecipeHead_t))) {
                tool::Logging(myName_.c_str(), "send the restore-login response error.\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong operation type from client: %u\n",
                clientID);
            exit(EXIT_FAILURE);
        }
    }

    double totalTime = 0;
    gettimeofday(&sTime, NULL);
    for (auto it : thList) {
        it->join();
    }
    gettimeofday(&eTime, NULL);
    totalTime += tool::GetTimeDiff(sTime, eTime);

    // clean up
    for (auto it : thList) {
        delete it;
    }
    thList.clear();

    double offlineTime = 0;
    if(optType == OFFLINE_OPT){
        tool::Logging(myName_.c_str(), "Process Offline\n");
        gettimeofday(&sTime, NULL);
        absIndexObj_->ProcessOff(&outClient->_recvChunkBuf,&outClient->_upOutSGX);
        absIndexObj_->Ecall_time++;
        gettimeofday(&eTime, NULL);
        tool::Logging(myName_.c_str(), "Process Offline end\n");
        Ecall_GetOfflineInfo(eidSGX_,&enclaveInfo);
    }

    if(optType == OFFLINE_OPT){
        offlineTime = tool::GetTimeDiff(sTime, eTime);
    }else{
        offlineTime = 0;
    }


#if(INDEX_INFO == 1)

    fp2ChunkDB_->GetIndexSize();

#endif

    
    // clean up client variables 
    switch (optType) {
        case UPLOAD_OPT:
        case OFFLINE_OPT: {
            Ecall_Destroy_Client(eidSGX_, outClient->_upOutSGX.sgxClient);
            break;
        }
        case DOWNLOAD_OPT: {
            Ecall_Destroy_Client(eidSGX_, outClient->_resOutSGX.sgxClient);
            break;
        }
        default: {
            tool::Logging(myName_.c_str(), "wrong opt type.\n");
            exit(EXIT_FAILURE);
        }
    }

    // print the info
    double speed = static_cast<double>(outClient->_uploadDataSize) / 1024.0 / 1024.0 
        / enclaveInfo.enclaveOnlineTime;
    
    totalRecvLogicalSize_ += outClient->_uploadDataSize;
    if (optType == UPLOAD_OPT || optType == OFFLINE_OPT) {
        total_online_time += enclaveInfo.enclaveOnlineTime;
        total_offline_time += offlineTime;

        logFile_ << backupid << ", " 
            << speed << ", "
            << enclaveInfo.enclaveOnlineTime << ", "
            << total_online_time / (backupid + 1) << ", "
            << outClient->_uploadDataSize << ", " 
            << enclaveInfo.onlineCompress_size << ", "
            << enclaveInfo.offlineCompress_size << ", "
            << offlineTime << ", " 
            << total_offline_time / (backupid + 1) << ", " 
            << 1. * totalRecvLogicalSize_ / enclaveInfo.offlineCompress_size <<endl;
        Ecall_UpdateOnlineInfo(eidSGX_);
        logFile_.flush();

#if (SGX_BREAKDOWN == 1)
        double dataMB = static_cast<double>(outClient->_uploadDataSize) / 1024.0 / 1024.0;
        tool::Logging(myName_.c_str(), "data tran time: %lf\n", 
            (enclaveInfo.dataTranTime + keyExchangeTime * 1000.0) / dataMB);
        tool::Logging(myName_.c_str(), "fingerprint time: %lf\n",
            enclaveInfo.fpTime / dataMB);
        tool::Logging(myName_.c_str(), "freq counting time: %lf\n",
            enclaveInfo.freqTime / dataMB);
        tool::Logging(myName_.c_str(), "first-dedup time: %lf\n",
            enclaveInfo.firstDedupTime / dataMB);
        tool::Logging(myName_.c_str(), "second-dedup time: %lf\n",
            enclaveInfo.secondDedupTime / dataMB);
        tool::Logging(myName_.c_str(), "compression time: %lf\n",
            enclaveInfo.compTime / dataMB);
        tool::Logging(myName_.c_str(), "encryption time: %lf\n",
            enclaveInfo.encTime / dataMB);
#endif

#if(EDR_BREAKDOWN == 1)
    double dataMB = static_cast<double>(outClient->_uploadDataSize) / 1024.0 / 1024.0;
    total_backupsize += dataMB;
    total_keyExchangeTime += keyExchangeTime;
    double total_dedupTime = enclaveInfo.fpTime+enclaveInfo.freqTime+enclaveInfo.firstDedupTime+enclaveInfo.secondDedupTime;
    breakdownFile_ << backupid << "," << dataMB << "," <<(enclaveInfo.dataTranTime + total_keyExchangeTime * 1000.0)/total_backupsize<<","
    <<enclaveInfo.fpTime/total_backupsize<<","
    <<enclaveInfo.freqTime/total_backupsize<<","
    <<enclaveInfo.firstDedupTime/total_backupsize<<","
    <<enclaveInfo.secondDedupTime/total_backupsize<<","
    <<total_dedupTime/total_backupsize<<","
    <<enclaveInfo.sfTime/total_backupsize<<","
    <<enclaveInfo.checkTime/total_backupsize<<","
    <<enclaveInfo.lz4compressTime/total_backupsize<<","
    <<enclaveInfo.deltacompressTime/total_backupsize<<","
    <<enclaveInfo.encTime/total_backupsize<<","<<to_string(total_backupsize)<<endl;
    breakdownFile_.flush();
#endif

#if(INDEX_INFO == 1)
    indexsizeFile_ << backupid << "," 
    << fp2ChunkDB_->fpindexsize << "," 
    << 1. * fp2ChunkDB_->fpindexsize / totalRecvLogicalSize_ * 100 << "," 
    << fp2ChunkDB_->sfindexsize << "," 
    << 1. * fp2ChunkDB_->sfindexsize / totalRecvLogicalSize_ * 100 << "," 
    << fp2ChunkDB_->deltamapsize << ","
    << 1. * fp2ChunkDB_->deltamapsize / totalRecvLogicalSize_ * 100 << endl;
    indexsizeFile_.flush();
#endif



#if(CHUNK_INFO == 1)
    chunkinfoFile_<< backupid << ","<<enclaveInfo.logicalChunkNum << ","<<enclaveInfo.uniqueChunkNum << ","<<enclaveInfo.baseChunkNum <<","<<enclaveInfo.deltaChunkNum<<","<<enclaveInfo.logicalDataSize 
    << ","<<enclaveInfo.uniqueDataSize <<","<<enclaveInfo.baseDataSize<<","<<enclaveInfo.deltaDataSize<< ","<<enclaveInfo.lz4SaveSize<< ","<<enclaveInfo.DeltaSaveSize<<endl;
    chunkinfoFile_.flush();
#endif


#if(OFFLINE_INFO == 1)
    offlineinfoFile_ <<
    backupid << ","
    << enclaveInfo.Online_DeltaSaveSize << ","
    << 1. * enclaveInfo.Online_DeltaSaveSize / (enclaveInfo._Inline_LocalOcall + enclaveInfo._Inline_DeltaOcall + enclaveInfo._Inline_LoadOcall + enclaveInfo._Inline_RecipeOcall + dataWriterObj_->containerNum_) / 1024 << ","
    << enclaveInfo.Offline_DeltaSaveSize << ","
    << enclaveInfo.offlineDeltanum << ","
    << enclaveInfo.offlineDeDeltanum << ","
    << enclaveInfo.offlineDeletenum << ","
    << enclaveInfo.OfflinedeltaChunkNum << endl;
    offlineinfoFile_.flush();
#endif


#if(SGX_INFO == 1)
    sgxinfoFile_ 
    << backupid << ","
    << absIndexObj_->Ecall_time << ","
    << enclaveInfo._Inline_FPOcall + enclaveInfo._Inline_SFOcall << ","
    << enclaveInfo._Inline_LocalOcall + enclaveInfo._Inline_DeltaOcall << ","
    << enclaveInfo._Inline_LoadOcall + enclaveInfo._Inline_RecipeOcall + dataWriterObj_->containerNum_ << ","
    << enclaveInfo._Inline_FPOcall + enclaveInfo._Inline_SFOcall + enclaveInfo._Inline_LocalOcall + enclaveInfo._Inline_DeltaOcall + enclaveInfo._Inline_LoadOcall + enclaveInfo._Inline_RecipeOcall + dataWriterObj_->containerNum_ << ","
    << backupid + 1 << ","
    << enclaveInfo.offline_Ocall << ","
    << enclaveInfo._Inline_FPOcall << ","
    << enclaveInfo._Inline_SFOcall << ","
    << enclaveInfo._Inline_LocalOcall << ","
    << enclaveInfo._Inline_LoadOcall << ","
    << enclaveInfo._Inline_DeltaOcall << ","
    << enclaveInfo._Inline_RecipeOcall << ","
    << dataWriterObj_->containerNum_ << ","
    << enclaveInfo._inline_have_similar_chunk_num << ","
    << enclaveInfo._inline_need_load_container_num << ","
    << enclaveInfo._inlineDeltaChunkNum
    << endl;
    sgxinfoFile_.flush();
#endif


    }
    backupid++;
    if(optType == DOWNLOAD_OPT)
    {
        restoreFile_ << restoreId << "," << enclaveInfo.restoreTime << endl;
        restoreId++;
        restoreFile_.flush();
    }

    delete outClient;
    free(recvBuf.sendBuffer);
    tmpLock->unlock();

    tool::Logging(myName_.c_str(), "total running time of client %u: %lf\n", 
        clientID, totalTime);
    tool::Logging(myName_.c_str(), "total key exchange time of client %u: %lf\n", 
        clientID, keyExchangeTime); 

    return ;
}

/**
 * @brief check the file status
 * 
 * @param fullRecipePath the full recipe path
 * @param optType the operation type
 * @return true success
 * @return false fail
 */
bool ServerOptThread::CheckFileStatus(string& fullRecipePath, int optType) {
    if (tool::FileExist(fullRecipePath)) {
        // the file exists
        switch (optType) {
            case UPLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s exists, overwrite it.\n",
                    fullRecipePath.c_str());
                break;
            }
            case DOWNLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s exists, access it.\n",
                    fullRecipePath.c_str());
                break;
            }
        }
    } else {
        switch (optType) {
            case UPLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s not exists, create it.\n",
                    fullRecipePath.c_str());
                break;
            }
            case DOWNLOAD_OPT: {
                tool::Logging(myName_.c_str(), "%s not exists, restore reject.\n",
                    fullRecipePath.c_str());
                return false;
            }
        }
    }
    return true;
}