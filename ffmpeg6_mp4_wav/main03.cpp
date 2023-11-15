#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <thread>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

std::mutex mtx;

void write_wav_header(FILE* file, int sample_rate, int channels, int bits_per_sample, int total_samples) {
    char header[44];
    
    int block_align = channels * (bits_per_sample / 8);

    // WAV header
    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    //*(int*)(header + 4) = 36 + total_samples * block_align;
    //04~07 4字节size=文件大小-8字节
    header[4] = (char)((44 - 8 + total_samples * block_align) & 0xff);
    header[5] = (char)(((44 - 8 + total_samples * block_align) >> 8) & 0xff);
    header[6] = (char)(((44 - 8 + total_samples * block_align) >> 16) & 0xff);
    header[7] = (char)(((44 - 8 + total_samples * block_align) >> 24) & 0xff);

    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    //*(int*)(header + 16) = 16;
    //16~19 4字节过滤字节(一般为00000010H)
    header[16] = 16;
    header[17] = 0;
    header[18] = 0;
    header[19] = 0;
    //*(short*)(header + 20) = 1; // PCM format
    //20~21 2字节格式种类(值为1时, 表示数据为线性pcm编码)
    header[20] = 1;
    header[21] = 0;
    //*(short*)(header + 22) = channels;
    //22~23 2字节通道数, 单声道为1, 双声道为2
    header[22] = channels;
    header[23] = 0;
    //*(int*)(header + 24) = sample_rate;
    //24~27 4字节采样率
    header[24] = (char)(sample_rate & 0xff);
    header[25] = (char)((sample_rate >> 8) & 0xff);
    header[26] = (char)((sample_rate >> 16) & 0xff);
    header[27] = (char)((sample_rate >> 24) & 0xff);
    //*(int*)(header + 28) = byte_rate;
    //28~31 4字节比特率(Byte率 = 采样频率 * 音频通道数 * 每次采样得到的样本位数 / 8)
    int byte_rate = sample_rate * channels * (bits_per_sample / 8);
    header[28] = (char)(byte_rate & 0xff);
    header[29] = (char)((byte_rate >> 8) & 0xff);
    header[30] = (char)((byte_rate >> 16) & 0xff);
    header[31] = (char)((byte_rate >> 24) & 0xff);
    //*(short*)(header + 32) = block_align;
    //32~33 2字节数据块长度(每个样本的字节数 = 通道数 * 每次采样得到的样本位数 / 8)
    header[32] = block_align;
    header[33] = 0;
    //*(short*)(header + 34) = bits_per_sample;
    //34~35 2字节每个采样点的位数
    header[34] = bits_per_sample;
    header[35] = 0;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    //*(int*)(header + 40) = total_samples * block_align;
    //40~43 4字节 pcm音频数据大小
    header[40] = (char)((total_samples * block_align) & 0xff);
    header[41] = (char)(((total_samples * block_align) >> 8) & 0xff);
    header[42] = (char)(((total_samples * block_align) >> 16) & 0xff);
    header[43] = (char)(((total_samples * block_align) >> 24) & 0xff);

    fwrite(header, 1, 44, file);
}

int mp4ToWav(std::string inPath, std::string outPath) {
    //const char* input_filename = "rtsp://10.52.8.106:554/stream1";test.mp4
    auto start = std::chrono::high_resolution_clock::now();

    //const char* input_filename = "./input/test.mp4";
    //std::string full_path = "./output/test";
    //full_path.append(std::to_string(1));
    //full_path.append(".wav");
    //const char* output_filename = full_path.c_str();
    const char* input_filename = inPath.c_str();
    const char* output_filename = outPath.c_str();

    AVFormatContext* input_format_ctx = NULL;
    if (avformat_open_input(&input_format_ctx, input_filename, NULL, NULL) != 0) {
        fprintf(stderr, "Could not open input file\n");
        return -1;
    }

    // Retrieve input stream information
    if (avformat_find_stream_info(input_format_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&input_format_ctx);
        return -1;
    }

    // Find the audio stream
    int audio_stream_index = -1;
    for (int i = 0; i < input_format_ctx->nb_streams; i++) {
        if (input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index == -1) {
        fprintf(stderr, "Could not find audio stream\n");
        avformat_close_input(&input_format_ctx);
        return -1;
    }

    AVCodecParameters* input_codec_params = input_format_ctx->streams[audio_stream_index]->codecpar;

    // Open input codec
    const AVCodec* input_codec = avcodec_find_decoder(input_codec_params->codec_id);
    if (!input_codec) {
        fprintf(stderr, "Unsupported codec\n");
        avformat_close_input(&input_format_ctx);
        return -1;
    }

    AVCodecContext* input_codec_ctx = avcodec_alloc_context3(input_codec);
    if (!input_codec_ctx) {
        fprintf(stderr, "Failed to allocate codec context\n");
        avformat_close_input(&input_format_ctx);
        return -1;
    }
    if (avcodec_parameters_to_context(input_codec_ctx, input_codec_params) < 0) {
        fprintf(stderr, "Failed to copy codec parameters to codec context\n");
        avcodec_free_context(&input_codec_ctx);
        avformat_close_input(&input_format_ctx);
        return -1;
    }
    if (avcodec_open2(input_codec_ctx, input_codec, NULL) < 0) {
        fprintf(stderr, "Failed to open codec\n");
        avcodec_free_context(&input_codec_ctx);
        avformat_close_input(&input_format_ctx);
        return -1;
    }

    // Output WAV file settings
    //int output_sample_rate = 44100;
    int output_channels = input_codec_ctx->channels;

    // Initialize FFmpeg resampler
    SwrContext* resample_ctx = swr_alloc_set_opts(NULL,
        av_get_default_channel_layout(input_codec_ctx->channels),
        AV_SAMPLE_FMT_S16,
        input_codec_ctx->sample_rate,
        av_get_default_channel_layout(input_codec_ctx->channels),
        input_codec_ctx->sample_fmt,
        input_codec_ctx->sample_rate,
        0, NULL);
    if (!resample_ctx || swr_init(resample_ctx) < 0) {
        fprintf(stderr, "Failed to initialize resampler\n");
        avcodec_free_context(&input_codec_ctx);
        avformat_close_input(&input_format_ctx);
        return -1;
    }

    // Open output file
    FILE* output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Could not open output file\n");
        swr_free(&resample_ctx);
        avcodec_free_context(&input_codec_ctx);
        avformat_close_input(&input_format_ctx);
        return -1;
    }

    // 计算音频帧的总样本数
    int total_samples = 0;
    // 写入 WAV 文件头
    int sample_rate = input_codec_ctx->sample_rate;
    int channels = input_codec_ctx->channels;
    //channels = 1;
    int bits_per_sample = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * 8;
    //std::cout << "bits_per_sample = " << bits_per_sample << std::endl;
    write_wav_header(output_file, sample_rate, channels, bits_per_sample, total_samples);
    fseek(output_file, 44, SEEK_SET);

    // Read and resample audio data
    AVPacket packet;
    av_init_packet(&packet);
    AVFrame* input_frame = av_frame_alloc();
    int buffer_size = av_samples_get_buffer_size(NULL, output_channels, input_codec_ctx->frame_size, AV_SAMPLE_FMT_S16, 1);
    uint8_t* resampled_buffer = (uint8_t*)av_malloc(buffer_size);
    while (av_read_frame(input_format_ctx, &packet) >= 0) {
        if (packet.stream_index == audio_stream_index) {
            int ret = avcodec_send_packet(input_codec_ctx, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error sending a packet for decoding\n");
                break;
            }
            while (ret >= 0) {
                ret = avcodec_receive_frame(input_codec_ctx, input_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    break;
                }

                // Resample the audio data
                int samples = swr_convert(resample_ctx, &resampled_buffer, input_frame->nb_samples, (const uint8_t**)input_frame->data, input_frame->nb_samples);
                if (samples < 0) {
                    fprintf(stderr, "Error during resampling\n");
                    break;
                }
                int bytes_written = fwrite(resampled_buffer, 1, samples * output_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16), output_file);
                if (bytes_written != samples * output_channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)) {
                    fprintf(stderr, "Error writing resampled data to output file\n");
                    break;
                }
                total_samples += input_frame->nb_samples;
            }
        }
        av_packet_unref(&packet);
    }
    std::unique_lock<std::mutex> lock(mtx);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << output_filename << "线程wav 生成完成,用时:" << duration/1000 << std::endl;
    lock.unlock();
    // 更新 WAV 文件头中的样本数
    fseek(output_file, 0, SEEK_SET);
    write_wav_header(output_file, sample_rate, channels, bits_per_sample, total_samples);

    // Clean up resources
    fclose(output_file);
    av_frame_free(&input_frame);
    av_free(resampled_buffer);
    swr_free(&resample_ctx);
    avcodec_free_context(&input_codec_ctx);
    avformat_close_input(&input_format_ctx);
    return 0;
}

int main(int argc, char* argv[]) {
    std::cout << "将需要转格式的*.mp4文件放在input目录下。" << std::endl;
    std::cout << "生成的*.wav文件保存在output目录下。" << std::endl;
    std::cout << "请指定线程处理上线，超过的文件不进行转换" << std::endl;
    int maxThreadCount;
    std::cin >> maxThreadCount;

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;

    int threadCount = 0;
    //文件扫描
    WIN32_FIND_DATA findData;
    HANDLE hFind = FindFirstFile("./input/*.mp4", &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening directory" << std::endl;
        return -1;
    }
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        char* mp4FileName = findData.cFileName;
        //输入mp4
        std::string inPath = "./input/";
        inPath.append(mp4FileName);
        //输出wav
        std::string outPath = "./output/";
        outPath.append(mp4FileName);
        std::string findStr = "mp4";
        std::string replaceStr = "wav";
        size_t pos = outPath.find(findStr);
        while (pos != std::string::npos) {
            outPath.replace(pos, findStr.length(), replaceStr);
            pos = outPath.find(findStr, pos + replaceStr.length());
        }
        if (threadCount <  maxThreadCount) {
            std::cout << "扫描到mp4文件" << inPath << "线程开始执行，生成wav文件" << outPath << std::endl;
            threads.push_back(std::thread(mp4ToWav, inPath, outPath));
        } else {
            std::cout << "扫描到mp4文件" << inPath << "超出线程上线，不生成wav文件" << std::endl;
        }
        threadCount++;

    } while (FindNextFile(hFind, &findData) != 0);
    FindClose(hFind);

    // 创建并启动多个线程
    //for (int i = 0; i < numThreads; ++i) {
    //    std::cout << i << "线程开始执行" << std::endl;
    //    threads.push_back(std::thread(mp4ToWav, i));
    //}

    // 等待所有线程完成
    for (std::thread& t : threads) {
        t.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    std::cout << "所有线程完成,用时(毫秒):" << duration/1000 << std::endl;
    system("pause");
    return 0;
}