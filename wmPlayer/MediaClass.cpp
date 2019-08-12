#include "stdafx.h"
#include "common.h"
#include "UtilTool.h"
#include "MediaClass.h"
#include "SDLClass.h"
#include <windows.h>

SynchronizeStatus::SynchronizeStatus():current_pts(0),duration_pts(0),pre_next_pts(0)
{

}
SynchronizeStatus::~SynchronizeStatus()
{

}

void SynchronizeStatus::init()
{
	this->current_pts = 0;
	this->duration_pts = 0;
	this->pre_next_pts = 0;
}

void SynchronizeStatus::set_current_pts(int64_t current_pts, int64_t duration_pts)
{
	std::lock_guard<std::mutex> lgd(m_mt);
	this->current_pts = current_pts;
	this->duration_pts = duration_pts;
	this->pre_next_pts = current_pts + duration_pts;
}

int64_t SynchronizeStatus::get_current_pts()
{
	std::lock_guard<std::mutex> lgd(m_mt);
	return this->current_pts;
}

int64_t SynchronizeStatus::get_duration_pts()
{
	std::lock_guard<std::mutex> lgd(m_mt);
	return this->duration_pts;
}

int64_t SynchronizeStatus::get_pre_next_pts()
{
	std::lock_guard<std::mutex> lgd(m_mt);
	return this->pre_next_pts;
}

std::mutex SynchronizeStatus::m_mt;

MediaClass::MediaClass(SDLClass *instance):
	videoThreadStatus(FALSE),audioThreadStatus(FALSE), pFormatCtx(NULL),
	media_status(MEDIA_STATUS::INIT),progress_duration_time(100000),
	progress_current_time(0),progress_current_pts(0), audio_multiple(1),
	progress_current_frame_v(0),filesize(100),fps(25),dot_start_frame(0),
	dot_end_frame(0), dot_start_time(0),dot_end_time(0), speedNum(1),seek_status(SEEK_STATUS::NORMAL),
	seek_sec(0),audioSwitch(TRUE), play_mode(PLAY_MODE::ASC), play_mode_seek_flag(FALSE),
	playerStatus(PLAY_STATUS::PLAY),playFrameNum(0), loop_lock(TRUE),
	iformat_name(NULL), delay_pts(0), left_audio(0), right_audio(0)
{
	this->sdl_instance = instance;
}


MediaClass::~MediaClass()
{
}

//Logger& MediaClass::logger = Logger::get("MediaClass");
Logger& MediaClass::logger = Logger::get(Logger::ROOT);
//std::mutex MediaClass::m_mt;
//std::mutex MediaClass::audioQueue_mt;
//std::mutex MediaClass::videoQueue_mt;

BOOL MediaClass::initGlobalVar()
{
	logger.information("initGlobalVar()");

	pFormatCtx = NULL;
	pCodecCtx = NULL;
	audioCodecCtx = NULL;
	pCodec = NULL;
	audioCodec = NULL;
	pFrame = NULL;
	audioFrame = NULL;
	packet = NULL;
	au_convert_ctx = NULL;
	img_convert_ctx = NULL;
	v_start_pts = 0;
	a_start_pts = 0;
	start_pts = 0;
	out_buffer_size = 0;

	progress_duration_time = 100000;
	progress_current_time = 0;
	progress_current_pts = 0;
	progress_current_frame_v = 0;
	filesize = 100;
	fps = 25;
	dot_start_frame = 0;
	dot_end_frame = 0;
	dot_start_time = 0;
	dot_end_time = 0;
	speedNum = 1;
	playFrameNum = 0;
	delay_pts = 0;
	left_audio = 0;
	right_audio = 0;
	audio_multiple = 1;
	media_status = MEDIA_STATUS::RUNNING;
	videoThreadStatus = FALSE;
	audioThreadStatus = FALSE;
	//fileEnd = FALSE;
	loop_lock = TRUE;

	return TRUE;
}

BOOL MediaClass::initVideo()
{
	const AVRational avr = { 1, AV_TIME_BASE };

	int screen_w = 800, screen_h = 500;

	AVStream *stream = pFormatCtx->streams[videoindex];
	v_start_pts = stream->start_time;
	v_start_pts = av_rescale_q(v_start_pts, stream->time_base, avr);

	//获取帧率
	fps = stream->avg_frame_rate.num / stream->avg_frame_rate.den;
	logger.information("fps-->%d", fps);

	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		logger.error("Codec not found.");
		return FALSE;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
		logger.error("Could not open codec.");
		return FALSE;
	}

	pFrame = av_frame_alloc();

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	screen_w = 800;
	screen_h = (screen_w - 200 - 50) * pCodecCtx->height / pCodecCtx->width + 60 + 40 + 30;

	//重设主窗口大小
	SetWindowPos(mainWnd, HWND_TOP, 0, 0, screen_w, screen_h, SWP_NOMOVE);
	//IYUV: Y + U + V  (3 planes)
	//YV12: Y + V + U  (3 planes)
	sdl_instance->CreateTexture(pCodecCtx->width, pCodecCtx->height);
	return TRUE;
}

BOOL MediaClass::initAudio()
{
	const AVRational avr = { 1, AV_TIME_BASE };

	SDL_AudioSpec wanted_spec;
	int64_t in_channel_layout;
	AVStream *stream = pFormatCtx->streams[audioindex];
	//a_start_pts = pFormatCtx->start_time;
	a_start_pts = stream->start_time;
	a_start_pts = av_rescale_q(a_start_pts, stream->time_base, avr);

	audioFrame = av_frame_alloc();
	// Get a pointer to the codec context for the audio stream
	audioCodecCtx = stream->codec;

	// Find the decoder for the audio stream
	audioCodec = avcodec_find_decoder(audioCodecCtx->codec_id);
	if (audioCodec == NULL) {
		logger.error("Codec not found.\n");
		return FALSE;
	}

	// Open codec
	if (avcodec_open2(audioCodecCtx, audioCodec, NULL)<0) {
		logger.error("Could not open codec.\n");
		return FALSE;
	}

	//音频重采样相关初始化
	//Out Audio Param
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	//nb_samples: AAC-1024 MP3-1152
	int out_nb_samples = audioCodecCtx->frame_size;
	AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
	//AVSampleFormat out_sample_fmt = audioCodecCtx->sample_fmt;
	int out_sample_rate = audioCodecCtx->sample_rate;
	int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	//Out Buffer Size
	out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

	wanted_spec.freq = out_sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = out_channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = out_nb_samples;
	wanted_spec.callback = fill_audio;
	wanted_spec.userdata = NULL;
	//wanted_spec.userdata = audioCodecCtx;

	//解决SDL_OpenAudio打开失败的问题：WASAPI can’t initialize audio client: No se ha llamado a CoInitialize
	CoInitialize(NULL);
	if (SDL_OpenAudio(&wanted_spec, NULL)<0) {
		logger.error("can't open audio!-->%s", std::string(SDL_GetError()));

		audioSwitch = FALSE;
		//return FALSE;
	}


	//FIX:Some Codec's Context Information is missing
	in_channel_layout = av_get_default_channel_layout(audioCodecCtx->channels);
	//Swr
	au_convert_ctx = swr_alloc();
	au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
		in_channel_layout, audioCodecCtx->sample_fmt, audioCodecCtx->sample_rate, 0, NULL);
	swr_init(au_convert_ctx);
	return TRUE;
}

PLAY_MODE MediaClass::getPlayMode()
{
	return this->play_mode;
}

MEDIA_STATUS MediaClass::getMediaStatus()
{
	return this->media_status;
}

//void MediaClass::setPlayMode(PLAY_MODE mode)
//{
//	loop_lock = FALSE;
//	if (this->play_mode == PLAY_MODE::DESC)
//	{
//		this->video_buffer.clear_left();
//		this->video_buffer.clear_right();
//		this->audio_buffer.clear_right();
//		this->audio_buffer.clear_left();
//		play_mode_seek_flag = TRUE;
//	}
//	else
//	{
//		this->video_buffer.clear_left();
//		this->video_buffer.clear_right();
//		this->audio_buffer.clear_right();
//		this->audio_buffer.clear_left();
//		play_mode_seek_flag = TRUE;
//	}
//	this->play_mode = mode;
//	loop_lock = TRUE;
//}

BOOL MediaClass::stopMedia()
{
	logger.information("stopMedia()");

	if (media_status == MEDIA_STATUS::INIT)
	{
		return TRUE;
	}

	media_status = MEDIA_STATUS::STOPPING;
	while (media_status == MEDIA_STATUS::STOPPING) {
		Sleep(100);
	}
	return TRUE;
}

BOOL MediaClass::reOpenMedia_Thread(char filePath[])
{
	logger.information("reOpenMedia_Thread()");

	if (stopMedia())
	{
		logger.information("media_hThread ThreadProc->" + std::string(filePath));
		openMedia_Thread(filePath);
		logger.information("media_hThread is ok");
	}

	return TRUE;
}

/*
* sec 单位：ms
*/
BOOL MediaClass::seek(int64_t sec)
{
	logger.information("seek()");
	loop_lock = FALSE;
	seek_status = SEEK_STATUS::SEEKING;
	seek_sec = sec * AV_TIME_BASE / 1000;
	loop_lock = TRUE;
	return TRUE;
}

BOOL MediaClass::reset()
{
	logger.information("reset()");
	//loop_lock = FALSE;
	int ret = av_seek_frame(pFormatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
	if (ret<0) {
		logger.warning("reset::av_seek_frame ==video== error");
	}

	//清空视频解码器缓存
	avcodec_flush_buffers(pFormatCtx->streams[videoindex]->codec);
	//fileEnd = FALSE;
	//loop_lock = TRUE;
	return TRUE;
}


/*
对ffmpeg的seek方法包装，实现特性如下：
1.对ts文件和非ts的区别处理；
2.以current_timestamp为seek基准找到之前的最近i帧；
*/
BOOL MediaClass::cipplayer_seek(AVFormatContext *s, int video_index, int64_t current_timestamp, AVPacket *packet)
{
	const int64_t BACK_TIME = 1 * AV_TIME_BASE;
	const AVRational avr = { 1, AV_TIME_BASE };

	int64_t offset_timestamp = 0;
	char * iformat_name = (char *)s->iformat->name;
	int64_t filesize = s->pb ? avio_size(s->pb) : 0;
	int64_t duration = s->duration;
	int64_t start_time = s->start_time;
	int64_t seek_pts = 0;
	while (TRUE)
	{
		seek_pts = current_timestamp - offset_timestamp;
		if (!strcmp(iformat_name, "mpegts"))
		{
			int64_t seek_sec = filesize * ((double)seek_pts / duration);
			int ret = av_seek_frame(s, -1, seek_sec, AVSEEK_FLAG_BYTE);
		}
		else
		{
			int64_t seek_sec = seek_pts;
			int ret = av_seek_frame(s, -1, seek_sec, AVSEEK_FLAG_BACKWARD);
		}

		//int i = 0;
		while (TRUE)
		{
			if (av_read_frame(s, packet) >= 0)
			{
				if (packet->stream_index == video_index)
				{
					int64_t pts = av_rescale_q(packet->pts, s->streams[packet->stream_index]->time_base, avr);
					pts = pts - start_time;
					if (seek_pts > 0 && pts >= current_timestamp)
					{
						offset_timestamp += BACK_TIME;
						break;
					}
					else if (packet->flags & AV_PKT_FLAG_KEY)
					{
						return TRUE;
					}
				}
			}
			else
			{
				return FALSE;
			}
		}
	}
}

/*
读取当前package，在firstFrame时直接越过i帧
*/
BOOL MediaClass::cipplayer_read_frame(AVFormatContext *s, int video_index, AVPacket *pkt, bool firstFrame)
{
	while (TRUE)
	{
		if (av_read_frame(pFormatCtx, pkt) < 0)
		{
			return FALSE;
		}
		//处理起始的非视频i帧
		if (firstFrame)
		{
			if (pkt->stream_index == video_index && pkt->flags & AV_PKT_FLAG_KEY)
			{
				return TRUE;
			}
			else
			{
				continue;
			}
		}
		else
		{
			return TRUE;
		}
	}
	
}

PLAY_STATUS MediaClass::getPlayStatus()
{
	return this->playerStatus;
}

//BOOL MediaClass::setPlayStatus(PLAY_STATUS status)
//{
//	this->playerStatus = status;
//
//	return true;
//}

BOOL MediaClass::play(PLAY_STATUS status,PLAY_MODE mode, float speedNum, int num)
{
	loop_lock = FALSE;
	//如果播放状态，模式和speedNum和当前状态都一致直接返回
	if (this->playerStatus == status && status == PLAY_STATUS ::PLAY && 
		this->play_mode == mode && this->speedNum == speedNum)
	{
		loop_lock = TRUE;
		return TRUE;
	}
	//如果status为PAUSE，播放暂停，其他参数无效
	if (status == PLAY_STATUS::PAUSE)
	{
		this->playerStatus = PLAY_STATUS::PAUSE;
		loop_lock = TRUE;
		return TRUE;
	}

	//清空缓存处理
	if (this->play_mode != mode)
	{
		this->video_buffer.clear_left();
		this->video_buffer.clear_right();
		this->audio_buffer.clear_right();
		this->audio_buffer.clear_left();
		play_mode_seek_flag = TRUE;
		
	}
	else if(status == PLAY_STATUS::PLAY && this->speedNum != 1 && speedNum == 1)
	{
		audio_buffer.clear_right();
		audio_buffer.clear_left();
	}
	
	//if (this->fileEnd)
	//{
	//	reset();
	//	fileEnd = FALSE;
	//	//this->callbackFun(1);
	//}


	this->playerStatus = status;
	this->play_mode = mode;

	if (status == PLAY_STATUS::PLAY)
	{
		this->speedNum = speedNum;
		this->playFrameNum = 0;
	}

	if (status == PLAY_STATUS::FRAME)
	{
		this->playFrameNum = this->playFrameNum + num;
		this->speedNum = 1;
	}

	loop_lock = TRUE;
	return TRUE;
}

//BOOL MediaClass::play_frame(int num)
//{
//	loop_lock = FALSE;
//	this->playFrameNum = this->playFrameNum + num;
//	this->playerStatus = PLAY_STATUS::FRAME;
//	loop_lock = TRUE;
//	return TRUE;
//}

//BOOL MediaClass::pause()
//{
//	this->playerStatus = PLAY_STATUS::PAUSE;
//	return TRUE;
//}

//BOOL MediaClass::play_speeddown()
//{
//	if (speedNum > 0.25)
//	{
//		if (speedNum / 2 == 1)
//		{
//			//clearAudioStream();
//			audio_buffer.clear_right();
//			audio_buffer.clear_left();
//		}
//		speedNum = speedNum / 2;
//	}
//	return TRUE;
//}
//
//BOOL MediaClass::play_speed()
//{
//	if (speedNum < 4)
//	{
//		if (speedNum * 2 == 1)
//		{
//			//clearAudioStream();
//			audio_buffer.clear_right();
//			audio_buffer.clear_left();
//		}
//		speedNum = speedNum * 2;
//	}
//	return TRUE;
//}

float MediaClass::getPlaySpeed()
{
	return speedNum;
}

//void MediaClass::clearAudioStream()
//{
//	audio_buffer.clear_right();
//	audio_buffer.clear_left();
//}

BOOL MediaClass::allowReadStream()
{
	logger.debug("allowReadStream():audioQueue-->%u,videoQueue-->%u", audio_buffer.get_right_count(), video_buffer.get_right_count());

	if (this->play_mode == PLAY_MODE::ASC)
	{
		if ((playerStatus == PLAY_STATUS::PLAY && speedNum == 1) ||
			playerStatus == PLAY_STATUS::PAUSE)
		{
			if (video_buffer.get_right_count() > 25 &&
				audioindex != -1 && audioSwitch && audio_buffer.get_right_count() > 25) {
				return FALSE;
			}
			else if (video_buffer.get_right_count() > 25 &&
				(audioindex == -1 || !audioSwitch))
			{
				return FALSE;
			}
			else
			{
				return TRUE;
			}
		}
		else if ((playerStatus == PLAY_STATUS::PLAY && speedNum != 1) ||
			(playerStatus == PLAY_STATUS::FRAME))
		{
			if (video_buffer.get_right_count() > 25) {
				return FALSE;
			}
			return TRUE;
		}
	}
	else if (this->play_mode == PLAY_MODE::DESC)
	{
		if (video_buffer.get_left_count() > 300) {
			return FALSE;
		}
		return TRUE;
	}
	
	return FALSE;
}

BOOL MediaClass::openMedia_Thread(char filePath[])
{
	logger.information("openMedia_Thread()");

	//thread
	DWORD threadID;
	HANDLE media_hThread;

	MediaThreadParam *mtp = new MediaThreadParam;
	mtp->filePath = filePath;
	mtp->m_instance = this;
	media_hThread = CreateThread(NULL, 0, openMedia_ThreadProc, mtp, 0, &threadID);	// 创建线程
	
	return TRUE;
}

BOOL MediaClass::openMedia(char filePath[])
{
	logger.information("openMedia()");

	int				i;
	int64_t startpts = 0;//点击播放按钮后第一包的瞬时帧的pts
	int64_t start_system_time = 0;////点击播放按钮后第一包瞬时时间戳(毫秒)
	int ret, got_picture;
	const AVRational avr = { 1, AV_TIME_BASE };


	//char filepath[] = "屌丝男士.mov";
	//char filepath[] = "Titanic.ts";
	//char filepath[] = "春晚是什么？.mov";
	//char filepath[] = "cctv1.ts";

	initGlobalVar();
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	if (avformat_open_input(&pFormatCtx, filePath, NULL, NULL) != 0) {
		logger.error("Couldn't open input stream.");
		return FALSE;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
		logger.error("Couldn't find stream information.");
		return FALSE;
	}
	packet = av_packet_alloc();

	//获取视音频所在index
	videoindex = -1;
	audioindex = -1;
	logger.information("pFormatCtx->nb_streams-->%d", pFormatCtx->nb_streams);
	for (i = 0; i<pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && videoindex  == -1) {
			videoindex = i;
		}
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && audioindex == -1) {
			audioindex = i;
		}
	}
	if (videoindex == -1) {
		logger.error("Didn't find a video stream.");
		return FALSE;
	}
	if (audioindex == -1) {
		logger.error("Didn't find a audio stream.");
		//return -1;
	}

	//计算文件总时长，单位:ms
	progress_duration_time = pFormatCtx->duration * 1000 / AV_TIME_BASE;
	logger.information("progress_duration_time-->%d", progress_duration_time);
	//计算起始时间的pts
	start_pts = pFormatCtx->start_time;

	//视频初始化
	if (videoindex != -1)
	{
		initVideo();
	}
	else
	{
		logger.error("the file is not video!");
		return FALSE;
	}


	sdl_instance->startLoadingImageSDL_Thread();

	//初始化音频
	if (audioindex != -1 && audioSwitch)
	{
		initAudio();
		//计算音视频时延差
		delay_pts = abs(a_start_pts - v_start_pts);
	}

	//得到文件大小
	filesize = pFormatCtx->pb ? avio_size(pFormatCtx->pb) : 0;
	iformat_name = (char *)pFormatCtx->iformat->name;

	//开启视频播放线程
	videoPlay_Thread();
	if (audioindex != -1 && audioSwitch)
	{
		//开启SDL音频播放指令
		SDL_PauseAudio(0);
		//开启音频播放线程
		audioPlay_Thread();
		//audioRange_Thread();
	}


	//清空队列
	//clearAudioStream();
	audio_buffer.clear_right();
	audio_buffer.clear_left();
	//clearVideoStream();
	video_buffer.clear_right();
	video_buffer.clear_left();

	//初始化SynchronizeStatus
	this->ss.init();

	sdl_instance->endLoadingImageSDL();
	//开始播放,前seek到第一I帧
	reset();
	//BOOL firstFrame = TRUE;
	//在asc模式下为true表示可以将帧数送到buffer，false时忽略。
	//原因在于从DESC模式切换到ASC模式后，当前帧可能不是i帧，这时就要求从seek到的i帧到当前帧这段时区数据需要忽略掉。
	BOOL asc_package_valid = TRUE;
	BOOL desc_seek_flag = FALSE;
	while (media_status == MEDIA_STATUS::RUNNING) 
	{
		if (loop_lock && (allowReadStream() || seek_status == SEEK_STATUS::SEEKING)) {
			BOOL no_file_end_flag = TRUE;

			//在倒放模式下，如果play_mode_seek_flag标志位为true则进行seek操作
			//播放模式切换和buffer不足的情况下都会向前seek
			if (this->play_mode == PLAY_MODE::DESC && (play_mode_seek_flag || desc_seek_flag))
			{
				Ref_VideoStreamFrame *ref_vsf = this->video_buffer.get_left();
				int64_t current_pts = this->progress_current_pts;
				if (ref_vsf != NULL)
				{
					current_pts = this->video_buffer.get_left()->vsf->current_pts;
				}
				//seek_status == SEEK_STATUS::SEEKING;
				seek_sec = current_pts - 2 * AV_TIME_BASE;
				avcodec_flush_buffers(pFormatCtx->streams[videoindex]->codec);
				if (!cipplayer_seek(pFormatCtx, videoindex, seek_sec, packet))
				{
					no_file_end_flag = FALSE;
				}
				play_mode_seek_flag = FALSE;
				desc_seek_flag = FALSE;
			}
			//在正常播放模式下，如果play_mode_seek_flag标志位为true则进行seek操作，这种情况发生在播放模式切换时
			else if (this->play_mode == PLAY_MODE::ASC && play_mode_seek_flag)
			{
				seek_sec = this->progress_current_pts;
				avcodec_flush_buffers(pFormatCtx->streams[videoindex]->codec);
				if (!cipplayer_seek(pFormatCtx, videoindex, seek_sec, packet))
				{
					no_file_end_flag = FALSE;
				}
				play_mode_seek_flag = FALSE;
				asc_package_valid = FALSE;
			}
			else if (seek_status == SEEK_STATUS::SEEKING) {
				audio_buffer.clear_right();
				audio_buffer.clear_left();
				video_buffer.clear_right();
				video_buffer.clear_left();
				avcodec_flush_buffers(pFormatCtx->streams[videoindex]->codec);

				seek_status = SEEK_STATUS::NORMAL;
				if (!cipplayer_seek(pFormatCtx, videoindex, seek_sec, packet))
				{
					no_file_end_flag = FALSE;
				}
			}
			else
			{
				if (av_read_frame(pFormatCtx, packet) < 0)
				{
					no_file_end_flag = FALSE;
				}
			}

			if (no_file_end_flag)
			{
				int64_t pts = av_rescale_q(checkFilterPTS(packet->pts), pFormatCtx->streams[packet->stream_index]->time_base, avr);
				if (packet->stream_index == videoindex) {
					int64_t current_pts = pts - start_pts; 
					int64_t current_time = current_pts * 1000 / AV_TIME_BASE;
					int64_t current_frame_v = 0;
					current_frame_v = fps * current_time / 1000;

					//开始解码
					logger.debug("avcodec_decode_video2:current_pts->%s,current_frame_v->%d",std::to_string(current_pts), current_frame_v);
					ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
					//logger.debug("SDL loop%d second timest->%lld", progress_current_frame, UtilTool::getInstance()->getSystemTime());
					if (ret < 0) {
						logger.error("Decode Error.\n");
						/*return false;*/
						continue;
					}
					if (got_picture) {
						AVFrame *pFrameYUV = av_frame_alloc();
						uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));

						avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
						sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

						VideoStreamFrame *vsf = new VideoStreamFrame(pFrameYUV, out_buffer, current_pts, current_frame_v);
						//pushVideoStream(vsf);
						Ref_VideoStreamFrame *ref_vsf = new Ref_VideoStreamFrame(vsf);
						if (play_mode == PLAY_MODE::ASC)
						{
							if (!asc_package_valid)
							{
								if (current_pts >= this->progress_current_pts)
								{
									asc_package_valid = TRUE;
								}
								
								delete ref_vsf;
								continue;
							}

							if (asc_package_valid)
							{
								video_buffer.push_right(ref_vsf);
							}							
						}
						else
						{
							//获取left区的最lef的值，如果是NULL，取progress_current_pts
							Ref_VideoStreamFrame *ref_tmp = this->video_buffer.get_left();
							int64_t tem_current_pts = this->progress_current_pts;
							if (ref_tmp != NULL)
							{
								tem_current_pts = ref_tmp->vsf->current_pts;
							}
							//倒放模式下，当前帧pts和buffer的最left的pts一致时从缓存灌入left区，否则插入stack
							if (current_pts == tem_current_pts)
							{
								video_buffer.push_left();
								desc_seek_flag = TRUE;
							}
							else
							{
								video_buffer.push_stack(ref_vsf);
							}
						}
						
					}
				}
				else if(packet->stream_index == audioindex && audioSwitch && play_mode == PLAY_MODE::ASC &&
					((speedNum == 1 && playerStatus == PLAY_STATUS::PLAY) || 
					playerStatus == PLAY_STATUS::PAUSE)) {
					ret = avcodec_decode_audio4(audioCodecCtx, audioFrame, &got_picture, packet);
					if (ret < 0) {
						logger.error("Error in decoding audio frame.\n");
						//return false;
						continue;
					}
					if (got_picture > 0) {
						uint8_t *audio_out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
						int out_size = swr_convert(au_convert_ctx, &audio_out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)audioFrame->data, audioFrame->nb_samples);
						logger.debug("out_size=%d,out_buffer_size=%d",out_size, out_buffer_size);

						AudioStreamFrame *asf = new AudioStreamFrame(audio_out_buffer, out_buffer_size, pts - start_pts, 
							audioFrame->nb_samples * AV_TIME_BASE / audioCodecCtx->sample_rate, audioFrame->nb_samples, audioCodecCtx->sample_rate);
						//pushAudioStream(asf);
						Ref_AudioStreamFrame *ref_asf = new Ref_AudioStreamFrame(asf);
						audio_buffer.push_right(ref_asf);
					}
				}
				//av_free_packet(packet);
				av_packet_unref(packet);
				//Sleep(1);
			}
			else
			{
				/*int i = 0;
				while (video_buffer.get_right_count() > 0 && audio_buffer.get_right_count() > 0 && i < 100)
				{
					i++;
					Sleep(100);
				}
				fileEnd = TRUE;
				this->callbackFun(0);*/
				Sleep(1);
			}
		}
		else
		{
			Sleep(1);
		}
	}

	logger.information("waiting for video/audio thread quit.");
	//确认音视频处理线程退出
	while (videoThreadStatus || audioThreadStatus)
	{
		Sleep(100);
	}

	logger.information("start free resource");
	if (audioindex != -1 && audioSwitch)
	{
		SDL_CloseAudio();

		swr_free(&au_convert_ctx);
		av_frame_free(&audioFrame);
		avcodec_close(audioCodecCtx);
	}

	sdl_instance->destroySDLTexture();

	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCtx);
	//在使用av_packet_alloc创建packet的时候，并没有给数据域分配空间，
	//数据域的空间实在av_read_frame内分配的，所以在每次循环的结束不能
	//忘记调用av_packet_unref减少数据域的引用技术，当引用技术减为0时，
	//会自动释放数据域所占用的空间。在循环结束后，调用av_packet_free来
	//释放AVPacket本身所占用的空间
	av_packet_free(&packet);
	avformat_close_input(&pFormatCtx);
	pFormatCtx = NULL;

	logger.information("media is end!");
	return TRUE;
}


BOOL MediaClass::videoPlay_Thread()
{
	logger.information("videoPlay_Thread()");
	if (videoThreadStatus)
	{
		return TRUE;
	}

	//thread
	DWORD threadID;
	HANDLE media_hThread;

	MediaThreadParam *mtp = new MediaThreadParam;
	mtp->m_instance = this;
	videoThreadStatus = TRUE;
	media_hThread = CreateThread(NULL, 0, videoPlay_ThreadProc, mtp, 0, &threadID);	// 创建线程

	return TRUE;
}

BOOL MediaClass::videoPlay()
{
	logger.debug("videoPlay()");
	while (media_status == MEDIA_STATUS::RUNNING)
	{
		if ((playerStatus == PLAY_STATUS::PLAY || (playerStatus == PLAY_STATUS::FRAME && playFrameNum > 0) ||
			(playerStatus == PLAY_STATUS::PAUSE && playFrameNum > 0)) &&
			((play_mode == PLAY_MODE::ASC && video_buffer.get_right_count() > 0) || (play_mode == PLAY_MODE::DESC && video_buffer.get_left_count() > 0)))
		{		
			Ref_VideoStreamFrame *ref_tmp = NULL;
			if (play_mode == PLAY_MODE::ASC)
			{
				ref_tmp = video_buffer.read();
			}
			else
			{
				ref_tmp = video_buffer.read_back();
			}
			
			if (ref_tmp == NULL)
			{
				continue;
			}
			Ref_VideoStreamFrame ref_frame = *ref_tmp;
			VideoStreamFrame * frame = ref_frame.vsf;

			//为播放帧数减1
			if (playerStatus == PLAY_STATUS::FRAME || playerStatus == PLAY_STATUS::PAUSE)
			{
				playFrameNum--;
			}
			int64_t pts = frame->current_pts;
			int current_frame_v = frame->current_frame_v;
						
			int delay_time = 1000 / fps / speedNum;

			/*logger.debug("pts-->%s,ss.get_current_pts()-->%s",
			std::to_string(pts), std::to_string(ss.get_current_pts()));*/
			int64_t a_pts = this->ss.get_current_pts();

			if (audioindex != -1 && audioSwitch && playerStatus == PLAY_STATUS::PLAY && speedNum == 1 && play_mode == PLAY_MODE::ASC)
			{
				//只在音频有效的情况下进行音视频同步操作
				if (a_pts == AV_NOPTS_VALUE || pts == AV_NOPTS_VALUE)
				{
					logger.error("videoPlay:AV_NOPTS_VALUE");
				}
				if (a_pts >= pts)
				{
					if (a_pts - pts >= delay_time * 10000)
					{
						delay_time = 0;
					}
					else
					{
						delay_time = delay_time - (a_pts - pts) / 10000;
					}
				}
				else
				{
					//video帧等待，知道audio帧pts大于video，此处不能用sleep，会导致在seek时sleep时间异常的长
					BOOL throwFlag = FALSE;
					while (media_status == MEDIA_STATUS::RUNNING)
					{
						a_pts = this->ss.get_current_pts();
						if (a_pts >= pts || play_mode != PLAY_MODE::ASC)
						{
							break;
						}
						if (((delay_pts >= 1 * AV_TIME_BASE) && (pts - a_pts < delay_pts * 2)) || 
							((delay_pts < 1 * AV_TIME_BASE) && (pts - a_pts <= 1 * AV_TIME_BASE)))
						{
							Sleep(delay_time);
						}
						else
						{
							throwFlag = TRUE;
							break;
						}
					}
					if (throwFlag)
					{
						//视频pts比音频pts大太多，超过了delay_pts * 2，认为是异常帧，抛弃
						//delete frame;
						//delete &ref_frame;
						video_buffer.destroy_left();
						logger.information("throw frame!-->%s", std::to_string(frame->current_pts));
						continue;
					}
				}
			}

			//更新全局变量
			progress_current_pts = pts;
			progress_current_time = progress_current_pts * 1000 / AV_TIME_BASE;
			progress_current_frame_v = frame->current_frame_v;

			AVFrame *pFrameYUV = frame->pFrameYUV;
			logger.debug("updateSDLTexture-->%s", std::to_string(pts));
			sdl_instance->updateSDLTexture(pFrameYUV->data[0], pFrameYUV->linesize[0]);

			//delete frame;
			//delete &ref_frame;
			if (play_mode == PLAY_MODE::ASC)
			{
				video_buffer.destroy_left();
			}
			else
			{
				video_buffer.destroy_right();
			}

			if (delay_time > 10000 || delay_time < 0)
			{
				logger.warning("delay_time is warning!delay_time=%d", delay_time);
			}
			Sleep(delay_time);
		}
		else
		{
			Sleep(1);
		}

	}
	
	logger.debug("videoPlay is end!");

	return TRUE;
}

BOOL MediaClass::audioPlay_Thread()
{
	logger.information("audioPlay_Thread()");
	if (audioThreadStatus)
	{
		return TRUE;
	}

	//thread
	DWORD threadID;
	HANDLE media_hThread;

	MediaThreadParam *mtp = new MediaThreadParam;
	mtp->m_instance = this;
	audioThreadStatus = TRUE;
	media_hThread = CreateThread(NULL, 0, audioPlay_ThreadProc, mtp, 0, &threadID);	// 创建线程
	return TRUE;
}

BOOL MediaClass::audioPlay()
{
	logger.information("audioPlay()");
	while (media_status == MEDIA_STATUS::RUNNING)
	{
		if (playerStatus != PLAY_STATUS::PLAY || audio_buffer.get_right_count() <= 0)
		{
			Sleep(1);
			continue;
		}

		audio_len = 0;
		audio_pos = 0;


		logger.debug("audioQueue.front()");
		//audioStream = pullAudioStream();
		Ref_AudioStreamFrame *ref_tmp = audio_buffer.read();
		if (ref_tmp == NULL)
		{
			continue;
		}
		Ref_AudioStreamFrame ref_audioStream = *ref_tmp;
		AudioStreamFrame *audioStream = ref_audioStream.asf;

		//Set audio buffer (PCM data)
		audio_chunk = (Uint8 *)audioStream->stream;
		//Audio buffer length
		audio_len = audioStream->stream_size;
		audio_pos = audio_chunk;

		this->ss.set_current_pts(audioStream->current_pts, audioStream->duration_pts);
		logger.debug("set_current_pts-->%s",std::to_string(audioStream->current_pts));

		convertAudioRange(AV_SAMPLE_FMT_S16, (Uint8 *)audioStream->stream, audioStream->nb_samples);

		while (audio_len>0)//Wait until finish
			Sleep(1);

		//还原左右声道
		this->left_audio = 0;
		this->right_audio = 0;

		//delete audioStream;
		//Ref_AudioStreamFrame *ref_audioStream0 = &ref_audioStream;
		//delete ref_audioStream0;
		audio_buffer.destroy_left();
		//logger.debug("delete audioStream");
	}

	logger.debug("audioPlay is end!");
	return TRUE;
}

//按一个frame的平均采样值计算分贝数
BOOL MediaClass::convertAudioRange(AVSampleFormat out_sample_fmt, uint8_t *audioStream, int nb_samples)
{
	//int64_t range[nb_samples] = {};
	if (out_sample_fmt == AV_SAMPLE_FMT_S16)
	{
		int left_range = 0;
		int right_range = 0;
		for (int i = 0; i < nb_samples; i++)
		{
			//左声道
			uint8_t tem1 = *audioStream;
			audioStream += 1;
			uint8_t tem2 = *audioStream;
			audioStream += 1;
			left_range += abs(UtilTool::getInstance()->uint8_to_int16(tem2, tem1));

			//右声道
			uint8_t tem3 = *audioStream;
			audioStream += 1;
			uint8_t tem4 = *audioStream;
			audioStream += 1;
			right_range += abs(UtilTool::getInstance()->uint8_to_int16(tem4, tem3));
		}
		this->left_audio = left_range / nb_samples;
		this->right_audio = right_range / nb_samples;
	}
	return TRUE;
}

/**
检查pts是否为AV_NOPTS_VALUE
*/
int64_t MediaClass::checkFilterPTS(int64_t pts)
{
	if (pts == AV_NOPTS_VALUE)
	{
		logger.debug("pts is AV_NOPTS_VALUE!");
	}
	if (pts < 0)
	{
		logger.debug("pts is error!");
	}
	return pts;
}

/**
启动一个media解码刷新的线程
*/
DWORD WINAPI openMedia_ThreadProc(LPVOID lpParam)
{
	//static Logger& logger = Logger::get("openMedia_ThreadProc");
	static Logger& logger = Logger::get(Logger::ROOT);

	MediaThreadParam * mtp = (MediaThreadParam *)lpParam;
	logger.information("openMedia_ThreadProc()->" + std::string(mtp->filePath));
	MediaClass *m_instance = mtp->m_instance;
	if (!m_instance->openMedia(mtp->filePath))
	{
		logger.error("openMedia() is false!");
	}
	logger.information("openMedia_ThreadProc is end!");
	delete mtp;
	m_instance->media_status = MEDIA_STATUS::INIT;
	return 0;
}

/**
启动一个video处理线程
*/
DWORD WINAPI videoPlay_ThreadProc(LPVOID lpParam)
{
	//static Logger& logger = Logger::get("videoPlay_ThreadProc");
	static Logger& logger = Logger::get(Logger::ROOT);
	logger.information("videoPlay_ThreadProc()");
	MediaThreadParam * mtp = (MediaThreadParam *)lpParam;
	MediaClass *m_instance = mtp->m_instance;
	m_instance->videoPlay();
	logger.information("videoPlay_ThreadProc is end!");
	delete mtp;
	m_instance->videoThreadStatus = FALSE;
	return 0;
}

/**
启动一个audio处理线程
*/
DWORD WINAPI audioPlay_ThreadProc(LPVOID lpParam)
{
	//static Logger& logger = Logger::get("audioPlay_ThreadProc");
	static Logger& logger = Logger::get(Logger::ROOT);
	logger.information("audioPlay_ThreadProc()");
	MediaThreadParam * mtp = (MediaThreadParam *)lpParam;
	MediaClass *m_instance = mtp->m_instance;
	m_instance->audioPlay();
	delete mtp;
	logger.information("audioPlay_ThreadProc is end!");
	m_instance->audioThreadStatus = FALSE;
	return 0;
}


