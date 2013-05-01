
add_executable(dnsgw main.cpp dnsspeaker.cpp dnsquery.cpp haproxy.cpp haloadbalancer.cpp instrumenter.cpp)

add_precompiled_header(dnsgw ../shared/stdhdr.hpp )

target_link_libraries(dnsgw instrumentation shared log4cpp ${Boost_LIBRARIES})

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(dnsgw rt)
endif()

install(TARGETS dnsgw DESTINATION bin)