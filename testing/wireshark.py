import argparse, os, sys
import scapy.all as scapy
from scapy.utils import RawPcapReader
from scapy.layers.l2 import Ether
from scapy.layers.inet import IP, TCP
from bitstring import BitArray, BitStream, ReadError

super_string = ""
replay_txt = open('replay.as', 'w')
replay_txt.write('array<array<uint8>> replay = {')

def process_voice_packet(data):
	idx = data.find(b'\x35')
	if idx != -1:
		deltaPacket = BitStream(data[idx+1:])
		playerIdx = deltaPacket.read('uintle:8')
		length = deltaPacket.read('uintle:16')
		
		if playerIdx != 0:
			return
		
		print("Parse svc_voicedata, idx %d, len %d" % (playerIdx, length) )
		
		byte_str = "\t{"
		for x in range(0, length):
			byte_str += "%d, " % deltaPacket.read('uintle:8')
			
		byte_str = byte_str[:-2] + "},"
		replay_txt.write(byte_str + "\n")
		
		print(byte_str)
	
def process_sniffed_packets(packet):
	payload = bytes(packet.payload.payload)

	try:
		process_voice_packet(payload)
	except Exception as e:
		print("Failed to parse packet: %s" % e)

scapy.sniff(iface='Ethernet', filter="udp and port 27015", store=False, prn=process_sniffed_packets)