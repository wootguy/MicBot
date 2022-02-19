import os, subprocess, socket, sys, time


hostname = '192.168.254.158' # for twlz
#hostname = '192.168.126.128' # for VM testing
hostport = 1337
 
udp_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
udp_socket.bind((hostname, hostport))
server_addr = None

print("Waiting for server to contact us")

while True:
	udp_packet = udp_socket.recvfrom(1024)
	data = udp_packet[0]
	fromAddr = udp_packet[1]
	print("Received %d byte packet from %s" % (len(data), fromAddr))
	server_addr = fromAddr
	udp_socket.sendto(b'got that', fromAddr)
	print("Sending response. Time to start sending voice data.")
	break

packets_per_send = 1
buffer = b''
packetId = 0

udp_socket.settimeout(0)

sent_packets = []

p = subprocess.Popen('lib/steam_voice.exe', stdout=subprocess.PIPE)
for line in iter(p.stdout.readline, ''):
	idBytes = packetId.to_bytes(2, 'big')
	packet = idBytes + bytes.fromhex(line.decode().strip())
	sent_packets.append(packet)
	
	udp_socket.sendto(packet, server_addr)
	print("Send %d (%d bytes)" % (packetId, len(packet)))
	packetId += 1
	
	try:
		udp_packet = udp_socket.recvfrom(4)
		want_id = int.from_bytes(udp_packet[0], "big")
		print("Server wants %d again" % want_id)
		udp_socket.sendto(sent_packets[want_id], server_addr)
	except Exception as e:
		pass
		
p.stdout.close()
tcp_socket.close()

print('Done')
