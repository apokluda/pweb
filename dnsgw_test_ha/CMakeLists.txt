
add_executable(dnsgw_test_ha main.cpp)

add_precompiled_header(dnsgw_test_ha ../shared/stdhdr.hpp )

target_link_libraries(dnsgw_test_ha log4cpp ${Boost_LIBRARIES})

# On Linux we need to link with rt library
if(UNIX AND NOT APPLE)
    target_link_libraries(dnsgw_test_ha rt)
endif()