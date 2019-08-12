#include "stdafx.h"
#include "SDLClass.h"

//音频相关
//Buffer:
//|-----------|-------------|
//chunk-------pos---len-----|
Uint8  *audio_chunk;
Uint32  audio_len;
Uint8  *audio_pos;

/* Audio Callback
* The audio function callback takes the following parameters:
* stream: A pointer to the audio buffer to be filled
* len: The length (in bytes) of the audio buffer
*
*/
void  fill_audio(void *udata, Uint8 *stream, int len) {
	//static Logger& logger = Logger::get("SDLClass-fill_audio()");
	//logger.debug("fill_audio()");

	//SDL 2.0
	SDL_memset(stream, 0, len);
	if (audio_len == 0)		/*  Only  play  if  we  have  data  left  */
		return;
	len = (len>audio_len ? audio_len : len);	/*  Mix  as  much  data  as  possible  */

	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}


SDLClass::SDLClass()
{
	loadingThreadStatus = FALSE;
	RGB_Red.r = 255;
	RGB_Red.b = 0;
	RGB_Red.g = 0;
}


SDLClass::~SDLClass()
{
}

//Logger& SDLClass::logger = Logger::get("SDLClass");
Logger& SDLClass::logger = Logger::get(Logger::ROOT);

BOOL SDLClass::initSDL(HWND wnd)
{
	logger.information("initSDL()");
	this->wnd = wnd;
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO  | SDL_INIT_TIMER)) {
		//printf("Could not initialize SDL - %s\n", SDL_GetError());
		return FALSE;
	}

	//显示文字初始化
	//TTF_Init();

	//为了显示png图片，额外使用了图片库，所以要单独初始化
	//IMG_Init(IMG_INIT_JPG);

	/**
	screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	screen_w, screen_h, SDL_WINDOW_OPENGL);
	*/
	//HWND mediaWnd = (HWND)data;
	screen = SDL_CreateWindowFrom(this->wnd);
	//解决和windows的消息机制冲突了，因此windows和sdl同时监听到WM_RESIZE（mfc的）和SDL_WINDOWEVENT_RESIZED(SDL的)时难免会产生崩溃
	SDL_EventState(SDL_WINDOWEVENT, SDL_IGNORE);

	if (!screen) {
		//printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
		return FALSE;
	}
	sdlRenderer = SDL_CreateRenderer(screen, -1, 0);

	//获取media屏幕区域
	RECT rect1;
	GetClientRect(this->wnd, &rect1);
	loadingRect.x = rect1.right / 2 - 50;
	loadingRect.y = rect1.bottom / 3 - 10;
	loadingRect.w = 100;
	loadingRect.h = 30;

	return TRUE;
}

BOOL SDLClass::CreateTexture(int width, int height)
{
	this->sdlTexture = SDL_CreateTexture(this->sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);
	return TRUE;
}

BOOL SDLClass::updateSDLTexture(const void *pixels, int pitch)
{
	SDL_UpdateTexture(this->sdlTexture, NULL, pixels, pitch);

	SDL_RenderClear(this->sdlRenderer);
	//SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );  
	SDL_RenderCopy(this->sdlRenderer, this->sdlTexture, NULL, NULL);
	SDL_RenderPresent(this->sdlRenderer);
	return TRUE;
}

BOOL SDLClass::destroySDLTexture()
{
	SDL_DestroyTexture(this->sdlTexture);
	return TRUE;
}

BOOL SDLClass::startLoadingImageSDL()
{
	logger.information("startLoadingImageSDL()");

	//SDL_Surface * image = SDL_LoadBMP("D:\\work\\sihuatech\\grampus\\grampus.player\\loading.bmp");
	//SDL_Surface * image = SDL_LoadBMP("C:\\Users\\liujf\\Desktop\\grampus.player\\cipPlayer\\cipPlayer\\loading.bmp");
	char imagePath[512] = {};
	sprintf(imagePath, "%s%s", application_path, "\\loading.bmp");
	SDL_Surface * image = SDL_LoadBMP(imagePath);
	//因为要显示png图片，所以使用了外部库，sdl_image库当前支持jpg/png/webp/tiff图片格式
	//SDL_Surface * image = IMG_Load("C:\\Users\\liujf\\Desktop\\grampus.player\\cipPlayer\\cipPlayer\\loading.jpg");
	SDL_Texture *sdlTexture = SDL_CreateTextureFromSurface(sdlRenderer, image);

	while(loadingThreadStatus)
	{
		SDL_RenderClear(sdlRenderer);
		SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &loadingRect);
		SDL_RenderPresent(sdlRenderer);
		Sleep(100);
	}
	SDL_DestroyTexture(sdlTexture);
	SDL_FreeSurface(image);
	
	//按钮值为无效
	/*EnableWindow(playWnd,FALSE);
	EnableWindow(speedWnd, FALSE);
	EnableWindow(speeddownWnd, FALSE);
	EnableWindow(playFrameWnd, FALSE);
	EnableWindow(dotWnd, FALSE);*/
	return TRUE;
}

BOOL SDLClass::startLoadingImageSDL_Thread()
{
	logger.information("startLoadingImageSDL_Thread()");
	if (loadingThreadStatus)
	{
		return TRUE;
	}

	//thread
	DWORD threadID;
	HANDLE hThread;

	SDLThreadParam *stp = new SDLThreadParam;
	stp->m_instance = this;
	loadingThreadStatus = TRUE;
	hThread = CreateThread(NULL, 0, loadingImage_ThreadProc, stp, 0, &threadID);	// 创建线程
	
	return TRUE;
}

BOOL SDLClass::endLoadingImageSDL()
{
	logger.information("endLoadingImageSDL()");

	loadingThreadStatus = FALSE;

	SDL_RenderClear(sdlRenderer);
	SDL_RenderPresent(sdlRenderer);


	return TRUE;
}


BOOL SDLClass::closeSDL()
{
	logger.information("closeSDL()");
	try
	{
		SDL_DestroyRenderer(sdlRenderer);
		SDL_DestroyWindow(screen);
		SDL_Quit();
	}
	catch (std::exception ex)
	{
		logger.error("closeSDL error-->%s",ex.what());
	}

	logger.information("SDL is closed!");
	return TRUE;
}

/**
启动一个LOADING处理线程
*/
DWORD WINAPI loadingImage_ThreadProc(LPVOID lpParam)
{
	//static Logger& logger = Logger::get("audioPlay_ThreadProc");
	static Logger& logger = Logger::get(Logger::ROOT);
	logger.information("loadingImage_ThreadProc()");
	SDLThreadParam * stp = (SDLThreadParam *)lpParam;
	SDLClass *m_instance = stp->m_instance;
	m_instance->startLoadingImageSDL();
	delete stp;
	logger.information("audioPlay_ThreadProc is end!");
	m_instance->loadingThreadStatus = FALSE;
	return 0;
}
