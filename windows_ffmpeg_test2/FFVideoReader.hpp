#pragma once


extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/file.h>
}


struct  FrameInfor
{
    void*   _data;
    int     _dataSize;
    int     _width;
    int     _height;
    int64_t _pts;
    double  _timeBase;
    
};
class   FFVideoReader
{
public:
    AVFormatContext*_formatCtx;
    int             _videoIndex;
    AVCodecContext* _codecCtx;
    AVCodec*        _codec;
    AVFrame*        _frame;
    AVFrame*        _frameRGB;
    SwsContext*     _convertCtx;
public:
    int             _screenW;
    int             _screenH;

    int             _imageSize;
public:
    FFVideoReader()
    {
        _formatCtx  =   0;
        _videoIndex =   -1;
        _codecCtx   =   0;
        _codec      =   0;
        _frame      =   0;
        _frameRGB   =   0;
        _convertCtx =   0;
        _screenW    =   0;
        _screenH    =   0;
        
    }

    ~FFVideoReader()
    {
        sws_freeContext(_convertCtx);
        av_free(_frameRGB);
        av_free(_frame);
        avcodec_close(_codecCtx);
        avformat_close_input(&_formatCtx);
    }

    void    setup()
    {
        av_register_all();
        _formatCtx  =   avformat_alloc_context();
    }
    int     load(const char* filepath = "11.flv")
    {
        int     ret     =   0;

        //! ���ļ�
        if (avformat_open_input(&_formatCtx, filepath, NULL, NULL) != 0) 
        {
            return -1;
        }
        //! ����ļ����Ƿ����������
        if (avformat_find_stream_info(_formatCtx, NULL) < 0)
        {
            return -1;
        }
        //! ��ȡ��Ƶ������
        _videoIndex = -1;
        for (int i = 0; i < _formatCtx->nb_streams; i++)
        {
            if (_formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) 
            {
                _videoIndex = i;
                break;
            }
        }
        /**
        *   û����Ƶ�����򷵻�
        */
        if (_videoIndex == -1) 
        {
            return -1;
        }
        _codecCtx   =   _formatCtx->streams[_videoIndex]->codec;

        double dur  =   _formatCtx->duration/double(AV_TIME_BASE);
        _codec      =   avcodec_find_decoder(_codecCtx->codec_id);
        if (_codec == NULL)
        {
            return -1;
        }
        /**
        *   �򿪽�����
        */
        if (avcodec_open2(_codecCtx, _codec, NULL) < 0) 
        {
            return -1;
        }
        _frame      =   av_frame_alloc();
        _frameRGB   =   av_frame_alloc();

        _screenW    =   _codecCtx->width;
        _screenH    =   _codecCtx->height;

        _convertCtx =   sws_getContext(
                                    _codecCtx->width
                                    , _codecCtx->height
                                    , _codecCtx->pix_fmt
                                    , _codecCtx->width
                                    , _codecCtx->height
                                    , AV_PIX_FMT_RGB24
                                    , SWS_BICUBIC
                                    , NULL
                                    , NULL
                                    , NULL
                                    );

        int     numBytes    =   avpicture_get_size(AV_PIX_FMT_RGB24, _codecCtx->width,_codecCtx->height);
        uint8_t*buffer      =   (uint8_t *) av_malloc (numBytes * sizeof(uint8_t));
        avpicture_fill((AVPicture *)_frameRGB, buffer, AV_PIX_FMT_RGB24,_codecCtx->width, _codecCtx->height);
        _imageSize  =   numBytes;
        return  0;
    }

    bool    readFrame(FrameInfor& infor)
    {
        AVPacket packet;
        av_init_packet(&packet);
        for (;;) 
        {
            if (av_read_frame(_formatCtx, &packet)) 
            {
                av_free_packet(&packet);
                return false;
            }
            if (packet.stream_index != _videoIndex) 
            {
                continue;
            }
            int frame_finished = 0;

            int res = avcodec_decode_video2(_codecCtx, _frame, &frame_finished, &packet);

            if (frame_finished)
            {
                AVStream*   streams =   _formatCtx->streams[_videoIndex];
                double      tmbase  =   av_q2d(streams->time_base);
                int64_t     pts     =   _frame->pts;

                char        buf[128];
                sprintf(buf,"pts = %I64d     dts =  %I64d\n",packet.pts,packet.dts);
                int res = sws_scale(
                    _convertCtx
                    , (const uint8_t* const*)_frame->data
                    , _frame->linesize
                    , 0
                    , _codecCtx->height
                    , _frameRGB->data
                    , _frameRGB->linesize
                    );
                av_packet_unref(&packet);

                infor._data     =   _frameRGB->data[0];
                infor._dataSize =   _imageSize;
                infor._width    =   _screenW;
                infor._height   =   _screenH;
                infor._pts      =   _frame->pts;
                infor._timeBase =   av_q2d(streams->time_base);

                return  true;
            }
        }
        return  false;
    }
    void*   readFrame()
    {
        AVPacket packet;
        av_init_packet(&packet);
        for (;;) 
        {
            if (av_read_frame(_formatCtx, &packet)) 
            {
                av_free_packet(&packet);
                return 0;
            }
            if (packet.stream_index != _videoIndex) 
            {
                continue;
            }
            int frame_finished = 0;

            int res = avcodec_decode_video2(_codecCtx, _frame, &frame_finished, &packet);

            if (frame_finished)
            {
                AVStream*   streams =   _formatCtx->streams[_videoIndex];
                double      tmbase  =   av_q2d(streams->time_base);
                int64_t     pts     =   _frame->pts;

                char        buf[128];
                sprintf(buf,"pts = %I64d     dts =  %I64d\n",packet.pts,packet.dts);
                int res = sws_scale(
                    _convertCtx
                    , (const uint8_t* const*)_frame->data
                    , _frame->linesize
                    , 0
                    , _codecCtx->height
                    , _frameRGB->data
                    , _frameRGB->linesize
                    );
                av_packet_unref(&packet);

                return  _frameRGB->data[0];
            }
        }
        return  0;
    }
};

