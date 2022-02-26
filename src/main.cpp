#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include "pipes.h"
#include "ThreadInputBuffer.h"
#include "SteamVoiceEncoder.h"
#include "crc32.h"
#include "util.h"
#include "CommandQueue.h"
#include <cstring>

using namespace std;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::system_clock;

#define MAX_CHANNELS 16
#define PIPE_BUFFER_SIZE 16384

long long getTimeMillis() {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// ffmpeg -i test.wav -y -f s16le -ar 12000 -ac 1 - >\\.\pipe\MicBotPipe0
// ffmpeg -i test.m4a -y -f s16le -ar 12000 -ac 1 - >\\.\pipe\MicBotPipe1
// TODO: output raw pcm, not WAV

// Best packet delays:
//     Bitrate: 16000, SampleRate: 12000, Frames 1200 x 2, MaxSz: 478, delay: 0.200

void getValidFrameConfigs() {
	const int randomSampleSize = 1024*256;
	const int sampleRateCount = 1;
	const int frameDurationCount = 9;
	const int bitrateCount = 6;
	const int maxFramesPerPacket = 8;
	const float minPacketDelay = 0.05f; // script shouldn't send too fast

	uint8_t buffer[1024];
	int16_t* randomSamples;
	randomSamples = new int16_t[randomSampleSize];

	//int sampleRates[sampleRateCount] = {8000, 12000, 16000, 24000, 48000};
	int sampleRates[sampleRateCount] = {12000};
	//float frameDurations[frameDurationCount] = {2.5, 5, 10, 20, 40, 60, 80, 100, 120};
	float frameDurations[frameDurationCount] = {10, 20, 40, 60, 80, 100, 120};
	float bitrates[bitrateCount] = {16000, 22000, 24000, 28000, 32000, 64000};

	for (int i = 0; i < randomSampleSize; i++) {
		randomSamples[i] = rand() % 65536;
	}

	for (int b = 0; b < bitrateCount; b++) {
		int bitrate = bitrates[b];

		for (int r = 0; r < sampleRateCount; r++) {
			int sampleRate = sampleRates[r];
			fprintf(stderr, "\n");
			for (int i = 0; i < frameDurationCount; i++) {
				int frameSize = ((sampleRate / 1000) * frameDurations[i]) + 0.5f;

				for (int framesPerPacket = 2; framesPerPacket <= maxFramesPerPacket; framesPerPacket++) {
					SteamVoiceEncoder encoder(frameSize, framesPerPacket, sampleRate, bitrate);
					int samplesPerPacket = frameSize * framesPerPacket;

					float delay = (frameDurations[i] * framesPerPacket) / 1000.0f;
					if (delay < minPacketDelay) {
						continue; // too fast
					}

					int maxPacketSize = 0;
					for (int i = 0; i < randomSampleSize - samplesPerPacket; i += samplesPerPacket) {
						int sz = encoder.write_steam_voice_packet(randomSamples + i, samplesPerPacket);

						if (sz == -1) {
							fprintf(stderr, "Failed to encode\n");
						}
						if (sz > maxPacketSize) {
							maxPacketSize = sz;
						}
						if (maxPacketSize > 500) {
							break;
						}
					}

					if (maxPacketSize == 0) {
						fprintf(stderr, "uhhh\n");
					}

					if (maxPacketSize <= 500) {
						fprintf(stderr, "Bitrate: %d, SampleRate: %d, Frames %d x %d, MaxSz: %d, delay: %.3f, frameDur: %.1f\n",
							bitrate, sampleRate, frameSize, framesPerPacket, maxPacketSize, delay, frameDurations[i]);
					}
				}
			}
		}
	}
}

void pipe_test() {
	vector<ThreadInputBuffer*> inputStreams;
	int sampleRate = 12000; // opus allows: 8, 12, 16, 24, 48 khz
	int frameDuration = 10; // opus allows: 2.5, 5, 10, 20, 40, 60, 80, 100, 120
	int frameSize = (sampleRate / 1000) * frameDuration; // samples per frame
	int framesPerPacket = 5; // 2 = minimum amount of frames for sven to accept packet
	int packetDelay = frameDuration*framesPerPacket; // millesconds between output packets
	int samplesPerPacket = frameSize*framesPerPacket;
	int opusBitrate = 32000; // 32khz = steam default

	fprintf(stderr, "Sampling rate     : %d Hz\n", sampleRate);
	fprintf(stderr, "Frame size        : %d samples\n", frameSize);
	fprintf(stderr, "Frames per packet : %d\n", framesPerPacket);
	fprintf(stderr, "Samples per packet: %d\n", samplesPerPacket);
	fprintf(stderr, "Packet delay      : %d ms\n", packetDelay);
	fprintf(stderr, "Opus bitrate      : %d bps\n", opusBitrate);

	//getValidFrameConfigs();

	crc32_init();
	SteamVoiceEncoder encoder(frameSize, framesPerPacket, sampleRate, opusBitrate);
	
	int16_t* inBuffer = new int16_t[samplesPerPacket];
	float* mixBuffer = new float[samplesPerPacket];
	int16_t* outputBuffer = new int16_t[samplesPerPacket];
	long long nextPacketMillis = getTimeMillis();

	for (int i = 0; i < MAX_CHANNELS; i++) {
		ThreadInputBuffer* stream = new ThreadInputBuffer(PIPE_BUFFER_SIZE);
		stream->startPipeInputThread("MicBotPipe" + to_string(i));

		inputStreams.push_back(stream);
	}

	vector<int16_t> allSamples;

	int packetCount = 0;

	CommandQueue commands;
	commands.putCommand("../tts/tts0.mp3");

	while (1) {
		while (getTimeMillis() < nextPacketMillis) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		nextPacketMillis += packetDelay;

		// the plugin plays packets 0.1ms faster than normal because otherwise the mic
		// starts to cut out after a minute or so. The packets should be generated
		// slightly faster too so that the plugin doesn't slowly deplete, causing a gap once empty.
		if (packetCount++ % 10 == 0) {
			nextPacketMillis -= 1;
		}

		memset(mixBuffer, 0, samplesPerPacket*sizeof(float));
		
		vector<ThreadInputBuffer*> newStreams;

		int activeStreamds = 0;
		for (int i = 0; i < inputStreams.size(); i++) {
			if (inputStreams[i]->isFinished()) {
				fprintf(stderr, "Finished %s\n", inputStreams[i]->resourceName.c_str());
				delete inputStreams[i];
				continue;
			}

			newStreams.push_back(inputStreams[i]);

			if (inputStreams[i]->read((char*)inBuffer, samplesPerPacket * sizeof(int16_t))) {
				// can't read yet
				continue;
			}

			activeStreamds++;
			// ../tts/tts0.mp3
			for (int k = 0; k < samplesPerPacket; k++) {
				mixBuffer[k] += (float)inBuffer[k] / 32768.0f;
			}
		}

		if (activeStreamds) {
			//fprintf(stderr, "Mixed %d samples from %d streams\n", samplesPerPacket, activeStreamds);

			for (int k = 0; k < samplesPerPacket; k++) {
				outputBuffer[k] = clampf(mixBuffer[k], -1.0f, 1.0f) * 32767.0f;
				//allSamples.push_back(outputBuffer[k]);
			}

			encoder.write_steam_voice_packet(outputBuffer, samplesPerPacket);
		}
		else {
			//fprintf(stderr, "No active streamds\n");			
			printf("SILENCE\n");
			/*
			if (allSamples.size()) {
				WriteOutputWav("mixer.wav", allSamples);
				printf("BOKAY");
			}
			*/
		}
		fflush(stdout);

		inputStreams = newStreams; // remove any finished streamds
		//fprintf(stderr, "Total streams %d\n", inputStreams.size());

		while (commands.hasCommand()) {
			string command = commands.getNextCommand();

			if (command.find(".mp3") != string::npos) {
				ThreadInputBuffer* mp3Input = new ThreadInputBuffer(PIPE_BUFFER_SIZE);
				mp3Input->startMp3InputThread(command, sampleRate);
				inputStreams.push_back(mp3Input);
				fprintf(stderr, "Play mp3: %s\n", command.c_str());
			}
		}
	}
}

int main(int argc, const char **argv)
{
	pipe_test();

	return 0;
}
