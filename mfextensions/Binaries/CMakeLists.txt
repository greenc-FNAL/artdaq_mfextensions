cet_make_exec(msgsender SOURCE msgsender.cc 
LIBRARIES
${Boost_PROGRAM_OPTIONS_LIBRARY}
${Boost_SYSTEM_LIBRARY}
${MF_MESSAGELOGGER}
${FHICLCPP}
)

cet_make_exec(MFTest SOURCE MFTest.cc 
LIBRARIES
${MF_MESSAGELOGGER}
${MF_UTILITIES}
${FHICLCPP}
)

cmake_minimum_required(VERSION 2.8.11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt5Widgets)
find_package(Qt5Core)

cet_make_exec(msgserver SOURCE msgserver.cc ReceiverManager.cc
LIBRARIES
MFExtensions
${Boost_PROGRAM_OPTIONS_LIBRARY}
${MF_MESSAGELOGGER}
${FHICLCPP}
Qt5::Core
)

cet_make_exec(msgviewer SOURCE msgviewer.cc mvdlg.cc qt_mf_msg.cc ReceiverManager.cc
LIBRARIES
MFExtensions
${MF_MESSAGELOGGER}
${FHICLCPP}
Qt5::Widgets
Qt5::Core
)

install_source()
install_headers()

