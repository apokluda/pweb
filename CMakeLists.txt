cmake_minimum_required (VERSION 2.6)
project (pWeb)

add_subdirectory( mongoose lib )

include_directories( mongoose )

add_subdirectory( src bin )
