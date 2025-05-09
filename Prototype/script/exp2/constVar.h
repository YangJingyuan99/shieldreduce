/**
 * @file constVar.h
 * @author Ruilin Wu(202222080631@std.uestc.edu.cn)
 * @brief define the const variables
 * @version 0.1
 * @date 2023-12-10
 *
 * @copyright Copyright (c) 2020
 *
 */

#ifndef CONST_VAR_H
#define CONST_VAR_H

// for enclave lib path
static const char ENCLAVE_PATH[] = "../lib/storeEnclave.signed.so";

// For out-enclave breakdown
#define CHUNKING_BREAKDOWN 1
#define DATAWRITER_BREAKDOWN 0
#define RESTORE_WRITER_BREAKDOWN 0

// for GC
#define IS_MERGE_CONTAINER 1
#define QUICK_CHECK 0

// For SGX_Breakdown
#define SGX_BREAKDOWN 0
#define IMPACT_OF_TOP_K 1
#define MULTI_CLIENT 0
#define EDR_BREAKDOWN 1

#define INDEX_INFO 1
#define CHUNK_INFO 1
#define OFFLINE_INFO 1
#define SGX_INFO 1

#define OFFLINE_THRESHOLD 0.03
#define GREEDY_THRESHOLD 0.0
#define CONTAINER_SEPARATE 1
#define SF_SINGLE_THREAD 1
#define MeGA_THRESOLD 3
#define OFFLINE 1

// the length of the hash
enum ENCRYPT_SET
{
    AES_256_GCM = 0,
    AES_128_GCM = 1,
    AES_256_CFB = 2,
    AES_128_CFB = 3
};
enum HASH_SET
{
    SHA_256 = 0,
    MD5 = 1,
    SHA_1 = 2
};
static const uint32_t CRYPTO_BLOCK_SIZE = 16;
static const uint32_t CHUNK_HASH_SIZE = 32;
static const uint32_t CHUNK_ENCRYPT_KEY_SIZE = 32;
static const uint32_t HASH_TYPE = SHA_256;
static const uint32_t CIPHER_TYPE = AES_256_GCM;

// the size of chunk
static const uint32_t MIN_CHUNK_SIZE = 4096;
static const uint32_t AVG_CHUNK_SIZE = 8192;
static const uint32_t MAX_CHUNK_SIZE = 16384;

// the size of segment
static const uint32_t AVG_SEGMENT_SIZE = 1024 * 1024 * 10;     // 10MB default
static const uint32_t MIN_SEGMENT_SIZE = AVG_SEGMENT_SIZE / 2; // 5MB default
static const uint32_t MAX_SEGMENT_SIZE = AVG_SEGMENT_SIZE * 2; // 20MB default
static const uint32_t DIVISOR = ((AVG_SEGMENT_SIZE - MIN_SEGMENT_SIZE) / AVG_CHUNK_SIZE);
static const uint32_t PATTERN = 1;

// the type of chunker
enum CHUNKER_TYPE
{
    FIXED_SIZE_CHUNKING = 0,
    FAST_CDC,
    FSL_TRACE,
    UBC_TRACE
};

// the setting of the container
static const uint32_t MAX_CONTAINER_SIZE = 1 << 22; // container size: 4MB
static const uint32_t CONTAINER_ID_LENGTH = 7;
static const uint32_t SEGMENT_ID_LENGTH = 16;

// define the data type of the MQ
enum DATA_TYPE_SET
{
    DATA_CHUNK = 0,
    RECIPE_END,
    DATA_SEGMENT_END_FLAG
};

// configure for sparse index
static const uint32_t SPARSE_SAMPLE_RATE = 6;
static const uint32_t SPARSE_CHAMPION_NUM = 10;
static const uint32_t SPARSE_MANIFIEST_CAP_NUM = 10;

enum INDEX_TYPE_SET
{
    SECURE_MEGA = 0,
    DEBE = 1,
    FORWORD = 2,
    ONLY_FORWORD = 3,
    FREQ_INDEX = 4
};

enum SSL_CONNECTION_TYPE
{
    IN_SERVERSIDE = 0,
    IN_CLIENTSIDE
};

// for SSL connection
static const char SERVER_CERT[] = "../key/server/server.crt";
static const char SERVER_KEY[] = "../key/server/server.key";
static const char CLIENT_CERT[] = "../key/client/client.crt";
static const char CLIENT_KEY[] = "../key/client/client.key";
static const char CA_CERT[] = "../key/ca/ca.crt";

// for network message protocol code
enum PROTCOL_CODE_SET
{
    CLIENT_UPLOAD_CHUNK = 0,
    CLIENT_UPLOAD_RECIPE_END,
    CLIENT_LOGIN_UPLOAD,
    CLIENT_LOGIN_DOWNLOAD,
    CLIENT_RESTORE_READY,
    SERVER_RESTORE_CHUNK,
    SERVER_RESTORE_FINAL,
    SERVER_LOGIN_RESPONSE,
    SERVER_FILE_NON_EXIST,
    SGX_RA_MSG01,
    SGX_RA_MSG2,
    SGX_RA_MSG3,
    SGX_RA_MSG4,
    SGX_RA_NEED,
    SGX_RA_NOT_NEED,
    SGX_RA_NOT_SUPPORT,
    SESSION_KEY_INIT,
    SESSION_KEY_REPLY,
    CLIENT_LOGIN_OFFLINE
};

static const uint32_t CHUNK_QUEUE_SIZE = 8192;
static const uint32_t CONTAINER_QUEUE_SIZE = 32;
static const uint32_t CONTAINER_CAPPING_VALUE = 16;

static const uint32_t SGX_PERSISTENCE_BUFFER_SIZE = 2 * 1024 * 1024;

enum TWO_PATH_STATUS
{
    UNIQUE = 0,
    TMP_UNIQUE = 1,
    DUPLICATE = 2,
    TMP_DUPLICATE = 3
};

enum ENCLAVE_TRUST_STATUS
{
    ENCLAVE_TRUSTED = 0,
    ENCLAVE_UNTRUSTED = 1
};

enum JOB_DONE_FLAG
{
    NOT_DONE = 0,
    IS_DONE = 1
};

enum CONTAINER_TYPE
{
    BASE_CONTAINER = 0,
    DELTA_CONTAINER = 1
};

enum DELTA_STATUS
{
    NO_DELTA = 0,
    DELTA = 1,
    DELAY_DELTA = 2,
    OUT_DELTA = 3,
};

static const uint32_t MAX_SGX_MESSAGE_SIZE = 4 * 1024;

#define ENABLE_SGX_RA 0
#define TEST_IN_CSE 0

static const uint32_t THREAD_STACK_SIZE = 8 * 1024 * 1024;
static const uint32_t SESSION_KEY_BUFFER_SIZE = 65;

enum OPT_TYPE
{
    UPLOAD_OPT = 0,
    DOWNLOAD_OPT,
    RA_OPT,
    OFFLINE_OPT
};

enum LOCK_TYPE
{
    SESSION_LCK_WRITE = 0,
    SESSION_LCK_READ,
    TOP_K_LCK_WRITE,
    TOP_K_LCK_READ
};

#endif