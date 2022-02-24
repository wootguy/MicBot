#include <webm/callback.h>
#include <webm/file_reader.h>
#include <webm/status.h>
#include <webm/webm_parser.h>


#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <iostream>
#include "opus/opus.h"
#include "pipes.h"
#include "curl/curl.h"

#include "opus_util.h"

using namespace std;


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

struct MemoryStruct {
	char* memory;
	size_t size;
};

int totalBytes = 0;

static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;

	//memcpy(&(mem->memory[mem->size]), contents, realsize);

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	printf("Loaded some memory %d\n", realsize);
	totalBytes += realsize;

	return realsize;
}

// youtube-dl -f worstaudio https://www.youtube.com/watch?v=Twi92KYddW4 -g --get-filename --skip-download


void curl_test() {
	CURL* curl_handle;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_ALL);

	/* init the curl session */
	curl_handle = curl_easy_init();

	/* specify URL to get */
	//curl_easy_setopt(curl_handle, CURLOPT_URL, "https://rr3---sn-5hne6nzs.googlevideo.com/videoplayback?expire=1645663487&ei=n4AWYtLqHcOD6dsPy_SQiAs&ip=107.191.105.136&id=o-ADyMD6HjKfLJRCvj5kNQmP2HFZa7U1nBFOPS6R_C7EwG&itag=251&source=youtube&requiressl=yes&mh=43&mm=31%2C26&mn=sn-5hne6nzs%2Csn-5ualdn76&ms=au%2Conr&mv=u&mvi=3&pl=24&vprv=1&mime=audio%2Fwebm&ns=thV4GblUswHqjOYsOKmfEuEG&gir=yes&clen=499858631&dur=36000.881&lmt=1574588899335329&mt=1645641428&fvip=6&keepalive=yes&fexp=24001373%2C24007246&c=WEB&txp=5311222&n=3CoZ6lAZXOjC-ShIUzuU&sparams=expire%2Cei%2Cip%2Cid%2Citag%2Csource%2Crequiressl%2Cvprv%2Cmime%2Cns%2Cgir%2Cclen%2Cdur%2Clmt&sig=AOq0QJ8wRAIgbxaySSeec0Cr5aruVibh_YZs8uM_PA3EFpf0_5vnHTYCIBmJBvlL5c-RpbTKx7gAvsUs8IxXAmXFlyaLPEB1aqJO&lsparams=mh%2Cmm%2Cmn%2Cms%2Cmv%2Cmvi%2Cpl&lsig=AG3C_xAwRgIhAOU9OWxVScwCv_chL3nahgofMxdfguEGg3P5CChbv3n3AiEA1COWlGhpSdIQpo6RycIkZER2vD57gYRZyCA0CyzAYac%3D");
	curl_easy_setopt(curl_handle, CURLOPT_URL, "https://rr1---sn-a5mekned.googlevideo.com/videoplayback/expire/1645683363/ei/Q84WYrfHL9vKkga14YeABA/ip/47.157.183.178/id/fb310911d6d950ff/itag/139/source/youtube/requiressl/yes/mh/_a/mm/31,29/mn/sn-a5mekned,sn-a5msen7l/ms/au,rdu/mv/m/mvi/1/pl/18/initcwndbps/1600000/spc/4ocVC0dClDSVjK9edaynN_6U1ort/vprv/1/ratebypass/yes/mime/audio%2Fmp4/otfp/1/gir/yes/clen/673898/lmt/1621805188000584/dur/110.387/mt/1645661335/fvip/1/keepalive/yes/fexp/24001373,24007246/sparams/expire,ei,ip,id,itag,source,requiressl,spc,vprv,ratebypass,mime,otfp,gir,clen,lmt,dur/sig/AOq0QJ8wRQIhAM6TrPQlsUrZMCcdPSnuVAJHOuBVEJGFPkiIuxto2jHSAiA8lTFaQFxhwCl5MfnSSJAcZ_bCUZJfwmJ-2STyrp9vog%3D%3D/lsparams/mh,mm,mn,ms,mv,mvi,pl,initcwndbps/lsig/AG3C_xAwRAIgD50dG55ySDsx1AgKCoc5PSLdd7n3gPqv79YEUOAyDL8CIGu6hY3ZHRhp1Qx2SDQ2IY1tlUGcvOauidlfvV_H1ONi/");

	/* send all data to this function  */
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	//curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)&chunk);

	/* some servers do not like requests that are made without a user-agent
	   field, so we provide one */
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_CAINFO, "cacert.pem");
	curl_easy_setopt(curl_handle, CURLOPT_CAPATH, "cacert.pem");
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
	//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

	/* get it! */
	res = curl_easy_perform(curl_handle);

	/* check for errors */
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
	}
	else {
		/*
		 * Now, our chunk.memory points to a memory block that is chunk.size
		 * bytes big and contains the remote file.
		 *
		 * Do something nice with it!
		 */

		printf("downloaded %d bytes\n", totalBytes);
	}

	/* cleanup curl stuff */
	curl_easy_cleanup(curl_handle);	
}

int webm_test();
int mp4_test();



int main(int argc, const char **argv)
{
	//webm_test();
	//curl_test();
	//encode_test();
	mp4_test();

	curl_global_cleanup();

	return 0;
}
