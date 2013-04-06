
add_executable(pweb main.cpp)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(pweb mongoose ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
