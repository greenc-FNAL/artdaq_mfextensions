cet_make_library(LIBRARY_NAME MFExtensions SOURCE
    throttle.cc
    suppress.cc
LIBRARIES
Boost::regex
TRACE::TRACE
    )

install_headers()
install_source()
