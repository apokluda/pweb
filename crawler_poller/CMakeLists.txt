add_executable( crawler main.cpp manconnection.cpp )

add_precompiled_header( crawler ../shared/stdhdr.hpp )

find_package( CURL REQUIRED )
include_directories( ${CURL_INCLUDE_DIRS} )
target_link_libraries( crawler shared log4cpp ${CURL_LIBRARIES} ${Boost_LIBRARIES} )

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries( crawler rt )
endif()
