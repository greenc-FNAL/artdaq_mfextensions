file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_make(LIBRARY_NAME MFReceivers
  EXCLUDE UDP_receiver.cc LogReader_receiver.cc
  LIBRARIES
  ${FHICLCPP}
  ${CETLIB}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  Qt5::Core
)

#basic_plugin("UDP" "receiver"
#  ${Boost_SYSTEM_LIBRARY}
#  ${Boost_THREAD_LIBRARY}
#  ${MF_MESSAGELOGGER}
#  ${MF_UTILITIES}
#  ${FHICLCPP}
#  ${CETLIB}
#  pthread
#)
 
#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

basic_plugin("LogReader" "receiver"
  MFReceivers
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${MF_MESSAGELOGGER}
  ${MF_UTILITIES}
  ${FHICLCPP}
  ${CETLIB}
  Qt5::Core
)

install_headers()
install_source()
