/**
 * @file inMemoryIndex.h
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief implement a in-memory index
 * @version 0.1
 * @date 2023-05-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef IN_MEMORY_INDEX_H
#define IN_MEMORY_INDEX_H

#include "absDatabase.h"
#include "configure.h"

class InMemoryDatabase : public AbsDatabase {
    protected:
        /*data*/
        unordered_map<string, string> indexObj_;
        unordered_map<string,vector<string>>* sfObj_;

    public:
        /**
         * @brief Construct a new In Memory Database object
         * 
         */
        InMemoryDatabase() {};

        /**
         * @brief Construct a new In Memory Database object
         * 
         * @param dbName the path of the db file
         */
        InMemoryDatabase(std::string dbName);

        /**
         * @brief Destroy the In Memory Database object
         * 
         */
        virtual ~InMemoryDatabase();

        /**
         * @brief open a database
         * 
         * @param dbName the db path
         * @return true success
         * @return false fails
         */
        bool OpenDB(std::string dbName);

        /**
         * @brief execute query over database
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        bool Query(const std::string& key, std::string& value);

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        bool Insert(const std::string& key, const std::string& value);


        /**
         * @brief insert the (key, value) pair
         * 
         * @param key 
         * @param buffer 
         * @param bufferSize 
         * @return true 
         * @return false 
         */
        bool InsertBuffer(const std::string& key, const char* buffer, size_t bufferSize);

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
        bool InsertBothBuffer(const char* key, size_t keySize, const char* buffer,
            size_t bufferSize);

        /**
         * @brief query the (key, value) pair
         * 
         * @param key 
         * @param keySize 
         * @param value 
         * @return true 
         * @return false 
         */
        bool QueryBuffer(const char* key, size_t keySize, std::string& value);

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
        bool InsertSF(const char* key, size_t keySize, const char* buffer,size_t bufferSize, uint8_t updateflag);

        /**
         * @brief query the superfeature(sf,fp) pair
         * 
         * @param key superfeature
         * @param keySize suprerfeature size
         * @param value the string of basechunk hash
         * @return true 
         * @return false 
         */
        bool QuerySF(const char* key, size_t keySize, std::string& value);

        /**
         * @brief insert the DeltaIndex(basechunk,deltachunk) pair
         * 
         * @param baseChunkFp basechunk hash
         * @param deltaChunkFp deltachunk hash
         */
        void InsertDeltaIndex(std::string baseChunkFp, std::string deltaChunkFp);

        /**
         * @brief query the DeltaIndex(basechunk,deltachunk) pair
         * 
         * @param baseChunkFp basechunk hash
         * @param result the vector of deltachunks
         * @return true 
         * @return false 
         */
        bool QueryDeltaIndex(std::string baseChunkFp, std::vector<string>& result);

        /**
         * @brief get size of all Indexes
         * @return true 
         * @return false 
         */
        bool GetIndexSize();



};

#endif