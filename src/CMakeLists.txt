
add_executable(pweb main.cpp)

find_package(Threads)

if (WIN32)
    set( OPENSSL_ROOT_DIR C:/OpenSSL-Win64/ )
endif ()

find_package(OpenSSL)

target_link_libraries(pweb mongoose ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
