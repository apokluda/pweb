
add_executable(dnsgw stdhdr.cpp main.cpp dnsspeaker.cpp dnsquery.cpp haproxy.cpp haloadbalancer.cpp)

add_precompiled_header(dnsgw stdhdr.hpp )

set(LOG4CPP_LIB log4cpp)
if (WIN32)
    set(LOG4CPP_LIB debug "log4cppD")
    set(LOG4CPP_LIB optimized "log4cppLIB")
endif (WIN32)

target_link_libraries(dnsgw ${LOG4CPP_LIB} ${Boost_LIBRARIES})

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(dnsgw rt)
endif()