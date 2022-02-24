import scapy.all as scapy
from scapy.utils import RawPcapReader
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, TCP
from bitstring import BitArray, BitStream, ReadError
import argparse, os, sys, time

last_counter = 0
clock_adjust = 0

time_since_last = 0

def process_voice_packet(packet, data):
	global last_counter
	global time
	global clock_adjust
	
	idx = data.find(b'\x35')
	if idx != -1:
		deltaPacket = BitStream(data[idx+1:])
		playerIdx = deltaPacket.read('uintle:8')
		length = deltaPacket.read('uintle:16')
		steamid = deltaPacket.read('uintle:64')
		ptype = deltaPacket.read('uintle:8')
		rate = deltaPacket.read('uintle:16')
		ptype2 = deltaPacket.read('uintle:8')
		opusLen = deltaPacket.read('uintle:16')
		unknown0 = deltaPacket.read('uintle:16')
		clock = deltaPacket.read('uintle:16') # when mic stops it adds huge amount, then resets to 0
		
		if steamid != 76561198271731523:
			return
		
		diff = int(clock) - int(last_counter)
		last_counter = clock
		seconds = (clock / 1000.0)*20.0
		'''
		realtime = time.time() + clock_adjust
		if abs(realtime - seconds) > 0.5:
			clock_adjust = seconds - time.time()
			if (realtime > seconds):
				print("TOO SLOW")
			else:
				print("TOO FAST")
		
		print("ptype %d, rate %d, ptype2 %d, opusLen %3d, %4X %4X (%+6d) (%.2f - %.2f)"
				% (ptype, rate, ptype2, opusLen, unknown0, clock, diff, seconds, realtime) )
		'''
		
		print("ptype %d, rate %d, ptype2 %d, opusLen %3d, %d"
				% (ptype, rate, ptype2, opusLen, unknown0) )
	
def process_sniffed_packets(packet):
	payload = bytes(packet.payload.payload)

	try:
		process_voice_packet(packet, payload)
	except Exception as e:
		print("Failed to parse packet: %s" % e)

scapy.sniff(iface='Ethernet', filter="udp and port 27015", store=False, prn=process_sniffed_packets)