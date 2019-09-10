
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
  ${MESSAGEFACILITY_LIBS}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
	Qt5::Widgets
	Qt5::Core
	pthread
	${Boost_SYSTEM_LIBRARY}
	${Boost_THREAD_LIBRARY}
)
endif()

basic_plugin( "UDP" "mfPlugin"
  ${MESSAGEFACILITY_LIBS}
${MF_MESSAGESERVICE}
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
  ${CETLIB}
	${CETLIB_EXCEPT}
  pthread
)
basic_plugin( "OTS" "mfPlugin"
  ${MESSAGEFACILITY_LIBS}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
)


basic_plugin( "TRACE" "mfPlugin"
  ${MESSAGEFACILITY_LIBS}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
)


basic_plugin( "Friendly" "mfPlugin"
  ${MESSAGEFACILITY_LIBS}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
)

basic_plugin( "MultiFile" "mfPlugin"
  ${MESSAGEFACILITY_LIBS}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
)
    
basic_plugin( "ANSI" "mfPlugin"
  ${MESSAGEFACILITY_LIBS}
     ${MF_MESSAGESERVICE}
     ${FHICLCPP}
     ${CETLIB}
	${CETLIB_EXCEPT}
     )

install_fhicl(SUBDIRS fcl)
install_headers()
install_source()
