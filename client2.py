import os, subprocess, socket, sys, time, datetime


#hostname = '192.168.254.158' # for twlz
hostname = '192.168.254.106' # for VM testing
hostport = 1337

udp_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
udp_socket.bind((hostname, hostport))
server_addr = None

server_timeout = 5 # time in seconds to wait for server heartbeat before disconnecting
last_heartbeat = datetime.datetime.now()
packets_per_send = 1
buffer = b''
packetId = 0

udp_socket.settimeout(0)

sent_packets = []
sent_packets_first_id = 0 # packet id of the first packet in sent_packets

p = subprocess.Popen('lib/steam_voice.exe', stdout=subprocess.PIPE)
for line in iter(p.stdout.readline, ''):
	idBytes = packetId.to_bytes(2, 'big')
	try:
		packet = idBytes + bytes.fromhex(line.decode().strip())
	except Exception as e:
		print(e)
		continue
	
	if server_addr:
		udp_socket.sendto(packet, server_addr)
		sent_packets.append((packetId, packet))
		print("Send %d (%d bytes)" % (packetId, len(packet)))
		
		packetId = (packetId + 1) % 65536
		#packetId = (packetId + 1) % 200
		
		if (len(sent_packets) > 128):
			sent_packets = sent_packets[(len(sent_packets) - 128):]
	else:
		print("Waiting for heartbeat on %s:%d" % (hostname, hostport))
		time.sleep(1)
	
	# handle some requests from the server
	for x in range(0, 4):
		try:
			udp_packet = udp_socket.recvfrom(1024)
			if udp_packet[0] == b'dere':
				if not server_addr or udp_packet[1][0] != server_addr[0] or udp_packet[1][1] != server_addr[1]:
					print("Server address changed! Must have restarted.")
					server_addr = udp_packet[1]
			else:
				want_id = int.from_bytes(udp_packet[0], "big")
				
				found = False
				for packet in sent_packets:
					if packet[0] == want_id:
						udp_socket.sendto(b'resent' + packet[1], server_addr)
						found = True
						print("Resending %d" % want_id)
						break
				if not found:
					print("Server wanted %d, which is not in sent history" % want_id)
				
			last_heartbeat = datetime.datetime.now()
		except Exception as e:
			break
		
	time_since_last_heartbeat = (datetime.datetime.now() - last_heartbeat).total_seconds()
	
	if (time_since_last_heartbeat > server_timeout and server_addr):
		print("Server isn't there anymore! Probably!!! Wait for reconnect....")
		server_addr = None
		
				
p.stdout.close()
p.terminate()

