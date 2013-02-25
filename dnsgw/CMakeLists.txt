
add_executable(dnsgw stdhdr.cpp main.cpp dnsspeaker.cpp dnsquery.cpp haspeaker.cpp)

add_precompiled_header(dnsgw stdhdr.hpp)

target_link_libraries(dnsgw log4cpp ${Boost_LIBRARIES} rt)