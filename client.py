# TODO:
# reload the filename occasionaly
# use steam ids not names (more plugin reliance tho)
# youtube shorts links dont work
# yt opening actual videos
# persist settings in case of crash
# linux mic stops working

import time, os, sys, queue, random, pafy, vlc, pyglet, datetime
from threading import Thread
from gtts import gTTS
from pyglet.media import Player
from pydub import AudioSegment, effects

sven_root = '../../../..'
csound_path = os.path.join(sven_root, 'svencoop_downloads/sound/twlz')
tts_enabled = True
g_media_players = []
g_chatsounds = []
tts_id = 0
g_tts_players = {}
lock_queue = queue.Queue()
log_queue = queue.Queue()
command_prefix = '~'
dance_emotes = [ # cyber_kizuna_dance
	'.e 180 loop 2',
	'.e 181 iloop 1',
	'.e 187 iloop 2'
	'.e chain 2.4 loop 191 192'
	'.e 194 loop 3.5',
	'.e 195 loop 3.5',
	'.e 196 loop 2.2',
	'.e 197 loop 2.4',
	'.e chain 2 loop 198 199',
	'.e 200 loop 2',
	'.e 201 loop 2.4',
	'.e 202 loop 1.7',
	'.e chain 1.8 loop 203 204_1.4',
	'.e chain 2.4 loop 205 206 207 208_3.45',
	'.e 209 loop 1.47',
	'.e 210 loop 1.75',
	'.e 211 loop 1.65',
	'.e 212 loop 5',
	'.e 213 loop 3.4',
	'.e 214 loop 6',
	'.e 215 loop 1.7',
	'.e 216 loop 1.6',
	'.e 217 loop 3.8',
	'.e 218 loop 8',
	'.e 219 loop 1.6',
	'.e chain 1.6 loop 220 221 222_1.2',
	'.e chain 1.6 loop 223 224 225 226_4.35',
	'.e chain 1.7 loop 227 228 229_1.1',
	'.e chain 1.85 loop 230 231 232_1.5',
	'.e chain 1.85 loop 233 234 235 236_1.3',
	'.e chain 1.85 loop 237 238 239 240_7.2',
	'.e chain 1.7 loop 241 242 243 244 245 246 247 248 249 250 251 252 253 254 255'
]
g_valid_langs = {
	'af': {'tld': 'com', 'code': 'af', 'name': 'African'},
	'afs': {'tld': 'co.za', 'code': 'en', 'name': 'South African'},
	'ar': {'tld': 'com', 'code': 'ar', 'name': 'arabic'},
	'au': {'tld': 'com.au', 'code': 'en', 'name': 'Austrailian'},
	'bg': {'tld': 'com', 'code': 'bg', 'name': 'bulgarian'},
	'bn': {'tld': 'com', 'code': 'bn', 'name': 'Bengali'},
	'br': {'tld': 'com.br', 'code': 'pt', 'name': 'brazilian'},
	'bs': {'tld': 'com', 'code': 'bs', 'name': 'Bosnian'},
	'ca': {'tld': 'ca', 'code': 'en', 'name': 'Canadian'},
	'cn': {'tld': 'com', 'code': 'zh-CN', 'name': 'chinese'},
	'cs': {'tld': 'com', 'code': 'cs', 'name': 'Czech'},
	'ct': {'tld': 'com', 'code': 'ca', 'name': 'Catalan'},
	'cy': {'tld': 'com', 'code': 'cy', 'name': 'Welsh'},
	'da': {'tld': 'com', 'code': 'da', 'name': 'Danish'},
	'de': {'tld': 'com', 'code': 'de', 'name': 'German'},
	'el': {'tld': 'com', 'code': 'el', 'name': 'Greek'},
	'en': {'tld': 'com', 'code': 'en', 'name': 'american'},
	'eo': {'tld': 'com', 'code': 'eo', 'name': 'Esperanto'},
	'es': {'tld': 'es', 'code': 'es', 'name': 'spanish'},
	'et': {'tld': 'com', 'code': 'et', 'name': 'Estonian'},
	'fc': {'tld': 'ca', 'code': 'fr', 'name': 'french canadian'},
	'fi': {'tld': 'com', 'code': 'fi', 'name': 'Finnish'},
	'fr': {'tld': 'fr', 'code': 'fr', 'name': 'french'},
	'gu': {'tld': 'com', 'code': 'gu', 'name': 'Gujarati'},
	'hi': {'tld': 'com', 'code': 'hi', 'name': 'Hindi'},
	'hr': {'tld': 'com', 'code': 'hr', 'name': 'Croatian'},
	'hu': {'tld': 'com', 'code': 'hu', 'name': 'Hungarian'},
	'hy': {'tld': 'com', 'code': 'hy', 'name': 'Armenian'},
	'is': {'tld': 'com', 'code': 'is', 'name': 'Icelandic'},
	'id': {'tld': 'com', 'code': 'id', 'name': 'Indonesian'},
	'in': {'tld': 'co.in', 'code': 'en', 'name': 'Indian'},
	'ir': {'tld': 'ie', 'code': 'en', 'name': 'Irish'},
	'it': {'tld': 'com', 'code': 'it', 'name': 'Italian'},
	'ja': {'tld': 'com', 'code': 'ja', 'name': 'Japanese'},
	'jw': {'tld': 'com', 'code': 'jw', 'name': 'Javanese'},
	'km': {'tld': 'com', 'code': 'km', 'name': 'Khmer'},
	'kn': {'tld': 'com', 'code': 'kn', 'name': 'Kannada'},
	'ko': {'tld': 'com', 'code': 'ko', 'name': 'Korean'},
	'la': {'tld': 'com', 'code': 'la', 'name': 'Latin'},
	'lv': {'tld': 'com', 'code': 'lv', 'name': 'Latvian'},
	'ma': {'tld': 'com', 'code': 'es', 'name': 'Mexican American'},
	'mk': {'tld': 'com', 'code': 'mk', 'name': 'Macedonian'},
	'mr': {'tld': 'com', 'code': 'mr', 'name': 'Marathi'},
	'mx': {'tld': 'com.mx', 'code': 'es', 'name': 'Mexican'},
	'my': {'tld': 'com', 'code': 'my', 'name': 'Myanmar (Burmese)'},
	'ne': {'tld': 'com', 'code': 'ne', 'name': 'Nepali'},
	'nl': {'tld': 'com', 'code': 'nl', 'name': 'Dutch'},
	'no': {'tld': 'com', 'code': 'no', 'name': 'Norwegian'},
	'pl': {'tld': 'com', 'code': 'pl', 'name': 'Polish'},
	'pt': {'tld': 'pt', 'code': 'pt', 'name': 'portuguese'},
	'ro': {'tld': 'com', 'code': 'ro', 'name': 'Romanian'},
	'ru': {'tld': 'com', 'code': 'ru', 'name': 'Russian'},
	'si': {'tld': 'com', 'code': 'si', 'name': 'Sinhala'},
	'sk': {'tld': 'com', 'code': 'sk', 'name': 'Slovak'},
	'sq': {'tld': 'com', 'code': 'sq', 'name': 'Albanian'},
	'sr': {'tld': 'com', 'code': 'sr', 'name': 'Serbian'},
	'su': {'tld': 'com', 'code': 'su', 'name': 'Sundanese'},
	'sv': {'tld': 'com', 'code': 'sv', 'name': 'Swedish'},
	'sw': {'tld': 'com', 'code': 'sw', 'name': 'Swahili'},
	'ta': {'tld': 'com', 'code': 'ta', 'name': 'Tamil'},
	'te': {'tld': 'com', 'code': 'te', 'name': 'Telugu'},
	'th': {'tld': 'com', 'code': 'th', 'name': 'Thai'},
	'tl': {'tld': 'com', 'code': 'tl', 'name': 'Filipino'},
	'tr': {'tld': 'com', 'code': 'tr', 'name': 'Turkish'},
	'tw': {'tld': 'com', 'code': 'zh-TW', 'name': 'taiwanese'},
	'ek': {'tld': 'co.uk', 'code': 'en', 'name': 'british'},
	'uk': {'tld': 'com', 'code': 'uk', 'name': 'Ukrainian'},
	'ur': {'tld': 'com', 'code': 'ur', 'name': 'Urdu'},
	'vi': {'tld': 'com', 'code': 'vi', 'name': 'Vietnamese'}
}


if os.name == 'nt':
	from pynput.keyboard import Key, Controller

	keyboard = Controller()

	# http://www.flint.jp/misc/?q=dik&lang=en
	def PressButton(code):
		keyboard.press(code)
		keyboard.release(code)
		
	def PressButtons():
		PressButton(Key.f8)
	
	def chat_sven(text):
		global lock_queue
		
		while not lock_queue.empty():
			time.sleep(0.1)
		
		lock_queue.put(1)
		
		PressButton('y')
		time.sleep(0.1)
		for c in text:
			time.sleep(0.01)
			PressButton(c)
		time.sleep(0.1)
		PressButton(Key.enter)
		
		lock_queue.get()	

	def console_cmd_sven(text):
		global lock_queue
		
		while not lock_queue.empty():
			time.sleep(0.1)
		
		lock_queue.put(1)
		
		PressButton('`')
		time.sleep(0.1)
		for c in text:
			time.sleep(0.01)
			PressButton(c)
		time.sleep(0.1)
		PressButton(Key.enter)
		time.sleep(0.1)
		PressButton('`')
		
		lock_queue.get()
		
	def close_console():
		global lock_queue
		
		while not lock_queue.empty():
			time.sleep(0.1)
		
		lock_queue.put(1)

		for c in 'toggleconsole':
			time.sleep(0.01)
			PressButton(c)
		time.sleep(0.1)
		PressButton(Key.enter)
		time.sleep(0.1)
		PressButton(Key.enter)
		
		lock_queue.get()
else:
	def PressButton(code):
		pass
		
	def PressButtons():
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) F8')
		
	def chat_sven(text):
		global lock_queue
		
		while not lock_queue.empty():
			time.sleep(0.1)
		
		lock_queue.put(1)
		
		lock_queue.put(1)
		os.system('xdotool type --window $(xdotool search --class svencoop | head -n1) y')
		time.sleep(0.1)
		os.system('xdotool type --delay 0 --window $(xdotool search --class svencoop | head -n1) \'' + text + "'")
		time.sleep(0.1)
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) Return')
		lock_queue.get()
		
	def console_cmd_sven(text):
		global lock_queue
		
		while not lock_queue.empty():
			time.sleep(0.1)
		
		lock_queue.put(1)
		os.system('xdotool type --window $(xdotool search --class svencoop | head -n1) `')
		time.sleep(0.1)
		os.system('xdotool type --delay 0 --window $(xdotool search --class svencoop | head -n1) \'' + text + "'")
		time.sleep(0.1)
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) Return')
		time.sleep(0.1)
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) `')
		lock_queue.get()
		
	def close_console():
		global lock_queue
		
		while not lock_queue.empty():
			time.sleep(0.1)
		
		lock_queue.put(1)
		os.system('xdotool type --delay 0 --window $(xdotool search --class svencoop | head -n1) \'' + 'toggleconsole' + "'")
		time.sleep(0.1)
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) Return')
		time.sleep(0.1)
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) Return')
		lock_queue.get()

def follow(thefile):
	'''generator function that yields new lines in a file
	'''
	# seek the end of the file
	thefile.seek(0, os.SEEK_END)
	
	last_day = datetime.datetime.today().day
	
	# start infinite loop
	while True:
		# read last line of file
		line = thefile.readline()        # sleep if file hasn't been updated
		if not line:
			time.sleep(0.1)
			if last_day != datetime.datetime.today().day:
				print("New day started. Stop following this log")
				time.sleep(1)
				return
			continue

		yield line

def load_all_chatsounds():
	file1 = open('chatsounds.txt', 'r')
	for line in file1.readlines():
		g_chatsounds.append(line.split()[0])

def playsound_async(speaker, sound, pitch):
	if speaker in g_tts_players:
		if g_tts_players[speaker].playing:
			g_tts_players[speaker].pause()
		g_tts_players[speaker].delete()
	
	player = g_tts_players[speaker] = Player()
	
	source = pyglet.media.load(sound, streaming=False)
	player.pitch = pitch
	player.queue(source)
	player.play()

def format_time(seconds):
	hours = int(seconds / (60*60))
	
	if hours > 0:
		remainder = int(seconds - hours*60*60)
		return "%dh %dm" % (hours, int(remainder / 60))
	else:
		minutes = int(seconds / 60)
		if minutes > 0:
			return "%dm %ds" % (minutes, int(seconds % 60))
		else:
			return "%ds" % int(seconds)

def playtube_async(url, offset, asker):
	global tts_id
	global g_media_players
	
	# https://youtu.be/GXv1hDICJK0 (age restricted)
	# https://youtu.be/-zEJEdbZUP8 (crashes or doesn't play on yt-dlp)
	
	# https://www.olivieraubert.net/vlc/python-ctypes/doc/ (Ctrl+f MediaPlayer)
	try:		
		print("Fetch best audio " + url)
		video = pafy.new(url)
		best = video.getbestaudio()
		playurl = best.url
		#print("BEST URL: " + playurl)
		
		print("Create vlc instance")
		Instance = vlc.Instance()
		player = Instance.media_player_new()
		media = Instance.media_new(playurl)
		media.add_option('start-time=%d' % offset)
		player.set_media(media)
		if player.play() == -1:
			raise Exception("Failed to play the video")
		
		g_media_players.append(player)
		print("Play offset %d: " % offset + video.title)
		chat_sven("/me - " + video.title + "  [" + format_time(int(video.length)) + "]")
		
		rand_idx = random.randrange(0, len(dance_emotes))
		chat_sven(dance_emotes[rand_idx])
	except Exception as e:
		print(e)
		
		chat_sven("/me failed to play a video from " + str(asker) + ".")
		t = Thread(target = play_tts, args =('', str(e), tts_id, "en", 100, ))
		t.daemon = True
		t.start()
		tts_id += 1

def play_tts(speaker, text, id, lang, pitch):	
	# Language in which you want to convert
	language = g_valid_langs[lang]['code']
	tld = g_valid_langs[lang]['tld']
	  
	# Passing the text and language to the engine, 
	# here we have marked slow=False. Which tells 
	# the module that the converted audio should 
	# have a high speed
	#print("Translating %d" % id)
	myobj = gTTS(text=text, tld=tld, lang=language, slow=False)
	  
	# Saving the converted audio in a mp3 file named
	# welcome
	fname = 'tts/tts%d' % id + '.mp3'
	try:
		myobj.save(fname)
	except Exception as e:
		print(e)
		return
		
	totalCaps = sum(1 for c in text if c.isupper())
	totalLower = sum(1 for c in text if c.islower())
	
	#print("Normalize %d" % (id))
	output = 'tts/tts%d.wav' % id
	
	rawsound = AudioSegment.from_file(fname, "mp3")  
	normalizedsound = effects.normalize(rawsound)  
	
	if totalCaps > totalLower:
		normalizedsound = normalizedsound + 1000
	else:
		normalizedsound = normalizedsound + 5
	
	normalizedsound.export(output, format="wav")
	 
	#print("Play %d" % id)
	# Playing the converted file
	playsound_async(speaker, output, pitch)

	os.remove(fname)
	os.remove(output)

def find_console_log():
	global sven_root
	
	logs = []
	
	for file in os.listdir(sven_root):
		if file.startswith('console-') and file.endswith('.log'):
			logs.append(file)
			
	logs.sort()
	
	if len(logs) == 0:
		print("No console log found. Add -condebug to Sven Co-op's launch options and start the game.")
		print("If you did that, then make sure this script is in this folder: svencoop_addon/plugins/scripts/MicBot/")
		sys.exit()
	
	return os.path.join(sven_root, logs[len(logs)-1])


load_all_chatsounds()

# press mic record key over and over
def button_loop():
	global lock_queue
	
	while True:
		if lock_queue.empty():
			if os.name == 'nt':
				PressButtons()
				time.sleep(0.5)
			else:
				PressButtons()
				time.sleep(0.5)

def message_loop():
	global log_queue
	
	filename = find_console_log()
	print("Following log file: " + filename)
	
	logfile = open(filename, encoding='utf8', errors='ignore')
	loglines = follow(logfile)   
	 # iterate over the generator
	for line in loglines:
		log_queue.put(line)
	
	# restart log following
	t = Thread(target = message_loop, args =( ))
	t.daemon = True
	t.start()

t = Thread(target = message_loop, args =( ))
t.daemon = True
t.start()

t = Thread(target = button_loop, args =( ))
t.daemon = True
t.start()



while True:
	wasplaying = len(g_media_players) > 0
				
	for idx, player in enumerate(g_media_players):
		if not player.is_playing() and not player.will_play():
			g_media_players.pop(idx)
			break
			
	if len(g_media_players) == 0 and wasplaying:
		chat_sven('.e off')
		
	if len(g_media_players) > 0:
		PressButton(Key.right)
	
	line = None
	try:
		line = log_queue.get(True, 0.05)
	except Exception as e:
		pass
	
	if not line:
		continue
	
	#print(line.strip())
	
	if line.startswith('MicBot\\'):
		print(line.strip())
		
		line = line[len('MicBot\\'):]
		name = line[:line.find("\\")]
		line = line[line.find("\\")+1:]
		lang = line[:line.find("\\")]
		line = line[line.find("\\")+1:]
		pitch = float(line[:line.find("\\")]) / 100
		line = line[line.find("\\")+1:]
		
		had_prefix = line.startswith(command_prefix)
		if had_prefix:
			line = line[1:]

		#print(name + ": " + line.strip())
		
		if line.startswith('https://www.youtube.com') or line.startswith('https://youtu.be'):				
			args = line.split()
			offset = 0
			try:
				if len(args) >= 2:
					timecode = args[1]
					if ':' in timecode:
						minutes = timecode[:timecode.find(':')]
						seconds = timecode[timecode.find(':')+1:]
						offset = int(minutes)*60 + int(seconds)
					else:
						offset = int(timecode)
			except Exception as e:
				print(e)
		
			t = Thread(target = playtube_async, args =(args[0], offset, name, ))
			t.daemon = True
			t.start()
			continue
			
		if line.startswith('.mstop'):
			args = line.split()
			arg = args[1] if len(args) > 1 else ""
		
			if arg == "":
				for player in g_media_players:
					player.stop()
				g_media_players = []
				chat_sven(".e off")
			elif arg == "last":
				for player in g_media_players[1:]:
					player.stop()
				g_media_players = g_media_players[:1]
			elif arg == "first":
				for player in g_media_players[:-1]:
					player.stop()
				g_media_players = g_media_players[-1:]
				
			if arg == "" or arg == 'speak':
				for key, player in g_tts_players.items():
					if player.playing:
						player.pause()
					player.delete()
				g_tts_players = {}
			
		
			t = Thread(target = play_tts, args =(name, 'stop ' + arg, tts_id, lang, pitch, ))
			t.daemon = True
			t.start()
			tts_id += 1
			continue
			
		if line.startswith('.msay'):
			sayText = line[len('.msay '):].strip()
			
			if sayText.startswith('.mbot'):
				continue
			
			chat_sven(sayText)
			continue
			
		if line.startswith('.munstuck'):
			chat_sven("") # 
			continue
		
		if tts_enabled:			
			if not had_prefix and line.strip().lower() in g_chatsounds:
				continue
			
			t = Thread(target = play_tts, args =(name, line, tts_id, lang, pitch, ))
			t.start()
			tts_id += 1
		
	
	
