#ifndef ECALL_NEW_In_CONTAINER_H
#define ECALL_NEW_In_CONTAINER_H

#if(MULTI_CLIENT == 0)
#define Cachesize 10
#else
#define Cachesize 10
#endif

#include "commonEnclave.h"
#include "functional"
#include <utility>
#include "../../../include/lruCache.h"
#include <iostream>
#include <stdio.h>
using namespace std;

class InContainercache{
    private:
        unordered_map<string,vector<pair<RecipeEntry_t,string>>> *SF_INC[Cachesize];
        lru11::Cache<string, uint32_t>* inCache_;
        uint8_t** containerPool_;
        uint64_t cacheSize_ = 0;
        size_t currentIndex_ = 0;
        string myName_ = "InContainercache";

    public:
        /**
         * @brief Construct a new Incantainercache object
         * 
         */
        InContainercache();

        /**
         * @brief Destory a new Incantainercache object
         * 
         */
        ~InContainercache();

        
        /**
         * @brief load the container of target chunk
         * 
         * @param name the string of container
         * @param data the pointer to the container buffer
         * @param length the length of container buffer
         */
        void InsertToCache(string& name, const char* data, uint32_t length);

        /**
         * @brief load the container of target chunk
         * 
         * @param name the string of container
         * @param data the pointer to the container buffer
         * @param length the length of container buffer
         */
        void InsertToCache_Offline(string& name, const char* data, uint32_t length);

        /**
         * @brief Confirm if the container is in the cache
         * 
         * @param name the string of container
         * @return true
         * @return false
         */
        bool ExistsInCache(string& name);

        /**
         * @brief Get the target container content
         * 
         * @param name the string of container
         * @return the pointer to the target container content
         */
        uint8_t* ReadFromCache(string& name);
        
};
#endif