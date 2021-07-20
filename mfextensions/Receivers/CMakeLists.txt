file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_make(LIBRARY_NAME MFReceivers
  EXCLUDE UDP_receiver.cc LogReader_receiver.cc
  LIBRARIES
  fhiclcpp
  cetlib
  cetlib_except
  MF_MessageLogger
Qt5::Widgets
Qt5::Core
)

basic_plugin("UDP" "receiver"
  MFReceivers
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  MF_MessageLogger
  fhiclcpp
  cetlib
  cetlib_except
  Qt5::Core
  pthread
)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

basic_plugin("LogReader" "receiver"
  MFReceivers
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  MF_MessageLogger
  fhiclcpp
  cetlib
  cetlib_except
  Qt5::Core
)

install_headers(SUBDIRS detail)
install_source(SUBDIRS detail)
