add_executable( test test.cpp )
add_precompiled_header( test ../shared/stdhdr.hpp .. )

target_link_libraries( test ${Boost_LIBRARIES} )