import os, subprocess, socket, sys, time, datetime, queue
from threading import Thread

#hostname = '192.168.254.158' # for twlz
hostname = '192.168.254.106' # for VM testing
hostport = 1337
our_addr = (hostname, hostport)

command_queue = queue.Queue()
reconnect_tcp = False

server_timeout = 5 # time in seconds to wait for server heartbeat before disconnecting

def command_loop():
	global our_addr
	global command_queue
	global reconnect_tcp
	
	while True:
		tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		tcp_socket.bind(our_addr)
		tcp_socket.listen(1)

		data_stream = ''
	
		print("Waiting for command socket connection")
		connection, client = tcp_socket.accept()
	 
		try:
			print("Command socket connected")
			tcp_socket.settimeout(1)
			# Receive and print data 32 bytes at a time, as long as the client is sending something
			while True:
				try:
					data = connection.recv(32)
				except Exception as e:
					if reconnect_tcp:
						print("Closing command socket")
						reconnect_tcp = False
						connection.close()
						tcp_socket.settimeout(0)
						break
					continue
				data_stream += data.decode()
				
				if '\n' in data_stream:
					command = data_stream[:data_stream.find('\n')]
					data_stream = data_stream[data_stream.find('\n')+1:]
					print("Process command: " + command)
	 
		except Exception as e:
			connection.close()

def transmit_voice():
	global server_timeout
	
	udp_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
	udp_socket.bind(our_addr)
	udp_socket.settimeout(0)
	
	server_addr = None

	last_heartbeat = datetime.datetime.now()
	packetId = 0

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
			#print("Send %d (%d bytes)" % (packetId, len(packet)))
			
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
						reconnect_tcp = True
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


t = Thread(target = command_loop, args =( ))
t.daemon = True
t.start()

transmit_voice()