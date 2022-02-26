#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include "opus/opus.h"
#include "pipes.h"
#include "PipeInputBuffer.h"

#include "opus_util.h"

using namespace std;
using std::chrono::milliseconds;
using std::chrono::duration_cast;
using std::chrono::system_clock;


struct OpusFrame {
	uint16_t length;
	uint16_t sequence;
	uint8_t* data;
};

static void crc32_generate_table(uint32_t(&table)[256])
{
	uint32_t polynomial = 0xEDB88320;
	for (uint32_t i = 0; i < 256; i++)
	{
		uint32_t c = i;
		for (size_t j = 0; j < 8; j++)
		{
			if (c & 1) {
				c = polynomial ^ (c >> 1);
			}
			else {
				c >>= 1;
			}
		}
		table[i] = c;
	}
}

static uint32_t crc32_update(uint32_t(&table)[256], uint32_t initial, const void* buf, size_t len)
{
	uint32_t c = initial ^ 0xFFFFFFFF;
	const uint8_t* u = static_cast<const uint8_t*>(buf);
	for (size_t i = 0; i < len; ++i)
	{
		c = table[(c ^ u[i]) & 0xFF] ^ (c >> 8);
	}
	return c ^ 0xFFFFFFFF;
}

vector<int16_t> readWav(string fname) {
	wav_hdr header;
	ifstream in(fname, ios::binary);
	in.read((char*)&header, sizeof(header));

	int sampleCount = header.Subchunk2Size / (header.bitsPerSample / 8);

	vector<int16_t> samples;
	samples.resize(sampleCount);
	in.read((char*)&samples[0], sampleCount * sizeof(int16_t));


	for (int i = 0; i < sampleCount; i++) {
		samples[i] = (int16_t)((float)samples[i] * 0.4f);
	}

	return samples;
}

void encode_test() {
	vector<int16_t> allSamples = readWav("touch12_left.wav");

	ofstream outfile("replay.as");
	outfile << "array<array<uint8>> replay = {\n";

	int frameSize = 480;
	int* err = NULL;
	uint16_t sampleRate = 12000; // game sounds like 8k no matter how high you set this

	//OpusEncoder* encoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_VOIP, err);
	//OpusEncoder* encoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_AUDIO, err);
	OpusEncoder* encoder = opus_encoder_create(sampleRate, 1, OPUS_APPLICATION_RESTRICTED_LOWDELAY, err);

	// 160 + 32k   + 4 frames + 0.08 delay = best quality
	// 160 + 32k   + 3 frames + 0.06 delay = best quality, smaller packets
	// 480 + 26k   + 2 frames + 0.12 delay = good quality, slow packet rate
	// 160 + 26k   + 2 frames + 0.04 delay = good quality, small packets
	// 480 + 14.5k + 4 frames + 0.24 delay = bad quality, slowest packet rate

	opus_encoder_ctl(encoder, OPUS_SET_BITRATE(32000));
	//opus_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_WIDEBAND));
	//opus_encoder_ctl(encoder, OPUS_SET_VBR(0));
	//opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(10));
	//opus_encoder_ctl(encoder, OPUS_SET_SIGNAL_REQUEST, OPUS_SIGNAL_VOICE);
	//opus_encoder_ctl(encoder, OPUS_SET_DTX_REQUEST, 1);

	if (err != OPUS_OK) {
		printf("FAILED TO CREATE ENCODER\n");
		return;
	}

	uint8_t buffer[1024];
	vector<OpusFrame> frames;
	uint16_t sequence = 0;

	uint32_t table[256];
	crc32_generate_table(table);

	float timeBetweenPackets = 0.05f;
	int sentFrames = 0;
	int totalPackets = 0;

	for (int i = 0; i < allSamples.size() - frameSize; i += frameSize) {
		int length = opus_encode(encoder, &allSamples[i], frameSize, buffer, 1024);
		if (length < 0) {
			printf("Failed with %d\n", length);
			break;
		}

		OpusFrame frame;
		frame.length = length;
		frame.sequence = sequence++;
		frame.data = new uint8_t[length];
		memcpy(frame.data, buffer, length);

		frames.push_back(frame);

		if (frames.size() >= 2) {
			sentFrames += frames.size();

			vector<uint8_t> packet;
			packet.push_back(67);
			packet.push_back(151);
			packet.push_back(144);
			packet.push_back(18);
			packet.push_back(1);
			packet.push_back(0);
			packet.push_back(16);
			packet.push_back(1);
			packet.push_back(11);
			packet.push_back(sampleRate & 0xff);
			packet.push_back(sampleRate >> 8);
			packet.push_back(6);
			packet.push_back(0); // placeholder for payload length
			packet.push_back(0); // placeholder for payload length

			for (int k = 0; k < frames.size(); k++) {
				uint16_t len = frames[k].length;
				uint16_t seq = frames[k].sequence;

				packet.push_back(len & 0xff);
				packet.push_back(len >> 8);
				packet.push_back(seq & 0xff);
				packet.push_back(seq >> 8);

				for (int b = 0; b < frames[k].length; b++) {
					uint8_t byte = frames[k].data[b];
					packet.push_back(byte);
				}
				delete[] frames[k].data;
			}

			uint16_t payloadLength = (packet.size() + 4) - 18; // total packet size - header size
			packet[12] = payloadLength & 0xff;
			packet[13] = payloadLength >> 8;

			uint32_t crc32 = crc32_update(table, 0, &packet[0], packet.size());
			packet.push_back((crc32 >> 0) & 0xff);
			packet.push_back((crc32 >> 8) & 0xff);
			packet.push_back((crc32 >> 16) & 0xff);
			packet.push_back((crc32 >> 24) & 0xff);

			outfile << "\t{";

			for (int k = 0; k < packet.size(); k++) {
				outfile << (int)packet[k];

				if (k < packet.size() - 1) {
					outfile << ", ";
				}
			}

			outfile << "},\n";

			if (totalPackets++ % 32 == 0)
				printf("Wrote %d frames in packet (%d bytes)\n", frames.size(), packet.size());
			frames.clear();

			if (packet.size() > 500) {
				printf("TOO MUCH DATA %d\n", packet.size());
				break;
			}
		}
	}

	outfile << "};\n";
	outfile.close();
}

#define MAX_CHANNELS 4
#define PIPE_BUFFER_SIZE 16384

PipeInputBuffer* g_pipes[MAX_CHANNELS];
int g_sampling_rate = 12000; // should be something opus supports and is divisble by packet delay
int g_packet_delay = 100; // millesconds between output packets

long long getTimeMillis() {
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void WriteOutputWav(vector<int16_t>& allSamples) {
	wav_hdr header;
	header.ChunkSize = allSamples.size() * 2 + sizeof(wav_hdr) - 8;
	header.Subchunk2Size = allSamples.size() * 2 + sizeof(wav_hdr) - 44;

	string fname = "mixer.wav";
	ofstream out(fname, ios::binary);
	out.write(reinterpret_cast<const char*>(&header), sizeof(header));

	for (int z = 0; z < allSamples.size(); z++) {
		out.write(reinterpret_cast<char*>(&allSamples[z]), sizeof(int16_t));
	}

	out.close();
	printf("Wrote file!\n");
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

void pipe_test() {
	for (int i = 0; i < MAX_CHANNELS; i++) {
		g_pipes[i] = new PipeInputBuffer("MicBotPipe" + to_string(i), PIPE_BUFFER_SIZE);
	}

	int samplesPerPacket = g_sampling_rate / (1000/g_packet_delay);

	int16_t* pipeBuffer = new int16_t[samplesPerPacket];
	float* mixBuffer = new float[samplesPerPacket];

	vector<int16_t> allSamples;

	long long nextPacketMillis = getTimeMillis();

	while (1) {
		while (getTimeMillis() < nextPacketMillis) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		nextPacketMillis += g_packet_delay;

		memset(mixBuffer, 0, samplesPerPacket*sizeof(float));

		bool wroteAnySamples = false;

		for (int i = 0; i < MAX_CHANNELS; i++) {
			if (g_pipes[i]->read((char*)pipeBuffer, samplesPerPacket * sizeof(int16_t))) {
				// can't read yet
				continue;
			}

			wroteAnySamples = true;

			for (int k = 0; k < samplesPerPacket; k++) {
				mixBuffer[k] += (float)pipeBuffer[k] / 32768.0f;
			}
		}

		if (wroteAnySamples) {
			for (int i = 0; i < samplesPerPacket; i++) {
				int16_t clamped = clampf(mixBuffer[i], -1.0f, 1.0f) * 32767;
				allSamples.push_back(clamped);
			}

			if (allSamples.size() > 12000 * 5) {
				WriteOutputWav(allSamples);
				allSamples.clear();
			}
		}

		// TODO: encode
	}
}

int main(int argc, const char **argv)
{
	pipe_test();

	return 0;
}
