cet_script( startMsgViewer.sh  udp_send_mfmsg.py )

cet_make_exec(mf_simple 
LIBRARIES
${FHICLCPP}
${MF_MESSAGELOGGER}
${CETLIB}
)