cmake_minimum_required (VERSION 2.6)
project (pWeb)

# Note: There are a few things that could use fixing up when I have the time;
# 1) Currently the same pre-compiled header is generated in each subdirectory.
#    This isn't really an issue on my desktop. By default, CMake runs "make -j8"
#    so all the precompiled headers are build in parallel, but on a slower
#    machine or one with fewer cores, this could make a difference. I could
#    probably edit the <project_dir>/cmake/Modules/PrecompiledHeader.cmake
#    file in order to share the same precompiled header accross all subdirectories.
# 2) The dnsgw_test_client, crawler and manager do not compile on Windows
#    because cURL is missing on the build machines.

set(pWeb_VERSION_MAJOR 1)
set(pWeb_VERSION_MINOR 0)
set(pWeb_VERSION_BUILD 0)

configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)
include_directories("${PROJECT_BINARY_DIR}")

set(LOG4CPP_INCLUDE_DIR "" CACHE FILEPATH "Path to log4cpp includes")
include_directories(${LOG4CPP_INCLUDE_DIR})

set(LOG4CPP_LINK_DIR "" CACHE FILEPATH "Path to log4cpp libraries")
link_directories(${LOG4CPP_LINK_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(PrecompiledHeader)

add_subdirectory( mongoose mongoose.lib )
include_directories( mongoose )

add_definitions(-DBOOST_ALL_DYN_LINK)

set( Boost_USE_STATIC_LIBS      OFF  )
set( Boost_USE_MULTITHREADED    ON  )
set( Boost_USE_STATIC_RUNTIME   OFF )
find_package( Boost 1.53.0 COMPONENTS program_options thread date_time chrono atomic serialization system REQUIRED )
include_directories( ${Boost_INCLUDE_DIRS} )

# Must come after Boost stuff
add_subdirectory( shared shared.lib )
include_directories( shared )

add_subdirectory( dnsgw_instrumentation instrumentation.lib )
include_directories( dnsgw_instrumentation )

add_subdirectory( dnsgw dnsgw.bin )

add_subdirectory( crawler_protocol crawler_protocol.lib )
include_directories( crawler_protocol )

add_subdirectory( crawler_manager crawler_manager.bin )

add_subdirectory( crawler_poller crawler_poller.bin)

if (UNIX)
    # klink headers unnecessarily pull in plafom-specific stuff and don't work on Windows
    add_subdirectory( dnsgw_test_ha dnsgw_test_ha.bin )
    
    # The the dnsgw_test_client needs libcurl on Windows. There are Windows versions of
    # libcur available on the cURL download page at http://curl.haxx.se/download.html#Win32
    # but I don't have the time to get cURL working on Windows at the moment--other priorities
    # take precidence.
    add_subdirectory( dnsgw_test_client dnsgw_test_client.bin )
endif()

# Not much is happening in this folder right now
#add_subdirectory( pwclient pwclient.bin )

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE  
     "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set (CPACK_PACKAGE_VERSION_MAJOR "${pWeb_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${pWeb_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${pWeb_VERSION_BUILD}")
include (CPack)
