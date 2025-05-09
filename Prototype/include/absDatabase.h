/**
 * @file absDatabase.h
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief 
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef BASICDEDUP_ABS_DATABASE_H
#define BASICDEDUP_ABS_DATABASE_H

#include "configure.h"

using namespace std;

class AbsDatabase {
    protected:
        // the name of the database
        std::string dbName_;
        std::string sfdbName_;
    public:
        // for delta index
        unordered_map<string, vector<string>> delta_index;
        // for local index
        unordered_map<string,string> _local_tmp_map;
        vector<pair<string,string>> _local_map;
        unordered_map<string, vector<string>> skipMap;
        // for cold index
        unordered_map<string,vector<pair<uint32_t,uint32_t>>> _cold_map;
        vector<string> _cold_container_vector;
        set<string> _cold_container_set;

        //for index size
        uint64_t deltamapsize;
        uint64_t fpindexsize;
        uint64_t sfindexsize;

        /**
         * @brief Construct a new Abs Database object
         * 
         */
        AbsDatabase(); 

        /**
         * @brief Destroy the Abs Database object
         * 
         */
        virtual ~AbsDatabase() {}; 

        /**
         * @brief open a database
         * 
         * @param dbName the db path 
         * @return true success
         * @return false fail
         */
        virtual bool OpenDB(std::string dbName) = 0;


        /**
         * @brief execute query over database
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        virtual bool Query(const std::string& key, std::string& value) = 0;

        /**
         * @brief insert the (key, value) pair
         * 
         * @param key key
         * @param value value
         * @return true success
         * @return false fail
         */
        virtual bool Insert(const std::string& key, const std::string& value) = 0;


        /**
         * @brief insert the (key, value) pair
         * 
         * @param key 
         * @param buffer 
         * @param bufferSize 
         * @return true 
         * @return false 
         */
        virtual bool InsertBuffer(const std::string& key, const char* buffer, size_t bufferSize) = 0;

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
        virtual bool InsertBothBuffer(const char* key, size_t keySize, const char* buffer,
            size_t bufferSize) = 0;

        /**
         * @brief query the (key, value) pair
         * 
         * @param key 
         * @param keySize 
         * @param value 
         * @return true 
         * @return false 
         */
        virtual bool QueryBuffer(const char* key, size_t keySize, std::string& value) = 0;

        /**
         * @brief query the superfeature(sf,fp) pair
         * 
         * @param key superfeature
         * @param keySize suprerfeature size
         * @param value the string of basechunk hash
         * @return true 
         * @return false 
         */
        virtual bool QuerySF(const char* key, size_t keySize, std::string& value) = 0;

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
        virtual bool InsertSF(const char* key, size_t keySize, const char* buffer,size_t bufferSize, uint8_t updateflag) = 0;

        /**
         * @brief insert the DeltaIndex(basechunk,deltachunk) pair
         * 
         * @param baseChunkFp basechunk hash
         * @param deltaChunkFp deltachunk hash
         */
        virtual void InsertDeltaIndex(std::string baseChunkFp, std::string deltaChunkFp) = 0;

        /**
         * @brief query the DeltaIndex(basechunk,deltachunk) pair
         * 
         * @param baseChunkFp basechunk hash
         * @param result the vector of deltachunks
         * @return true 
         * @return false 
         */
        virtual bool QueryDeltaIndex(std::string baseChunkFp, std::vector<std::string>& result) = 0;

        /**
         * @brief get size of all Indexes
         * @return true 
         * @return false 
         */
        virtual bool GetIndexSize() = 0;
};


#endif // !BASICDEDUP_ABS_DATABASE_H

