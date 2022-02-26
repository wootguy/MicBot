#include "util.h"
#include <fstream>

using namespace std;

float clampf(float val, float min, float max) {
	if (val > max) {
		return max;
	}
	else if (val < min) {
		return min;
	}

	return val;
}

void WriteOutputWav(string fname, vector<int16_t>& allSamples) {
	wav_hdr header;
	header.ChunkSize = allSamples.size() * 2 + sizeof(wav_hdr) - 8;
	header.Subchunk2Size = allSamples.size() * 2 + sizeof(wav_hdr) - 44;

	ofstream out(fname, ios::binary);
	out.write(reinterpret_cast<const char*>(&header), sizeof(header));

	for (int z = 0; z < allSamples.size(); z++) {
		out.write(reinterpret_cast<char*>(&allSamples[z]), sizeof(int16_t));
	}

	out.close();
	printf("Wrote file!\n");
}