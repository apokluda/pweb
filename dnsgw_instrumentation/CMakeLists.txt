add_library(instrumentation STATIC metric.cpp)
add_precompiled_header( instrumentation ../shared/stdhdr.hpp .. )