cmake_minimum_required (VERSION 2.6)
project (pWeb)

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

add_subdirectory( shared shared.lib )
include_directories( shared )

# At the moment (Feb 25), we're not using date_time or regex directly.
# One of the other libraries depends on it and MSVC can't
# find it because cmake doesn't set the lib directory.

add_definitions(-DBOOST_ALL_DYN_LINK)

set( Boost_USE_STATIC_LIBS      OFF  )
set( Boost_USE_MULTITHREADED    ON  )
set( Boost_USE_STATIC_RUNTIME   OFF )
find_package( Boost 1.53.0 COMPONENTS program_options thread date_time chrono regex atomic serialization system REQUIRED )
include_directories( ${Boost_INCLUDE_DIRS} )

add_subdirectory( dnsgw dnsgw.bin )
add_subdirectory( dnsgw_test_client dnsgw_test_client.bin )

# klink headers unnecessarily pull in plafom-specific stuff
if (UNIX)
    add_subdirectory( dnsgw_test_ha dnsgw_test_ha.bin )
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
