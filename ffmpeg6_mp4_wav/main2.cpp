#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

void write_wav_header(FILE* file, int sample_rate, int channels, int bits_per_sample, int total_samples) {
    char header[44];
    int byte_rate = sample_rate * channels * (bits_per_sample / 8);
    int block_align = channels * (bits_per_sample / 8);

    // WAV header
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    *(int*)(header + 4) = 36 + total_samples * block_align;
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    *(int*)(header + 16) = 16;
    *(short*)(header + 20) = 1; // PCM format
    *(short*)(header + 22) = channels;
    *(int*)(header + 24) = sample_rate;
    *(int*)(header + 28) = byte_rate;
    *(short*)(header + 32) = block_align;
    *(short*)(header + 34) = bits_per_sample;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    *(int*)(header + 40) = total_samples * block_align;

    fwrite(header, 1, 44, file);
}

int main() {
    // WAV file settings
    const char* output_filename = "whitenoise.wav";
    int sample_rate = 44100; // Hz
    int channels = 2;
    int bits_per_sample = 16;
    int duration_seconds = 5; // Duration of the white noise in seconds

    // Calculate total number of samples per channel
    int total_samples_per_channel = sample_rate * duration_seconds;

    // Total number of samples for all channels
    int total_samples = total_samples_per_channel * channels;

    // Open output file
    FILE* output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Could not open output file\n");
        return -1;
    }

    // Write WAV header
    write_wav_header(output_file, sample_rate, channels, bits_per_sample, total_samples);

    // Generate and write white noise samples
    srand(time(NULL));
    for (int i = 0; i < total_samples; i++) {
        int16_t sample = rand() % (1 << (bits_per_sample - 1));
        fwrite(&sample, sizeof(int16_t), 1, output_file);
    }

    // Close the file
    fclose(output_file);

    return 0;
}
