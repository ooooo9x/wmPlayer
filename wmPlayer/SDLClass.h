#pragma once
#include "common.h"

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

class SDLClass
{
public:
	SDLClass();
	~SDLClass();
	BOOL initSDL(HWND wnd);
	BOOL closeSDL();

	BOOL CreateTexture(int width,int height);
	BOOL updateSDLTexture(const void *pixels, int pitch);
	BOOL destroySDLTexture();

	BOOL startLoadingImageSDL_Thread();
	BOOL startLoadingImageSDL();
	BOOL endLoadingImageSDL();

	BOOL loadingThreadStatus;

	
	//“Ù∆µœ‡πÿ
	//void pushAudioStream(AudioStreamFrame *asf);
	//void startPlayAudio();
private:
	HWND wnd;
	SDL_Renderer* sdlRenderer;
	SDL_Texture* sdlTexture;
	SDL_Window * screen;
	//SDL_Rect sdlRect;
	//SDL_Thread *video_tid;
	//SDL_Event event;

	SDL_Color RGB_Red;
	SDL_Rect loadingRect;

	//std::queue<AudioStreamFrame *> audioQueue;

	//logger
	static Logger& logger;
};

extern Uint8  *audio_chunk;
extern Uint32  audio_len;
extern Uint8  *audio_pos;
void  fill_audio(void *udata, Uint8 *stream, int len);
DWORD WINAPI loadingImage_ThreadProc(LPVOID lpParam);

class SDLThreadParam
{
public:
	SDLThreadParam() {}
	~SDLThreadParam() {}

	SDLClass *m_instance;
};