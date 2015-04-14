cet_make(
  EXCLUDE UDP_receiver.cc LogReader_receiver.cc
  LIBRARIES
  ${FHICLCPP}
  ${CETLIB}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
)

basic_plugin("UDP" "receiver"
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_THREAD_LIBRARY}
  ${FHICLCPP}
  ${CETLIB}
  pthread
)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

#basic_plugin("LogReader" "receiver"
#  ${Boost_SYSTEM_LIBRARY}
#  ${Boost_REGEX_LIBRARY}
#  ${FHICLCPP}
#  ${CETLIB}
#)

install_headers()
install_source()
