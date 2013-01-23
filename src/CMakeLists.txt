
add_executable(pweb main.cpp)

find_package(Threads)
find_package(OpenSSL)

message(STATUS ${OPENSSL_ROOT_DIR})
message(STATUS ${LIB_EAY_RELEASE})

target_link_libraries(pweb mongoose ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
