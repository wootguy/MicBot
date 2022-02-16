# TODO:
# reload the filename occasionaly
# use steam ids not names (more plugin reliance tho)
# youtube shorts links dont work
# yt opening actual videos
# persist settings in case of crash
# linux mic stops working

import time, os, sys, queue, random
from threading import Thread
import pafy
import vlc
from gtts import gTTS
import pyglet
from pyglet.media import Player
from pydub import AudioSegment, effects

if os.name == 'nt':
	from pynput.keyboard import Key, Controller

	keyboard = Controller()

	# http://www.flint.jp/misc/?q=dik&lang=en
	def PressButton(code):
		keyboard.press(code)
		keyboard.release(code)
		
	def PressButtons():
		PressButton(Key.f8)
		PressButton(Key.space)
		
	def chat_sven(text):
		global lock_queue
		
		while not lock_queue.empty():
			time.sleep(0.1)
		
		lock_queue.put(1)
		
		PressButton('y')
		time.sleep(0.1)
		for c in text:
			PressButton(c)
		time.sleep(0.1)
		PressButton(Key.enter)
		
		lock_queue.get()
else:
	def PressButton(code):
		pass
		
	def PressButtons():
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
log_queue = queue.Queue()
command_prefix = '~'
dance_emotes = [
	'.e 180 loop 2',
	'.e 181 iloop 1',
	'.e 187 iloop 2'
]


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
		chat_sven("/me - " + video.title)
		
		rand_idx = random.randrange(0, len(dance_emotes))
		print("EMOTE: " + dance_emotes[rand_idx])
		chat_sven(dance_emotes[rand_idx])
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
def button_loop():
	global lock_queue
	global players
	
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
	
	logfile = open(filename, encoding='utf8', errors='ignore')
	loglines = follow(logfile)   
	 # iterate over the generator
	for line in loglines:
		log_queue.put(line)
		print(line)

t = Thread(target = button_loop, args =( ))
t.daemon = True
t.start()

t = Thread(target = message_loop, args =( ))
t.daemon = True
t.start()

while True:
	wasplaying = len(players) > 0
				
	for idx, player in enumerate(players):
		if not player.is_playing() and not player.will_play():
			players.pop(idx)
			break
			
	if len(players) == 0 and wasplaying:
		chat_sven('.e off')
		
	if len(players) > 0:
		PressButton(Key.right)
	
	line = None
	try:
		line = log_queue.get(True, 0.05)
	except Exception as e:
		pass
	
	if not line:
		continue
	
	print(line.strip())
	
	if line.startswith('MicBot\\'):
		line = line[len('MicBot\\'):]
		name = line[:line.find("\\")]
		line = line[line.find("\\")+1:]
		print(name + ": " + line.strip())
		
		if line.startswith('https://www.youtube.com') or line.startswith(command_prefix + 'https://www.youtube.com') or line.startswith('https://youtu.be') or line.startswith(command_prefix + 'https://youtu.be'):
			if line.startswith(command_prefix):
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
				chat_sven(".e off")
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
			
			if line.startswith(command_prefix):
				line = line[1:]
			
			t = Thread(target = play_tts, args =(name, line, tts_id, ))
			t.start()
			tts_id += 1
		
	
	
