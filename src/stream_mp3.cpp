#include "stream_mp3.h"
#include "util.h"
#include <stdio.h>
#include <vector>
#include <fstream>

// https://github.com/lieff/minimp3
#define MINIMP3_IMPLEMENTATION
//#define MINIMP3_ONLY_MP3
#include "minimp3.h"

using namespace std;

int resamplePcm(int16_t* pcm_old, int16_t* pcm_new, int oldRate, int newRate, int numSamples) {
	float samplesPerStep = (float)oldRate / newRate;
	int numSamplesNew = (float)numSamples / samplesPerStep;
	float t = 0;

	if (newRate > oldRate) {
		printf("Resampling to higher rate might have terrible quality!");
	}

	for (int i = 0; i < numSamplesNew; i++) {
		int newIdx = t;
		pcm_new[i] = pcm_old[newIdx];
		t += samplesPerStep;
	}	

	return numSamplesNew;
}

// mixes samples in-place without a new array
int mixStereoToMono(int16_t* pcm, int numSamples) {
	
	for (int i = 0; i < numSamples / 2; i++) {
		float left = ((float)pcm[i * 2] / 32768.0f);
		float right = ((float)pcm[i * 2 + 1] / 32768.0f);
		pcm[i] = clampf(left + right, -1.0f, 1.0f) * 32767;
	}

	return numSamples / 2;
}

void streamMp3(string fileName, ThreadInputBuffer* inputBuffer, int sampleRate) {
	FILE* file = fopen(fileName.c_str(), "rb");
	if (!file) {
		printf("Unable to open: %s\n", fileName.c_str());
		return;
	}

	mp3dec_t mp3d;
	mp3dec_init(&mp3d);

	int16_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
	int16_t resampledPcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
	uint8_t buffer[16384];

	const int bufferSize = 16384; // 16kb = recommended minimum
	int readPos = 0;
	int readSize = 16384;
	int bufferLeft = 0;

	vector<int16_t> allSamples;

	while (1) {
		int readBytes = fread(buffer + readPos, 1, readSize, file);
		if (readBytes == 0) {
			break;
		}
		bufferLeft += readBytes;

		mp3dec_frame_info_t info;
		int samples = mp3dec_decode_frame(&mp3d, buffer, bufferLeft, pcm, &info);
		samples *= info.channels;

		// remove the read bytes from the buffer
		int bytesRead = info.frame_bytes;
		memmove(buffer, buffer + bytesRead, bufferSize - bytesRead);
		readSize = bytesRead;
		readPos = bufferSize - bytesRead;
		bufferLeft -= bytesRead;

		if (info.channels == 2) {
			samples = mixStereoToMono(pcm, samples);
		}

		int writeSamples = resamplePcm(pcm, resampledPcm, info.hz, 12000, samples);
		//int writeSamples = samples;
		//memcpy(resampledPcm, pcm, writeSamples*sizeof(int16_t));

		size_t bytesLeftToWrite = writeSamples * sizeof(int16_t);
		size_t writePos = 0;		

		//printf("Read %d file bytes, %d samples, %d frame bytes\n", readBytes, samples, bytesRead);

		while (bytesLeftToWrite) {
			size_t written = inputBuffer->write((char*)resampledPcm + writePos, bytesLeftToWrite);
			bytesLeftToWrite -= written;
			writePos += written;

			if (!written) {
				// buffer not ready for writing yet
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}
	
	inputBuffer->flush();
}