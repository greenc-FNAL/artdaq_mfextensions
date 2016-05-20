basic_plugin( "UDP" "mfPlugin"
  ${MF_MESSAGELOGGER}
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
  ${CETLIB}
  pthread
)

basic_plugin( "TRACE" "mfPlugin"
    ${MF_MESSAGELOGGER}
    ${FHICLCPP}
    ${CETLIB}
    trace
)

basic_plugin( "MultiFile" "mfPlugin"
    ${MF_MESSAGELOGGER}
    ${FHICLCPP}
    ${CETLIB}
)
    


install_fhicl(SUBDIRS fcl)
install_headers()
install_source()
