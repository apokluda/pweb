
add_executable(pweb main.cpp)

find_package(Threads)
find_package(OpenSSL)

find_library(LIB_EAY_RELEASE
                NAMES
                    libeay32MD
                    libeay32
                PATHS
                    ${OPENSSL_ROOT_DIR}/lib/VC
            )

            find_library(SSL_EAY_DEBUG
                NAMES
                    ssleay32MDd
                    ssleay32
                    ssl
                PATHS ${OPENSSL_ROOT_DIR}/lib/VC
            )

            find_library(SSL_EAY_RELEASE
                NAMES
                    ssleay32MD
                    ssleay32
                    ssl
                PATHS
                    ${OPENSSL_ROOT_DIR}/lib/VC
            )

target_link_libraries(pweb mongoose ${OPENSSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

