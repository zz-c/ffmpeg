#include <iostream>
#include <thread>
#include "Test.h"
extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h" 
}

double r2d(AVRational r)
{
	return r.den == 0 ? 0 : (double)r.num / (double)r.den;
}
void xSleep(int ms)
{
	//c++ 11
	std::chrono::milliseconds du(ms);
	std::this_thread::sleep_for(du);
}

void saveFrame2Ppm(AVFrame* pFrame, int width, int height, int iFrame) {

	FILE* pFile;		//文件指针
	char szFilename[32];//文件名（字符串）
	int y;				//

	sprintf_s(szFilename, "rtspToPpm%04d.ppm", iFrame);	//生成文件名
	//pFile = fopen(szFilename, "wb");			//打开文件，只写入
	fopen_s(&pFile, szFilename, "wb");
	if (pFile == NULL) {
		return;
	}

	//getch();

	fprintf(pFile, "P6\n%d %d\n255\n", width, height);//在文档中加入，必须加入，不然PPM文件无法读取

	for (y = 0; y < height; y++) {
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
	}

	fclose(pFile);

}

/**
* 将AVFrame(YUV420格式)保存为JPEG格式的图片
*
* @param width YUV420的宽
* @param height YUV42的高
*
*/
int saveFrame2JPEG(AVFrame* pFrame, int width, int height, int iIndex) {
	AVCodecContext* pCodeCtx = NULL;


	AVFormatContext* pFormatCtx = avformat_alloc_context();
	// 设置输出文件格式
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

	// 创建并初始化输出AVIOContext
	if (avio_open(&pFormatCtx->pb, "test.jpg", AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}

	// 构建一个新stream
	AVStream* pAVStream = avformat_new_stream(pFormatCtx, 0);
	if (pAVStream == NULL) {
		return -1;
	}

	AVCodecParameters* parameters = pAVStream->codecpar;
	parameters->codec_id = pFormatCtx->oformat->video_codec;
	parameters->codec_type = AVMEDIA_TYPE_VIDEO;
	parameters->format = AV_PIX_FMT_YUVJ420P;
	parameters->width = pFrame->width;
	parameters->height = pFrame->height;

	AVCodec* pCodec = avcodec_find_encoder(pAVStream->codecpar->codec_id);

	if (!pCodec) {
		printf("Could not find encoder ");
		return -1;
	}

	pCodeCtx = avcodec_alloc_context3(pCodec);
	if (!pCodeCtx) {
		fprintf(stderr, "Could not allocate video codec context ");
		exit(1);
	}

	if ((avcodec_parameters_to_context(pCodeCtx, pAVStream->codecpar)) < 0) {
		fprintf(stderr, "Failed to copy %s codec parameters to decoder context ",
			av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
		return -1;
	}
	AVRational Rat;
	Rat.num = 1;
	Rat.den = 25;
	pCodeCtx->time_base = Rat;

	if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
		printf("Could not open codec.");
		return -1;
	}

	int ret = avformat_write_header(pFormatCtx, NULL);
	if (ret < 0) {
		printf("write_header fail ");
		return -1;
	}

	int y_size = width * height;

	//Encode
	// 给AVPacket分配足够大的空间
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	// 编码数据
	ret = avcodec_send_frame(pCodeCtx, pFrame);
	if (ret < 0) {
		printf("Could not avcodec_send_frame.");
		return -1;
	}

	// 得到编码后数据
	ret = avcodec_receive_packet(pCodeCtx, &pkt);
	if (ret < 0) {
		printf("Could not avcodec_receive_packet");
		return -1;
	}

	ret = av_write_frame(pFormatCtx, &pkt);

	if (ret < 0) {
		printf("Could not av_write_frame");
		return -1;
	}

	av_packet_unref(&pkt);

	//Write Trailer
	av_write_trailer(pFormatCtx);


	avcodec_close(pCodeCtx);
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	return 0;
}


/**
 * 初始化解封装使用avformat_open_input打开MP4文件，并设置延时等属性TestDemux
 */
void Test::testReadMp4() {
	std::cout << "test01..." << std::endl;
	const char* filePath = "E:/clib/data/test.mp4";
	//初始化封装库
	av_register_all();
	//初始化网络库 （可以打开rtsp rtmp http 协议的流媒体视频）
	avformat_network_init();
	//参数设置
	AVDictionary* dictionaryOpts = NULL;
	//设置rtsp流已tcp协议打开
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//网络延时时间
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);

	//解封装上下文
	AVFormatContext* inAVFormatContext = NULL;
	int re = avformat_open_input(
		&inAVFormatContext,
		filePath,
		0,  // 0表示自动选择解封器
		&dictionaryOpts //参数设置，比如rtsp的延时时间
	);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		std::cout << "open " << filePath << " failed! :" << buf << std::endl;
		return;
	}
	std::cout << "open " << filePath << " success! " << std::endl;


	//获取流信息 
	re = avformat_find_stream_info(inAVFormatContext, 0);

	//总时长 毫秒
	int totalMs = inAVFormatContext->duration / (AV_TIME_BASE / 1000);
	std::cout << "total time ms = " << totalMs << std::endl;
	//打印视频流详细信息
	av_dump_format(inAVFormatContext, 0, filePath, 0);

	//音视频索引，读取时区分音视频
	int videoStream = 0;
	int audioStream = 1;

	//获取音视频流信息 （遍历，函数获取） videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	for (int i = 0; i < inAVFormatContext->nb_streams; i++)
	{
		AVStream* stream = inAVFormatContext->streams[i];
		std::cout << "codec_id = " << stream->codecpar->codec_id << std::endl;
		std::cout << "format = " << stream->codecpar->format << std::endl;

		//音频 AVMEDIA_TYPE_AUDIO
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStream = i;
			std::cout << i << "音频信息" << std::endl;
			std::cout << "sample_rate = " << stream->codecpar->sample_rate << std::endl;
			//AVSampleFormat;
			std::cout << "channels = " << stream->codecpar->channels << std::endl;
			//一帧数据？？ 单通道样本数 
			std::cout << "frame_size = " << stream->codecpar->frame_size << std::endl;
			//1024 * 2 * 2 = 4096  fps = sample_rate/frame_size

		}
		//视频 AVMEDIA_TYPE_VIDEO
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			std::cout << i << "视频信息" << std::endl;
			std::cout << "width=" << stream->codecpar->width << std::endl;
			std::cout << "height=" << stream->codecpar->height << std::endl;
			//帧率 fps 分数转换
			std::cout << "video fps = " << r2d(stream->avg_frame_rate) << std::endl;
		}
	}

	//获取视频流
	videoStream = av_find_best_stream(inAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	//malloc AVPacket并初始化
	AVPacket* pkt = av_packet_alloc();
	for (;;)
	{
		int re = av_read_frame(inAVFormatContext, pkt);
		if (re != 0)
		{
			//循环播放
			std::cout << "==============================end==============================" << std::endl;
			int ms = 3000; //三秒位置 根据时间基数（分数）转换
			long long pos = (double)ms / (double)1000 * r2d(inAVFormatContext->streams[pkt->stream_index]->time_base);
			av_seek_frame(inAVFormatContext, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		std::cout << "pkt->size = " << pkt->size << std::endl;
		//显示的时间
		std::cout << "pkt->pts = " << pkt->pts << std::endl;
		//转换为毫秒，方便做同步
		std::cout << "pkt->pts ms = " << pkt->pts * (r2d(inAVFormatContext->streams[pkt->stream_index]->time_base) * 1000) << std::endl;
		//解码时间
		std::cout << "pkt->dts = " << pkt->dts << std::endl;
		if (pkt->stream_index == videoStream)
		{
			std::cout << "图像" << std::endl;
		}
		if (pkt->stream_index == audioStream)
		{
			std::cout << "音频" << std::endl;
		}
		//释放，引用计数-1 为0释放空间
		av_packet_unref(pkt);
		//XSleep(500);
	}

	av_packet_free(&pkt);

	if (inAVFormatContext)
	{
		//释放封装上下文，并且把inAVFormatContext置0
		avformat_close_input(&inAVFormatContext);
	}
}

int Test::testRtsp()
{
	std::cout << "testRtsp..." << std::endl;
	//初始化封装库
	av_register_all();
	//初始化网络库 （可以打开rtsp rtmp http 协议的流媒体视频）
	avformat_network_init();

	const char* url = "rtsp://10.52.8.106:554/stream2";
	//const char* url = "rtsp://172.26.144.239";

	AVFormatContext* pFormatCtx = avformat_alloc_context();
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
	//获取视频流中的编解码上下文
	//AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;过时
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
	//根据编解码上下文中的编码id查找对应的解码
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("%s", "找不到解码器\n");
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("%s", "解码器无法打开\n");
		return -1;
	}
	//输出视频信息
	printf("视频的pix_fmt：%d\n", pCodecCtx->pix_fmt);//AV_PIX_FMT_YUV420P
	printf("视频的宽高：%d,%d\n", pCodecCtx->width, pCodecCtx->height);
	printf("视频解码器的名称：%s\n", pCodec->name);


	AVPacket* packet = av_packet_alloc();// (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();

	AVFrame* pFrameRGB = av_frame_alloc();
	//分配视频帧需要内存 (存放原始数据用)
	int numBytes;		//需要的内存大小
	uint8_t* buffer = NULL;
	//获取需要的内存大小
	/*
	1. av_image_fill_arrays 函数来关联 frame 和我们刚才分配的内存
	2. av_malloc 是一个 FFmpeg 的 malloc，
	主要是对 malloc 做了一些封装来保证地址对齐之类的事情，
	它会保证你的代码不发生内存泄漏、多次释放或其他 malloc 问题
	*/
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);//获取需要的内存大小
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	//关联frame和刚才分配的内容
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

	SwsContext* sws_ctx = NULL;
	sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

	while (1) {
		ret = av_read_frame(pFormatCtx, packet);
		if (ret < 0) {
			fprintf(stderr, "error or end of file: %d\n", ret);
			continue;
		}
		if (packet->stream_index == audio_stream_index) {
			fprintf(stdout, "audio stream, packet size: %d\n", packet->size);
			continue;
		}
		if (packet->stream_index == video_stream_index) {
			fprintf(stdout, "video stream, packet size: %d\n", packet->size);

			//ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, packet);
			ret = avcodec_send_packet(pCodecCtx, packet);
			av_packet_unref(packet);
			if (ret < 0) {
				printf("%s", "解码完成");
			}
			//if (frameFinished) {
				//将视频帧原来的格式pCodecCtx->pix_fmt转换成RGB
				//saveFrame2JPEG(pFrame, pCodecCtx->width, pCodecCtx->height, cnt);
				//break;
			//}
			ret = avcodec_receive_frame(pCodecCtx, pFrame);
			if (ret != 0) {
				fprintf(stderr, "avcodec_receive_frame failed !\n");
				char errbuf[128];
				const char* errbuf_ptr = errbuf;
				av_strerror(ret, errbuf, sizeof(errbuf));
				fprintf(stderr, "avcodec_receive_frame failed %s\n", errbuf);
			}
			else {
				saveFrame2JPEG(pFrame, pCodecCtx->width, pCodecCtx->height, 0);

				sws_scale(sws_ctx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				saveFrame2Ppm(pFrameRGB, pCodecCtx->width, pCodecCtx->height, 0);

				break;
			}
			//break;
		}


	}

	avformat_free_context(pFormatCtx);
}

int Test::testCamera()
{
	std::cout << "testCamera..." << std::endl;
	av_register_all();
	avdevice_register_all();

	//查找输入方式
	AVInputFormat* inputFormat = av_find_input_format("dshow");
	AVDictionary* format_opts = nullptr;
	av_dict_set_int(&format_opts, "rtbufsize", 3041280 * 100, 0);//解决[video input] too full or near too full 默认大小3041280
	//av_dict_set(&format_opts, "avioflags", "direct", 0);
	//av_dict_set(&format_opts, "video_size", "1280x720", 0);
	//av_dict_set(&format_opts, "framerate", "30", 0);
	//av_dict_set(&format_opts, "vcodec", "mjpeg", 0);

	AVFormatContext* pFormatCtx = avformat_alloc_context();
	const char* psDevName = "video=USB Camera";

	int ret = avformat_open_input(&pFormatCtx, psDevName, inputFormat, &format_opts);
	if (ret < 0)
	{
		std::cout << "AVFormat Open Input Error!" << std::endl;
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
	fprintf(stdout, "Number of elements in AVFormatContext.streams: %d\n", pFormatCtx->nb_streams);
	for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
		const AVStream* stream = pFormatCtx->streams[i];
		fprintf(stdout, "type of the encoded data: %d\n", stream->codecpar->codec_id);
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_stream_index = i;
			fprintf(stdout, "dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d\n",
				stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
		}
	}
	//获取视频流中的编解码上下文
	//AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;过时
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
	//根据编解码上下文中的编码id查找对应的解码
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("%s", "找不到解码器\n");
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("%s", "解码器无法打开\n");
		return -1;
	}
	//输出视频信息
	printf("视频的pix_fmt：%d\n", pCodecCtx->pix_fmt);//AV_PIX_FMT_YUYV422
	printf("视频的宽高：%d,%d\n", pCodecCtx->width, pCodecCtx->height);
	printf("视频解码器的名称：%s\n", pCodec->name);

	AVPacket* packet = av_packet_alloc();// (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();

	//AVFrame* pFrameRGB = av_frame_alloc();
	//int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height, 1);//获取需要的内存大小
	//uint8_t* buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	//av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

	//AVFrame* swsframe = av_frame_alloc();
	//av_image_alloc(swsframe->data,swsframe->linesize,pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_YUV420P,1);

	//SwsContext* sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	while (1) {
		ret = av_read_frame(pFormatCtx, packet);
		if (ret < 0) {
			fprintf(stderr, "error or end of file: %d\n", ret);
			continue;
		}
		if (packet->stream_index == video_stream_index) {
			fprintf(stdout, "video stream, packet size: %d\n", packet->size);

			//ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, packet);
			ret = avcodec_send_packet(pCodecCtx, packet);
			av_packet_unref(packet);


			ret = avcodec_receive_frame(pCodecCtx, pFrame);
			if (ret != 0) {
				fprintf(stderr, "avcodec_receive_frame failed !\n");
			}
			else {
				fprintf(stderr, "avcodec_receive_frame success !\n");
				//sws_scale(sws_ctx, (uint8_t const* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, swsframe->data, swsframe->linesize);
				//saveFrame2JPEG(swsframe, pCodecCtx->width, pCodecCtx->height, 10);
				break;
			}
			//break;
		}
	}
	//avformat_free_context(pFormatCtx);
}

int Test::testMp4ToFlv() {
	AVOutputFormat* ofmt = NULL;  // 输出格式
	AVFormatContext* ifmt_ctx = NULL, * ofmt_ctx = NULL; // 输入、输出是上下文环境
	AVPacket pkt;
	const char* in_filename = "E:/clib/data/test.mp4";
	const char* out_filename = "E:/clib/data/test-stream.flv";
	int ret, i;
	int stream_index = 0;
	int* stream_mapping = NULL; // 数组用于存放输出文件流的Index
	int stream_mapping_size = 0; // 输入文件中流的总数量

	//初始化封装库
	av_register_all();
	//初始化网络库 （可以打开rtsp rtmp http 协议的流媒体视频）
	avformat_network_init();
	//参数设置
	AVDictionary* dictionaryOpts = NULL;
	//设置rtsp流已tcp协议打开
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//网络延时时间
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);



	// 打开输入文件为ifmt_ctx分配内存
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		fprintf(stderr, "Could not open input file '%s'", in_filename);
		goto end;
	}

	// 检索输入文件的流信息
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		fprintf(stderr, "Failed to retrieve input stream information");
		goto end;
	}

	// 打印输入文件相关信息
	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	// 为输出上下文环境分配内存
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) {
		fprintf(stderr, "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	// 输入文件流的数量
	stream_mapping_size = ifmt_ctx->nb_streams;

	// 分配stream_mapping_size段内存，每段内存大小是sizeof(*stream_mapping)
	stream_mapping = (int*)av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
	if (!stream_mapping) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	// 输出文件格式
	ofmt = ofmt_ctx->oformat;

	// 遍历输入文件中的每一路流，对于每一路流都要创建一个新的流进行输出
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		AVStream* out_stream; // 输出流
		AVStream* in_stream = ifmt_ctx->streams[i]; // 输入流
		AVCodecParameters* in_codecpar = in_stream->codecpar; // 输入流的编解码参数

		// 只保留音频、视频、字幕流，其他的流不需要
		if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
			stream_mapping[i] = -1;
			continue;
		}

		// 对于输出的流的index重写编号
		stream_mapping[i] = stream_index++;

		// 创建一个对应的输出流
		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			fprintf(stderr, "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		// 直接将输入流的编解码参数拷贝到输出流中
		ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
		if (ret < 0) {
			fprintf(stderr, "Failed to copy codec parameters\n");
			goto end;
		}
		out_stream->codecpar->codec_tag = 0;
	}

	// 打印要输出的多媒体文件的详细信息
	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", out_filename);
			goto end;
		}
	}

	// 写入新的多媒体文件的头
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		goto end;
	}

	while (1) {
		AVStream* in_stream, * out_stream;

		// 循环读取每一帧数据
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0) // 读取完后退出循环
			break;

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		if (pkt.stream_index >= stream_mapping_size ||
			stream_mapping[pkt.stream_index] < 0) {
			av_packet_unref(&pkt);
			continue;
		}

		pkt.stream_index = stream_mapping[pkt.stream_index]; // 按照输出流的index给pkt重新编号
		out_stream = ofmt_ctx->streams[pkt.stream_index]; // 根据pkt的stream_index获取对应的输出流

		// 对pts、dts、duration进行时间基转换，不同格式时间基都不一样，不转换会导致音视频同步问题
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_PASS_MINMAX);//AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_PASS_MINMAX);//AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;

		// 将处理好的pkt写入输出文件
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			break;
		}
		av_packet_unref(&pkt);
	}

	// 写入新的多媒体文件尾
	av_write_trailer(ofmt_ctx);
end:

	avformat_close_input(&ifmt_ctx);

	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_closep(&ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);

	av_freep(&stream_mapping);

	if (ret < 0 && ret != AVERROR_EOF) {
		fprintf(stderr, "Error occurred\n");
		return 1;
	}
	std::cout << "flv finish" << std::endl;
	return 0;
}