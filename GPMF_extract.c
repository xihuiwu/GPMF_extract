#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "source/GPMF_parser.h"
#include "source/GPMF_mp4reader.h"

#define MAX_UNITS		64
#define MAX_UNITLEN		8

void getData(GPMF_stream *ms, size_t mp4, uint32_t num_payloads, uint32_t four_cc) {
	uint32_t* payload;
	uint32_t payloadsize;
	GPMF_ERR ret = GPMF_OK;
	// 
	for (uint32_t index = 0; index < num_payloads; index++) {
		// get current payload
		payloadsize = GetPayloadSize(mp4, index);
		payload = GetPayload(mp4, payload, index);
		if (payload == NULL) break;

		// get begin timestamp and end timestamp of current payload
		double begin = 0.0, end = 0.0;
		ret = GetPayloadTime(mp4, index, &begin, &end);
		if (ret != GPMF_OK) break;

		// initialize a GPMF_stream for parsing a particular buffer
		ret = GPMF_Init(ms, payload, payloadsize);
		if (ret != GPMF_OK) break;

		// start reading the buffer
		while (GPMF_OK == GPMF_FindNext(ms, STR2FOURCC("STRM"), GPMF_RECURSE_LEVELS | GPMF_TOLERANT)) {
			if (GPMF_VALID_FOURCC(four_cc)) {
				if (GPMF_OK == GPMF_FindNext(ms, four_cc, GPMF_RECURSE_LEVELS | GPMF_TOLERANT)) {
					continue;
				}
			}

			char* rawdata = (char*)GPMF_RawData(ms);
			uint32_t key = GPMF_Key(ms);
			GPMF_SampleType type = GPMF_Type(ms);
			uint32_t num_samples = GPMF_Repeat(ms);
			uint32_t num_elements = GPMF_ElementsInStruct(ms);
			uint32_t buffersize = num_samples * num_elements * sizeof(double);
			GPMF_stream ms_cp;
			double* ptr, * tmpbuffer = (double*)malloc(buffersize);

			char units[MAX_UNITS][MAX_UNITLEN] = { "" };
			uint32_t num_units = 1;

			char complextype[MAX_UNITS] = { "" };
			uint32_t type_samples = 1;

			if (num_samples && tmpbuffer) {
				uint32_t i, j;

				// get units for the specific FOUR CC
				GPMF_CopyState(ms, &ms_cp);
				if (GPMF_OK == GPMF_FindPrev(&ms_cp, GPMF_KEY_SI_UNITS, GPMF_CURRENT_LEVEL | GPMF_TOLERANT) ||
					GPMF_OK == GPMF_FindPrev(&ms_cp, GPMF_KEY_UNITS, GPMF_CURRENT_LEVEL | GPMF_TOLERANT)) {
					char* data = (char*)GPMF_RawData(&ms_cp);
					uint32_t ssize = GPMF_StructSize(&ms_cp);
					if (ssize > MAX_UNITLEN - 1) ssize = MAX_UNITLEN - 1;
					num_units = GPMF_Repeat(&ms_cp);

					for (i = 0; i < num_units && i < MAX_UNITS; i++) {
						memcpy(units[i], data, ssize);
						units[i][ssize] = 0;
						data += ssize;
					}
				}
				// get complex type
				GPMF_CopyState(ms, &ms_cp);
				type_samples = 0;
				if (GPMF_OK == GPMF_FindPrev(&ms_cp, GPMF_KEY_TYPE, GPMF_CURRENT_LEVEL | GPMF_TOLERANT)) {
					char* data = (char*)GPMF_RawData(&ms_cp);
					uint32_t ssize = GPMF_StructSize(&ms_cp);
					if (ssize > MAX_UNITLEN - 1) ssize = MAX_UNITLEN - 1;
					type_samples = GPMF_Repeat(&ms_cp);

					for (i = 0; i < type_samples && i < MAX_UNITS; i++) {
						complextype[i] = data[i];
					}
				}

				// extract samples into tmpbuffer
				if (GPMF_OK == GPMF_ScaledData(ms, tmpbuffer, buffersize, 0, num_samples, GPMF_TYPE_DOUBLE)) {
					ptr = tmpbuffer;
					int pos = 0;
					for (i = 0; i < num_samples; i++) {
						for (j = 0; j < num_elements; j++) {
							if (type == GPMF_TYPE_STRING_ASCII) {
								printf("%c", rawdata[pos]);
								pos++;
								ptr++;
							}
							else if (type_samples = 0) {
								printf("%.3f%s, ", *ptr++, units[j % num_units]);
							}
						}
					}
				}
				free(tmpbuffer);
			}
		}
		GPMF_Free(ms);
	}
}

int main(int argc, char* argv[]) {
	GPMF_ERR ret = GPMF_OK;
	GPMF_stream metadata_stream;
	GPMF_stream* ms = &metadata_stream;
	double metadatalength;
	uint32_t accl = STR2FOURCC("ACCL");
	uint32_t gyro = STR2FOURCC("GYRO");
	uint32_t gps = STR2FOURCC("GPS5");

	char* filename;
	filename = "hero8.mp4";
	size_t mp4 = OpenMP4Source(filename, MOV_GPMF_TRAK_TYPE, MOV_GPMF_TRAK_SUBTYPE);
	if (mp4 == 0) {
		printf("error: this file is either invalid or it does not contain GPMF data\n\n");
		return -1;
	}

	metadatalength = GetDuration(mp4);
	if (metadatalength > 0.0) {
		// get number of payloads
		uint32_t num_payloads = GetNumberPayloads(mp4);

		// get frame rate of the video file
		uint32_t rate_num, rate_de;
		uint32_t frame_num = GetVideoFrameRateAndCount(mp4, &rate_num, &rate_de);
		printf("Video Frame Rate: %.3f\n Number of Frames: %d", (float)rate_num/rate_de, frame_num);

		// read acceleration data
		getData(ms, mp4, num_payloads, accl);

		// read gyro data
		getData(ms, mp4, num_payloads, gyro);

		// read GPS data
		getData(ms, mp4, num_payloads, gps);
	}

	// end the stream
	CloseSource(mp4);

	return (int)ret;
}
