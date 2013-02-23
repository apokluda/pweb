
add_executable(dnsgw main.cpp dnsspeaker.cpp dnsquery.cpp haspeaker.cpp)

add_precompiled_header(dnsgw stdhdr.hpp)

target_link_libraries(dnsgw log4cpp ${Boost_LIBRARIES})