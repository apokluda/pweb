# This file is part of pWeb

# TODO: These definitions should be configurable by the user
# (We may want to keep these definitions as the default)
add_definitions( -DNO_CGI -DNO_SSL_DL )
#add_definitions( -DSSL_LIB="libssl.so.1.0.0" -DCRYPTO_LIB="libcrypto.so.1.0.0" )

add_library( mongoose STATIC mongoose.c )
set_property(TARGET mongoose PROPERTY COMPILE_DEFINITIONS_RELEASE NDEBUG)
