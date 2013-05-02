add_library( protocol STATIC messages.cpp )
add_precompiled_header( protocol ../shared/stdhdr.hpp )
