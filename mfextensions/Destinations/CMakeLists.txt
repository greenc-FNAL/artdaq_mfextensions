
find_package(CURL)

if(CURL_FOUND)
  include_directories(${CURL_INCLUDE_DIRS})
SET(CMAKE_C_FLAGS "-std=gnu99 -fPIC" CACHE STRING "" FORCE)
cet_make_library(LIBRARY_NAME curl_send_message 
                 SOURCE detail/curl_send_message.c
                 LIBRARIES 
				 ${CURL_LIBRARIES}
                           WITH_STATIC_LIBRARY)

basic_plugin( "SMTP" "mfPlugin"
curl_send_message
    ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
	Qt5::Widgets
	Qt5::Core
	pthread
)
endif()

basic_plugin( "UDP" "mfPlugin"
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
${MF_MESSAGESERVICE}
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
  ${CETLIB}
	${CETLIB_EXCEPT}
  pthread
)

basic_plugin( "TRACE" "mfPlugin"
    ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
)


basic_plugin( "Friendly" "mfPlugin"
    ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
)

basic_plugin( "GenFile" "mfPlugin"
    ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
)

basic_plugin( "MultiFile" "mfPlugin"
    ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
${MF_MESSAGESERVICE}
    ${FHICLCPP}
    ${CETLIB}
	${CETLIB_EXCEPT}
)
    
basic_plugin( "ANSI" "mfPlugin"
     ${MF_MESSAGELOGGER}
     ${MF_UTILITIES}
     ${MF_MESSAGESERVICE}
     ${FHICLCPP}
     ${CETLIB}
	${CETLIB_EXCEPT}
     )

install_fhicl(SUBDIRS fcl)
install_headers()
install_source()
