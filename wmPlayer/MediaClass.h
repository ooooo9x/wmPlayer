#pragma once
#include "common.h"
#include "SDLClass.h"
#include "PlayBuffer.h"


#define __STDC_CONSTANT_MACROS
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

class SynchronizeStatus
{
public:
	SynchronizeStatus();
	~SynchronizeStatus();

	void init();
	void set_current_pts(int64_t current_pts, int64_t duration_pts);
	int64_t get_current_pts();
	int64_t get_duration_pts();
	int64_t get_pre_next_pts();

private:
	int64_t current_pts;
	int64_t duration_pts;
	int64_t pre_next_pts;

	static std::mutex m_mt;
};


class VideoStreamFrame
{
public:
	VideoStreamFrame(AVFrame *pFrameYUV, uint8_t *out_buffer, int64_t current_pts, int64_t current_frame_v)
	{
		this->pFrameYUV = pFrameYUV;
		this->out_buffer = out_buffer;
		this->current_pts = current_pts;
		this->current_frame_v = current_frame_v;
		this->reference_count = 1;
	}

	~VideoStreamFrame()
	{
		av_free(out_buffer);
		av_frame_free(&pFrameYUV);
	}

	void ref()
	{
		std::lock_guard<std::mutex> lgd(mt);
		reference_count++;
	}

	int unref()
	{
		std::lock_guard<std::mutex> lgd(mt);
		reference_count--;
		return reference_count;
	}

	AVFrame *pFrameYUV;
	uint8_t *out_buffer;
	int64_t current_pts;
	int64_t current_frame_v;
	//long num;
private:
	int reference_count;
	std::mutex mt;
};

class  Ref_VideoStreamFrame
{
public:

	//使用int*指针初始化ptr，注意必须要放在初始化列表中
	Ref_VideoStreamFrame(VideoStreamFrame * vsf)
	{
		this->vsf = vsf;
	}

	//拷贝构造函数，又有一个变量指向了这块内存
	Ref_VideoStreamFrame(const Ref_VideoStreamFrame & rhs)
	{
		vsf = rhs.vsf;//将右操作数的引用计数对象赋值给左操作数
		vsf->ref();//将它们的应用计数加1
	}

	//赋值操作符，右操作数的引用计数要减1，左操作数的引用计数要加1
	Ref_VideoStreamFrame & operator=(const Ref_VideoStreamFrame & rhs)
	{
		if (&rhs == this)
			return *this;
		if (vsf->unref() == 0)//赋值操作符，首先将当前类的引用计数减1
		{
			//cout << "delete Ref_ptr" << endl;
			delete vsf;
		}
		vsf = rhs.vsf;//将右操作数的引用计数赋值给当前对象
		vsf->ref();//引用计数加1
		return *this;
	}

	//析构函数，引用计数要减1，如果减为0，删除这块内存
	~Ref_VideoStreamFrame()
	{
		if (vsf->unref() == 0)
		{
			//cout << "delete Ref_ptr" << endl;
			delete vsf;
		}
	}

	VideoStreamFrame * vsf;
};

class AudioStreamFrame
{
public:
	AudioStreamFrame(uint8_t *stream, int stream_size, int64_t current_pts, int64_t duration_pts, int nb_samples, int sample_rate)
	{
		this->stream = stream;
		this->stream_size = stream_size;
		this->nb_samples = nb_samples;
		this->sample_rate = sample_rate;
		this->current_pts = current_pts;
		this->duration_pts = duration_pts;
		this->pre_next_pts = current_pts + duration_pts;
		this->reference_count = 1;
		//this->num = 0;
	}	

	~AudioStreamFrame()
	{
		av_free(stream);
	}

	void ref()
	{
		std::lock_guard<std::mutex> lgd(mt);
		reference_count++;
	}

	int unref()
	{
		std::lock_guard<std::mutex> lgd(mt);
		reference_count--;
		return reference_count;
	}

	uint8_t *stream;
	int stream_size;
	int nb_samples;
	int sample_rate;
	int64_t current_pts;
	int64_t duration_pts;
	int64_t pre_next_pts;
	long num;

private:
	int reference_count;
	std::mutex mt;
};

class  Ref_AudioStreamFrame
{
public:

	//使用int*指针初始化ptr，注意必须要放在初始化列表中
	Ref_AudioStreamFrame(AudioStreamFrame * asf)
	{
		this->asf = asf;
	}

	//拷贝构造函数，又有一个变量指向了这块内存
	Ref_AudioStreamFrame(const Ref_AudioStreamFrame & rhs)
	{
		asf = rhs.asf;//将右操作数的引用计数对象赋值给左操作数
		asf->ref();//将它们的应用计数加1
	}

	//赋值操作符，右操作数的引用计数要减1，左操作数的引用计数要加1
	Ref_AudioStreamFrame & operator=(const Ref_AudioStreamFrame & rhs)
	{
		if (&rhs == this)
			return *this;
		if (asf->unref() == 0)//赋值操作符，首先将当前类的引用计数减1
		{
			//cout << "delete Ref_ptr" << endl;
			delete asf;
		}
		asf = rhs.asf;//将右操作数的引用计数赋值给当前对象
		asf->ref();//引用计数加1
		return *this;
	}

	//析构函数，引用计数要减1，如果减为0，删除这块内存
	~Ref_AudioStreamFrame()
	{
		if (asf->unref() == 0)
		{
			//cout << "delete Ref_ptr" << endl;
			delete asf;
		}
	}

	AudioStreamFrame * asf;
};

//0.暂停;1.播放中;2:帧播放；
enum PLAY_STATUS
{
	PAUSE = 0,
	PLAY = 1,
	FRAME = 2
};

//0:以加载媒体文件，程序开始处理;1.正在停止程序；2.已经停止,初始态；
enum MEDIA_STATUS
{
	RUNNING = 0,
	STOPPING = 1,
	INIT = 2
};

//0.以完成seek，常态；1.seek操作；
enum SEEK_STATUS
{
	NORMAL = 0,
	SEEKING = 1
};

//0：顺序播放；1：倒序播放；
enum PLAY_MODE
{
	ASC = 0,
	DESC = 1
};

class MediaClass
{
public:
	MediaClass(SDLClass *instance);
	~MediaClass();
	BOOL openMedia_Thread(char filepath[]);
	BOOL openMedia(char filepath[]);
	BOOL reOpenMedia_Thread(char filePath[]);
	BOOL stopMedia();
	BOOL seek(int64_t sec);//将文件seek到指定位置
	BOOL play(PLAY_STATUS status, PLAY_MODE mode, float speedNum, int num);
	//BOOL play_frame(int num);
	//BOOL pause();
	//BOOL play_speeddown();
	//BOOL play_speed();
	//void setPlayMode(PLAY_MODE mode);
	//void clearAudioStream();

	PLAY_STATUS getPlayStatus();
	float getPlaySpeed();
	PLAY_MODE getPlayMode();
	MEDIA_STATUS getMediaStatus();

	/*下面方法设为public是线程调用需要，不对外部调用*/
	BOOL videoPlay();
	BOOL videoPlay_Thread();
	BOOL audioPlay();
	BOOL audioPlay_Thread();
	BOOL videoThreadStatus;//视频处理线程状态
	BOOL audioThreadStatus;//音频处理线程状态
	/*上面方法设为public是线程调用需要，不对外部调用*/

	//回调函数
	void(*callbackFun)(int eventCode);


	MEDIA_STATUS media_status;

	//放大倍数
	int audio_multiple;
	//左声道
	int left_audio;
	//右声道
	int right_audio;

	//媒体总时长，单位ms
	int progress_duration_time;
	//媒体当前时间点，单位ms
	int progress_current_time;
	
	//媒体当前pts (in AV_TIME_BASE units)
	int64_t progress_current_pts;
	//媒体当前帧num
	int progress_current_frame_v;

	int fps;
	//打点起始点时间，单位ms
	int dot_start_time;
	//打点起始点帧
	int dot_start_frame;
	//打点结束点时间，单位ms
	int dot_end_time;
	//打点结束点帧
	int dot_end_frame;

private:
	BOOL initGlobalVar();
	int64_t checkFilterPTS(int64_t pts);
	BOOL convertAudioRange(AVSampleFormat out_sample_fmt, uint8_t *audioStream, int nb_samples);
	BOOL cipplayer_seek(AVFormatContext *s, int video_index, int64_t current_timestamp, AVPacket *packet);
	BOOL cipplayer_read_frame(AVFormatContext *s, int video_index, AVPacket *pkt,bool firstFrame);
	BOOL allowReadStream();
	BOOL initVideo();
	BOOL initAudio();
	BOOL reset();//将文件seek到起始位置

	//sdl操作类
	SDLClass *sdl_instance;

	AVFormatContext	*pFormatCtx;
	AVCodecContext	*pCodecCtx;
	AVCodecContext *audioCodecCtx;
	AVCodec			*pCodec;
	AVCodec *audioCodec;
	AVFrame	*pFrame; 
	AVFrame *audioFrame;
	AVPacket *packet;
	SwrContext *au_convert_ctx;
	SwsContext *img_convert_ctx;
	//媒体start pts (in AV_TIME_BASE units)
	int64_t v_start_pts;
	int64_t a_start_pts;
	int64_t start_pts;
	int out_buffer_size;//音频Out Buffer Size
	int	videoindex;
	int audioindex;

	SEEK_STATUS seek_status;
	//int av_seek_frame_status;//1.需要执行av_seek_frame；0.av_seek_frame已经执行；
	int64_t seek_sec;

	//音频开关，FALSE时忽略音频
	BOOL audioSwitch;
	//当前播放状态
	PLAY_STATUS playerStatus;
	//当前还需前进帧数
	int playFrameNum;	
	//播放模式，0：顺序播放；1：倒序播放；
	PLAY_MODE play_mode;
	//play_mode切换时的seek标志
	BOOL play_mode_seek_flag;
	//播放倍速
	float speedNum;

	//媒体文件大小
	int64_t filesize;
	//媒体格式名
	char *iformat_name;

	//视频、音频队列
	PlayBuffer<Ref_AudioStreamFrame> audio_buffer;
	PlayBuffer<Ref_VideoStreamFrame> video_buffer;

	//音视频时延差abs(a_start_pts - v_start_pts)
	int64_t delay_pts;

	//音视频同步状态
	SynchronizeStatus ss;

	//循环体锁，在操作时，暂停循环处理
	BOOL loop_lock;
	//是否已到文件末尾
	//BOOL fileEnd;

	//logger
	static Logger& logger;
};

class MediaThreadParam
{
public:
	MediaThreadParam() {}
	~MediaThreadParam() {}

	MediaClass *m_instance;
	char *filePath;
};

DWORD WINAPI openMedia_ThreadProc(LPVOID lpParam);
DWORD WINAPI videoPlay_ThreadProc(LPVOID lpParam);
DWORD WINAPI audioPlay_ThreadProc(LPVOID lpParam);
DWORD WINAPI audioRange_ThreadProc(LPVOID lpParam);
//DWORD WINAPI FrameSynchronizer_ThreadProc(LPVOID lpParam);

