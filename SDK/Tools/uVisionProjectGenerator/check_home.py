#!/usr/bin/python
from struct import *
import sys

def xorlist(l):
  c=0x00
  for a in l:
    c=c^a
  return c




def home2str(h):
	return ("%2.2x%2.2x%2.2x%2.2x" % (h[0],h[1],h[2],h[3]))


def get_ttls():
  ttlxor=[]
  for i in range(0,5):
    ii=4-i  
    ttl2 = i<<4 | ii
    for j in range(i+1,4):
      jj=4-j
      ttl1 = j<<4 | jj
      xor = ttl1 ^ttl2
#      print "ttl1: %2.2x  ttl2: %2.2x" %(ttl1,ttl2)
      ttlxor.append( xor )
  #The frame recieved by the first repeater has 0xFA set in SessionTxRandomInterval ttl1 xor ttl2 == 0
  ttlxor.append( 0xFA )

  return sorted(set(ttlxor))


if __name__ == '__main__':

   

  staticBytes2=[0x45, 0x19, 0x22, 0x07]
  ttlxor = get_ttls();
  
  home = [int(sys.argv[1][0:2],16) , int(sys.argv[1][2:4],16) , int(sys.argv[1][4:6],16) , int(sys.argv[1][6:8],16)]
  print "homeID ", home2str(home),

  badNodes=[]
  for s in range(1,232):
    for t in ttlxor:
      crc= (xorlist(home + staticBytes2) ^0xFF ^ t ^ s	)
      if crc==2:
        badNodes.append(s)
        break;

  print "Bad nodes " + str(badNodes)




