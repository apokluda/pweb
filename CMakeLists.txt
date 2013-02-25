cmake_minimum_required (VERSION 2.6)
project (pWeb)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(PrecompiledHeader)

add_subdirectory( mongoose mongoose.lib )

include_directories( mongoose )

set( Boost_USE_STATIC_LIBS      ON  )
set( Boost_USE_MULTITHREADED    ON  )
set( Boost_USE_STATIC_RUNTIME   OFF )
find_package( Boost 1.53.0 COMPONENTS program_options thread system )
include_directories( ${Boost_INCLUDE_DIRS} )

add_subdirectory( dnsgw dnsgw.bin )
add_subdirectory( pwclient pwclient.bin )
