#src/app
set(SGXOPENSSL_INCLUDE_PATH /opt/intel/sgxssl/include)
set(SGXOPENSSL_LIBRARY_PATH /opt/intel/sgxssl/lib64)
include_directories(${SGXOPENSSL_INCLUDE_PATH})
link_directories(${SGXOPENSSL_LIBRARY_PATH})

add_executable(ShieldReduceServer dbeServer.cc)
target_link_libraries(ShieldReduceServer ${FINAL_OBJ})

add_executable(ShieldReduceClient dbeClient.cc)
target_link_libraries(ShieldReduceClient ${FINAL_OBJ})

add_executable(compressSYN synCompress.cc)
target_link_libraries(compressSYN lz4 UtilCore ${OPENSSL_LIBRARY_OBJ})