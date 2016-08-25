#!/usr/bin/env python
 # This file (udp_send_artdaq.py) was created by Ron Rechenmacher <ron@fnal.gov> on
 # Jan 15, 2015. "TERMS AND CONDITIONS" governing this file are in the README
 # or COPYING file. If you do not have such a file, one can be obtained by
 # contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
 # $RCSfile: .emacs.gnu,v $
 # rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";

import sys
import socket
USAGE='send host:port'

# first byte is cmd
# 2nd byte is seqnum (yes, just 8 bits)
# rest is data (up to 1498)

buf=''

def main(argv):
    print('len(argv)=%d'%(len(argv),))
    if len(argv) != 2: print(USAGE); sys.exit()
    node,port = argv[1].split(':')
    buf='MF: 01-Jan-1970 01:01:01'
    buf+="|" + node
    buf+="|" + node
    buf+="|WARNING"
    buf+="|Test Message"
    buf+="|UDP Send MFMSG"
    buf+="|udp_send_mfmsg.py"
    buf+="|1"
    buf+="|Run 0, Subrun 0, Event 0"
    buf+="|UDP Test program"
    buf+="|This is the ARTDAQ UDP test string.\n\t It contains exactly 111 characters, making for a total size of 113 bytes."
    s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
    s.sendto( buf, (node,int(port)) )
    pass


if __name__ == "__main__": main(sys.argv)
