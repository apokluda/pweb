
add_executable(dnsgw_test_client main.cpp ha_config.cpp ha_register.cpp)

add_precompiled_header( dnsgw_test_client ../shared/stdhdr.hpp )

find_package( CURL REQUIRED )
include_directories( ${CURL_INCLUDE_DIRS} )
target_link_libraries(dnsgw_test_client shared log4cpp ${CURL_LIBRARIES} ${Boost_LIBRARIES})

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(dnsgw_test_client rt)
endif()
