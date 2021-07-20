
find_package(CURL 7.20.0)

if(CURL_FOUND)
  include_directories(${CURL_INCLUDE_DIRS})
  SET(CMAKE_C_FLAGS "-std=gnu11 -fPIC -Wno-implicit-fallthrough" CACHE STRING "" FORCE)
cet_make_library(LIBRARY_NAME curl_send_message 
                 SOURCE detail/curl_send_message.c
                 LIBRARIES 
				 ${CURL_LIBRARIES}
                           WITH_STATIC_LIBRARY)

basic_plugin( "SMTP" "mfPlugin"
curl_send_message
  MF_MessageLogger
${MF_MESSAGESERVICE}
    fhiclcpp
    cetlib
	cetlib_except
	Qt5::Widgets
	Qt5::Core
	pthread
	${Boost_SYSTEM_LIBRARY}
	${Boost_THREAD_LIBRARY}
)
endif()

basic_plugin( "UDP" "mfPlugin"
  MF_MessageLogger
${MF_MESSAGESERVICE}
  ${Boost_SYSTEM_LIBRARY}
  fhiclcpp
  cetlib
	cetlib_except
  pthread
)
basic_plugin( "OTS" "mfPlugin"
  MF_MessageLogger
${MF_MESSAGESERVICE}
    fhiclcpp
    cetlib
	cetlib_except
)


basic_plugin( "TRACE" "mfPlugin"
  MF_MessageLogger
${MF_MESSAGESERVICE}
    fhiclcpp
    cetlib
	cetlib_except
)


basic_plugin( "Friendly" "mfPlugin"
  MF_MessageLogger
${MF_MESSAGESERVICE}
    fhiclcpp
    cetlib
	cetlib_except
)

basic_plugin( "MultiFile" "mfPlugin"
  MF_MessageLogger
${MF_MESSAGESERVICE}
    fhiclcpp
    cetlib
	cetlib_except
)
    
basic_plugin( "ANSI" "mfPlugin"
  MF_MessageLogger
     ${MF_MESSAGESERVICE}
     fhiclcpp
     cetlib
	cetlib_except
     )

install_fhicl(SUBDIRS fcl)
install_headers()
install_source()
