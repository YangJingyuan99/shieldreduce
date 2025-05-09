#include "../../include/ecallOffline.h"


int skip_num = 0;


void OFFLineBackward::Insert_local(string oldchunkhash, string newchunkhash)
{
    local_basemap[oldchunkhash] = newchunkhash;
    return;
}

OFFLineBackward::OFFLineBackward()
{
    // Allocate memory for the temporary containers
    tmpOldContainer = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    tmpNewContainer = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    tmpDeltaContainer = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    tmpColdContainer = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
}

OFFLineBackward::~OFFLineBackward()
{
    free(tmpOldContainer);
    free(tmpNewContainer);
    free(tmpDeltaContainer);
    free(tmpColdContainer);
}

void OFFLineBackward::Print()
{
   int basechunk_num = 0;

    for(unordered_map<string, vector<string>>::iterator it = delta_map.begin(); it != delta_map.end(); it++)
    {
        basechunk_num += it->second.size();
    }


    //Enclave::Logging("deltamap", "delta map size is %d\n", basechunk_num);
}

void OFFLineBackward::Easy_update(UpOutSGX_t *upOutSGX , EcallCrypto* cryptoObj_){
    // 声明相关临时变量
    
    //Enclave::Logging("DEBUG", "easy update begin\n");
    EnclaveClient *sgxClient = (EnclaveClient *)upOutSGX->sgxClient;
    EVP_CIPHER_CTX *cipherCtx = sgxClient->_cipherCtx;
    OutQueryEntry_t* outEntry = upOutSGX->outQuery->outQueryBase;

    size_t counter = 0;
    string old_basechunkhash;
    old_basechunkhash.resize(CHUNK_HASH_SIZE, 0);
    string new_basechunkhash;
    new_basechunkhash.resize(CHUNK_HASH_SIZE, 0);

    RecipeEntry_t* old_recipe = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t)); // store recipe for data chunk
    if(!old_recipe)
    {
        Enclave::Logging("malloc", "old recipe\n");
    }
    RecipeEntry_t* new_recipe = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t)); // store recipe for new base chunk
    if(!new_recipe)
    {
        Enclave::Logging("malloc", "new recipe\n");
    }

    uint8_t* old_basechunksf;
    old_basechunksf = (uint8_t*)malloc(3*CHUNK_HASH_SIZE);
    //uint8_t* old_basechunkfp;
    uint8_t* new_basechunksf;
    new_basechunksf = (uint8_t*)malloc(3*CHUNK_HASH_SIZE);
    //uint8_t* new_basechunkfp;

    uint8_t* old_container;
    size_t old_container_size;
    //char* old_basecontainerID = (char*)malloc(CONTAINER_ID_LENGTH);
    string old_basecontainerID;
    old_basecontainerID.resize(CONTAINER_ID_LENGTH, 0);

    uint8_t* new_container;
    size_t new_container_size;
    //char* new_basecontainerID = (char*)malloc(CONTAINER_ID_LENGTH);
    string new_basecontainerID;
    new_basecontainerID.resize(CONTAINER_ID_LENGTH, 0);

    pair<uint32_t, uint32_t> basepair; // user for updating cold_basemap
    pair<uint32_t, uint32_t> deltapair; // same with above

    string delta_chunkhash; // TODO
    delta_chunkhash.resize(CHUNK_HASH_SIZE, 0);
    RecipeEntry_t* delta_recipe = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t)); // store recipe for delta chunk
    if(!delta_recipe)
    {
        Enclave::Logging("malloc", "delta recipe\n");
    }
    uint8_t *delta_container;
    size_t delta_contaienr_size;

    uint8_t* old_basechunkFP;


    uint8_t* backbuffer = upOutSGX->test_buffer;
/*     for(int i = 0;i<CHUNK_HASH_SIZE;i++){
        Enclave::Logging("DEBUG", "02x\n",backbuffer[0]);
    }
 */

    uint32_t backnum;

    GreeyOfflineSize = _offlineCurrBackup_size * GREEDY_THRESHOLD;
    uint64_t _onlineBackup_size = _offlineCurrBackup_size;

    //Enclave::Logging("DEBUG", "in\n");
    Ocall_GetLocal(upOutSGX->outClient);
    _offline_Ocall++;
    memcpy(&backnum,backbuffer,sizeof(uint32_t));
    //Enclave::Logging("DEBUG", "out map size:%d\n",backnum);
    backbuffer += sizeof(uint32_t);

    while(backnum != 0){
        for(int i = 0;i < backnum;i++){
            string old_basechunkstr;
            string new_basechunkstr;
            old_basechunkstr.assign((char*)backbuffer,CHUNK_HASH_SIZE);
            backbuffer+=CHUNK_HASH_SIZE;
            new_basechunkstr.assign((char*)backbuffer,CHUNK_HASH_SIZE);
            backbuffer+=CHUNK_HASH_SIZE;
            local_basemap[old_basechunkstr] =  new_basechunkstr;
        }
        backbuffer = upOutSGX->test_buffer;
        Ocall_GetLocal(upOutSGX->outClient);
        _offline_Ocall++;
        memcpy(&backnum,backbuffer,sizeof(uint32_t));
        //Enclave::Logging("DEBUG", "next batch size:%d\n",backnum);
        backbuffer += sizeof(uint32_t);


    if(local_basemap.size() != 0)
    {
        //Enclave::Logging("DEBUG", "local base map size is %d\n", local_basemap.size());
        for(unordered_map<string, string>::iterator it = local_basemap.begin(); it != local_basemap.end(); it++)
        {

            if(GreeyOfflineSize >= _offlineCurrBackup_size){
                old_basechunkhash = it->first;
                memcpy(&outEntry->chunkHash, (uint8_t*)&old_basechunkhash[0], CHUNK_HASH_SIZE);
                Ocall_OneRecipe(upOutSGX->outClient);
                cryptoObj_->AESCBCDec(cipherCtx, (uint8_t*)&outEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)old_recipe);
                memcpy(old_basechunksf,&old_recipe->superfeature,3*CHUNK_HASH_SIZE);
                memcpy((uint8_t*)&outEntry->superfeature, old_basechunksf, 3*CHUNK_HASH_SIZE);
                Ocall_OFFline_updateIndex(upOutSGX->outClient, 0);

                new_basechunkhash = it->second;
                memcpy(&outEntry->chunkHash, (uint8_t*)&new_basechunkhash[0], CHUNK_HASH_SIZE);
                Ocall_OneRecipe(upOutSGX->outClient);
                cryptoObj_->AESCBCDec(cipherCtx, (uint8_t*)&outEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)new_recipe);            
                memcpy(new_basechunksf,&new_recipe->superfeature,3*CHUNK_HASH_SIZE);
                memcpy((uint8_t*)&outEntry->superfeature, new_basechunksf, 3*CHUNK_HASH_SIZE);
                Ocall_OFFline_updateIndex(upOutSGX->outClient, 1);

                memset(upOutSGX->process_buffer, 0 ,1000 * CHUNK_HASH_SIZE);
                memcpy(upOutSGX->process_buffer, (uint8_t*)&old_basechunkhash[0], CHUNK_HASH_SIZE);
                Ocall_QueryDeltaIndex(upOutSGX->outClient);

                _offline_Ocall++;
                continue;
            }


            //Enclave::Logging("DEBUG", "Backup OnlineSize: %d, GreedyThresold: %f, GreedySize: %d, OfflineSize: %d\n",_onlineBackup_size,Greedy_thresold,GreeyOfflineSize,_offlineCurrBackup_size);

            //Enclave::Logging("debug", "Base begin\n");
            old_basechunkhash = it->first;
            // Get old chunk recipe
            memcpy(&outEntry->chunkHash, (uint8_t*)&old_basechunkhash[0], CHUNK_HASH_SIZE);
            Ocall_OneRecipe(upOutSGX->outClient);
            cryptoObj_->AESCBCDec(cipherCtx, (uint8_t*)&outEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)old_recipe);
            // Get old container
            //Enclave::Logging("DEBUG", "old container id is %s\n", old_recipe->containerName);
            //Enclave::Logging("DEBUG", "old recipe offset is %d\n", old_recipe->offset);
            memcpy(&outEntry->chunkAddr.containerName, old_recipe->containerName, CONTAINER_ID_LENGTH);

            Ocall_OneContainer(upOutSGX->outClient);
            if(outEntry->offlineFlag == false)
            {
                continue;
            }
            memcpy(tmpOldContainer, outEntry->containerbuffer, MAX_CONTAINER_SIZE);
            old_container = tmpOldContainer;
            old_basecontainerID.assign((char*)&old_recipe->containerName, CONTAINER_ID_LENGTH);
            // Get old chunk sf
            //old_basechunksf = GetChunk_SF(old_container, old_recipe);
            memcpy(old_basechunksf,&old_recipe->superfeature,3*CHUNK_HASH_SIZE);
            memcpy((uint8_t*)&outEntry->superfeature, old_basechunksf, 3*CHUNK_HASH_SIZE);

    /*         for(int i = 0;i<CHUNK_HASH_SIZE;i++){
                Enclave::Logging("DEBUG", "%02x\n", old_basechunkFP[i]);
                Enclave::Logging("DEBUG", "%02x\n", old_basechunkhash[i]);
                
            } */

            // update sf index: remove old basechunksf
            
            Ocall_OFFline_updateIndex(upOutSGX->outClient, 0);
            //Enclave::Logging("DEBUG", "Old BaseChunk load finished\n");

            _offline_Ocall++;

            new_basechunkhash = it->second;
            // Get new chunk recipe
            memcpy(&outEntry->chunkHash, (uint8_t*)&new_basechunkhash[0], CHUNK_HASH_SIZE);
            Ocall_OneRecipe(upOutSGX->outClient);
            cryptoObj_->AESCBCDec(cipherCtx, (uint8_t*)&outEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)new_recipe);
            // Get new container
            memcpy(&outEntry->chunkAddr.containerName, new_recipe->containerName, CONTAINER_ID_LENGTH);            


            Ocall_OneContainer(upOutSGX->outClient);
            if(outEntry->offlineFlag == false)
            {
                skip_num++;
                Enclave::Logging("DEBUG", "skip total num: %d\n", skip_num);
                continue;
            }
            memcpy(tmpNewContainer, outEntry->containerbuffer, MAX_CONTAINER_SIZE);
            new_container = tmpNewContainer;
            new_basecontainerID.assign((char*)&new_recipe->containerName, CONTAINER_ID_LENGTH);
            // Get new chunk sf
            //new_basechunksf = GetChunk_SF(new_container, new_recipe);

            memcpy(new_basechunksf,&new_recipe->superfeature,3*CHUNK_HASH_SIZE);
            memcpy((uint8_t*)&outEntry->superfeature, new_basechunksf, 3*CHUNK_HASH_SIZE);
  
            // update sf index: remove old basechunksf
            //memcpy((uint8_t*)&outEntry->superfeature, new_basechunksf, 3*CHUNK_HASH_SIZE);
            Ocall_OFFline_updateIndex(upOutSGX->outClient, 1);

            _offline_Ocall++;

           //Enclave::Logging("DEBUG", "New BaseChunk load finished\n");

            //Enclave::Logging("debug", "Base chunk process begin\n");
     
            // Get original chunk content & update code base map
            uint8_t *old_chunk_content_crypt = GetChunk_content(old_container, old_recipe, true, basepair);
            uint8_t *new_chunk_content_crypt = GetChunk_content(new_container, new_recipe, false, basepair);

            uint8_t *old_chunk_IV = GetChunk_IV(old_container, old_recipe);
            uint8_t *new_chunk_IV = GetChunk_IV(new_container, new_recipe);
        
            size_t old_chunk_size = old_recipe->length;
            size_t new_chunk_size = new_recipe->length;

            //Enclave::Logging("debug", "content iv\n");

            uint8_t* old_chunk_content_decrypt = (uint8_t *)malloc(MAX_CHUNK_SIZE); // store old chunk content after decryptyion
            if(!old_chunk_content_decrypt)
            {
                Enclave::Logging("malloc", "old_chunk_content_decrypt\n");
            }
            uint8_t* new_chunk_content_decrypt = (uint8_t *)malloc(MAX_CHUNK_SIZE); // store new chunk content after decryptyion
            if(!new_chunk_content_decrypt)
            {
                Enclave::Logging("malloc", "new_chunk_content_decrypt\n");
            }
            cryptoObj_->DecryptionWithKeyIV(cipherCtx, old_chunk_content_crypt, old_chunk_size, // decrpytion
                Enclave::enclaveKey_, old_chunk_content_decrypt, old_chunk_IV);
            cryptoObj_->DecryptionWithKeyIV(cipherCtx, new_chunk_content_crypt, new_chunk_size, 
                Enclave::enclaveKey_, new_chunk_content_decrypt, new_chunk_IV);

            uint8_t *old_chunk_content_decompression = (uint8_t *)malloc(MAX_CHUNK_SIZE); // store old chunk content after decompression
            uint8_t *new_chunk_content_decompression = (uint8_t *)malloc(MAX_CHUNK_SIZE); // store new chunk content after decompression

            //Enclave::Logging("debug", "decrypt decompress\n");

            uint8_t *old_chunk;
            uint8_t *new_chunk;

            int old_refchunksize = LZ4_decompress_safe((char*)old_chunk_content_decrypt, (char*)old_chunk_content_decompression, old_chunk_size, MAX_CHUNK_SIZE);
            //int old_refchunksize = LZ4_decompress_fast((char*)old_chunk_content_decrypt, (char*)old_chunk_content_decompression, old_chunk_size);
            if(old_refchunksize < 0)
            {
                // old_refchunksize < 0 means chunk didn't do compression
                old_refchunksize = old_chunk_size;
                old_chunk = old_chunk_content_decrypt;
            } else 
            {
                old_chunk = old_chunk_content_decompression;
            }
            //Enclave::Logging("DEBUG", "Old BaseChunk decompression finished\n");

            bool delta_flag = true; // TODO
            
            int new_refchunksize = LZ4_decompress_safe((char*)new_chunk_content_decrypt, (char*)new_chunk_content_decompression, new_chunk_size, MAX_CHUNK_SIZE);
            //int new_refchunksize = LZ4_decompress_fast((char*)new_chunk_content_decrypt, (char*)new_chunk_content_decompression, new_chunk_size);
            if(new_refchunksize < 0)
            {
                // old_refchunksize < 0 means chunk didn't do compression
                new_refchunksize = new_chunk_size;
                new_chunk = new_chunk_content_decrypt;
            } else 
            {
                new_chunk = new_chunk_content_decompression;
            }
            //Enclave::Logging("DEBUG", "New BaseChunk decompression finished\n");

            //Enclave::Logging("debug", "Base mark\n");

            memset(upOutSGX->process_buffer, 0 ,1000 * CHUNK_HASH_SIZE);
            memcpy(upOutSGX->process_buffer, (uint8_t*)&old_basechunkhash[0], CHUNK_HASH_SIZE);
            Ocall_QueryDeltaIndex(upOutSGX->outClient);
            int delta_index_num = upOutSGX->deltaInfo->QueryNum;
            if(delta_index_num!=0)
            {
                for(int i=0;i<delta_index_num;i++)
                {
                    string tmpDelta;
                    tmpDelta.assign((char*)upOutSGX->process_buffer + (i+1)*CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
                    Insert_delta(old_basechunkhash, tmpDelta);
                }
            }
            
            auto delta_it = delta_map.find(old_basechunkhash);
            //Enclave::Logging("DEBUG","delta map size is %d\n", delta_map[old_basechunkhash].size());
            //counter += delta_map[old_basechunkhash].size();
            if(delta_it != delta_map.end()) // exist in delta map
            {
                //Enclave::Logging("debug", "Delta begin\n");
                //Enclave::Logging("DEBUG", "processing delta chunk begin\n");
                for(size_t i = 0; i < delta_map[old_basechunkhash].size(); i++)
                {
                    bool tmp_delta_flag = true;
                    // Get delta chunk recipe
                    delta_chunkhash = delta_map[old_basechunkhash][i];
                    memcpy(&outEntry->chunkHash, (uint8_t*)&delta_chunkhash[0], CHUNK_HASH_SIZE);
                    Ocall_OneRecipe(upOutSGX->outClient);
                    //Enclave::Logging("DEBUG", "Get delta chunk recipe\n");
                    cryptoObj_->AESCBCDec(cipherCtx, (uint8_t*)&outEntry->chunkAddr, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)delta_recipe);
                    //Enclave::Logging("DEBUG", "Get delta chunk recipe\n");
                    // Get delta chunk container
                    string delta_containerID;
                    delta_containerID.resize(CONTAINER_ID_LENGTH, 0);
                    delta_containerID.assign((char *)&delta_recipe->containerName, CONTAINER_ID_LENGTH);
                    memcpy(&outEntry->chunkAddr.containerName, delta_recipe->containerName, CONTAINER_ID_LENGTH);
                    //Enclave::Logging("DEBUG", "delta recipe container name is %s\n", delta_containerID.c_str());
#if(CONTAINER_SEPARATE == 1)
                    Ocall_OneDeltaContainer(upOutSGX->outClient);
#else
                    Ocall_OneContainer(upOutSGX->outClient);
#endif
                    _offline_Ocall++;
                    memcpy(tmpDeltaContainer, outEntry->containerbuffer, MAX_CONTAINER_SIZE);
                    delta_container = tmpDeltaContainer;
                    //Enclave::Logging("DEBUG", "Get delta chunk container\n");
                    // Get delta chunk fp & sf & Iv
                    uint8_t* delta_fp = GetChunk_FP(delta_container, delta_recipe);
                    uint8_t* delta_sf = GetChunk_SF(delta_container, delta_recipe);
                    uint8_t* delta_iv = GetChunk_IV(delta_container, delta_recipe);
                    //Enclave::Logging("DEBUG", "Get delta chunk fp sf iv\n");
                    // Get delta chunk content
                    uint8_t* delta_chunk_content_crypt = GetChunk_content(delta_container, delta_recipe, true, deltapair);
                    uint8_t* delta_chunk_content_decrypt = (uint8_t*)malloc(MAX_CHUNK_SIZE);
                    size_t delta_size = delta_recipe->length;
                    cryptoObj_->DecryptionWithKeyIV(cipherCtx, delta_chunk_content_crypt, delta_size, 
                        Enclave::enclaveKey_, delta_chunk_content_decrypt, delta_iv);
                    //Enclave::Logging("DEBUG", "Get delta chunk content\n");

                    uint8_t* new_delta_content;
                    size_t new_chunk_size = 0;
                    // Get new delta chunk: do delta compression based on new basechunk
                    new_delta_content = GetNew_deltachunk(delta_chunk_content_decrypt, delta_size, old_chunk, old_refchunksize, 
                            new_chunk, new_refchunksize, &new_chunk_size, tmp_delta_flag);
                    //Enclave::Logging("DEBUG", "Get new delta chunk\n");
                    //Enclave::Logging("DELTAFLAG5", "tmp delta flag is %d\n", tmp_delta_flag);
                    if(tmp_delta_flag) // update delta map
                    {
                        // update statics
                        delta_recipe->length = new_chunk_size;
                        memcpy(delta_recipe->basechunkHash, (uint8_t*)&new_basechunkhash[0], CHUNK_HASH_SIZE);
                        // encrpyt before insert into container
                        uint8_t* delta_content_crypt = (uint8_t*)malloc(MAX_CHUNK_SIZE);
                        cryptoObj_->EncryptWithKeyIV(cipherCtx, new_delta_content, new_chunk_size, Enclave::enclaveKey_, 
                            delta_content_crypt, delta_iv);
                        // update container
                        InsertHot_container(delta_content_crypt, delta_recipe, delta_fp, delta_sf, delta_iv);

                        uint8_t* outRecipe = (uint8_t*)malloc(sizeof(RecipeEntry_t));
                        cryptoObj_->AESCBCEnc(cipherCtx, (uint8_t*)delta_recipe, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, 
                            outRecipe);
                        bool status;
                        //Ocall_UpdateIndexStoreBuffer(&status, &delta_chunkhash[0], CHUNK_HASH_SIZE, (uint8_t*)&outRecipe, sizeof(RecipeEntry_t));
                        //memcpy(&outEntry->chunkAddr, outRecipe, sizeof(RecipeEntry_t));

                        status = UpdateIndexStore(delta_chunkhash, (char*)outRecipe, sizeof(RecipeEntry_t));

                        free(outRecipe);
                        // if(!status)
                        //Enclave::Logging("debug","update index failed\n");
                        
                        
                       // Insert_delta(new_basechunkhash, delta_chunkhash); /2222222

                        memset(upOutSGX->process_buffer, 0 , 1000 * CHUNK_HASH_SIZE);
                        memcpy(upOutSGX->process_buffer, (uint8_t*)&new_basechunkhash[0], CHUNK_HASH_SIZE);
                        memcpy(upOutSGX->process_buffer + CHUNK_HASH_SIZE, (uint8_t*)&delta_chunkhash[0], CHUNK_HASH_SIZE);
                        Ocall_UpdateDeltaIndex(upOutSGX->outClient);

                        _offline_Ocall++;

                        uint8_t* Coldbuffer = upOutSGX->out_buffer;
                        memcpy(Coldbuffer,&delta_containerID[0],CONTAINER_ID_LENGTH);
                        Coldbuffer += CONTAINER_ID_LENGTH;
                        memcpy(Coldbuffer,&deltapair.first,sizeof(uint32_t));
                        Coldbuffer += sizeof(uint32_t);
                        memcpy(Coldbuffer,&deltapair.second,sizeof(uint32_t));
                        Ocall_ColdInsert(upOutSGX->outClient);

                        _offline_Ocall++;
                        
                        //cold_basemap[delta_containerID].push_back(deltapair);
                        free(delta_content_crypt);
                    } else 
                    {
                        delta_flag = tmp_delta_flag;
                    }

                    free(delta_chunk_content_crypt);
                    free(delta_chunk_content_decrypt);
                    free(new_delta_content);
                    free(delta_fp);
                    free(delta_sf);
                    free(delta_iv);
                    
                }
                delta_map.erase(it->first);
            }   
            
            //Enclave::Logging("debug", "Base process\n");
            uint8_t *new_delta_content;
            size_t new_delta_size;
            //Enclave::Logging("debug", "xdelta begin\n");
            new_delta_content = xd3_encode(old_chunk, old_refchunksize, new_chunk, new_refchunksize, &new_delta_size);
            //Enclave::Logging("debug", "xdelta end\n");
            uint8_t* recc_chunk;
            size_t recc_size;
            recc_chunk = xd3_decode(new_delta_content, new_delta_size, new_chunk, new_refchunksize, &recc_size);
            if (recc_size == 0)
            {
                //Enclave::Logging("debug", "NO delta!!! base_recc_chunk_size: %d\n",recc_size);

                delta_flag = false;
            }else{
                    _offlineCompress_size -= old_chunk_size;
                    _offlineCurrBackup_size -= old_chunk_size;
                    _offlineCompress_size += new_delta_size;
                    _offlineCurrBackup_size += new_delta_size;


                    _offlinedeltaChunkNum++;
                    _deltaChunkNum++;
                    _baseChunkNum--;

                    _deltaDataSize += new_delta_size;
                    _baseDataSize -= old_chunk_size;

                    _Offline_DeltaSaveSize += (old_refchunksize - new_delta_size);
                    _lz4SaveSize -= (old_refchunksize - old_chunk_size);
                    _DeltaSaveSize += (old_refchunksize - new_delta_size);

                    _offlineDeltanum++;

                
            }
            free(recc_chunk);

            //Enclave::Logging("debug", " de xdelta end\n");
            if (delta_flag)
            {

                old_recipe->length = new_delta_size;

                //Enclave::Logging("debug", " 1\n");
                memcpy(&old_recipe->basechunkHash, (uint8_t*)&new_basechunkhash[0], CHUNK_HASH_SIZE);
                //Enclave::Logging("debug", " 2\n");
                uint8_t* new_delta_content_crypt = (uint8_t*)malloc(MAX_CHUNK_SIZE);
                if(!new_delta_content_crypt)
                {
                    Enclave::Logging("malloc", "new_delta_content_crypt\n");
                }
                //Enclave::Logging("debug", " 3\n");
                cryptoObj_->EncryptWithKeyIV(cipherCtx, new_delta_content, new_delta_size, Enclave::enclaveKey_, 
                    new_delta_content_crypt, old_chunk_IV);
                //Enclave::Logging("debug", " 4\n");
                InsertHot_container(new_delta_content_crypt, old_recipe, (uint8_t*)&old_basechunkhash[0], old_basechunksf, old_chunk_IV);
                //Enclave::Logging("debug", " 5\n");
                uint8_t* outRecipe = (uint8_t*)malloc(sizeof(RecipeEntry_t));
                //Enclave::Logging("debug", " 6\n");
                cryptoObj_->AESCBCEnc(cipherCtx, (uint8_t*)old_recipe, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, 
                    outRecipe);
                //Enclave::Logging("debug", " 7\n");
                bool status;


                //Ocall_UpdateIndexStoreBuffer(&status, &old_basechunkhash[0], CHUNK_HASH_SIZE, (uint8_t*)&outRecipe, sizeof(RecipeEntry_t));
                //Enclave::Logging("debug", " 8\n");
                // memcpy(&outEntry->chunkAddr, outRecipe, sizeof(RecipeEntry_t));

                status = UpdateIndexStore(old_basechunkhash, (char*)outRecipe, sizeof(RecipeEntry_t));

                //Enclave::Logging("debug", " 9\n");
                free(outRecipe);

                memset(upOutSGX->process_buffer, 0 ,1000*CHUNK_HASH_SIZE);
                memcpy(upOutSGX->process_buffer, (uint8_t*)&new_basechunkhash[0], CHUNK_HASH_SIZE);
                memcpy(upOutSGX->process_buffer + CHUNK_HASH_SIZE, (uint8_t*)&old_basechunkhash[0], CHUNK_HASH_SIZE);
                Ocall_UpdateDeltaIndex(upOutSGX->outClient);
                
                
                //Insert_delta(new_basechunkhash, old_basechunkhash);
                uint8_t* Coldbuffer = upOutSGX->out_buffer;
                memcpy(Coldbuffer,&old_basecontainerID[0],CONTAINER_ID_LENGTH);
                Coldbuffer += CONTAINER_ID_LENGTH;
                memcpy(Coldbuffer,&basepair.first,sizeof(uint32_t));
                Coldbuffer += sizeof(uint32_t);
                memcpy(Coldbuffer,&basepair.second,sizeof(uint32_t));

                Ocall_ColdInsert(upOutSGX->outClient);

                _offline_Ocall++;
                //cold_basemap[old_basecontainerID].push_back(basepair); 
                //Enclave::Logging("debug", " 10\n");
                free(new_delta_content_crypt);    
                free(new_delta_content);
            }

            //Enclave::Logging("debug", "Base end\n");
             
            free(old_chunk_content_crypt);
            free(old_chunk_IV);
            free(new_chunk_IV);
            free(old_chunk_content_decrypt);
            free(old_chunk_content_decompression);
            free(new_chunk_content_crypt);
            free(new_chunk_content_decrypt);
            free(new_chunk_content_decompression);   
        }
    }
    //Enclave::Logging("debug", "Cold begin\n");
    local_basemap.clear();
    }
 
    if(hot_container.currentSize!=0)
    {  
        Ocall_SavehotContainer(&hot_container.containerID[0], hot_container.body, hot_container.currentSize);

        _offline_Ocall++;
    }
    
    //Enclave::Logging("debug", "Cold update\n");
    Ocall_Coldrevise(upOutSGX->outClient);
    _offline_Ocall++;
    uint8_t* Coldbuffer = upOutSGX->out_buffer;
    Ocall_GetCold(upOutSGX->outClient);
    _offline_Ocall++;
    uint32_t ColdContainernum;
    memcpy(&ColdContainernum,Coldbuffer,sizeof(uint32_t));
    Coldbuffer += sizeof(uint32_t);

    
    while(ColdContainernum > 0){
        //Enclave::Logging("DEBUG", "cold map size:%d\n",ColdContainernum);
    for(int i = 0;i<ColdContainernum;i++){
        string ColdContainerID;
        ColdContainerID.assign((char*)Coldbuffer,CONTAINER_ID_LENGTH);
        //Enclave::Logging("DEBUG", "cold containerID :%s\n",ColdContainerID.c_str());
        Coldbuffer += CONTAINER_ID_LENGTH;
        uint32_t Coldnum;
        memcpy(&Coldnum,Coldbuffer,sizeof(uint32_t));
        Coldbuffer += sizeof(uint32_t);
        //Enclave::Logging("DEBUG", "cold num:%d\n",Coldnum);
        for(int j = 0;j < Coldnum;j++){
            pair<uint32_t,uint32_t> coldpair;
            uint32_t coldoffset;
            uint32_t coldlength;
            memcpy(&coldoffset,Coldbuffer,sizeof(uint32_t));
            Coldbuffer += sizeof(uint32_t);
            //Enclave::Logging("DEBUG", "offset:%d\n",coldoffset);
            memcpy(&coldlength,Coldbuffer,sizeof(uint32_t));
            Coldbuffer += sizeof(uint32_t);
            //Enclave::Logging("DEBUG", "length:%d\n",coldlength);
            coldpair = {coldoffset,coldlength};
            cold_basemap[ColdContainerID].push_back(coldpair);
        }
    }
    Ocall_GetCold(upOutSGX->outClient);
    _offline_Ocall++;
    Coldbuffer = upOutSGX->out_buffer;
    memcpy(&ColdContainernum,Coldbuffer,sizeof(uint32_t));
    Coldbuffer += sizeof(uint32_t);
    //Enclave::Logging("DEBUG", "next cold map size:%d\n",ColdContainernum);
    for (unordered_map<string, vector<pair<uint32_t, uint32_t>>>::iterator it = cold_basemap.begin(); it != cold_basemap.end(); it++)
    {
        Update_cold_container(upOutSGX, it->first, it->second, cryptoObj_, cipherCtx);
        _offlineDeletenum++;
        //Enclave::Logging("DEBUG", "Cold Container done:%s\n",it->first.c_str());
    }
    
    cold_basemap.clear();

    }

    Print();
    
    delta_map.clear();
    free(old_basechunksf);
    free(new_basechunksf);
    free(old_recipe);
    free(new_recipe);
    free(delta_recipe);
    //Enclave::Logging("debug", "Cold end\n");
    return;
}

void OFFLineBackward::Init() {
    Container_t* hot_container_ptr = &hot_container;
    Ocall_CreateUUID((uint8_t*)hot_container_ptr->containerID, CONTAINER_ID_LENGTH);
    string tmpContainerStr;
    tmpContainerStr.resize(CONTAINER_ID_LENGTH, 0);
    tmpContainerStr.assign((char*)hot_container_ptr->containerID, CONTAINER_ID_LENGTH);
    //Enclave::Logging("DEBUG", "Container id  is %s\n", tmpContainerStr.c_str());
    hot_container_ptr->currentSize = 0;
    //Enclave::Logging("DEBUG", "Init finished\n");
}

// RecipeEntry_t* GetRecipe(uint8_t* chunkhash) {

// }


uint8_t* OFFLineBackward::GetChunk_SF(uint8_t*tmpcontainer,RecipeEntry_t* tmprecipe)
{
    uint32_t offset = tmprecipe->offset;
    uint32_t length = tmprecipe->length;
    uint8_t *tmpchunkSF;
    string tmpContainerStr;
    tmpContainerStr.resize(CONTAINER_ID_LENGTH, 0);
    tmpContainerStr.assign((char *)tmprecipe->containerName, CONTAINER_ID_LENGTH);
    tmpchunkSF = (uint8_t *)malloc(3 * CHUNK_HASH_SIZE);
    memcpy(tmpchunkSF, tmpcontainer + offset + sizeof(RecipeEntry_t) + CHUNK_HASH_SIZE, 3 * CHUNK_HASH_SIZE);
    return tmpchunkSF;
}

uint8_t* OFFLineBackward::GetChunk_FP(uint8_t*tmpcontainer,RecipeEntry_t* tmprecipe)
{
    uint32_t offset = tmprecipe->offset;
    uint32_t length = tmprecipe->length;
    uint8_t *tmpchunkFP;
    string tmpContainerStr;
    tmpContainerStr.resize(CONTAINER_ID_LENGTH, 0);
    tmpContainerStr.assign((char *)tmprecipe->containerName, CONTAINER_ID_LENGTH);
    tmpchunkFP = (uint8_t *)malloc(CHUNK_HASH_SIZE);
    if(!tmpchunkFP)
    {
        Enclave::Logging("malloc", "tmpchunkFP\n");
    }
    memcpy(tmpchunkFP, tmpcontainer + offset + sizeof(RecipeEntry_t), CHUNK_HASH_SIZE);
    return tmpchunkFP;
}

uint8_t* OFFLineBackward::GetChunk_IV(uint8_t* tmpcontainer, RecipeEntry_t* tmprecipe)
{
    uint32_t offset = tmprecipe->offset;
    uint32_t length = tmprecipe->length;
    uint8_t* tmpchunkIV;
    string tmpContainerStr;
    tmpContainerStr.resize(CONTAINER_ID_LENGTH, 0);
    tmpContainerStr.assign((char *)tmprecipe->containerName, CONTAINER_ID_LENGTH);
    tmpchunkIV = (uint8_t*)malloc(CRYPTO_BLOCK_SIZE);
    if(!tmpchunkIV)
    {
        Enclave::Logging("malloc", "tmpchunkIV\n");
    }
    memcpy(tmpchunkIV, tmpcontainer + offset + sizeof(RecipeEntry_t) + 4 * CHUNK_HASH_SIZE + length, CRYPTO_BLOCK_SIZE);
    return tmpchunkIV;
}

uint8_t* OFFLineBackward::GetChunk_content(uint8_t*tmpcontainer,RecipeEntry_t* tmprecipe,bool cold_flag,pair<uint32_t,uint32_t> &tmppair)
{
    uint32_t offset = tmprecipe->offset;
    uint32_t length = tmprecipe->length;
     
    pair<uint32_t, uint32_t> _tmppair;
    _tmppair = {offset, length + 4 * CHUNK_HASH_SIZE + sizeof(RecipeEntry_t) + CRYPTO_BLOCK_SIZE};

    uint8_t *tmpchunkcontent;
    string tmpContainerStr;
    tmpContainerStr.resize(CONTAINER_ID_LENGTH, 0);
    tmpContainerStr.assign((char *)tmprecipe->containerName, CONTAINER_ID_LENGTH);
    if (cold_flag == true)
    {
        tmppair = _tmppair;
    }
    // ?
    //tmpchunkcontent = (uint8_t *)malloc(MAX_CHUNK_SIZE + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE);
    tmpchunkcontent = (uint8_t *)malloc(MAX_CHUNK_SIZE);
    memcpy(tmpchunkcontent, tmpcontainer + offset + sizeof(RecipeEntry_t) + 4 * CHUNK_HASH_SIZE, length);
    // tool::Logging(myName_.c_str(), "containerid: %s , offset: %d\n",tmpContainerStr.c_str(),offset);
    return tmpchunkcontent;
}

uint8_t* OFFLineBackward::GetNew_deltachunk(uint8_t *old_deltachunk, size_t old_deltasize, uint8_t *old_basechunk,size_t old_basesize, uint8_t* new_basechunk,size_t new_basesize,size_t *new_delta_size,bool &delta_flag)
{
    uint8_t *old_unique_chunk;
    size_t old_unique_chunk_size;
    old_unique_chunk = xd3_decode(old_deltachunk, old_deltasize, old_basechunk, old_basesize, &old_unique_chunk_size);

    // fprintf(stderr, "Unique chunk size:%d\n",old_unique_chunk_size);

    uint8_t *new_delta_chunk;
    size_t new_delta_chunk_size;
    new_delta_chunk = xd3_encode(old_unique_chunk, old_unique_chunk_size, new_basechunk, new_basesize, &new_delta_chunk_size);
    uint8_t *recc_chunk;
    size_t recc_size;
    recc_chunk = xd3_decode(new_delta_chunk, new_delta_chunk_size, new_basechunk, new_basesize, &recc_size);
    if (recc_size == 0)
    {
        //Enclave::Logging("DE BUG", "NO_Delta: %d\n",recc_size);
        delta_flag = false;
    }else{
        _offlineCompress_size -= old_deltasize;
        _offlineCompress_size += new_delta_chunk_size;

        _offlineCurrBackup_size -= old_deltasize;
        _offlineCurrBackup_size += new_delta_chunk_size;


        _Offline_DeltaSaveSize += (old_deltasize - new_delta_chunk_size);

        _DeltaSaveSize += (old_deltasize - new_delta_chunk_size);


        _deltaDataSize -= old_deltasize;
        _deltaDataSize += new_delta_chunk_size;

        _offlineDeltanum++;
        _offlineDeDeltanum++;

    }
    free(old_unique_chunk);
    free(recc_chunk);
    *new_delta_size = new_delta_chunk_size;
    return new_delta_chunk;
}

uint8_t* OFFLineBackward::xd3_decode(const uint8_t *in, size_t in_size, const uint8_t *ref, size_t ref_size, size_t *res_size)
{
    const auto max_buffer_size = (in_size + ref_size) * 2;
    uint8_t *buffer;
    buffer = (uint8_t *)malloc(max_buffer_size);
    size_t sz;
    xd3_decode_memory(in, in_size, ref, ref_size, buffer, &sz, max_buffer_size, 0);
    uint8_t *res;
    res = (uint8_t *)malloc(sz);
    if(!res)
    {
        Enclave::Logging("malloc", "decode res\n");
    }
    *res_size = sz;
    memcpy(res, buffer, sz);
    free(buffer);
    return res;
}

uint8_t* OFFLineBackward::xd3_encode(const uint8_t *in, size_t in_size, const uint8_t *ref, size_t ref_size, size_t *res_size)
{
    size_t sz;
    const auto max_buffer_size = (in_size + ref_size) * 2;
    uint8_t *buffer;
    buffer = (uint8_t *)malloc(max_buffer_size);
    xd3_encode_memory(in, in_size, ref, ref_size, buffer, &sz, max_buffer_size, 0);
    uint8_t *res;
    res = (uint8_t *)malloc(sz);
    if(!res)
    {
        Enclave::Logging("malloc", "encode res\n");
    }
    *res_size = sz;
    memcpy(res, buffer, sz);
    free(buffer);
    return res;
}

void OFFLineBackward::InsertHot_container(uint8_t* tmpchunkcontent, RecipeEntry_t* tmprecipe, uint8_t* tmpfp, uint8_t* tmpsf, 
uint8_t* tmpIV)
{
    Container_t *hot_container_ptr = &hot_container;
    uint32_t chunkSize = tmprecipe->length;
    uint32_t saveOffset = hot_container_ptr->currentSize; // TODO
    uint32_t writeOffset = saveOffset;
    if(chunkSize + saveOffset + sizeof(RecipeEntry_t) + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE < MAX_CONTAINER_SIZE) // container size enough
    {
        // write recipe into container
        tmprecipe->offset = saveOffset;
        tmprecipe->deltaFlag = 1;
        memcpy(&tmprecipe->containerName[0], (uint8_t*)&hot_container_ptr->containerID[0], CONTAINER_ID_LENGTH);
        
        memcpy(hot_container_ptr->body + writeOffset, (uint8_t*)tmprecipe, sizeof(RecipeEntry_t));
        writeOffset += sizeof(RecipeEntry_t);
        // write chunk hash int container
        memcpy(hot_container_ptr->body + writeOffset, tmpfp, CHUNK_HASH_SIZE);
        writeOffset += CHUNK_HASH_SIZE;
        memcpy(hot_container_ptr->body + writeOffset, tmpsf, 3 * CHUNK_HASH_SIZE);
        writeOffset += 3 * CHUNK_HASH_SIZE;
        memcpy(hot_container_ptr->body + writeOffset, tmpchunkcontent, chunkSize);
        writeOffset += chunkSize;
        memcpy(hot_container_ptr->body + writeOffset, tmpIV, CRYPTO_BLOCK_SIZE);
    } else
    {
        //SaveHot_container(hot_container);
        //Enclave::Logging("DEBUG", "hot container id is %s\n", hot_container.containerID);
        Ocall_SavehotContainer(&hot_container.containerID[0], hot_container.body, hot_container.currentSize);
        Ocall_CreateUUID((uint8_t*)&hot_container_ptr->containerID[0], CONTAINER_ID_LENGTH);
        string NewContainerStr;
        hot_container_ptr->currentSize = 0;
        // reset this container during the ocall
        saveOffset = 0;
        writeOffset = saveOffset;
        tmprecipe->offset = saveOffset;
        tmprecipe->deltaFlag = 1;
        memcpy(&tmprecipe->containerName[0], (uint8_t*)&hot_container_ptr->containerID[0], CONTAINER_ID_LENGTH);
        memcpy(hot_container_ptr->body + writeOffset, (uint8_t*)tmprecipe, sizeof(RecipeEntry_t));
        writeOffset += sizeof(RecipeEntry_t);
        // write chunk hash int container
        memcpy(hot_container_ptr->body + writeOffset, tmpfp, CHUNK_HASH_SIZE);
        writeOffset += CHUNK_HASH_SIZE;
        memcpy(hot_container_ptr->body + writeOffset, tmpsf, 3 * CHUNK_HASH_SIZE);
        writeOffset += 3 * CHUNK_HASH_SIZE;
        memcpy(hot_container_ptr->body + writeOffset, tmpchunkcontent, chunkSize);
        writeOffset += chunkSize;
        memcpy(hot_container_ptr->body + writeOffset, tmpIV, CRYPTO_BLOCK_SIZE);
    }
    hot_container_ptr->currentSize = hot_container_ptr->currentSize + chunkSize + sizeof(RecipeEntry_t) + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE;
}


void OFFLineBackward::Insert_delta(string basechunkhash, string deltachunkhash)
{
    delta_map[basechunkhash].push_back(deltachunkhash);
    return ;
}

bool OFFLineBackward::myCompare(pair<uint32_t, uint32_t> &v1, pair<uint32_t, uint32_t> &v2)
{
    return v1.first < v2.first;
}

void OFFLineBackward::Update_cold_container(UpOutSGX_t *upOutSGX, string container_id, vector<pair<uint32_t,uint32_t>> &cold_basechunk, 
EcallCrypto* cryptoObj_, EVP_CIPHER_CTX *cipherCtx)
{
    size_t Cold_container_size;
    bool delta_flag;
    // get container
    OutQueryEntry_t* outEntry = upOutSGX->outQuery->outQueryBase;
    memcpy(&outEntry->chunkAddr.containerName[0], (uint8_t*)&container_id[0], CONTAINER_ID_LENGTH);
    //Enclave::Logging("DBBUG", "container_id is %s\n", container_id.c_str());
    Ocall_OneColdContainer(upOutSGX->outClient, &delta_flag);
    Cold_container_size = outEntry->containersize;
    // memcpy(tmpColdContainer, outEntry->containerbuffer, Cold_container_size);
    uint8_t *Cold_container = outEntry->containerbuffer;
    
    //Enclave::Logging("CLOD", "container size  is %d\n", Cold_container_size);
    
    // sort
    sort(cold_basechunk.begin(), cold_basechunk.end(), myCompare);
    // for(int i=0;i<cold_basechunk.size();i++)
    //     Enclave::Logging("DBBUG", "offset is %d and len is %d\n", cold_basechunk[i].first, cold_basechunk[i].second);
    //Enclave::Logging("DBBUG", "sort finished\n");
    uint8_t* Cold_new_container;
    Cold_new_container = (uint8_t*)malloc(MAX_CONTAINER_SIZE);
    if(Cold_new_container == NULL){
        Enclave::Logging("DBBUG", "MALLOC NULLLLLLLLLLLL\n");
    }
    uint32_t saveset = 0;
    uint32_t newset = 0;
    uint32_t coldset = 0;
    RecipeEntry_t *tmpRecipeEncrypt;
    tmpRecipeEncrypt = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t));
    RecipeEntry_t *tmpRecipeDecrypt;
    tmpRecipeDecrypt = (RecipeEntry_t*)malloc(sizeof(RecipeEntry_t));
    size_t Cold_new_size = 0;

    //uint8_t* tmpHashStr = (uint8_t*)malloc(CHUNK_HASH_SIZE);
    string tmpHashStr;
    tmpHashStr.resize(CHUNK_HASH_SIZE, 0);
    // string tmpRecipeStr;
    // uint8_t* tmpRecipeStr = (uint8_t*)malloc(sizeof(RecipeEntry_t));
    // tmpRecipeStr.resize(sizeof(RecipeEntry_t), 0);

    //Enclave::Logging("DBBUG", "sort2222 finished\n");

   

    for (int i = 0; i < cold_basechunk.size(); i++)
    {
        //Enclave::Logging("DBBUG", "cold base chunk size is  %d\n", cold_basechunk.size());
        while (saveset < cold_basechunk[i].first)
        {
            //Enclave::Logging("DBBUG", "write cold container\n");
            memcpy((uint8_t*)tmpRecipeDecrypt, Cold_container + saveset, sizeof(RecipeEntry_t));
            //Enclave::Logging("DBBUG", "tmpRecipe offset is %d\n", tmpRecipeDecrypt->offset);
            tmpHashStr.assign((char*)(Cold_container + saveset + sizeof(RecipeEntry_t)), CHUNK_HASH_SIZE);
            //memcpy((uint8_t*)&tmpHashStr[0], Cold_container + saveset + sizeof(RecipeEntry_t), CHUNK_HASH_SIZE);
            saveset += sizeof(RecipeEntry_t);
            tmpRecipeDecrypt->offset = tmpRecipeDecrypt->offset - coldset;
            //Enclave::Logging(myName_.c_str(), "offset: %d\n",tmpRecipe->offset);
            //memset(tmpRecipeEncrypt, 0, sizeof(RecipeEntry_t));
            //Enclave::Logging(myName_.c_str(), "length: %d\n",tmpRecipeDecrypt->length);
            //Enclave::Logging(myName_.c_str(), "offset: %d\n",tmpRecipeDecrypt->offset);
            memcpy(Cold_new_container + newset, tmpRecipeDecrypt, sizeof(RecipeEntry_t));
            //Enclave::Logging("DBBUG", "memcpy1\n");
            newset += sizeof(RecipeEntry_t);
            memcpy(Cold_new_container + newset, Cold_container + saveset, tmpRecipeDecrypt->length + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE);
            //Enclave::Logging("DBBUG", "memcpy\n");
            saveset += tmpRecipeDecrypt->length + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE;
            newset += tmpRecipeDecrypt->length + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE;
            // tool::Logging(myName_.c_str(), "length: %d\n",tmpRecipe->length);
            cryptoObj_->AESCBCEnc(cipherCtx, (uint8_t*)tmpRecipeDecrypt, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)tmpRecipeEncrypt);
            //Enclave::Logging("DBBUG", "aes\n");
            //tmpRecipeStr.assign((char *)tmpRecipeEncrypt, sizeof(RecipeEntry_t));
            //memcpy(tmpRecipeStr, (uint8_t*)tmpRecipeEncrypt, sizeof(RecipeEntry_t));
            bool status;
            //Ocall_UpdateIndexStoreBuffer(&status, &tmpHashStr[0], CHUNK_HASH_SIZE, (uint8_t*)tmpRecipeEncrypt, sizeof(RecipeEntry_t));
            status = UpdateIndexStore(tmpHashStr, (char*)tmpRecipeEncrypt, sizeof(RecipeEntry_t));
            //Enclave::Logging("DBBUG", "update\n");
            
        }
        coldset += cold_basechunk[i].second;
        saveset += cold_basechunk[i].second;
    }
    //Enclave::Logging("DBBUG", "11111111111111111111\n");
    while (saveset < Cold_container_size)
    {
        //memset(tmpRecipeDecrypt, 0, sizeof(RecipeEntry_t));
        memcpy((uint8_t*)tmpRecipeDecrypt, Cold_container + saveset, sizeof(RecipeEntry_t));
        //cryptoObj_->AESCBCDec(cipherCtx, (uint8_t*)tmpRecipeEncrypt, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)tmpRecipeDecrypt);
        tmpHashStr.assign((char*)(Cold_container + saveset + sizeof(RecipeEntry_t)), CHUNK_HASH_SIZE);
        //memcpy(tmpHashStr, Cold_container + saveset + sizeof(RecipeEntry_t), CHUNK_HASH_SIZE);
        saveset += sizeof(RecipeEntry_t);
        tmpRecipeDecrypt->offset = tmpRecipeDecrypt->offset - coldset;
        // Enclave::Logging(myName_.c_str(), "length: %d\n",tmpRecipeDecrypt->length);
        // Enclave::Logging(myName_.c_str(), "offset: %d\n",tmpRecipeDecrypt->offset);
        //memset(tmpRecipeEncrypt, 0, sizeof(RecipeEntry_t));
    
        memcpy(Cold_new_container + newset, tmpRecipeDecrypt, sizeof(RecipeEntry_t));
        newset += sizeof(RecipeEntry_t);
        memcpy(Cold_new_container + newset, Cold_container + saveset, tmpRecipeDecrypt->length + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE);
        saveset += tmpRecipeDecrypt->length + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE;
        newset += tmpRecipeDecrypt->length + 4 * CHUNK_HASH_SIZE + CRYPTO_BLOCK_SIZE;

        cryptoObj_->AESCBCEnc(cipherCtx, (uint8_t*)tmpRecipeDecrypt, sizeof(RecipeEntry_t), Enclave::indexQueryKey_, (uint8_t*)tmpRecipeEncrypt);
        //tmpRecipeStr.assign((char *)tmpRecipeEncrypt, sizeof(RecipeEntry_t));
        bool status;
        //Ocall_UpdateIndexStoreBuffer(&status, &tmpHashStr[0], CHUNK_HASH_SIZE, (uint8_t*)tmpRecipeEncrypt, sizeof(RecipeEntry_t));
        status = UpdateIndexStore(tmpHashStr, (char*)tmpRecipeEncrypt, sizeof(RecipeEntry_t));
    }

    Cold_new_size = Cold_container_size - coldset;

    //Enclave::Logging("debug", "Container Cold\n");
    Ocall_SaveColdContainer(&container_id[0], Cold_new_container, Cold_new_size, &delta_flag);
    _offline_Ocall++;
    free(tmpRecipeEncrypt);
    free(tmpRecipeDecrypt);
    free(Cold_new_container);
    //Enclave::Logging(myName_.c_str(), "NEW_CONTAINER_down\n");
    return;
}

bool OFFLineBackward::UpdateIndexStore(const string& key, const char* buffer, 
    size_t bufferSize) {
    bool status;

    Ocall_UpdateIndexStoreBuffer(&status, key.c_str(), key.size(), 
        (const uint8_t*)buffer, bufferSize);

    _offline_Ocall++;
    return status;
}

void OFFLineBackward::CleanLocal_Index(){

    Ocall_CleanLocalIndex();
    _offline_Ocall++;
}