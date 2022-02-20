import socket, sys, time, datetime, os
from threading import Thread

# "client" that generates the voice data
#hostname = '47.157.183.178' # for twlz
hostname = '192.168.254.106' # for VM test
hostport = 1337
client_address = (hostname, hostport)

voice_data_file = '../../../../svencoop/scripts/plugins/store/_fromvoice.txt'
voice_cmd_file = '../../../../svencoop/scripts/plugins/store/_tovoice.txt'

# higher than 16 and plugin might send packets so fast that the stream cuts out
# special throttling logic is needed for higher buffer sizes
buffer_max = 8
buffered_buffers = 4 # number of buffers to hold onto before sending to the plugin. Higher = fewer lost packets

last_file_write = datetime.datetime.now()
last_heartbeat = datetime.datetime.now()
min_time_between_writes = 0.1 # give the plugin time to load the file. Keep value in sync with plugin
time_between_heartbeats = 1 # time between packets to the client, letting it know the server is still listening

def follow(thefile):
	'''generator function that yields new lines in a file
	'''
	# seek the end of the file
	thefile.seek(0, os.SEEK_END)
	
	# start infinite loop
	while True:
		# read last line of file
		line = thefile.readline()        # sleep if file hasn't been updated
		if not line:
			time.sleep(0.1)
			continue

		yield line
		
def command_loop():
	global client_address
	global voice_cmd_file
	
	while True:
		try:
			# Create a connection to the server application on port 81
			tcp_socket = socket.create_connection(client_address)
			print("Command socket connected")

			logfile = open(voice_cmd_file, encoding='utf8', errors='ignore')
			loglines = follow(logfile)

			for line in loglines:
				print("Send command: " + line.strip())
				tcp_socket.sendall(line.encode())
				
			tcp_socket.close()
		except Exception as e:
			print(e)

# Tell the voice client that we're still listening.
# This also initiates a connection without having to open a port on the server running this script.
# UDP traffic is allowed in once the server tries to communicate on the port.
def heartbeat(socket):
	global time_since_last_heartbeat
	global last_heartbeat
	global client_address
	
	time_since_last_heartbeat = (datetime.datetime.now() - last_heartbeat).total_seconds()
		
	if time_since_last_heartbeat > time_between_heartbeats:
		socket.sendto(b'dere', client_address)
		last_heartbeat = datetime.datetime.now()
		#print("heartbeat %s:%d" % client_address)
		
def send_packets_to_plugin(socket, all_packets):
	global last_file_write
	global min_time_between_writes
	global buffered_buffers
	global buffer_max
	global voice_data_file
	global client_address
	
	time_since_last_write = (datetime.datetime.now() - last_file_write).total_seconds()
	if len(all_packets) <= buffer_max*buffered_buffers or time_since_last_write < min_time_between_writes:
		return
	
	last_file_write = datetime.datetime.now()
	
	with open(voice_data_file, "w") as f:
		lost = 0
		for packet in all_packets[:buffer_max]:
			if type(packet) is int:
				lost += 1
				f.write('00\n')
			else:
				f.write(packet)
		
		all_packets = all_packets[buffer_max:]
		
		still_missing = 0
		for idx, packet in enumerate(all_packets):
			if type(packet) is int:
				socket.sendto(packet.to_bytes(2, 'big'), client_address)
				still_missing += 1
				#print("  Asked to resend %d" % (packet))
				
		print("Wrote %d packets (%d lost, %d buffered, %d requested)" % (buffer_max, lost, len(all_packets), still_missing))

def receive_voice_data():
	global client_address

	all_packets = []
	expectedPacketId = -1

	udp_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
	udp_socket.settimeout(1)
	print("Contacting voice data client %s:%d" % (hostname, hostport))

	# Receive and print data 32 bytes at a time, as long as the client is sending something
	while True:
		heartbeat(udp_socket)
		
		try:
			udp_packet = udp_socket.recvfrom(1024)
		except socket.timeout:
			continue
		
		data = udp_packet[0]
		#print("Got %d (%d bytes)" % (packetId, len(data)))
		
		is_resent = data[:6] == b'resent'
		if is_resent:
			data = data[6:]
			
		packetId = int.from_bytes(data[:2], "big")
		hexString = ''.join(format(x, '02x') for x in data[2:]) + '\n'
		
		if is_resent:
			# got a resent packet, which we asked for earlier
			recovered = False
			for idx, packet in enumerate(all_packets):
				if type(packet) is int and packet == packetId:
					all_packets[idx] = hexString
					recovered = True
					#print("  Recovered %d" % packetId)
			if not recovered:
				#print("  %d is too or was recovered already" % packetId)
				pass
				
		elif expectedPacketId - packetId > 100 or expectedPacketId == -1:
			# packet counter looped back to 0, or we just reconnected to the client
			expectedPacketId = packetId + 1
			all_packets.append(hexString)
		
		elif packetId > expectedPacketId:
			# larger counter than expected. A packet was lost or sent out of order. Ask for the missing ones.
			#print("Expected %d but got %d" % (expectedPacketId, packetId))
			for x in range(expectedPacketId, packetId):
				all_packets.append(x)
				udp_socket.sendto(x.to_bytes(2, 'big'), client_address)
				#print("  Asked to resend %d" % x)
				
			expectedPacketId = packetId + 1
			all_packets.append(hexString)
		
		else:
			# normal packet update. Counter was incremented by 1 as expected
			expectedPacketId = packetId + 1
			all_packets.append(hexString)
			
		send_packets_to_plugin(udp_socket, all_packets)

t = Thread(target = command_loop, args =( ))
t.daemon = True
t.start()

receive_voice_data()