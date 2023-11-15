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

	FILE* pFile;		//�ļ�ָ��
	char szFilename[32];//�ļ������ַ�����
	int y;				//

	sprintf_s(szFilename, "rtspToPpm%04d.ppm", iFrame);	//�����ļ���
	//pFile = fopen(szFilename, "wb");			//���ļ���ֻд��
	fopen_s(&pFile, szFilename, "wb");
	if (pFile == NULL) {
		return;
	}

	//getch();

	fprintf(pFile, "P6\n%d %d\n255\n", width, height);//���ĵ��м��룬������룬��ȻPPM�ļ��޷���ȡ

	for (y = 0; y < height; y++) {
		fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
	}

	fclose(pFile);

}

/**
* ��AVFrame(YUV420��ʽ)����ΪJPEG��ʽ��ͼƬ
*
* @param width YUV420�Ŀ�
* @param height YUV42�ĸ�
*
*/
int saveFrame2JPEG(AVFrame* pFrame, int width, int height, int iIndex) {
	AVCodecContext* pCodeCtx = NULL;


	AVFormatContext* pFormatCtx = avformat_alloc_context();
	// ��������ļ���ʽ
	pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);

	// ��������ʼ�����AVIOContext
	if (avio_open(&pFormatCtx->pb, "test.jpg", AVIO_FLAG_READ_WRITE) < 0) {
		printf("Couldn't open output file.");
		return -1;
	}

	// ����һ����stream
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
	// ��AVPacket�����㹻��Ŀռ�
	AVPacket pkt;
	av_new_packet(&pkt, y_size * 3);

	// ��������
	ret = avcodec_send_frame(pCodeCtx, pFrame);
	if (ret < 0) {
		printf("Could not avcodec_send_frame.");
		return -1;
	}

	// �õ����������
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
 * ��ʼ�����װʹ��avformat_open_input��MP4�ļ�����������ʱ������TestDemux
 */
void Test::testReadMp4() {
	std::cout << "test01..." << std::endl;
	const char* filePath = "E:/clib/data/test.mp4";
	//��ʼ����װ��
	av_register_all();
	//��ʼ������� �����Դ�rtsp rtmp http Э�����ý����Ƶ��
	avformat_network_init();
	//��������
	AVDictionary* dictionaryOpts = NULL;
	//����rtsp����tcpЭ���
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//������ʱʱ��
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);

	//���װ������
	AVFormatContext* inAVFormatContext = NULL;
	int re = avformat_open_input(
		&inAVFormatContext,
		filePath,
		0,  // 0��ʾ�Զ�ѡ������
		&dictionaryOpts //�������ã�����rtsp����ʱʱ��
	);
	if (re != 0)
	{
		char buf[1024] = { 0 };
		av_strerror(re, buf, sizeof(buf) - 1);
		std::cout << "open " << filePath << " failed! :" << buf << std::endl;
		return;
	}
	std::cout << "open " << filePath << " success! " << std::endl;


	//��ȡ����Ϣ 
	re = avformat_find_stream_info(inAVFormatContext, 0);

	//��ʱ�� ����
	int totalMs = inAVFormatContext->duration / (AV_TIME_BASE / 1000);
	std::cout << "total time ms = " << totalMs << std::endl;
	//��ӡ��Ƶ����ϸ��Ϣ
	av_dump_format(inAVFormatContext, 0, filePath, 0);

	//����Ƶ��������ȡʱ��������Ƶ
	int videoStream = 0;
	int audioStream = 1;

	//��ȡ����Ƶ����Ϣ ��������������ȡ�� videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	for (int i = 0; i < inAVFormatContext->nb_streams; i++)
	{
		AVStream* stream = inAVFormatContext->streams[i];
		std::cout << "codec_id = " << stream->codecpar->codec_id << std::endl;
		std::cout << "format = " << stream->codecpar->format << std::endl;

		//��Ƶ AVMEDIA_TYPE_AUDIO
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStream = i;
			std::cout << i << "��Ƶ��Ϣ" << std::endl;
			std::cout << "sample_rate = " << stream->codecpar->sample_rate << std::endl;
			//AVSampleFormat;
			std::cout << "channels = " << stream->codecpar->channels << std::endl;
			//һ֡���ݣ��� ��ͨ�������� 
			std::cout << "frame_size = " << stream->codecpar->frame_size << std::endl;
			//1024 * 2 * 2 = 4096  fps = sample_rate/frame_size

		}
		//��Ƶ AVMEDIA_TYPE_VIDEO
		else if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStream = i;
			std::cout << i << "��Ƶ��Ϣ" << std::endl;
			std::cout << "width=" << stream->codecpar->width << std::endl;
			std::cout << "height=" << stream->codecpar->height << std::endl;
			//֡�� fps ����ת��
			std::cout << "video fps = " << r2d(stream->avg_frame_rate) << std::endl;
		}
	}

	//��ȡ��Ƶ��
	videoStream = av_find_best_stream(inAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	//malloc AVPacket����ʼ��
	AVPacket* pkt = av_packet_alloc();
	for (;;)
	{
		int re = av_read_frame(inAVFormatContext, pkt);
		if (re != 0)
		{
			//ѭ������
			std::cout << "==============================end==============================" << std::endl;
			int ms = 3000; //����λ�� ����ʱ�������������ת��
			long long pos = (double)ms / (double)1000 * r2d(inAVFormatContext->streams[pkt->stream_index]->time_base);
			av_seek_frame(inAVFormatContext, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
			continue;
		}
		std::cout << "pkt->size = " << pkt->size << std::endl;
		//��ʾ��ʱ��
		std::cout << "pkt->pts = " << pkt->pts << std::endl;
		//ת��Ϊ���룬������ͬ��
		std::cout << "pkt->pts ms = " << pkt->pts * (r2d(inAVFormatContext->streams[pkt->stream_index]->time_base) * 1000) << std::endl;
		//����ʱ��
		std::cout << "pkt->dts = " << pkt->dts << std::endl;
		if (pkt->stream_index == videoStream)
		{
			std::cout << "ͼ��" << std::endl;
		}
		if (pkt->stream_index == audioStream)
		{
			std::cout << "��Ƶ" << std::endl;
		}
		//�ͷţ����ü���-1 Ϊ0�ͷſռ�
		av_packet_unref(pkt);
		//XSleep(500);
	}

	av_packet_free(&pkt);

	if (inAVFormatContext)
	{
		//�ͷŷ�װ�����ģ����Ұ�inAVFormatContext��0
		avformat_close_input(&inAVFormatContext);
	}
}

int Test::testRtsp()
{
	std::cout << "testRtsp..." << std::endl;
	//��ʼ����װ��
	av_register_all();
	//��ʼ������� �����Դ�rtsp rtmp http Э�����ý����Ƶ��
	avformat_network_init();

	const char* url = "rtsp://10.52.8.106:554/stream2";
	//const char* url = "rtsp://172.26.144.239";

	AVFormatContext* pFormatCtx = avformat_alloc_context();
	//��������
	AVDictionary* dictionaryOpts = NULL;
	//����rtsp����tcpЭ���
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//������ʱʱ��
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
	//��ȡ��Ƶ���еı����������
	//AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;��ʱ
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
	//���ݱ�����������еı���id���Ҷ�Ӧ�Ľ���
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("%s", "�Ҳ���������\n");
		return -1;
	}
	//�򿪽�����
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("%s", "�������޷���\n");
		return -1;
	}
	//�����Ƶ��Ϣ
	printf("��Ƶ��pix_fmt��%d\n", pCodecCtx->pix_fmt);//AV_PIX_FMT_YUV420P
	printf("��Ƶ�Ŀ�ߣ�%d,%d\n", pCodecCtx->width, pCodecCtx->height);
	printf("��Ƶ�����������ƣ�%s\n", pCodec->name);


	AVPacket* packet = av_packet_alloc();// (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();

	AVFrame* pFrameRGB = av_frame_alloc();
	//������Ƶ֡��Ҫ�ڴ� (���ԭʼ������)
	int numBytes;		//��Ҫ���ڴ��С
	uint8_t* buffer = NULL;
	//��ȡ��Ҫ���ڴ��С
	/*
	1. av_image_fill_arrays ���������� frame �����Ǹղŷ�����ڴ�
	2. av_malloc ��һ�� FFmpeg �� malloc��
	��Ҫ�Ƕ� malloc ����һЩ��װ����֤��ַ����֮������飬
	���ᱣ֤��Ĵ��벻�����ڴ�й©������ͷŻ����� malloc ����
	*/
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);//��ȡ��Ҫ���ڴ��С
	buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));
	//����frame�͸ղŷ��������
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
				printf("%s", "�������");
			}
			//if (frameFinished) {
				//����Ƶ֡ԭ���ĸ�ʽpCodecCtx->pix_fmtת����RGB
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

	//�������뷽ʽ
	AVInputFormat* inputFormat = av_find_input_format("dshow");
	AVDictionary* format_opts = nullptr;
	av_dict_set_int(&format_opts, "rtbufsize", 3041280 * 100, 0);//���[video input] too full or near too full Ĭ�ϴ�С3041280
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
	//��ȡ��Ƶ���еı����������
	//AVCodecContext* pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;��ʱ
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(NULL);
	if (pCodecCtx == NULL)
	{
		printf("Could not allocate AVCodecContext\n");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[video_stream_index]->codecpar);
	//���ݱ�����������еı���id���Ҷ�Ӧ�Ľ���
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("%s", "�Ҳ���������\n");
		return -1;
	}
	//�򿪽�����
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("%s", "�������޷���\n");
		return -1;
	}
	//�����Ƶ��Ϣ
	printf("��Ƶ��pix_fmt��%d\n", pCodecCtx->pix_fmt);//AV_PIX_FMT_YUYV422
	printf("��Ƶ�Ŀ�ߣ�%d,%d\n", pCodecCtx->width, pCodecCtx->height);
	printf("��Ƶ�����������ƣ�%s\n", pCodec->name);

	AVPacket* packet = av_packet_alloc();// (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();

	//AVFrame* pFrameRGB = av_frame_alloc();
	//int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height, 1);//��ȡ��Ҫ���ڴ��С
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
	AVOutputFormat* ofmt = NULL;  // �����ʽ
	AVFormatContext* ifmt_ctx = NULL, * ofmt_ctx = NULL; // ���롢����������Ļ���
	AVPacket pkt;
	const char* in_filename = "E:/clib/data/test.mp4";
	const char* out_filename = "E:/clib/data/test-stream.flv";
	int ret, i;
	int stream_index = 0;
	int* stream_mapping = NULL; // �������ڴ������ļ�����Index
	int stream_mapping_size = 0; // �����ļ�������������

	//��ʼ����װ��
	av_register_all();
	//��ʼ������� �����Դ�rtsp rtmp http Э�����ý����Ƶ��
	avformat_network_init();
	//��������
	AVDictionary* dictionaryOpts = NULL;
	//����rtsp����tcpЭ���
	av_dict_set(&dictionaryOpts, "rtsp_transport", "tcp", 0);
	//������ʱʱ��
	av_dict_set(&dictionaryOpts, "max_delay", "500", 0);



	// �������ļ�Ϊifmt_ctx�����ڴ�
	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		fprintf(stderr, "Could not open input file '%s'", in_filename);
		goto end;
	}

	// ���������ļ�������Ϣ
	if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
		fprintf(stderr, "Failed to retrieve input stream information");
		goto end;
	}

	// ��ӡ�����ļ������Ϣ
	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	// Ϊ��������Ļ��������ڴ�
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) {
		fprintf(stderr, "Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	// �����ļ���������
	stream_mapping_size = ifmt_ctx->nb_streams;

	// ����stream_mapping_size���ڴ棬ÿ���ڴ��С��sizeof(*stream_mapping)
	stream_mapping = (int*)av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping));
	if (!stream_mapping) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	// ����ļ���ʽ
	ofmt = ofmt_ctx->oformat;

	// ���������ļ��е�ÿһ·��������ÿһ·����Ҫ����һ���µ����������
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		AVStream* out_stream; // �����
		AVStream* in_stream = ifmt_ctx->streams[i]; // ������
		AVCodecParameters* in_codecpar = in_stream->codecpar; // �������ı�������

		// ֻ������Ƶ����Ƶ����Ļ����������������Ҫ
		if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
			in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
			stream_mapping[i] = -1;
			continue;
		}

		// �������������index��д���
		stream_mapping[i] = stream_index++;

		// ����һ����Ӧ�������
		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			fprintf(stderr, "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		// ֱ�ӽ��������ı��������������������
		ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
		if (ret < 0) {
			fprintf(stderr, "Failed to copy codec parameters\n");
			goto end;
		}
		out_stream->codecpar->codec_tag = 0;
	}

	// ��ӡҪ����Ķ�ý���ļ�����ϸ��Ϣ
	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			fprintf(stderr, "Could not open output file '%s'", out_filename);
			goto end;
		}
	}

	// д���µĶ�ý���ļ���ͷ
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		fprintf(stderr, "Error occurred when opening output file\n");
		goto end;
	}

	while (1) {
		AVStream* in_stream, * out_stream;

		// ѭ����ȡÿһ֡����
		ret = av_read_frame(ifmt_ctx, &pkt);
		if (ret < 0) // ��ȡ����˳�ѭ��
			break;

		in_stream = ifmt_ctx->streams[pkt.stream_index];
		if (pkt.stream_index >= stream_mapping_size ||
			stream_mapping[pkt.stream_index] < 0) {
			av_packet_unref(&pkt);
			continue;
		}

		pkt.stream_index = stream_mapping[pkt.stream_index]; // �����������index��pkt���±��
		out_stream = ofmt_ctx->streams[pkt.stream_index]; // ����pkt��stream_index��ȡ��Ӧ�������

		// ��pts��dts��duration����ʱ���ת������ͬ��ʽʱ�������һ������ת���ᵼ������Ƶͬ������
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_PASS_MINMAX);//AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_PASS_MINMAX);//AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;

		// ������õ�pktд������ļ�
		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			break;
		}
		av_packet_unref(&pkt);
	}

	// д���µĶ�ý���ļ�β
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