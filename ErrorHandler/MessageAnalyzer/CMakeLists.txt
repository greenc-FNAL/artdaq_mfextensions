file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/moc_dummy.cpp "TEST")
file(GLOB MOCFILES ${CMAKE_CURRENT_BINARY_DIR}/moc_*.cpp)
message(STATUS "MOCFILES=${MOCFILES}")
file(REMOVE ${MOCFILES})

cet_make(LIBRARY_NAME MessageAnalyzer
  LIBRARIES
  ${FHICLCPP}
  ${CETLIB}
  ${CETLIB_EXCEPT}
  ${MESSAGEFACILITY_LIBS}
Qt5::Widgets
Qt5::Core
${Boost_REGEX_LIBRARY}
${Boost_THREAD_LIBRARY}
pthread
)

install_headers()
install_source()
