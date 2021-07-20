file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_make(LIBRARY_NAME Components
  LIBRARIES
  fhiclcpp
  cetlib
  cetlib_except
  MF_MessageLogger
Qt5::Widgets
Qt5::Core
mfextensions_Receivers_LogReader_receiver
MessageAnalyzer
)

install_headers()
install_source()
