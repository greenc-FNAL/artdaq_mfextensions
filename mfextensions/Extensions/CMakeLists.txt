cet_make_library(LIBRARY_NAME MFExtensions SOURCE
    throttle.cc
    suppress.cc
LIBRARIES
${Boost_SYSTEM_LIBRARY}
${Boost_REGEX_LIBRARY}
    )

install_headers()
install_source()
