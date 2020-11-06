#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "source/GPMF_parser.h"
#include "source/GPMF_mp4reader.h"

void getData(size_t mp4, uint32_t four_cc) {
	char* filename;
	if (four_cc == STR2FOURCC("ACCL")){
		filename = "ACCL.csv";
	}
	else if (four_cc == STR2FOURCC("GYRO")){
		filename = "GYRO.csv";
	}
	else if (four_cc == STR2FOURCC("GPS5")){
		filename = "GPS5.csv";
	}
	else{
		filename = "";
	}

	if (filename == "") {
		printf("Invalid file name");
		return;
	}

	FILE* f = fopen(filename,"w");
	if (f == NULL) {
		printf("Not able to create CSV file");
		return;
	}

	uint32_t* payload = NULL;
	uint32_t payloadsize;
	GPMF_ERR ret;
	GPMF_stream gs;
	uint32_t num_payloads = GetNumberPayloads(mp4);
	// loop through every payload
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
		ret = GPMF_Init(&gs, payload, payloadsize);
		if (ret != GPMF_OK) break;

		// start reading the buffer
		while (GPMF_OK == GPMF_FindNext(&gs, STR2FOURCC("STRM"), GPMF_RECURSE_LEVELS | GPMF_TOLERANT)) {
			if (GPMF_VALID_FOURCC(four_cc)) {
				if (GPMF_OK != GPMF_FindNext(&gs, four_cc, GPMF_RECURSE_LEVELS | GPMF_TOLERANT)) {
					continue;
				}
			}

			uint32_t key = GPMF_Key(&gs);
			uint32_t num_samples = GPMF_Repeat(&gs);
			uint32_t num_elements = GPMF_ElementsInStruct(&gs);
			uint32_t buffersize = num_samples * num_elements * sizeof(double);
			double* tmpbuffer = (double*)malloc(buffersize);

			if (num_samples && tmpbuffer) {
				uint32_t i, j;

				// extract all samples from tmpbuffer
				if (GPMF_OK == GPMF_ScaledData(&gs, tmpbuffer, buffersize, 0, num_samples, GPMF_TYPE_DOUBLE)) {
					double* ptr = tmpbuffer;
					for (i = 0; i < num_samples; i++) {
						for (j = 0; j < num_elements; j++) {
							double val = *ptr;
							ptr++;
							fprintf(f, "%f,", val);
							//printf("%.2f ", val);
						}
						fprintf(f, "\n");
						//printf("\n");
					}
				}
				free(tmpbuffer);
			}
		}
		GPMF_Free(&gs);
	}
	fclose(f);
	return;
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		printf("error: there is no specified video file\n\n");
		return -1;
	}

	double metadatalength;
	uint32_t accl = STR2FOURCC("ACCL");
	uint32_t gyro = STR2FOURCC("GYRO");
	uint32_t gps = STR2FOURCC("GPS5");

	/*char* filename;
	filename = "samples/hero8.mp4";*/
	size_t mp4 = OpenMP4Source(argv[1], MOV_GPMF_TRAK_TYPE, MOV_GPMF_TRAK_SUBTYPE);
	if (mp4 == 0) {
		printf("error: this file is either invalid or it does not contain GPMF data\n\n");
		return -1;
	}

	metadatalength = GetDuration(mp4);
	if (metadatalength > 0.0) {
		// get frame rate of the video file
		uint32_t rate_num, rate_de;
		uint32_t frame_num = GetVideoFrameRateAndCount(mp4, &rate_num, &rate_de);
		printf("Video Frame Rate: %.3f\nNumber of Frames: %d\n\n", (float)rate_num/rate_de, frame_num);

		printf("begin extracting...\n");

		// read acceleration data
		getData(mp4, accl);

		// read gyro data
		getData(mp4, gyro);

		// read GPS data
		getData(mp4, gps);
		printf("done extraction\n\n");
	}

	// end the stream
	CloseSource(mp4);

	return 0;
}
