
add_executable(dnsgw stdhdr.cpp main.cpp dnsspeaker.cpp dnsquery.cpp haspeaker.cpp)

add_precompiled_header(dnsgw stdhdr.hpp )

set(LOG4CPP_LIB log4cpp)
if (WIN32)
    set(LOG4CPP_LIB "log4cppLIB")
endif (WIN32)

target_link_libraries(dnsgw ${LOG4CPP_LIB} ${Boost_LIBRARIES} rt)