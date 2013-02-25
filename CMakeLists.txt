cmake_minimum_required (VERSION 2.6)
project (pWeb)

set(LOG4CPP_INCLUDE_DIR "" CACHE FILEPATH "Path to log4cpp includes")
include_directories(${LOG4CPP_INCLUDE_DIR})

set(LOG4CPP_LINK_DIR "" CACHE FILEPATH "Path to log4cpp libraries")
link_directories(${LOG4CPP_LINK_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(PrecompiledHeader)

add_subdirectory( mongoose mongoose.lib )

include_directories( mongoose )

# At the moment (Feb 25), we're not using date_time directly.
# One of the other libraries depends on it and MSVC can't
# find it because cmake doesn't set the lib directory.

set( Boost_USE_STATIC_LIBS      ON  )
set( Boost_USE_MULTITHREADED    ON  )
set( Boost_USE_STATIC_RUNTIME   OFF )
find_package( Boost 1.53.0 COMPONENTS program_options thread date_time system )
include_directories( ${Boost_INCLUDE_DIRS} )

add_subdirectory( dnsgw dnsgw.bin )
add_subdirectory( pwclient pwclient.bin )
