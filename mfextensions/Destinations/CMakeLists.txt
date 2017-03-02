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
