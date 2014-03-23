#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")

add_executable( asynchttprequest_test main.cpp )

find_package( CURL REQUIRED )
include_directories( ${CURL_INCLUDE_DIRS} )

target_link_libraries( asynchttprequest_test shared ${CURL_LIBRARIES} ${Boost_LIBRARIES} )
