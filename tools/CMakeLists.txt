cet_script(ALWAYS_COPY startMsgViewer.sh  udp_send_mfmsg.py )

cet_make_exec(NAME mf_simple 
LIBRARIES
TRACE::TRACE
messagefacility::MF_MessageLogger
)

install_headers()
install_source()