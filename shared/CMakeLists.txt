add_library(shared STATIC stdhdr.cpp metric.cpp)
add_precompiled_header(shared stdhdr.hpp )
