add_subdirectory(MessageAnalyzer)
add_subdirectory(Components)


cet_make_exec(MsgAnalyzer SOURCE MsgAnalyzer.cc MsgAnalyzerDlg.cpp MsgBox.cpp
LIBRARIES
MessageAnalyzer
Components
MF_MessageLogger
fhiclcpp
Qt5::Widgets
Qt5::Core
)

#install_fhicl(SUBDIRS config)
install_headers()
install_source()

