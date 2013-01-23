
add_executable(pweb main.cpp)

find_package(Threads)
find_package(OpenSSL)

target_link_libraries(pweb mongoose ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
