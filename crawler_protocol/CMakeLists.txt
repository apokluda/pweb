add_library( protocol STATIC messages.cpp bufreadwrite.cpp )
add_precompiled_header( protocol ../shared/stdhdr.hpp )
