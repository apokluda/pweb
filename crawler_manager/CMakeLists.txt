add_executable( manager main.cpp signals.cpp pollerconnector.cpp )

add_precompiled_header( manager ../shared/stdhdr.hpp )

target_link_libraries( manager shared log4cpp ${Boost_LIBRARIES} )

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries( manager rt )
endif()
