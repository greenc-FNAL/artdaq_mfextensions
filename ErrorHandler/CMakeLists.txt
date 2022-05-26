add_subdirectory(MessageAnalyzer)
add_subdirectory(Components)


cet_make_exec(NAME MsgAnalyzer SOURCE MsgAnalyzer.cc MsgAnalyzerDlg.cpp MsgBox.cpp
LIBRARIES
Qt5::Core
Qt5::Widgets
	messagefacility::MF_MessageLogger
	TRACE::TRACE
	Boost::thread
	artdaq_mfextensions::Components
	artdaq_mfextensions::MFReceivers
)

#install_fhicl(SUBDIRS config)
install_headers()
install_source()

