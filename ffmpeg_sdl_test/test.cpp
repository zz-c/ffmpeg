#include <iostream>
#include<SDL.h>

#include "Test.h"

int Test::testRtsp()
{
	std::cout << "ff testRtsp..." << std::endl;
	//std::cout << avcodec_configuration() << std::endl;
	//初始化封装库
	//av_register_all();
	//初始化网络库 （可以打开rtsp rtmp http 协议的流媒体视频）
	avformat_network_init();

	const char* url = "rtsp://10.52.8.106:554/stream1";

	pFormatCtx = avformat_alloc_context();
	//参数设置
	AVDictionary* dictionaryOpts = NULL;
	//设置rtsp流已tcp协议打开
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//网络延时时间
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);

	int ret = -1;
	ret = avformat_open_input(&pFormatCtx, url, 0, &dictionaryOpts);
	if (ret != 0) {
		fprintf(stderr, "fail to open url: %s, return value: %d\n", url, ret);
		return -1;
	}

	// Read packets of a media file to get stream information
	ret = avformat_find_stream_info(pFormatCtx, nullptr);
	if (ret < 0) {
		fprintf(stderr, "fail to get stream information: %d\n", ret);
		return -1;
	}

	// audio/video stream index
	int video_stream_index = -1;
	int audio_stream_index = -1;
	fprintf(stdout, "Number of elements in AVFormatContext.streams: %d\n", pFormatCtx->nb_streams);
	for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
		const AVStream* stream = pFormatCtx->streams[i];
		fprintf(stdout, "type of the encoded data: %d\n", stream->codecpar->codec_id);
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			fprintf(stdout, "dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d\n",
				stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
		}
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
			fprintf(stdout, "audio sample format: %d\n", stream->codecpar->format);
		}
	}

	pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL) {
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
	//根据编解码上下文中的编码id查找对应的解码
	const AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("%s", "找不到解码器\n");
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("%s", "解码器无法打开\n");
		return -1;
	}
	//输出视频信息
	printf("视频的pix_fmt：%d\n", pCodecCtx->pix_fmt);//AV_PIX_FMT_YUV420P
	printf("视频的宽高：%d,%d\n", pCodecCtx->width, pCodecCtx->height);
	printf("视频解码器的名称：%s\n", pCodec->name);


	pPacket = av_packet_alloc();// (AVPacket*)av_malloc(sizeof(AVPacket));
	pFrame = av_frame_alloc();

	//sdl
	SwsContext* pSwsCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, this->w, this->h, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (nullptr == pSwsCtx) {
		printf("get sws context failed\n");
		return -1;
	}

	AVFrame* pFrameYuv = av_frame_alloc();
	av_image_alloc(pFrameYuv->data, pFrameYuv->linesize, this->w, this->h, AV_PIX_FMT_YUV420P, 1);

	SDL_Texture* texture = nullptr;
	texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_TARGET, this->w, this->h);

	while (1) {
		ret = av_read_frame(pFormatCtx, pPacket);
		if (ret < 0) {
			fprintf(stderr, "error or end of file: %d\n", ret);
			continue;
		}
		if (pPacket->stream_index == audio_stream_index) {
			//fprintf(stdout, "audio stream, packet size: %d\n", packet->size);
			continue;
		}
		if (pPacket->stream_index == video_stream_index) {
			fprintf(stdout, "video stream, packet size: %d\n", pPacket->size);
			ret = avcodec_send_packet(pCodecCtx, pPacket);
			av_packet_unref(pPacket);
			if (ret < 0) {
				printf("%s", "解码完成");
			}
			ret = avcodec_receive_frame(pCodecCtx, pFrame);
			if (ret != 0) {
				//fprintf(stderr, "avcodec_receive_frame failed !\n");
				char errbuf[128];
				const char* errbuf_ptr = errbuf;
				av_strerror(ret, errbuf, sizeof(errbuf));
				fprintf(stderr, "avcodec_receive_frame failed %s\n", errbuf);
				continue;
			}
			ret = sws_scale(pSwsCtx, pFrame->data, pFrame->linesize, 0, pFrame->height, pFrameYuv->data, pFrameYuv->linesize);
			if (ret < 0) {
				printf("sws_scale implement failed\n");
				break;
			}

			SDL_RenderClear(render);
			SDL_UpdateYUVTexture(texture, nullptr,
				pFrameYuv->data[0], pFrameYuv->linesize[0],
				pFrameYuv->data[1], pFrameYuv->linesize[1],
				pFrameYuv->data[2], pFrameYuv->linesize[2]);
			SDL_RenderCopy(render, texture, nullptr, nullptr);
			SDL_RenderPresent(render);
		}
	}

	return 0;
}

