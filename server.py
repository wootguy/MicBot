import socket, sys, time

# "client" that generates the voice data
#hostname = '47.157.183.178' # for twlz
hostname = '192.168.126.128' # for VM test
hostport = 1337

udp_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
client_address = (hostname, hostport)

udp_socket.settimeout(2)
while True:
	print("Contacting voice data client...")
	udp_socket.sendto(b'send me shit', client_address)
	
	try:
		udp_packet = udp_socket.recvfrom(1024)
		print("Got %s from %s" % (udp_packet[0], udp_packet[1]))
		break
	except socket.timeout:
		time.sleep(1)	

# higher than 16 and plugin might send packets so fast that the stream cuts out
# special throttling logic is needed for higher buffer sizes
buffer_max = 16

all_packets = []

last_block_send = 0

expectedPacketId = 0

while True:
	print("Waiting for packets")

	# Receive and print data 32 bytes at a time, as long as the client is sending something
	while True:
		try:
			udp_packet = udp_socket.recvfrom(1024)
		except socket.timeout:
			continue
		
		data = udp_packet[0]
		
		packetId = int.from_bytes(data[:2], "big")
		payload = data[2:]
		
		#print("Got %d (%d bytes)" % (packetId, len(data)))
		hexString = ''.join(format(x, '02x') for x in payload)
		
		if packetId > expectedPacketId:
			#print("Expected %d but got %d" % (expectedPacketId, packetId))
			for x in range(expectedPacketId, packetId):
				all_packets.append("00\n")
				udp_socket.sendto(x.to_bytes(2, 'big'), client_address)
				print("  Asked to resend %d" % x)
				
			expectedPacketId = packetId + 1
			all_packets.append(hexString + "\n")
		elif packetId < expectedPacketId:
			#print("Recovered %d" % packetId)
			all_packets[packetId] = hexString + "\n"
		else:
			expectedPacketId = packetId + 1
			all_packets.append(hexString + "\n")
			
		
		if (len(all_packets) - last_block_send >= buffer_max*2):
			with open("voice.dat", "w") as f:
				lost = 0
				for packet in all_packets[last_block_send:last_block_send+buffer_max]:
					if len(packet) <= 3:
						lost += 1
					f.write(packet)
				
				print("Wrote %d packets in block %d-%d (%d loss)" % (buffer_max, last_block_send, last_block_send+buffer_max, lost))
				last_block_send += buffer_max
				
				for idx, packet in enumerate(all_packets[last_block_send:]):
					if len(packet) <= 3:
						udp_socket.sendto((last_block_send+idx).to_bytes(2, 'big'), client_address)
						print("  Asked to resend %d" % (last_block_send+idx))