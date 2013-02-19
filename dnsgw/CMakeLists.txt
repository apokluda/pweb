
add_executable(dnsgw main.cpp)

add_precompiled_header(dnsgw stdhdr.h)

find_package(Threads)

target_link_libraries(dnsgw ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
