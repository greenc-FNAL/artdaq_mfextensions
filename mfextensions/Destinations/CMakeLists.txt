basic_plugin( "UDP" "mfPlugin"
  ${MF_MESSAGELOGGER}
  ${Boost_SYSTEM_LIBRARY}
  ${FHICLCPP}
  ${CETLIB}
  pthread
)

install_headers()
install_source()
