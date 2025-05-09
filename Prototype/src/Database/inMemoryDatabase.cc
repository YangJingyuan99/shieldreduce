/**
 * @file inMemoryDatabase.cc
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief implement the interface of in-memory index
 * @version 0.1
 * @date 2023-05-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/inMemoryDatabase.h"


/**
 * @brief Construct a new In Memory Database object
 * 
 * @param dbName the path of the db file
 */
InMemoryDatabase::InMemoryDatabase(std::string dbName) {
    this->OpenDB(dbName);
}

/**
 * @brief Destroy the In Memory Database object
 * 
 */
InMemoryDatabase::~InMemoryDatabase() {
    // perisistent the indexFile to the disk
    ofstream dbFile;
    dbFile.open(dbName_, ios_base::trunc | ios_base::binary);
    int itemSize = 0;
    for (auto it = indexObj_.begin(); it != indexObj_.end(); it++) {
        // write the key
        itemSize = it->first.size();
        dbFile.write((char*)&itemSize, sizeof(itemSize));
        dbFile.write(it->first.c_str(), itemSize);

        // write the value
        itemSize = it->second.size();
        dbFile.write((char*)&itemSize, sizeof(itemSize));
        dbFile.write(it->second.c_str(), itemSize);
    }
    dbFile.close();


    sfdbName_ = dbName_+"_sf1";
    ofstream sfdbFile;
    sfdbFile.open(sfdbName_, ios_base::trunc | ios_base::binary);
    itemSize = 0;
    int sf_version = 0; //sf_version
    for(int i = 0 ;i<3;i++){
        sf_version = i;
        for(auto it = sfObj_[i].begin();it != sfObj_[i].end();it++){
            for(int j = 0;j < it->second.size();j++){
            //write sf_version
            sfdbFile.write((char*)&sf_version,sizeof(sf_version));
            //write the key
            itemSize = it->first.size();
            sfdbFile.write((char*)&itemSize, sizeof(itemSize));
            sfdbFile.write(it->first.c_str(), itemSize);
            //write the value
            itemSize = it->second[j].size();
            sfdbFile.write((char*)&itemSize, sizeof(itemSize));
            sfdbFile.write(it->second[j].c_str(), itemSize);
            }
        }

    }
    sfdbFile.close();
}

/**
 * @brief open a database
 * 
 * @param dbName the db path
 * @return true success
 * @return false fails
 */
bool InMemoryDatabase::OpenDB(std::string dbName) {
    dbName_ = dbName;
    // check whether there exists the index
    ifstream dbFile;
    dbFile.open(dbName_, ios_base::in | ios_base::binary);
    if (!dbFile.is_open()) {
        fprintf(stderr, "InMemoryDatabase: cannot open the db file.\n");
    }

    size_t beginSize = dbFile.tellg();
    dbFile.seekg(0, ios_base::end);
    size_t fileSize = dbFile.tellg();
    fileSize = fileSize - beginSize;

    if (fileSize == 0) {
        // db file not exist
        fprintf(stderr, "InMemoryDatabase: db file file is not exists, create a new one.\n");
    } else {
        // db file exist, load
        dbFile.seekg(0, ios_base::beg);
        bool isEnd = false;
        int itemSize = 0;
        string key;
        string value;
        while (!isEnd) {
            // read key
            dbFile.read((char*)&itemSize, sizeof(itemSize));
            if (itemSize == 0) {
                break;
            }
            key.resize(itemSize, 0); 
            dbFile.read((char*)&key[0], itemSize);

            // read value
            dbFile.read((char*)&itemSize, sizeof(itemSize));
            value.resize(itemSize, 0);
            dbFile.read((char*)&value[0], itemSize);
            
            // update the index
            indexObj_.insert(make_pair(key, value));
            itemSize = 0;
            // update the read flag
            isEnd = dbFile.eof();
        }
    }
    dbFile.close();
    fprintf(stderr, "InMemoryDatabase: loaded FP index size: %lu\n", indexObj_.size());


    sfObj_ = new unordered_map<string,vector<string>>[3];
    fprintf(stderr, "InMemoryDatabase: sfdb_index is created\n");

    sfdbName_ = dbName+"_sf1";
    ifstream sfdbFile;
    sfdbFile.open(sfdbName_, ios_base::in | ios_base::binary);
    if (!sfdbFile.is_open()) {
        fprintf(stderr, "InMemoryDatabase: cannot open the sfdb file.\n");
    }
    size_t beginsfSize = sfdbFile.tellg();
    sfdbFile.seekg(0, ios_base::end);
    size_t filesfSize = sfdbFile.tellg();
    filesfSize = filesfSize - beginsfSize;
    if (filesfSize == 0) {
        fprintf(stderr, "InMemoryDatabase: sfdb_file file is empty, create a new one.\n");
    } else {
        // db file exist, load
        sfdbFile.seekg(0, ios_base::beg);
        bool isEnd = false;
        int itemSize = 0;
        int sf_version =0;
        string key;
        string value;
        while (!isEnd) {
            //read sf_version 
            sfdbFile.read((char*)&sf_version, sizeof(sf_version));

            // read key
            sfdbFile.read((char*)&itemSize, sizeof(itemSize));
            if (itemSize == 0) {
                break;
            }
            key.resize(itemSize, 0); 
            sfdbFile.read((char*)&key[0], itemSize);

            // read value
            sfdbFile.read((char*)&itemSize, sizeof(itemSize));
            value.resize(itemSize, 0);
            sfdbFile.read((char*)&value[0], itemSize);
            
            // update the index
            sfObj_[sf_version][key].push_back(value);
            itemSize = 0;
            // update the read flag
            isEnd = sfdbFile.eof();
        }
    }
    sfdbFile.close();
    int sfObj_size = sfObj_[0].size()+sfObj_[1].size()+sfObj_[2].size();
    fprintf(stderr, "InMemoryDatabase: loaded SF size: %d\n",sfObj_size);

    return true;
}

/**
 * @brief execute query over database
 * 
 * @param key key
 * @param value value
 * @return true success
 * @return false fail
 */
bool InMemoryDatabase::Query(const std::string& key, std::string& value) {
    auto findResult = indexObj_.find(key);
    if (findResult != indexObj_.end()) {
        // it exists in the index
        value.assign(findResult->second);
        return true;
    } 
    return false;
}


/**
 * @brief insert the (key, value) pair
 * 
 * @param key key
 * @param value value
 * @return true success
 * @return false fail
 */
bool InMemoryDatabase::Insert(const std::string& key, const std::string& value) {
    indexObj_[key] = value;
    return true;
}


/**
 * @brief insert the (key, value) pair
 * 
 * @param key 
 * @param buffer 
 * @param bufferSize 
 * @return true 
 * @return false 
 */
bool InMemoryDatabase::InsertBuffer(const std::string& key, const char* buffer, size_t bufferSize) {
    string valueStr;
    valueStr.assign(buffer, bufferSize);
    indexObj_[key] = valueStr;
    return true;
}

/**
 * @brief insert the (key, value) pair
 * 
 * @param key 
 * @param keySize 
 * @param buffer 
 * @param bufferSize 
 * @return true 
 * @return false 
 */
bool InMemoryDatabase::InsertBothBuffer(const char* key, size_t keySize, const char* buffer,
    size_t bufferSize) {
    string keyStr;
    string valueStr;
    keyStr.assign(key, keySize);
    valueStr.assign(buffer, bufferSize);
    indexObj_[keyStr] = valueStr;
    return true;
}

/**
 * @brief query the (key, value) pair
 * 
 * @param key 
 * @param keySize 
 * @param value 
 * @return true 
 * @return false 
 */
bool InMemoryDatabase::QueryBuffer(const char* key, size_t keySize, std::string& value) {
    string keyStr;
    keyStr.assign(key, keySize);
    auto findResult = indexObj_.find(keyStr);
    if (findResult != indexObj_.end()) {
        // it exists in the index
        value.assign(findResult->second);
        return true;
    }
    return false;
}

/**
 * @brief insert the superfeature(sf,fp) pair
 * 
 * @param key superfeature
 * @param keySize suprerfeature size
 * @param buffer chunk hash
 * @param bufferSize the size of chunkhash
 * @param updateflag the flag of update SFindex
 * @return true 
 * @return false 
 */
bool InMemoryDatabase::InsertSF(const char* key, size_t keySize, const char* buffer,
    size_t bufferSize, uint8_t updateflag) {
    string keyStr;
    string valueStr;
    for(int i =0;i<3;i++){
        keyStr.assign(key+i*CHUNK_HASH_SIZE, CHUNK_HASH_SIZE);
        valueStr.assign(buffer, CHUNK_HASH_SIZE);
        if(updateflag == 0){
            sfObj_[i][keyStr].push_back(valueStr);
        }else if(updateflag == 1){
            //fprintf(stderr, "update\n");
            sfObj_[i].erase(keyStr);
            sfObj_[i][keyStr].push_back(valueStr);
        }else if(updateflag == 2){
            sfObj_[i].erase(keyStr);
        }
        //sfObj_[i][keyStr].push_back(valueStr);
    }
    return true;
}


/**
 * @brief query the superfeature(sf,fp) pair
 * 
 * @param key superfeature
 * @param keySize suprerfeature size
 * @param value the string of basechunk hash
 * @return true 
 * @return false 
 */
bool InMemoryDatabase::QuerySF(const char* key, size_t keySize, std::string& value){
    string keyStr;

    for(int i=0;i<3;i++){
        keyStr.assign(key+CHUNK_HASH_SIZE*i, CHUNK_HASH_SIZE);
        if(sfObj_[i].count(keyStr)){
            //fprintf(stderr, "the %d size\n",sfObj_[i][keyStr].size());
        value=sfObj_[i][keyStr].front();
        //
        return true;
        }
        // it exists in the index
    }
    return false;
}

/**
* @brief insert the DeltaIndex(basechunk,deltachunk) pair
 * 
 * @param baseChunkFp basechunk hash
 * @param deltaChunkFp deltachunk hash
 */
void InMemoryDatabase::InsertDeltaIndex(std::string baseChunkFp, std::string deltaChunkFp)
{
    delta_index[baseChunkFp].push_back(deltaChunkFp);
}

/**
 * @brief query the DeltaIndex(basechunk,deltachunk) pair
 * 
 * @param baseChunkFp basechunk hash
 * @param result the vector of deltachunks
 * @return true 
 * @return false 
 */
bool InMemoryDatabase::QueryDeltaIndex(std::string baseChunkFp, std::vector<string>& result)
{
    if(delta_index.find(baseChunkFp) != delta_index.end())
    {
        result.assign(delta_index[baseChunkFp].begin(), delta_index[baseChunkFp].end());
        delta_index.erase(baseChunkFp);
        return true;
    }else
    {
    return false;
    }
}

/**
 * @brief get size of all Indexes
 * @return true 
 * @return false 
 */
bool InMemoryDatabase::GetIndexSize()
{
    uint64_t tmpSize = 0;
    uint64_t tmpNum = 0;

    for(unordered_map<string, vector<string>>::iterator it = delta_index.begin(); it != delta_index.end(); it++){
        tmpSize += CHUNK_HASH_SIZE;
        tmpSize += it->second.size()*CHUNK_HASH_SIZE;
        tmpNum += it->second.size();
    }
    
    deltamapsize = tmpSize;
    fpindexsize = indexObj_.size() * (CHUNK_HASH_SIZE + 48);
    tmpSize = 0;
    for(int i = 0;i < 3;i++){
        tmpSize += sfObj_[i].size()*2*CHUNK_HASH_SIZE;
    }
    sfindexsize = tmpSize;
    return true;
}