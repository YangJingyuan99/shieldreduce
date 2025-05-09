/**
 * @file leveldbDatabase.h
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief implementation the database based on leveldb
 * @version 0.1
 * @date 2023-01-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef BASICDEDUP_LEVELDB_H
#define BASICDEDUP_LEVELDB_H

#include "absDatabase.h"
#include <leveldb/db.h>
#include <leveldb/cache.h>
#include "configure.h"
#include <bits/stdc++.h>


class LeveldbDatabase : public AbsDatabase {
    protected:
        /* data */
        leveldb::DB* levelDBObj_ = NULL;
        leveldb::Options options_;
    public:
        /**
         * @brief Construct a new leveldb Database object
         * 
         */
        LeveldbDatabase() {};
        /**
         * @brief Construct a new Database object
         * 
         * @param dbName the path of the db file
         */
        LeveldbDatabase(std::string dbName);
        /**
         * @brief Destroy the leveldb Database object
         * 
         */
        virtual ~LeveldbDatabase();

        /**
         * @brief open a database
         * 
         * @param dbName the db path 
         * @return true success
         * @return false fail
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
         * @brief query the (key, value) pair
         * 
         * @param key 
         * @param keySize 
         * @param value 
         * @return true 
         * @return false 
         */
        bool QuerySF(const char* key, size_t keySize, std::string& value);

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



#endif // !BASICDEDUP_LEVELDB_H