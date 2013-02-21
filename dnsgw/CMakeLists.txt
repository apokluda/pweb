
add_executable(dnsgw main.cpp)

add_precompiled_header(dnsgw stdhdr.h)

target_link_libraries(dnsgw log4cpp ${Boost_LIBRARIES})
