file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_make(LIBRARY_NAME MessageAnalyzer
  LIBRARIES
	messagefacility::MF_MessageLogger
	Qt5::Core
	Qt5::Gui
	TRACE::TRACE
	Boost::system
	Boost::thread
	Boost::regex
)

install_headers()
install_source()
