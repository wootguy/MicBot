#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "opus/opus.h"

#include <vector>
#include <fstream>
#include <iostream>
#include <string>

bool m_PacketLossConcealment = true;
int m_nDecodeSeq = 0;
const int MAX_PACKET_LOSS = 10;
const int FRAME_SIZE = 160;
const int MAX_FRAME_SIZE = 3 * FRAME_SIZE;
const int BYTES_PER_SAMPLE = 2;

void ResetState(OpusDecoder* decoder) {
	opus_decoder_ctl(decoder, OPUS_RESET_STATE);
}

int VoiceEncoder_Opus_Decompress(OpusDecoder* decoder, const char* pCompressed, int compressedBytes, char* pUncompressed, int maxUncompressedBytes)
{
	const char* pReadPos = pCompressed;
	const char* pReadPosMax = &pCompressed[compressedBytes];

	char* pWritePos = pUncompressed;
	char* pWritePosMax = &pUncompressed[maxUncompressedBytes];

	int frameCount = 0;

	while (pReadPos < pReadPosMax)
	{
		uint16_t nPayloadSize = *(uint16_t*)pReadPos;
		pReadPos += sizeof(uint16_t);

		if (nPayloadSize == 0xFFFF)
		{
			ResetState(decoder);
			m_nDecodeSeq = 0;
			break;
		}

		if (m_PacketLossConcealment)
		{
			uint16_t nCurSeq = *(uint16_t*)pReadPos;
			pReadPos += sizeof(uint16_t);

			if (nCurSeq < m_nDecodeSeq)
			{
				ResetState(decoder);
			}
			else if (nCurSeq != m_nDecodeSeq)
			{
				int nPacketLoss = nCurSeq - m_nDecodeSeq;
				if (nPacketLoss > MAX_PACKET_LOSS) {
					nPacketLoss = MAX_PACKET_LOSS;
				}

				for (int i = 0; i < nPacketLoss; i++)
				{
					if ((pWritePos + MAX_FRAME_SIZE) >= pWritePosMax)
					{
						//Assert(false);
						printf("OVERFLOW WRITE\n");
						break;
					}

					int nBytes = opus_decode(decoder, 0, 0, (opus_int16*)pWritePos, FRAME_SIZE, 0);
					if (nBytes <= 0)
					{
						// raw corrupted
						continue;
					}

					pWritePos += nBytes * BYTES_PER_SAMPLE;
				}
			}

			m_nDecodeSeq = nCurSeq + 1;
		}

		if ((pReadPos + nPayloadSize) > pReadPosMax)
		{
			//Assert(false);
			printf("OVERFLOW READ\n");
			break;
		}

		if ((pWritePos + MAX_FRAME_SIZE) > pWritePosMax)
		{
			//Assert(false);
			printf("OVERFLOW WRITE\n");
			break;
		}

		memset(pWritePos, 0, MAX_FRAME_SIZE);

		if (nPayloadSize == 0)
		{
			// DTX (discontinued transmission)
			pWritePos += MAX_FRAME_SIZE;
			continue;
		}

		int nBytes = opus_decode(decoder, (const unsigned char*)pReadPos, nPayloadSize, (opus_int16*)pWritePos, 8192, 0);
		if (nBytes <= 0)
		{
			// raw corrupted
		}
		else
		{
			printf("Decoded %d from packet\n", nBytes);
			frameCount += 1;
			pWritePos += nBytes * BYTES_PER_SAMPLE;
		}

		pReadPos += nPayloadSize;
	}

	printf("Packet had %d frames", frameCount);

	return (pWritePos - pUncompressed) / BYTES_PER_SAMPLE;
}
