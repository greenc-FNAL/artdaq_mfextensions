cet_script(ALWAYS_COPY startMsgViewer.sh  udp_send_mfmsg.py )

cet_make_exec(mf_simple 
LIBRARIES
fhiclcpp
MF_MessageLogger
cetlib
)