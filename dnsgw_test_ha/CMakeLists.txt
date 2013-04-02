
add_executable(dnsgw_test stdhdr.cpp main.cpp)

add_precompiled_header(dnsgw_test stdhdr.hpp )

set(LOG4CPP_LIB log4cpp)
if (WIN32)
    set(LOG4CPP_LIB debug "log4cppD")
    set(LOG4CPP_LIB optimized "log4cppLIB")
endif (WIN32)

target_link_libraries(dnsgw_test ${LOG4CPP_LIB} ${Boost_LIBRARIES})

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(dnsgw_test rt)
endif()