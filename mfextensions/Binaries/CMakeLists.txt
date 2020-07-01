cet_make_exec(msgsender SOURCE msgsender.cc 
LIBRARIES
${Boost_PROGRAM_OPTIONS_LIBRARY}
${Boost_SYSTEM_LIBRARY}
${MF_MESSAGELOGGER}
${FHICLCPP}
)

cet_make_exec(MFTest SOURCE MFTest.cc 
LIBRARIES
  ${MESSAGEFACILITY_LIBS}
${FHICLCPP}
)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

# set(CMAKE_AUTOMOC_MOC_OPTIONS -p${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/Binaries)
cet_make_exec(msgserver SOURCE msgserver.cc
LIBRARIES
MFExtensions
MFReceivers
${Boost_PROGRAM_OPTIONS_LIBRARY}
${MF_MESSAGELOGGER}
${FHICLCPP}
Qt5::Core
)

cet_make_exec(msgviewer SOURCE msgviewer.cc mvdlg.cc
LIBRARIES
MFExtensions
MFReceivers
${MF_MESSAGELOGGER}
${FHICLCPP}
Qt5::Widgets
Qt5::Core
)

#install_source()
install_headers()

