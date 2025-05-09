#include "../../include/ecallinContainercache.h"




/**
 * @brief Create a in-container cache object
 */
InContainercache::InContainercache() {
    this->inCache_ = new lru11::Cache<string, uint32_t>(Cachesize, 0);
    cacheSize_ = Cachesize;
    containerPool_ = (uint8_t**) malloc(cacheSize_ * sizeof(uint8_t*));
    for (size_t i = 0; i < cacheSize_; i++) {
        containerPool_[i] = (uint8_t*) malloc(MAX_CONTAINER_SIZE * sizeof(uint8_t));
    }
    currentIndex_ = 0;

    for(int i = 0; i < Cachesize; i++){
    SF_INC[i] = new unordered_map<string,vector<pair<RecipeEntry_t,string>>>[3];
    }  //(sf,pair(fp,recipe)) ->(sf,pair(re_offset,fp_offset))
}
/**
 * @brief Destory a in-container cache object
 */
InContainercache::~InContainercache() {
    for (size_t i = 0; i < cacheSize_; i++) {
        free(containerPool_[i]);
    }
    free(containerPool_);
    delete inCache_;

    for(int i = 0; i < Cachesize; i++){
    delete[] SF_INC[i];
    } 
    
}

/**
 * @brief Add a new container into in-container cache
 *
 * @param name the name of new container
 * @param data the pointer of the new container content
 * @param length the size of new container
 */
void InContainercache::InsertToCache(string& name, const char* data, uint32_t length) {
    int readset = 0;
    RecipeEntry_t tempRecipe;
    string tempHashstr;
    string tempSF;
    pair<RecipeEntry_t,string>temp_sf_re;
    pair<string,string>temp_sf_fp;
    uint32_t tempoffset;
    uint32_t tempoffset_recipe;



    if (inCache_->size() + 1 > cacheSize_) { 
        // evict a item
        uint32_t replaceIndex = inCache_->pruneValue();
        memcpy(containerPool_[replaceIndex], data, length);
        inCache_->insert(name, replaceIndex);
        for(int i = 0;i<3;i++){
            SF_INC[replaceIndex][i].clear();
        }

    while(readset<length){
        memcpy(&tempRecipe,data+readset,sizeof(RecipeEntry_t));
        tempoffset_recipe = readset;
        readset += sizeof(RecipeEntry_t);
        tempHashstr.assign(data+readset,CHUNK_HASH_SIZE);
        readset += CHUNK_HASH_SIZE;
        tempoffset = readset;
        temp_sf_re = {tempRecipe,tempHashstr};
        for(int i =0;i<3;i++){
        tempSF.assign(data+readset, CHUNK_HASH_SIZE);
        SF_INC[replaceIndex][i][tempSF].push_back(temp_sf_re);
        readset += CHUNK_HASH_SIZE;
        }
        readset += tempRecipe.length;
        readset += CRYPTO_BLOCK_SIZE;
    }
    } else {
        // directly using current index
        memcpy(containerPool_[currentIndex_], data, length);
        inCache_->insert(name, currentIndex_);
        while(readset<length){
        memcpy(&tempRecipe,data+readset,sizeof(RecipeEntry_t));
        readset += sizeof(RecipeEntry_t);
        tempHashstr.assign(data+readset,CHUNK_HASH_SIZE);
        readset += CHUNK_HASH_SIZE;
        tempoffset = tempRecipe.offset;
        temp_sf_re = {tempRecipe,tempHashstr};
        //update in-inclave sf-index
        for(int i =0;i<3;i++){
        tempSF.assign(data+readset, CHUNK_HASH_SIZE);
        SF_INC[currentIndex_][i][tempSF].push_back(temp_sf_re);
        readset += CHUNK_HASH_SIZE;
        }
        readset += tempRecipe.length;
        readset += CRYPTO_BLOCK_SIZE;
    } 
        currentIndex_++;
    }

    
    return;
}

/**
 * @brief Check the container whether it has been saved to the in-container cache
 *
 * @param name the name of the container
 * 
 * @return the query restult
 */
bool InContainercache::ExistsInCache(string& name) {
    bool flag = false;
    flag = this->inCache_->contains(name);
    return flag;
}

/**
 * @brief get container content from in-container cache
 *
 * @param name the name of the container
 * 
 * @return the pointer of the container content
 */
uint8_t* InContainercache::ReadFromCache(string& name) {
    uint32_t index = this->inCache_->get(name);
  //  Enclave::Logging(myName_.c_str(), "in-container id: %d\n",index);
    return containerPool_[index];
}

void InContainercache::InsertToCache_Offline(string& name, const char* data, uint32_t length) {
    int readset = 0;
    RecipeEntry_t tempRecipe;
    string tempHashstr;
    string tempSF;
    pair<RecipeEntry_t,string>temp_sf_re;
    pair<string,string>temp_sf_fp;
    uint32_t tempoffset;
    uint32_t tempoffset_recipe;

    length = MAX_CONTAINER_SIZE;

    if (inCache_->size() + 1 > cacheSize_) { 
        // evict a item
        uint32_t replaceIndex = inCache_->pruneValue();
        memcpy(containerPool_[replaceIndex], data, length);
        inCache_->insert(name, replaceIndex);

    } else {
        // directly using current index
        memcpy(containerPool_[currentIndex_], data, length);
        inCache_->insert(name, currentIndex_);
        currentIndex_++;
    }

    return;
}