add_subdirectory(MessageAnalyzer)
add_subdirectory(Components)


cet_make_exec(MsgAnalyzer SOURCE MsgAnalyzer.cc MsgAnalyzerDlg.cpp
LIBRARIES
MessageAnalyzer
Components
${MF_MESSAGELOGGER}
${FHICLCPP}
Qt5::Widgets
Qt5::Core
)

install_headers()
install_source()

