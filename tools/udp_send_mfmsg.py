#!/usr/bin/env python
 # This file (udp_send_artdaq.py) was created by Ron Rechenmacher
 # <ron@fnal.gov> on
 # Jan 15, 2015. "TERMS AND CONDITIONS" governing this file are in the README
 # or COPYING file. If you do not have such a file, one can be obtained by
 # contacting Ron or Fermi Lab in Batavia IL, 60510, phone: 630-840-3000.
 # $RCSfile: .emacs.gnu,v $
 # rev="$Revision: 1.23 $$Date: 2012/01/23 15:32:40 $";
import sys
import socket
from random import randrange
USAGE = 'send host:port [count]'

# first byte is cmd
# 2nd byte is seqnum (yes, just 8 bits)
# rest is data (up to 1498)
buf=''

def main(argv):
    print('len(argv)=%d'%(len(argv),))
    if len(argv) < 2 or len(argv) > 3: 
        print(USAGE)
        sys.exit()
    node,port = argv[1].split(':')
    count = 1
    if len(argv) == 3: count = int(argv[2])

    print('node:port=%s:%d, count=%d'%(node,int(port),count))

    for ii  in range(0, count):
        sev = randrange(0,15)
        buf='MF: 01-Jan-1970 01:01:01'
        buf+="|%d" % ii
        buf+="|" + node
        buf+="|" + node
        if sev == 0:
            buf+="|ERROR"
        elif sev < 3:
            buf+="|WARNING"
        elif sev < 7:
            buf+="|INFO"
        else:
            buf+="|DEBUG"
        buf+="|Test Message"
        buf+="|UDP Send MFMSG"
        buf+="|udp_send_mfmsg.py"
        buf+="|1"
        buf+="|Run 0, Subrun 0, Event %d" % ii
        buf+="|UDP Test program"
        buf+="|This is the ARTDAQ UDP test string.\n\t It contains exactly 111 characters, making for a total size of 113 bytes."
        s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
        s.sendto( buf.encode('utf-8'), (node,int(port)) )
    pass


if __name__ == "__main__": main(sys.argv)
