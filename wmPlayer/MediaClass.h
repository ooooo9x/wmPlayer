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

	//ʹ��int*ָ���ʼ��ptr��ע�����Ҫ���ڳ�ʼ���б���
	Ref_VideoStreamFrame(VideoStreamFrame * vsf)
	{
		this->vsf = vsf;
	}

	//�������캯��������һ������ָ��������ڴ�
	Ref_VideoStreamFrame(const Ref_VideoStreamFrame & rhs)
	{
		vsf = rhs.vsf;//���Ҳ����������ü�������ֵ���������
		vsf->ref();//�����ǵ�Ӧ�ü�����1
	}

	//��ֵ���������Ҳ����������ü���Ҫ��1��������������ü���Ҫ��1
	Ref_VideoStreamFrame & operator=(const Ref_VideoStreamFrame & rhs)
	{
		if (&rhs == this)
			return *this;
		if (vsf->unref() == 0)//��ֵ�����������Ƚ���ǰ������ü�����1
		{
			//cout << "delete Ref_ptr" << endl;
			delete vsf;
		}
		vsf = rhs.vsf;//���Ҳ����������ü�����ֵ����ǰ����
		vsf->ref();//���ü�����1
		return *this;
	}

	//�������������ü���Ҫ��1�������Ϊ0��ɾ������ڴ�
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

	//ʹ��int*ָ���ʼ��ptr��ע�����Ҫ���ڳ�ʼ���б���
	Ref_AudioStreamFrame(AudioStreamFrame * asf)
	{
		this->asf = asf;
	}

	//�������캯��������һ������ָ��������ڴ�
	Ref_AudioStreamFrame(const Ref_AudioStreamFrame & rhs)
	{
		asf = rhs.asf;//���Ҳ����������ü�������ֵ���������
		asf->ref();//�����ǵ�Ӧ�ü�����1
	}

	//��ֵ���������Ҳ����������ü���Ҫ��1��������������ü���Ҫ��1
	Ref_AudioStreamFrame & operator=(const Ref_AudioStreamFrame & rhs)
	{
		if (&rhs == this)
			return *this;
		if (asf->unref() == 0)//��ֵ�����������Ƚ���ǰ������ü�����1
		{
			//cout << "delete Ref_ptr" << endl;
			delete asf;
		}
		asf = rhs.asf;//���Ҳ����������ü�����ֵ����ǰ����
		asf->ref();//���ü�����1
		return *this;
	}

	//�������������ü���Ҫ��1�������Ϊ0��ɾ������ڴ�
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

//0.��ͣ;1.������;2:֡���ţ�
enum PLAY_STATUS
{
	PAUSE = 0,
	PLAY = 1,
	FRAME = 2
};

//0:�Լ���ý���ļ�������ʼ����;1.����ֹͣ����2.�Ѿ�ֹͣ,��ʼ̬��
enum MEDIA_STATUS
{
	RUNNING = 0,
	STOPPING = 1,
	INIT = 2
};

//0.�����seek����̬��1.seek������
enum SEEK_STATUS
{
	NORMAL = 0,
	SEEKING = 1
};

//0��˳�򲥷ţ�1�����򲥷ţ�
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
	BOOL seek(int64_t sec);//���ļ�seek��ָ��λ��
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

	/*���淽����Ϊpublic���̵߳�����Ҫ�������ⲿ����*/
	BOOL videoPlay();
	BOOL videoPlay_Thread();
	BOOL audioPlay();
	BOOL audioPlay_Thread();
	BOOL videoThreadStatus;//��Ƶ�����߳�״̬
	BOOL audioThreadStatus;//��Ƶ�����߳�״̬
	/*���淽����Ϊpublic���̵߳�����Ҫ�������ⲿ����*/

	//�ص�����
	void(*callbackFun)(int eventCode);


	MEDIA_STATUS media_status;

	//�Ŵ���
	int audio_multiple;
	//������
	int left_audio;
	//������
	int right_audio;

	//ý����ʱ������λms
	int progress_duration_time;
	//ý�嵱ǰʱ��㣬��λms
	int progress_current_time;
	
	//ý�嵱ǰpts (in AV_TIME_BASE units)
	int64_t progress_current_pts;
	//ý�嵱ǰ֡num
	int progress_current_frame_v;

	int fps;
	//�����ʼ��ʱ�䣬��λms
	int dot_start_time;
	//�����ʼ��֡
	int dot_start_frame;
	//��������ʱ�䣬��λms
	int dot_end_time;
	//��������֡
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
	BOOL reset();//���ļ�seek����ʼλ��

	//sdl������
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
	//ý��start pts (in AV_TIME_BASE units)
	int64_t v_start_pts;
	int64_t a_start_pts;
	int64_t start_pts;
	int out_buffer_size;//��ƵOut Buffer Size
	int	videoindex;
	int audioindex;

	SEEK_STATUS seek_status;
	//int av_seek_frame_status;//1.��Ҫִ��av_seek_frame��0.av_seek_frame�Ѿ�ִ�У�
	int64_t seek_sec;

	//��Ƶ���أ�FALSEʱ������Ƶ
	BOOL audioSwitch;
	//��ǰ����״̬
	PLAY_STATUS playerStatus;
	//��ǰ����ǰ��֡��
	int playFrameNum;	
	//����ģʽ��0��˳�򲥷ţ�1�����򲥷ţ�
	PLAY_MODE play_mode;
	//play_mode�л�ʱ��seek��־
	BOOL play_mode_seek_flag;
	//���ű���
	float speedNum;

	//ý���ļ���С
	int64_t filesize;
	//ý���ʽ��
	char *iformat_name;

	//��Ƶ����Ƶ����
	PlayBuffer<Ref_AudioStreamFrame> audio_buffer;
	PlayBuffer<Ref_VideoStreamFrame> video_buffer;

	//����Ƶʱ�Ӳ�abs(a_start_pts - v_start_pts)
	int64_t delay_pts;

	//����Ƶͬ��״̬
	SynchronizeStatus ss;

	//ѭ���������ڲ���ʱ����ͣѭ������
	BOOL loop_lock;
	//�Ƿ��ѵ��ļ�ĩβ
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

