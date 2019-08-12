#pragma once
#define POCO_NO_UNWINDOWS
#define __STDC_CONSTANT_MACROS
#include "resource.h"
#include "commdlg.h"
#include <stdio.h>
#include <string>
#include <map> 
#include <queue>
#include <sys/timeb.h>
#include <sys/types.h>
#include <shellapi.h>
#include <CommCtrl.h>
#include <mutex>
#include <objbase.h>
#include <shlwapi.h>
#include <Windowsx.h>

#include "Poco/StreamCopier.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/JSON/JSON.h"
#include "Poco/JSON/Object.h"
#include "Poco/URI.h"
#include "Poco/Logger.h"
#include "Poco/Base64Encoder.h"
#include "Poco/Base64Decoder.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/ParseHandler.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/Logger.h"
#include "Poco/Channel.h"
#include "Poco/SplitterChannel.h"
#include "Poco/SimpleFileChannel.h"
#include "Poco/AutoPtr.h"
#include "Poco/FormattingChannel.h"
#include "Poco/PatternFormatter.h"
#include "Poco/Util/PropertyFileConfiguration.h"
#include "Poco/Util/LoggingConfigurator.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
};

#define IDB_WND_MEDIA     3301
#define IDB_WND_PROGRESS     3302
#define IDB_WND_TABLE     3303
#define IDB_WND_AUDIO     3304

#define IDB_BUTTON_PLAY     3311
#define IDB_BUTTON_PAUSE     3312
#define IDB_BUTTON_PLAY_BACK   3313
#define IDB_BUTTON_SPEED_BACK     3314
#define IDB_BUTTON_SPEED   3315
#define IDB_BUTTON_DOT_LEFT   3316
#define IDB_BUTTON_DOT_RIGHT   3317

#define IDB_BUTTON_PLAYBACKFRAME   3318
#define IDB_BUTTON_PLAYFRAME   3319
#define IDB_BUTTON_DOT_ADD   3320

#define IDB_BUTTON_OK 3321
#define IDB_BUTTON_COMBOX_SPEED  3322
#define IDB_BUTTON_COMBOX  3323

#define IDB_STATIC_INFO  3342
#define IDB_STATIC_TEMINFO 3343

#define IDB_MENU_UPDATE 3380
#define IDB_MENU_DELETE 3381

extern HINSTANCE hInst;
extern HWND mainWnd;//主窗口句柄
extern HWND mediaWnd;//视频窗口句柄
extern char * application_path;//程序根路径

using Poco::Logger;

