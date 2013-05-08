add_library(shared STATIC stdhdr.cpp service.cpp asynchttprequester.cpp)
add_precompiled_header( shared ../shared/stdhdr.hpp .. )
