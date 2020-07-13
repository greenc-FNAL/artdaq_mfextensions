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
import time
from random import randrange
USAGE = 'send host:port [count] [text file to send] [sleep time in ms]'

# first byte is cmd
# 2nd byte is seqnum (yes, just 8 bits)
# rest is data (up to 1498)
buf=''

def main(argv):
    print('len(argv)=%d'%(len(argv),))
    if len(argv) < 2 or len(argv) > 5: 
        print(USAGE)
        sys.exit()
    node,port = argv[1].split(':')
    portint = int(port)
    count = 1
    text = ""
    sleep_time = 0

    if len(argv) >= 3: count = int(argv[2])
    if len(argv) >= 4:
        f = open(argv[3], "r")
        text  = f.read()
    if len(argv) >= 5: sleep_time = float(argv[4]) / 1000

    print('node:port=%s:%d, count=%d'%(node,portint,count))
    for ii  in range(0, count):
        sev = randrange(0,15)
        buf='MF: 01-Jan-1970 01:01:01'
        buf+="|%d" % ii
        buf+="|" + node + "%d" % ii
        buf+="|" + node + "%d" % ii
        if sev == 0:
            buf+="|ERROR"
        elif sev < 3:
            buf+="|WARNING"
        elif sev < 7:
            buf+="|INFO"
        else:
            buf+="|DEBUG"
        buf+="|Test Message %d" % ii
        buf+="|UDP Send MFMSG %d" % ii
        buf+="|udp_send_mfmsg.py"
        buf+="|1"
        buf+="|Run 0, Subrun 0, Event %d" % ii
        buf+="|UDP Test program"
        buf+="|This is the ARTDAQ UDP test string.\n\t It contains exactly 111 characters, making for a total size of 113 bytes."
        buf+=text
        s = socket.socket( socket.AF_INET, socket.SOCK_DGRAM )
        s.sendto( buf.encode('uft-8'), (node,portint) )
        time.sleep(sleep_time)
    pass


if __name__ == "__main__": main(sys.argv)
