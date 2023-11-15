#include <stdio.h>
#include <iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}


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

int main(int argc, char* argv[]) {
    //const char* url = "rtsp://10.52.8.106:554/stream1";test.mp4
    const char* input_filename = "testu.mp4";
    const char* output_filename = "test.wav";
    AVFormatContext* format_ctx = NULL;
    AVCodecContext* codec_ctx = NULL;
    const AVCodec* decoder = NULL;
    AVPacket packet;
    AVFrame* frame = NULL;
    FILE* output_file = NULL;
    int ret;



    // 打开输入文件
    ret = avformat_open_input(&format_ctx, input_filename, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open input file\n");
        return -1;
    }

    // 获取音频流信息
    ret = avformat_find_stream_info(format_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return -1;
    }

    // 查找音频流
    int audio_stream_index = -1;
    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            std::cout << i << "音频信息" << std::endl;
            std::cout << "sample_rate = " << format_ctx->streams[i]->codecpar->sample_rate << std::endl;
            std::cout << "channels = " << format_ctx->streams[i]->codecpar->channels << std::endl;
            std::cout << "frame_size = " << format_ctx->streams[i]->codecpar->frame_size << std::endl;
            //std::cout << "block_align = " << format_ctx->streams[i]->codecpar->block_align << std::endl;
            audio_stream_index = i;
            break;
        }
    }
    if (audio_stream_index == -1) {
        fprintf(stderr, "Could not find audio stream\n");
        return -1;
    }

    // 获取音频解码器
    decoder = avcodec_find_decoder(format_ctx->streams[audio_stream_index]->codecpar->codec_id);
    if (!decoder) {
        fprintf(stderr, "Audio decoder not found\n");
        return -1;
    }

    // 创建解码器上下文
    codec_ctx = avcodec_alloc_context3(decoder);
    if (!codec_ctx) {
        fprintf(stderr, "Could not allocate codec context\n");
        return -1;
    }

    ret = avcodec_parameters_to_context(codec_ctx, format_ctx->streams[audio_stream_index]->codecpar);
    if (ret < 0) {
        fprintf(stderr, "Could not copy codec parameters to context\n");
        return -1;
    }

    // 打开解码器
    ret = avcodec_open2(codec_ctx, decoder, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec\n");
        return -1;
    }

    // 打开输出文件
    output_file = fopen(output_filename, "wb");
    if (!output_file) {
        fprintf(stderr, "Could not open output file\n");
        return -1;
    }

    // 计算音频帧的总样本数
    int total_samples = 0;
    // 写入 WAV 文件头
    int sample_rate = codec_ctx->sample_rate;
    int channels = codec_ctx->channels;
    //channels = 1;
    int bits_per_sample = av_get_bytes_per_sample(codec_ctx->sample_fmt) * 8;
    std::cout << "codec_ctx->sample_fmt = " << codec_ctx->sample_fmt << std::endl;
    std::cout << "sample_fmt = " << AV_SAMPLE_FMT_FLTP << std::endl;
    write_wav_header(output_file, sample_rate, channels, bits_per_sample, total_samples);
    fseek(output_file, 44, SEEK_SET);

    // 初始化音频帧
    frame = av_frame_alloc();

    // 读取音频数据并保存为 WAV 文件
    while (av_read_frame(format_ctx, &packet) >= 0) {
        if (total_samples>44100*3) {
            break;
        }
        if (packet.stream_index == audio_stream_index) {
            //fprintf(stdout, "audio stream, packet size: %d\n", packet.size);
            ret = avcodec_send_packet(codec_ctx, &packet);
            if (ret < 0) {
                fprintf(stderr, "Error sending packet for decoding\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    fprintf(stderr, "Error during decoding\n");
                    break;
                }
                //fprintf(stdout, "frame->nb_samples size: %d,%d\n", codec_ctx->sample_fmt, av_get_bytes_per_sample(codec_ctx->sample_fmt));
                // 在这里你可以将音频数据写入 WAV 文件
                // 使用 frame->data 和 frame->linesize 来访问音频数据
                // 写入解码后的音频数据到 WAV 文件
                for (int i = 0; i < frame->nb_samples; i++) {
                    for (int ch = 0; ch < channels; ch++) {
                        //if (ch!=0) {
                        //    continue;
                        //}
                        fwrite(frame->data[ch] + i * av_get_bytes_per_sample(codec_ctx->sample_fmt), 1, av_get_bytes_per_sample(codec_ctx->sample_fmt), output_file);
                    }
                }

                total_samples += frame->nb_samples;

            }
        }
        av_packet_unref(&packet);
    }
    std::cout << "total_samples:" << total_samples << std::endl;
    // 更新 WAV 文件头中的样本数
    fseek(output_file, 0, SEEK_SET);
    write_wav_header(output_file, sample_rate, channels, bits_per_sample, total_samples);


    // 清理资源
    av_frame_free(&frame);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    fclose(output_file);

    getchar();
    return 0;
}