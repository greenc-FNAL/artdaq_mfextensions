file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_make(LIBRARY_NAME Components
  LIBRARIES
  Boost::system
  Boost::thread
  messagefacility::MF_MessageLogger
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  TRACE::TRACE
  artdaq_mfextensions::MessageAnalyzer
)

install_headers()
install_source()
