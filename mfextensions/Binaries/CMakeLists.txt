cet_make_exec(NAME msgsender SOURCE msgsender.cc 
LIBRARIES
messagefacility::MF_MessageLogger
Boost::program_options
)

cet_make_exec(NAME MFTest SOURCE MFTest.cc 
LIBRARIES
fhiclcpp::fhiclcpp
messagefacility::MF_MessageLogger
)

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

# set(CMAKE_AUTOMOC_MOC_OPTIONS -p${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_SOURCE_DIR}/Binaries)
cet_make_exec(NAME msgserver SOURCE msgserver.cc
LIBRARIES
messagefacility::MF_MessageLogger
Qt5::Core
Boost::program_options
artdaq_mfextensions::MFReceivers
)

cet_make_exec(NAME msgviewer SOURCE msgviewer.cc mvdlg.cc
LIBRARIES
Qt5::Core
Qt5::Widgets
Boost::regex
fhiclcpp::fhiclcpp
messagefacility::MF_MessageLogger
TRACE::TRACE
artdaq_mfextensions::MFReceivers
artdaq_mfextensions::MFExtensions
)

install_headers()
install_source()

