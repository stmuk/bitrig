#!/usr/local/bin/python2.7
# check wether path mtu to dst is as expected

import os
from addr import *
from scapy.all import *

# usage: ping_mtu src dst size icmp-size

srcaddr=sys.argv[1]
dstaddr=sys.argv[2]
size=int(sys.argv[3])
expect=int(sys.argv[4])
pid=os.getpid()
hdr=IP(flags="DF", src=srcaddr, dst=dstaddr)/ICMP(id=pid)
payload="a" * (size - len(str(hdr)))
ip=hdr/payload
iplen=IP(str(ip)).len
eth=Ether(src=SRC_MAC, dst=PF_MAC)/ip
a=srp1(eth, iface=SRC_IF, timeout=2)

if a and a.payload.payload.type==3 and a.payload.payload.code==4:
	mtu=a.payload.payload.unused
	print "mtu=%d" % (mtu)
	if mtu != expect:
		print "MTU!=%d" % (expect)
		exit(1)
	iip=a.payload.payload.payload
	iiplen=iip.len
	if iiplen != iplen:
		print "inner IP len %d!=%d" % (iiplen, iplen)
		exit(1)
	isrc=iip.src
	if isrc != srcaddr:
		print "inner IP src %d!=%d" % (isrc, srcaddr)
		exit(1)
	idst=iip.dst
	if idst != dstaddr:
		print "inner IP dst %d!=%d" % (idst, dstaddr)
		exit(1)
	exit(0)
print "MTU=UNKNOWN"
exit(2)
