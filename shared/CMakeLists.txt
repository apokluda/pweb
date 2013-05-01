add_library(shared STATIC stdhdr.cpp service.cpp)
add_precompiled_header(shared stdhdr.hpp)
