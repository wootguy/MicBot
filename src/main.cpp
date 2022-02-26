#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include "pipes.h"
#include "PipeInputBuffer.h"
#include "SteamVoiceEncoder.h"
#include "crc32.h"

using namespace std;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::system_clock;

#define MAX_CHANNELS 4
#define PIPE_BUFFER_SIZE 16384

long long getTimeMillis() {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

// ffmpeg -i test.wav -y -f s16le -ar 12000 -ac 1 - >\\.\pipe\MicBotPipe0
// ffmpeg -i test.m4a -y -f s16le -ar 12000 -ac 1 - >\\.\pipe\MicBotPipe1
// TODO: output raw pcm, not WAV

float clampf(float val, float min, float max) {
	if (val > max) {
		return max;
	}
	else if (val < min) {
		return min;
	}

	return val;
}

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
			printf("\n");
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
							printf("Failed to encode\n");
						}
						if (sz > maxPacketSize) {
							maxPacketSize = sz;
						}
						if (maxPacketSize > 500) {
							break;
						}
					}

					if (maxPacketSize == 0) {
						printf("uhhh\n");
					}

					if (maxPacketSize <= 500) {
						printf("Bitrate: %d, SampleRate: %d, Frames %d x %d, MaxSz: %d, delay: %.3f\n", bitrate, sampleRate, frameSize, framesPerPacket, maxPacketSize, delay);
					}
				}
			}
		}
	}
}

void pipe_test() {
	PipeInputBuffer* g_pipes[MAX_CHANNELS];
	int sampleRate = 12000; // opus allows: 8, 12, 16, 24, 48 khz
	int frameDuration = 30; // opus allows: 2.5, 5, 10, 20, 40, 60, 80, 100, 120
	int frameSize = (sampleRate / 1000) * frameDuration; // samples per frame
	int framesPerPacket = 2; // 2 = minimum amount of frames for sven to accept packet
	int packetDelay = frameDuration*framesPerPacket; // millesconds between output packets
	int samplesPerPacket = frameSize*framesPerPacket;
	int opusBitrate = 32000; // 32khz = steam default

	printf("Sampling rate     : %d Hz\n", sampleRate);
	printf("Frame size        : %d samples\n", frameSize);
	printf("Frames per packet : %d\n", framesPerPacket);
	printf("Samples per packet: %d\n", samplesPerPacket);
	printf("Packet delay      : %d ms\n", packetDelay);
	printf("Opus bitrate      : %d bps\n", opusBitrate);

	getValidFrameConfigs();

	crc32_init();
	SteamVoiceEncoder encoder(frameSize, framesPerPacket, sampleRate, opusBitrate);
	
	int16_t* pipeBuffer = new int16_t[samplesPerPacket];
	float* mixBuffer = new float[samplesPerPacket];
	int16_t* outputBuffer = new int16_t[samplesPerPacket];
	long long nextPacketMillis = getTimeMillis();

	for (int i = 0; i < MAX_CHANNELS; i++) {
		g_pipes[i] = new PipeInputBuffer("MicBotPipe" + to_string(i), PIPE_BUFFER_SIZE);
	}

	int packetCount = 0;

	while (1) {
		while (getTimeMillis() < nextPacketMillis) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		nextPacketMillis += packetDelay;

		memset(mixBuffer, 0, samplesPerPacket*sizeof(float));
		
		bool isAnythingPlaying = false;
		for (int i = 0; i < MAX_CHANNELS; i++) {
			if (g_pipes[i]->read((char*)pipeBuffer, samplesPerPacket * sizeof(int16_t))) {
				// can't read yet
				continue;
			}

			isAnythingPlaying = true;

			for (int k = 0; k < samplesPerPacket; k++) {
				mixBuffer[k] += (float)pipeBuffer[k] / 32768.0f;
			}
		}

		if (isAnythingPlaying) {
			for (int k = 0; k < samplesPerPacket; k++) {
				outputBuffer[k] = clampf(mixBuffer[k], -1.0f, 1.0f) * 32767.0f;
			}

			encoder.write_steam_voice_packet(outputBuffer, samplesPerPacket);

			if (packetCount++ > 500) {
				encoder.finishTestFile();
				printf("Wrote test file\n");
			}
		}
	}
}

int main(int argc, const char **argv)
{
	pipe_test();

	return 0;
}
