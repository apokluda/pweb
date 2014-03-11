add_executable( poller main.cpp manconnection.cpp signals.cpp poller.cpp instrumentation.cpp )
add_precompiled_header( poller ../shared/stdhdr.hpp .. )

find_package( CURL REQUIRED )
include_directories( ${CURL_INCLUDE_DIRS} )
target_link_libraries( poller protocol shared log4cpp ${CURL_LIBRARIES} ${Boost_LIBRARIES} )

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries( poller rt )
endif()
