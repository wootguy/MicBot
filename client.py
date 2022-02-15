# pip install pafy gtts python-vlc pyglet pywin32 psutil pydub youtube_dl yt-dlp
# If yt-dlp fails, try: pip install --no-deps -U yt-dlp
# edit backend_youtube_dl.py in pafy library:
#    replace youtube_dl import with this:
#        import yt_dlp as youtube_dl
#    Remove like/dislike stuff from info
# Windows notes:
#    Need to install 64 bit version of VLC if 64 bit version of python, or 32 bit if 32 bit python
#	 Need to install ffmpeg and add it to system PATH
#    Might need to edit script to point to VLC folder to load DLLs
#    Some .net or visual studio stuff might be needed to install pip stuff
# Linux notes:
#    sudo apt install xdotool python3-gst-1.0 ffmpeg vlc
#    To get sven to record from speakers (not sure if all of these steps were required):
#        sudo apt install pavucontrol
#        pactl load-module module-loopback latency_msec=1
#        Set sound card profile to "Off" in Configuration tab (this will disable speaker output but I want that anyway)
#    yt_dlp can play more videos, but will crash on some too. So best to not make the backend_youtube_dl.py edit
# Usage:
#     - Start sven co-op
#     - bind F4 "+voicerecord;-voicerecord;+voicerecord"
#     - say .mbot
#     - Keep the game in focus and without the menu/console showing.
#	    The script will continue pressing F4 to keep the mic enabled across level changes
#     - Start this script

import time, os, sys, queue, signal
from threading import Thread
import pafy
import vlc
from gtts import gTTS
import pyglet
from pyglet.media import Player
from pydub import AudioSegment, effects

if os.name == 'nt':
	import win32api, win32gui, win32con, ctypes, win32process, psutil
	import ctypes, time
	# Bunch of stuff so that the script can send keystrokes to game #

	SendInput = ctypes.windll.user32.SendInput

	# C struct redefinitions 
	PUL = ctypes.POINTER(ctypes.c_ulong)
	class KeyBdInput(ctypes.Structure):
		_fields_ = [("wVk", ctypes.c_ushort),
					("wScan", ctypes.c_ushort),
					("dwFlags", ctypes.c_ulong),
					("time", ctypes.c_ulong),
					("dwExtraInfo", PUL)]

	class HardwareInput(ctypes.Structure):
		_fields_ = [("uMsg", ctypes.c_ulong),
					("wParamL", ctypes.c_short),
					("wParamH", ctypes.c_ushort)]

	class MouseInput(ctypes.Structure):
		_fields_ = [("dx", ctypes.c_long),
					("dy", ctypes.c_long),
					("mouseData", ctypes.c_ulong),
					("dwFlags", ctypes.c_ulong),
					("time",ctypes.c_ulong),
					("dwExtraInfo", PUL)]

	class Input_I(ctypes.Union):
		_fields_ = [("ki", KeyBdInput),
					 ("mi", MouseInput),
					 ("hi", HardwareInput)]

	class Input(ctypes.Structure):
		_fields_ = [("type", ctypes.c_ulong),
					("ii", Input_I)]

	# Actuals Functions
	def PressKey(hexKeyCode):
		extra = ctypes.c_ulong(0)
		ii_ = Input_I()
		ii_.ki = KeyBdInput( 0, hexKeyCode, 0x0008, 0, ctypes.pointer(extra) )
		x = Input( ctypes.c_ulong(1), ii_ )
		ctypes.windll.user32.SendInput(1, ctypes.pointer(x), ctypes.sizeof(x))

	def ReleaseKey(hexKeyCode):
		extra = ctypes.c_ulong(0)
		ii_ = Input_I()
		ii_.ki = KeyBdInput( 0, hexKeyCode, 0x0008 | 0x0002, 0, ctypes.pointer(extra) )
		x = Input( ctypes.c_ulong(1), ii_ )
		ctypes.windll.user32.SendInput(1, ctypes.pointer(x), ctypes.sizeof(x))

	# http://www.flint.jp/misc/?q=dik&lang=en
	def PressButton(code):
		global sven_hwnd

		PressKey(code)
		time.sleep(.05)
		ReleaseKey(code)
		
	def chat_sven(text):
		pass
else:
	def PressButton(code):
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) Return')
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) F8')
		
	def chat_sven(text):
		global lock_queue
		
		lock_queue.put(1)
		os.system('xdotool type --window $(xdotool search --class svencoop | head -n1) y')
		time.sleep(0.1)
		os.system('xdotool type --delay 0 --window $(xdotool search --class svencoop | head -n1) \'' + text + "'")
		time.sleep(0.1)
		os.system('xdotool key --window $(xdotool search --class svencoop | head -n1) Return')
		lock_queue.get()

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

players = []
sound_threads = []
g_chatsounds = []
tts_id = 0
g_accents = {}
g_pitches = {}
g_players = {}
lock_queue = queue.Queue()


def load_all_chatsounds():
	file1 = open('chatsounds.txt', 'r')
	for line in file1.readlines():
		g_chatsounds.append(line.split()[0])

def playsound_async(speaker, sound):	
	if speaker in g_players:
		if g_players[speaker].playing:
			g_players[speaker].pause()
		g_players[speaker].delete()
	
	player = g_players[speaker] = Player()
	
	source = pyglet.media.load(sound, streaming=False)
	player.pitch = (g_pitches[speaker] / 100.0) if speaker in g_pitches else 1
	player.queue(source)
	player.play()
	sound_threads.append(player)
	
def playtube_async(url, offset, player):
	global tts_id
	
	# https://youtu.be/GXv1hDICJK0 (age restricted)
	# https://youtu.be/-zEJEdbZUP8 (crashes or doesn't play on yt-dlp)
	try:
		global players
		print("Fetch best audio " + url)
		video = pafy.new(url)
		best = video.getbestaudio()
		playurl = best.url
		#print("BEST URL: " + playurl)
		
		print("Create vlc instance")
		Instance = vlc.Instance()
		player = Instance.media_player_new()
		media = Instance.media_new(playurl)
		media.get_mrl()
		media.add_option('start-time=%d' % offset)
		player.set_media(media)
		player.play()
		
		players.append(player)
		print("Play offset %d: " % offset + video.title)
		#chat_sven("/me played: " + video.title)
	except Exception as e:
		print(e)
		
		chat_sven("/me failed to play a video from " + player + ".")
		t = Thread(target = play_tts, args =('', str(e), tts_id, ))
		t.daemon = True
		t.start()
		tts_id += 1

def play_tts(speaker, text, id):	
	# Language in which you want to convert
	language = 'en'
	tld = 'com'
	
	if speaker in g_accents:
		language = g_accents[speaker]['code']
		tld = g_accents[speaker]['tld']
	  
	# Passing the text and language to the engine, 
	# here we have marked slow=False. Which tells 
	# the module that the converted audio should 
	# have a high speed
	print("Translating %d" % id)
	myobj = gTTS(text=text, tld=tld, lang=language, slow=False)
	  
	# Saving the converted audio in a mp3 file named
	# welcome
	fname = 'tts/tts%d' % id + '.mp3'
	myobj.save(fname)
	
	totalCaps = sum(1 for c in text if c.isupper())
	totalLower = sum(1 for c in text if c.islower())
	totalExclaim = sum(1 for c in text if c == '!')
	
	print("Normalize %d" % (id))
	output = 'tts/tts%d.wav' % id
	
	rawsound = AudioSegment.from_file(fname, "mp3")  
	normalizedsound = effects.normalize(rawsound)  
	
	if totalCaps > totalLower:
		normalizedsound = normalizedsound + 1000
	
	normalizedsound.export(output, format="wav")
	 
	print("Play %d" % id)
	# Playing the converted file
	playsound_async(speaker, output)

	os.remove(fname)
	os.remove(output)
	

sven_root = '../../../..'
csound_path = os.path.join(sven_root, 'svencoop_downloads/sound/twlz')
	
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

filename = find_console_log()
load_all_chatsounds()
print("Following log file: " + filename)

tts_enabled = True

# press mic record key over and over
def heartbeat_thread():
	global lock_queue
	
	while True:
		if lock_queue.empty():
			PressButton(0x3E) # F4
			PressButton(0x39) # space
			time.sleep(0.5)
	

'''
import cleverbotfree

cleverbot = cleverbotfree.Cleverbot(cleverbotfree.sync_playwright())

def cleverbot_chat(text):
	return c_b.single_exchange(user_input)

#c_b.close()
'''

t = Thread(target = heartbeat_thread, args =( ))
t.daemon = True
t.start()

logfile = open(filename, encoding='utf8', errors='ignore')
loglines = follow(logfile)   
 # iterate over the generator
for line in loglines:
	print(line)
	
	if line.startswith('MicBot\\'):
		line = line[len('MicBot\\'):]
		name = line[:line.find("\\")]
		line = line[line.find("\\")+1:]
		print(name + ": " + line.strip())
		
		if line.startswith('https://www.youtube.com') or line.startswith('!https://www.youtube.com') or line.startswith('https://youtu.be') or line.startswith('!https://youtu.be'):
			if line.startswith("!"):
				line = line[1:]
				
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
				for player in players:
					player.stop()
				players = []
			elif arg == "last":
				for player in players[1:]:
					player.stop()
				players = players[:1]
			elif arg == "first":
				for player in players[:-1]:
					player.stop()
				players = players[-1:]
				
			if arg == "" or arg == 'speak':
				for player in sound_threads:
					if player.playing:
						player.pause()
					player.delete()
				sound_threads = []
			
		
			t = Thread(target = play_tts, args =(name, 'stop ' + arg, tts_id, ))
			t.daemon = True
			t.start()
			tts_id += 1
			continue
		
		'''
		if line.startswith('!'):
			cmd = line[1:line.find(' ')]
			
			check_path = os.path.join(csound_path, cmd + '.wav')
			if os.path.exists(check_path):
				playsound_async(name, check_path)
				continue
		'''
		
		if tts_enabled:			
			if line.strip().lower() in g_chatsounds:
				continue
			
			if line.startswith('.mpitch') and len(line.split()) > 1:
				g_pitches[name] = int(line.split()[1])
				if name in g_players:
					g_players[name].pitch = g_pitches[name] / 100.0
				continue
			
			if line.startswith('.mlang '):
				lang = line[6:].strip().lower()
				
				valid_langs = {
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
				
				if lang in valid_langs:
					g_accents[name] = valid_langs[lang]
					msg = valid_langs[lang]['name'] + " accent, activated"
					t = Thread(target = play_tts, args =(name, msg, tts_id, ))
					t.daemon = True
					t.start()
					tts_id += 1
				else:
					msg = "invalid accent " + lang
					t = Thread(target = play_tts, args =(name, msg, tts_id, ))
					t.daemon = True
					t.start()
					tts_id += 1
				
				continue
			
			if line.startswith('!'):
				line = line[1:]
			
			#print("Cleverbot: " + cleverbot_chat(line))
			
			t = Thread(target = play_tts, args =(name, line, tts_id, ))
			t.start()
			tts_id += 1
		
	
	
