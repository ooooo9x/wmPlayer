//cipPlayer.cpp: 定义应用程序的入口点。
//
#include "stdafx.h"
#include "common.h"
#include "UtilTool.h"
#include "MediaClass.h"
#include "SDLClass.h"

// 全局变量: 
HINSTANCE hInst;                               // 当前实例
HWND mainWnd;                                  // 主窗口
HWND mediaWnd;                                 // 视频窗口
HWND audioWnd;

HWND progressWnd;

HWND playWnd; 
HWND pauseWnd;
HWND playBackWnd;
//HWND modeWnd;
HWND speedWnd;
HWND speedFrameComboWnd;
HWND speedBackWnd;

HWND playFrameWnd;
HWND playFrameComboWnd;
HWND playBackFrameWnd;

HWND dotWnd;
HWND dotLeftWnd;
HWND dotRightWnd;
HWND dotAddWnd; 
HWND dotDelWnd;
HWND okWnd;
HWND infoWnd;
HWND temInfoWnd;
HWND hwndListView;
HMENU hmenu;

#define MAX_LOADSTRING 100
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

int progress_width = 100;//进度条总像素宽度(不包含0点)
//char default_filePath[500] = {};
//char * filePath = default_filePath; //命令行的文件名
char filePath[500] = {};

char * application_path = NULL;

static ArgStruct intputAttr = {};

std::map<std::wstring, std::string> dotMap;
int listview_rclick_select_iItem = -1;


// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void wm_Create(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam);
LRESULT wm_Ctlcolorstatic(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam);
void wm_Notify(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam);
LRESULT wm_Command(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam);
void wm_Size(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam);
//void wm_MouseMove(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
//void media_callback_fucn(int eventCode);

SDLClass *sdl_instance = new SDLClass();
MediaClass *media_instance = new MediaClass(sdl_instance);
//DWORD threadID;
//HANDLE media_hThread;
DWORD fresh_progress_threadID;
HANDLE fresh_progress_hThread; 
DWORD WINAPI ThreadProc_freshProgress(LPVOID lpParam);


OPENFILENAME ofn;       // common dialog box structure
wchar_t szFile[260];       // buffer for file name
HWND hwnd;              // owner window
HANDLE hf;              // file handle

HPEN static hpen1;
HPEN static hpen2;
HPEN static hpen3;
HPEN static hpen_scale;
HBRUSH static infoWnd_hbrBkgnd; 
HBRUSH static tem_infoWnd_hbrBkgnd;
HBRUSH static progress_Bkgnd;
HBRUSH static audioRegin_Bkgnd;
HBRUSH static audio_column_Bkgnd;
HBRUSH static audio_valid_Bkgnd;
HFONT static hFont;

BOOL freshPlayInfo(HWND hWnd);
BOOL freshProgress();
BOOL drawProgressBar(HWND hWnd);
BOOL drawAudioRegin(HWND hWnd);
BOOL InitializeOpenFilename(HWND hWnd);

Logger& initLog();
//Logger& logger = Logger::get(Logger::ROOT);
Logger& logger = initLog();

BOOL CALLBACK MyDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EditDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	//initLog();
	logger.information("start cipPlayer ...");

	TCHAR chBuf[512];
	ZeroMemory(chBuf,512);
	// 取得当前exe的路径
	if(!GetModuleFileName(NULL, chBuf, 512))
	{
		logger.error("Get application root path failture!");
		return 1;
	}
	//PathRemoveFileSpec 函数的作用：将路径末尾的文件名和反斜杠去掉
	PathRemoveFileSpec(chBuf);
	application_path = UtilTool::getInstance()->wchar_t2char(chBuf);
	logger.information("application root path-->%s",std::string(application_path));

	LPWSTR *szArgList;
	int argCount;

	// 初始化全局字符串
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CIPPLAYER, szWindowClass, MAX_LOADSTRING);

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (szArgList == NULL)
	{
		logger.information("Unable to parse command line");
		MessageBox(NULL, L"Unable to parse command line", L"Error", MB_OK);
		return 1;
	}
	logger.information("szArgList is ok");

	if (argCount >= 2) {
		logger.information("arg is ok");
		//sprintf(filePath, "%ws", szArgList[1]);
		//wcscpy(szTitle,szArgList[1]);
		char * tem = UtilTool::getInstance()->wchar_t2char(szArgList[1]);
		std::string argStr = tem;
		//argStr = "ewoicGFyZW50SWQiOiJFWE8tMDAwMDAwMDAwMDAwMDAwMDAwMDAwMTQ1OTUwNCIsCiJwbGF5VXJsIjoiRDpcXFRERE9XTkxPQURcXFJFQUwxNTQ5MTk0NjI2MzM1ODEwIiwKInNlbmRVcmwiOiJodHRwOi8vMTI3LjAuMC4xOjQzMjEvdGVzdCIKfQ==";
		logger.information("original arg-->" + argStr);
		std::string s = UtilTool::getInstance()->base64Decoder(argStr);
		logger.information("decoder arg-->" + s);

		UtilTool::getInstance()->jsonParser(s, intputAttr);
		logger.information("arg struct-->" + intputAttr.parentId + "," + intputAttr.playUrl);

		ZeroMemory(filePath, sizeof(filePath));
		//filePath = (char *)intputAttr.playUrl.c_str();
		std::strcpy(filePath, intputAttr.playUrl.c_str());
		wchar_t * wStr = UtilTool::getInstance()->string2wchar_t(intputAttr.playUrl);
		wcscat(szTitle, L"-");
		wcscat(szTitle, wStr);
		delete[] tem;
		delete[] wStr;
	}

	LocalFree(szArgList);

	//注册mediaclass得回调函数
	//media_instance->callbackFun = media_callback_fucn;

	//初始化公共控件库
	InitCommonControls();

	//注册窗口类
    MyRegisterClass(hInstance);

    // 执行应用程序初始化: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
	logger.information("InitInstance is ok");

	if (!InitializeOpenFilename(hwnd))
	{
		return FALSE;
	}
	logger.information("InitializeOpenFilename is ok");

	if (!sdl_instance->initSDL(mediaWnd))
	{
		return FALSE;
	}
	logger.information("initSDL is ok");
	
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CIPPLAYER));

	// 创建进度条刷新线程
	fresh_progress_hThread = CreateThread(NULL, 0, ThreadProc_freshProgress, NULL, 0, &fresh_progress_threadID);
	logger.information("fresh_progress_hThread is ok");

	if (filePath != NULL && strlen(filePath) > 0)
	{
		logger.information("ThreadProc->" + std::string(filePath));
		media_instance->openMedia_Thread(filePath);
		logger.information("media_hThread is ok");
	}

    MSG msg;
    // 主消息循环: 
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

/**
启动一个freshProgress新的线程
*/
DWORD WINAPI ThreadProc_freshProgress(LPVOID lpParam)
{
	freshProgress();
	return 0;
}

BOOL freshProgress()
{
	while (true)
	{
		InvalidateRect(mainWnd,NULL, FALSE);
		Sleep(100);
	}

	return TRUE;
}


//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CIPPLAYER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CIPPLAYER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   //HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, (HMENU)IDB_WND_ONE, hInstance, nullptr);
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
	   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   mainWnd = hWnd;

   return TRUE;
}


//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
	{
		wm_Create(hWnd, message, wParam, lParam);
		break;
	}
		
	case WM_CTLCOLORSTATIC:
	{
		return wm_Ctlcolorstatic(hWnd, message, wParam, lParam);
	}	
	case WM_NOTIFY:
	{
		wm_Notify(hWnd, message, wParam, lParam);
		break;
	}
	//case WM_RBUTTONUP:
	//{
	//	UINT xPos = LOWORD(lParam);
	//	UINT yPos = HIWORD(lParam);

	//	RECT rc;
	//	GetClientRect(hwndListView, &rc);

	//	POINT pt = { xPos, yPos };
	//	ScreenToClient(hwndListView, &pt);

	//	if (PtInRect(&rc, pt))
	//	{
	//		TrackPopupMenu(hmenu, TPM_LEFTALIGN, xPos, yPos, 0, hwnd, NULL);

	//		return TRUE;
	//	}
	//	else
	//		return FALSE;
	//	//break;
	//}
	//case WM_MOUSEMOVE:
	//{
	//	wm_MouseMove(hWnd, message, wParam, lParam);
	//	break;
	//}
    case WM_COMMAND:
    {
		return wm_Command(hWnd, message, wParam, lParam);
    }
    case WM_PAINT:
    {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		freshPlayInfo(hWnd);
		drawProgressBar(hWnd);
		drawAudioRegin(hWnd);
		break;
    }  
	case WM_SIZE:
	{
		wm_Size(hWnd, message, wParam, lParam);
		break;
	}
    case WM_DESTROY:
	{
		media_instance->stopMedia();
		/*BOOL v1 = media_instance->audioThreadStatus;
		BOOL v2 = media_instance->videoThreadStatus;
		BOOL v3 = media_instance->media_status;*/

		DeleteObject(hpen1);//删除画笔
		DeleteObject(hpen2); 
		DeleteObject(hpen3);
		DeleteObject(hpen_scale);
		DeleteObject(infoWnd_hbrBkgnd);
		DeleteObject(tem_infoWnd_hbrBkgnd);
		DeleteObject(progress_Bkgnd);
		DeleteObject(audioRegin_Bkgnd);
		DeleteObject(hFont);

		sdl_instance->closeSDL();

		delete sdl_instance;
		delete media_instance;
		UtilTool::delInstance();
		logger.information("cipPlayer is closed!");

		/*Poco::Channel *ch = Logger::get(Logger::ROOT).getChannel();
		ch->close();*/
		Logger::get(Logger::ROOT).close();
		Logger::shutdown();
		PostQuitMessage(0);
		
	} 
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void wm_Create(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam)
{
	//创建三个按钮  
	RECT rect1;
	//GetWindowRect(mainWnd, &rect1);
	GetClientRect(hWnd, &rect1);

	//信息展示区
	temInfoWnd = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_LEFT,
		rect1.left, rect1.top, 99, 30, hWnd, (HMENU)IDB_STATIC_TEMINFO, hInst, NULL);

	infoWnd = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_RIGHT,
		rect1.left + 100, rect1.top, rect1.right - rect1.left - 300 - 50, 30, hWnd, (HMENU)IDB_STATIC_INFO, hInst, NULL);

	//视频播放区
	mediaWnd = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD,
		rect1.left + 3, rect1.top + 31, rect1.right - rect1.left - 200 - 50, rect1.bottom - rect1.top - 60 - 30, hWnd, (HMENU)IDB_WND_MEDIA, hInst, NULL);

	//音柱区
	audioWnd = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD,
		rect1.right - rect1.left - 200 - 46, rect1.top + 31, 46, rect1.bottom - rect1.top - 60 - 30, hWnd, (HMENU)IDB_WND_AUDIO, hInst, NULL);

	//进度条控制区
	progressWnd = CreateWindow(L"STATIC", NULL, WS_VISIBLE | WS_CHILD | SS_NOTIFY,
		rect1.left + 3, rect1.bottom - 60 + 3, rect1.right - rect1.left - 200 - 50, 27, hWnd, (HMENU)IDB_WND_PROGRESS, hInst, NULL);

	//表格区
	hwndListView = CreateWindowEx(WS_EX_LEFT,          // ex style
		WC_LISTVIEW,              // class name - defined in commctrl.h
		TEXT("打点信息"),                        // dummy text
		WS_DLGFRAME | WS_CHILD | LVS_AUTOARRANGE | LVS_REPORT,                   // style
		rect1.right - rect1.left - 199, rect1.top, 195, rect1.bottom - rect1.top - 60, hWnd, (HMENU)IDB_WND_TABLE, hInst, NULL);// no extra data

	LV_COLUMN   lvColumn;
	LV_ITEM     lvItem;
	/***添加表头***/
	lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	//lvColumn.cx = 92;//列宽，可以为每个列指定
	// 第一列
	lvColumn.iSubItem = 0;//用于控制添加的顺序，默认从前向后添加
	lvColumn.pszText = (LPWSTR)TEXT("打点名");//列标题
	lvColumn.cx = 70;
	ListView_InsertColumn(hwndListView, 0, &lvColumn);
	// 第二列
	lvColumn.iSubItem = 1;//子项索引
	lvColumn.pszText = (LPWSTR)TEXT("起始时间");
	lvColumn.cx = 60;
	ListView_InsertColumn(hwndListView, 1, &lvColumn);
	// 第三列
	lvColumn.iSubItem = 2;//子项索引
	lvColumn.pszText = (LPWSTR)TEXT("截至时间");
	lvColumn.cx = 60;
	ListView_InsertColumn(hwndListView, 2, &lvColumn);

	//初始化
	//ListView_DeleteAllItems(hwndListView);
	ListView_SetItemCount(hwndListView, 100);//设置项目数，分配内存使用
	ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT);//行全选中并高亮显示

	hmenu = CreatePopupMenu();
	InsertMenu(hmenu, IDB_MENU_UPDATE, MF_BYCOMMAND|MF_STRING, IDB_MENU_UPDATE, L"修改");
	InsertMenu(hmenu, IDB_MENU_DELETE, MF_BYCOMMAND|MF_STRING, IDB_MENU_DELETE, L"删除");
	//InsertMenu(hmenu, -1, MF_BYPOSITION | MF_STRING, IDB_MENU_UPDATE, L"修改");
	//InsertMenu(hmenu, -1, MF_BYPOSITION | MF_STRING, IDB_MENU_DELETE, L"删除");

	//按钮
	playBackWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 5, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_PLAY_BACK, hInst, NULL);
	HBITMAP bitmap_right = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PLAY_BACK));
	SendMessage(playBackWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_right);

	pauseWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 31, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_PAUSE, hInst, NULL);
	HBITMAP bitmap_pause = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PAUSE));
	SendMessage(pauseWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_pause);

	playWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 57, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_PLAY, hInst, NULL);
	HBITMAP bitmap_play = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PLAY));
	SendMessage(playWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_play);
	
	/*modeWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 57, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_MODE, hInst, NULL);
	HBITMAP bitmap_right = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_MODE_RIGHT));
	SendMessage(modeWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_right);*/

	speedBackWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 109, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_SPEED_BACK, hInst, NULL);
	HBITMAP bitmap_speeddown = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_SPEED_BACK));
	SendMessage(speedBackWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_speeddown);

	speedFrameComboWnd = CreateWindow(L"COMBOBOX", NULL, WS_VISIBLE | CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL | CBS_HASSTRINGS,
		rect1.left + 135, rect1.bottom - rect1.top - 60 + 32, 96, 200, hWnd, (HMENU)IDB_BUTTON_COMBOX_SPEED, hInst, NULL);
	TCHAR buf[100] = L"1/4倍速";
	//SendMessage(playFrameComboWnd, CB_ADDSTRING, 0, (LPARAM)buf);
	ComboBox_AddString(speedFrameComboWnd, buf);
	TCHAR buf1[100] = L"1/2倍速";
	ComboBox_AddString(speedFrameComboWnd, buf1);
	TCHAR buf2[100] = L"2倍速";
	ComboBox_AddString(speedFrameComboWnd, buf2);
	TCHAR buf3[100] = L"4倍速";
	ComboBox_AddString(speedFrameComboWnd, buf3);

	int idx = ComboBox_SelectString(speedFrameComboWnd, 0, L"2倍速");

	speedWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 233, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_SPEED, hInst, NULL);
	HBITMAP bitmap_speed = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_SPEED));
	SendMessage(speedWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_speed);


	playBackFrameWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 285, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_PLAYBACKFRAME, hInst, NULL);
	HBITMAP bitmap_play_back_frame = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PLAY_BACK_FRAME));
	SendMessage(playBackFrameWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_play_back_frame);

	playFrameComboWnd = CreateWindow(L"COMBOBOX", NULL, WS_VISIBLE | CBS_DROPDOWNLIST | WS_CHILD | WS_VSCROLL | CBS_HASSTRINGS,
		rect1.left + 311, rect1.bottom - rect1.top - 60 + 32, 96, 200, hWnd, (HMENU)IDB_BUTTON_COMBOX, hInst, NULL);
	TCHAR buf10[100] = L"1帧";
	//SendMessage(playFrameComboWnd, CB_ADDSTRING, 0, (LPARAM)buf);
	ComboBox_AddString(playFrameComboWnd, buf10);
	TCHAR buf11[100] = L"10帧";
	ComboBox_AddString(playFrameComboWnd, buf11);
	TCHAR buf12[100] = L"25帧";
	ComboBox_AddString(playFrameComboWnd, buf12);
	//TCHAR buf3[100] = L"退10秒";
	//ComboBox_AddString(playFrameComboWnd, buf3);
	int idx10 = ComboBox_SelectString(playFrameComboWnd, 0, L"1帧");
	
	playFrameWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 409, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_PLAYFRAME, hInst, NULL);
	HBITMAP bitmap_play_frame = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PLAY_FRAME));
	SendMessage(playFrameWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_play_frame);



	dotLeftWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 461, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_DOT_LEFT, hInst, NULL);
	HBITMAP bitmap_dot_left = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_DOT_LEFT));
	SendMessage(dotLeftWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_dot_left);
	dotRightWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 487, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_DOT_RIGHT, hInst, NULL);
	HBITMAP bitmap_dot_right = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_DOT_RIGHT));
	SendMessage(dotRightWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_dot_right);
	dotAddWnd = CreateWindow(L"BUTTON", NULL, WS_VISIBLE | WS_CHILD | BS_BITMAP,
		rect1.left + 513, rect1.bottom - rect1.top - 60 + 32, 24, 24, hWnd, (HMENU)IDB_BUTTON_DOT_ADD, hInst, NULL);
	HBITMAP bitmap_dot_add = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_DOT_ADD));
	SendMessage(dotAddWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_dot_add);



	//打点完成确认并关闭按钮
	okWnd = CreateWindow(L"BUTTON", TEXT("确认"), WS_VISIBLE | WS_CHILD,
		rect1.right - rect1.left - 80, rect1.bottom - rect1.top - 60 + 32, 72, 24, hWnd, (HMENU)IDB_BUTTON_OK, hInst, NULL);
	//HBITMAP bitmap_ok = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_OK));
	//SendMessage(okWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap_ok);


	//设置逻辑字体
	//LOGFONT lgf;
	//GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lgf), &lgf);
	//lgf.lfWeight = FW_NORMAL;
	//HFONT  hfont_static = CreateFontIndirect(&lgf);
	////设置控件的字体
	//SendMessage(infoWnd, WM_SETFONT, (WPARAM)hfont_static, NULL);

	//创建画笔static资源
	hpen1 = CreatePen(PS_SOLID, 3, RGB(109, 109, 109));
	hpen2 = CreatePen(PS_SOLID, 1, RGB(183, 183, 183)); 
	hpen3 = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
	hpen_scale = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));

	//HDC hdc1 = GetDC(progressWnd);
	//COLORREF crBkgnd1 = GetBkColor(hdc1);
	progress_Bkgnd = CreateSolidBrush(RGB(219, 219, 219));
	//ReleaseDC(progressWnd, hdc1);

	audio_column_Bkgnd = CreateSolidBrush(RGB(166, 166, 166));
	audio_valid_Bkgnd = CreateSolidBrush(RGB(0, 140, 64));

	HDC hdc2 = GetDC(infoWnd);
	COLORREF crBkgnd2 = GetBkColor(hdc2);
	infoWnd_hbrBkgnd = CreateSolidBrush(crBkgnd2);
	ReleaseDC(infoWnd, hdc2);

	HDC hdc3 = GetDC(temInfoWnd);
	COLORREF crBkgnd3 = GetBkColor(hdc3);
	tem_infoWnd_hbrBkgnd = CreateSolidBrush(crBkgnd3);
	ReleaseDC(temInfoWnd, hdc3);

	/*HDC hdc3 = GetDC(audioWnd);
	COLORREF crBkgnd3 = GetBkColor(hdc3);*/
	audioRegin_Bkgnd = CreateSolidBrush(RGB(219, 219, 219));
	//ReleaseDC(audioWnd, hdc3);

	//设置font
	LOGFONT lgf2;
	GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(lgf2), &lgf2);
	memset(&lgf2, 0, sizeof(LOGFONT));
	lgf2.lfHeight =14; // 字体大小
	hFont = CreateFontIndirect(&lgf2);
	SendMessage(okWnd, WM_SETFONT, (WPARAM)hFont, 0); 
	SendMessage(speedFrameComboWnd, WM_SETFONT, (WPARAM)hFont, 0);
	SendMessage(playFrameComboWnd, WM_SETFONT, (WPARAM)hFont, 0);

	if (intputAttr.mode == "1")
	{
		ShowWindow(playBackFrameWnd, SW_HIDE);
		ShowWindow(playFrameComboWnd, SW_HIDE);
		ShowWindow(playFrameWnd, SW_HIDE);

		ShowWindow(hwndListView, SW_HIDE);
		ShowWindow(okWnd, SW_HIDE);
		ShowWindow(dotLeftWnd, SW_HIDE);
		ShowWindow(dotRightWnd, SW_HIDE);
		ShowWindow(dotAddWnd, SW_HIDE);
	}
}

//void wm_MouseMove(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam)
//{
//	POINT curPoint;
//	if (!GetCursorPos(&curPoint))
//		return;
//
//	//char tem[100] = {};
//	//sprintf(tem, "curPoint1->x=%d,y=%d", curPoint.x, curPoint.y);
//	//logger.information(tem);
//	ScreenToClient(progressWnd, &curPoint);
//	//char tem1[100] = {};
//	//sprintf(tem1, "curPoint2->x=%d,y=%d", curPoint.x, curPoint.y);
//	//logger.information(tem1);
//	if (curPoint.x >= 0 && curPoint.y >= 0)
//	{
//		int64_t ct = 0;
//		ct = media_instance->progress_duration_time * curPoint.x / progress_width / 1000;
//		logger.information("this time is = %s", std::to_string(ct));
//	}
//}

LRESULT wm_Ctlcolorstatic(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam)
{
	//printf("hWnd-->%d\n", hWnd);
	//printf("infoWnd-->%d\n", infoWnd);
	//printf("lParam-->%d\n", (HWND)lParam);
	//printf("GetDlgItem-->%d\n", GetDlgItem(hWnd, IDB_STATIC_ONE));
	if ((HWND)lParam == GetDlgItem(hWnd, IDB_STATIC_INFO))//获得指定标签句柄用来对比
	{
		//printf("HELLO\n");
		HDC hdcStatic = (HDC)wParam;
		//SetBkColor(hdcStatic, RGB(0,0,0)); 
		return (INT_PTR)GetStockObject((NULL_BRUSH));//返回一个空画刷(必须)
	}
	else if ((HWND)lParam == GetDlgItem(hWnd, IDB_STATIC_TEMINFO))//获得指定标签句柄用来对比
	{
		//printf("HELLO\n");
		HDC hdcStatic = (HDC)wParam;
		//SetBkColor(hdcStatic, RGB(0,0,0)); 
		return (INT_PTR)GetStockObject((NULL_BRUSH));//返回一个空画刷(必须)
	}
	else if ((HWND)lParam == GetDlgItem(hWnd, IDB_WND_AUDIO))//获得指定标签句柄用来对比
	{
		HDC hdcStatic = (HDC)wParam;
		return (INT_PTR)GetStockObject((NULL_BRUSH));//返回一个空画刷(必须)
	}
	else if ((HWND)lParam == GetDlgItem(hWnd, IDB_WND_PROGRESS))
	{
		HDC hdcStatic = (HDC)wParam;
		//SetBkColor(hdcStatic, RGB(0,0,0)); 
		return (INT_PTR)GetStockObject((NULL_BRUSH));//返回一个空画刷(必须)
	}
	else if ((HWND)lParam == GetDlgItem(hWnd, IDB_WND_MEDIA))//获得指定标签句柄用来对比
	{
		//printf("HELLO\n");
		HDC hdcStatic = (HDC)wParam;
		//SetBkColor(hdcStatic, RGB(0,0,0)); 
		return (INT_PTR)GetStockObject((NULL_BRUSH));//返回一个空画刷(必须)
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void wm_Notify(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam)
{
	int wmId = LOWORD(wParam);
	// 分析菜单选择: 
	switch (wmId)
	{
	case IDB_WND_TABLE:
	{
		if (((LPNMHDR)lParam)->code == NM_RCLICK)
		{
			int iItem = ((LPNMITEMACTIVATE)lParam)->iItem;
			int iSubItem = ((LPNMITEMACTIVATE)lParam)->iSubItem;

			if (iItem >= 0)
			{
				listview_rclick_select_iItem = iItem;

				POINT point;
				GetCursorPos(&point);
				int x = point.x;
				int y = point.y;

				RECT rc;
				GetWindowRect(hwndListView, &rc);

				ScreenToClient(hWnd, &point);
				MapWindowPoints(NULL, hWnd, (LPPOINT)&rc, 2);

				if (PtInRect(&rc, point))
				{
					TrackPopupMenu(hmenu, TPM_LEFTALIGN, x, y, 0, hWnd, NULL);
				}
			}
			else
			{
				listview_rclick_select_iItem = -1;
			}
		}
		break;
	}
	}
}

LRESULT wm_Command(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);
	// 分析菜单选择: 
	switch (wmId)
	{
	case IDM_OPEN:
	{
		// Display the Open dialog box. 
		if (GetOpenFileName(&ofn) == TRUE)
			/*hf = CreateFile(ofn.lpstrFile,
			GENERIC_READ,
			0,
			(LPSECURITY_ATTRIBUTES)NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			(HANDLE)NULL);*/
		{
			//MessageBox(hWnd, szFile, TEXT("HELLO"), MB_OK | MB_ICONINFORMATION);
			char * temPath = UtilTool::getInstance()->wchar_t2char(szFile);
			//printf("filepath-->%s\n", temPath);

			logger.information("filepath-->" + std::string(temPath));
			//修改窗口title
			LoadStringW(hInst, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
			wcscat(szTitle, L"-");
			wcscat(szTitle, szFile);
			SetWindowText(mainWnd, szTitle);

			ZeroMemory(filePath, sizeof(filePath));
			std::strcpy(filePath,temPath);
			media_instance->reOpenMedia_Thread(filePath);
			delete[] temPath;
		}
		break;
	}
	case IDM_ABOUT:
		DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		break;
	case IDM_EXIT:
		DestroyWindow(hWnd);
		break;
	case IDB_WND_PROGRESS:
	{
		POINT curPoint;
		if (!GetCursorPos(&curPoint))
			return -1;

		//char tem[100] = {};
		//sprintf(tem,"curPoint1->x=%d,y=%d", curPoint.x, curPoint.y);
		//logger.information(tem);
		ScreenToClient(progressWnd, &curPoint);
		//char tem1[100] = {};
		//sprintf(tem1,"curPoint2->x=%d,y=%d", curPoint.x, curPoint.y);
		//logger.information(tem1);
		int64_t ct = 0;
		/*if (!strcmp(media_instance->iformat_name, "mpegts"))
		{
			ct = media_instance->filesize * curPoint.x / progress_width;
		}
		else
		{
			ct = media_instance->progress_duration_time * curPoint.x / progress_width;
		}*/
		ct = media_instance->progress_duration_time * ((double)curPoint.x / progress_width);
		//int ct = progress_total_frame_av * curPoint.x / progress_width;

		media_instance->seek(ct);
		break;
	}
	case IDB_BUTTON_PLAY:
		/*if (media_instance->getPlayStatus() == PLAY_STATUS::FRAME)
		{
			HBITMAP bitmap = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PAUSE));
			SendMessage(playWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
			media_instance->play();
		}
		else if (media_instance->getPlayStatus() == PLAY_STATUS::PAUSE)
		{
			HBITMAP bitmap = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PAUSE));
			SendMessage(playWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
			media_instance->play();
		}
		else
		{
			HBITMAP bitmap = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PLAY));
			SendMessage(playWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
			media_instance->pause();
		};*/
		media_instance->play(PLAY_STATUS::PLAY, PLAY_MODE::ASC, 1, 0);
		break;
	case IDB_BUTTON_PAUSE:
	{
		/*if (media_instance->getPlayMode() == PLAY_MODE::ASC)
		{
			HBITMAP bitmap = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_MODE_LEFT));
			SendMessage(modeWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
			media_instance->setPlayMode(PLAY_MODE::DESC);
		}
		else
		{
			HBITMAP bitmap = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_MODE_RIGHT));
			SendMessage(modeWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
			media_instance->setPlayMode(PLAY_MODE::ASC);
		}*/
		media_instance->play(PLAY_STATUS::PAUSE, PLAY_MODE::ASC, 1, 0);
		break;
	}
	case IDB_BUTTON_PLAY_BACK:
	{
		media_instance->play(PLAY_STATUS::PLAY, PLAY_MODE::DESC, 1, 0);
		break;
	}
	case IDB_BUTTON_SPEED_BACK:
	{
		float speedNum = 2;
		int idx = ComboBox_GetCurSel(speedFrameComboWnd);
		if (idx == 0)
		{
			speedNum = 0.25;
		}
		else if (idx == 1)
		{
			speedNum = 0.5;
		}
		else if (idx == 3)
		{
			speedNum = 4;
		}
		else
		{
			speedNum = 2;
		}
		media_instance->play(PLAY_STATUS::PLAY, PLAY_MODE::DESC, speedNum, 0);
		break;
	}
	case IDB_BUTTON_SPEED:
	{
		float speedNum = 2;
		int idx = ComboBox_GetCurSel(speedFrameComboWnd);
		if (idx == 0)
		{
			speedNum = 0.25;
		}
		else if (idx == 1)
		{
			speedNum = 0.5;
		}
		else if (idx == 3)
		{
			speedNum = 4;
		}
		else
		{
			speedNum = 2;
		}
		media_instance->play(PLAY_STATUS::PLAY, PLAY_MODE::ASC, speedNum, 0);
		break;
	}
	case IDB_BUTTON_COMBOX_SPEED:
	{
		if (wmEvent == CBN_SELCHANGE && media_instance->getPlayStatus() == PLAY_STATUS::PLAY && media_instance->getPlaySpeed() != 1)
		{
			float speedNum = 2;
			int idx = ComboBox_GetCurSel(speedFrameComboWnd);
			if (idx == 0)
			{
				speedNum = 0.25;
			}
			else if (idx == 1)
			{
				speedNum = 0.5;
			}
			else if (idx == 3)
			{
				speedNum = 4;
			}
			else
			{
				speedNum = 2;
			}
			media_instance->play(media_instance->getPlayStatus(), media_instance->getPlayMode(), speedNum, 0);
		}
		
		break;
	}
	case IDB_BUTTON_DOT_LEFT:
	{
		//MessageBox(hWnd, TEXT("打点功能暂未实现"), TEXT("HELLO"), MB_OK | MB_ICONINFORMATION);
		if(media_instance->progress_current_time >= media_instance->dot_end_time)
		{
			media_instance->dot_start_frame = media_instance->progress_current_frame_v;
			media_instance->dot_start_time = media_instance->progress_current_time;
			media_instance->dot_end_time = 0;
			media_instance->dot_end_frame = 0;
		}
		else if (media_instance->progress_current_time < media_instance->dot_end_time)
		{
			media_instance->dot_start_frame = media_instance->progress_current_frame_v;
			media_instance->dot_start_time = media_instance->progress_current_time;
		}
		break;
	}
	case IDB_BUTTON_DOT_RIGHT:
	{
		if (media_instance->progress_current_time > media_instance->dot_start_time)
		{
			media_instance->dot_end_frame = media_instance->progress_current_frame_v;
			media_instance->dot_end_time = media_instance->progress_current_time;
		}
		else if (media_instance->progress_current_time < media_instance->dot_start_time)
		{
			media_instance->dot_end_frame = media_instance->progress_current_frame_v;
			media_instance->dot_end_time = media_instance->progress_current_time;
			media_instance->dot_start_frame = 0;
			media_instance->dot_start_time = 0;
		}
		break;
	}
	case IDB_BUTTON_PLAYFRAME:
	{
		/*if (media_instance->getPlayStatus() == PLAY_STATUS::PLAY)
		{
			HBITMAP bitmap = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PLAY));
			SendMessage(playWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
		}*/
		int num = 1;
		int idx = ComboBox_GetCurSel(playFrameComboWnd);
		if (idx == 1)
		{
			//media_instance->increasePlayFrameNum(10);
			num = 10;
		}
		else if (idx == 2)
		{
			num = 25;
		}
		else
		{
			num = 1;
		}
		media_instance->play(PLAY_STATUS::FRAME, PLAY_MODE::ASC, 1,num);
		break;
	}
	case IDB_BUTTON_PLAYBACKFRAME:
	{
		int num = 1;
		int idx = ComboBox_GetCurSel(playFrameComboWnd);
		if (idx == 1)
		{
			//media_instance->increasePlayFrameNum(10);
			num = 10;
		}
		else if (idx == 2)
		{
			num = 25;
		}
		else
		{
			num = 1;
		}
		media_instance->play(PLAY_STATUS::FRAME, PLAY_MODE::DESC, 1, num);

		break;
	}
	case IDB_BUTTON_DOT_ADD:
	{
		//按钮值为无效
		/*EnableWindow(playWnd, FALSE);
		EnableWindow(speedWnd, FALSE);
		EnableWindow(speeddownWnd, FALSE);
		EnableWindow(playFrameWnd, FALSE);
		EnableWindow(dotLeftWnd, FALSE);
		EnableWindow(dotRightWnd, FALSE);*/
		if (media_instance->dot_start_time != 0 && media_instance->dot_end_time != 0)
		{
			//创建对话框，通过EndDialog的第二个参数作为返回值，判断点击了对话框中的控件
			int ret = DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG), mainWnd, MyDialogProc);

		}
		else
		{
			MessageBox(mainWnd, TEXT("打点信息不合法！"), TEXT("提示"), MB_OK | MB_ICONINFORMATION);
		}
		//按钮值为无效
		/*EnableWindow(playWnd, TRUE);
		EnableWindow(speedWnd, TRUE);
		EnableWindow(speeddownWnd, TRUE);
		EnableWindow(playFrameWnd, TRUE);
		EnableWindow(dotLeftWnd, TRUE);
		EnableWindow(dotRightWnd, TRUE);*/
		break;
	}
	case IDB_BUTTON_OK:
	{
		int a = ListView_GetItemCount(hwndListView);
		if (a > 0)
		{
			Poco::JSON::Object jsnObj;
			Poco::JSON::Array jsnArry;
			Poco::JSON::Object subObj1;
			Poco::JSON::Object subObj2;
			jsnObj.set("parentId", intputAttr.parentId);

			wchar_t wstrText[10][3][100] = {};
			for (int i = 0; i < a; i++)
			{
				Poco::JSON::Object subObj1;
				for (int j = 0; j < 3; j++)
				{
					ListView_GetItemText(hwndListView, i, j, wstrText[i][j], sizeof(wstrText[i][j]));
					std::wstring mystr = wstrText[i][j];
					switch (j)
					{
					case 0:
						subObj1.set("name", mystr);
						break;
					case 1:
					{
						std::string frameNum = dotMap[mystr];
						subObj1.set("startFrame", frameNum);
						break;
					}
					case 2:
					{
						std::string frameNum = dotMap[mystr];
						subObj1.set("endFrame", frameNum);
						jsnArry.add(subObj1);
						break;
					}
					default:
						break;
					}
				}
			}
			jsnObj.set("dots", jsnArry);
			std::string mystr = "";
			std::ostringstream ss;
			jsnObj.stringify(ss);
			mystr = ss.str();
			int result = UtilTool::getInstance()->sendHttp(mystr, intputAttr.sendUrl);
			if (result)
			{
				WCHAR buf1[16];
				wsprintfW(buf1, L"%d", result);
				WCHAR wcs[200];
				wcscpy(wcs, L"发送打点信息失败！code=");//这句很重要，否则wcs会是乱码。
				wcscat(wcs, buf1);
				MessageBox(mainWnd, wcs, TEXT("Error"), MB_OK);
			}
			else
			{
				MessageBox(mainWnd, TEXT("打点信息发送成功，将关闭程序！"), TEXT("Info"), MB_OK);
				DestroyWindow(hWnd);
			}
		}
		break;
	}
	case IDB_MENU_UPDATE:
	{
		int ret = DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG), mainWnd, EditDialogProc);
		break;
	}
	case IDB_MENU_DELETE:
	{
		if (listview_rclick_select_iItem >= 0)
		{
			/* 这就成功获取了行和列：iItem为行，iSubItem为列 */
			wchar_t buffer[20] = L"确认删除此打点信息？";
			int ret = MessageBox(hWnd, buffer, TEXT("删除确认"), MB_OKCANCEL);
			if (ret == 1)
			{
				LV_ITEM     lvItem;
				lvItem.iItem = listview_rclick_select_iItem;
				lvItem.iSubItem = 0;
				SendMessage(hwndListView, LVM_DELETEITEM, (WPARAM)listview_rclick_select_iItem,0);
			}
		}
		break;
	}
	default:
		return DefWindowProc(mainWnd, message, wParam, lParam);
	}
	return 0;
}

void wm_Size(HWND &hWnd, UINT &message, WPARAM &wParam, LPARAM &lParam)
{
	int wmId = LOWORD(wParam);
	// 分析菜单选择: 
	switch (wmId)
	{
	case SIZE_RESTORED:
	case SIZE_MAXIMIZED:
		RECT rect2;
		GetClientRect(hWnd, &rect2);
		//printf("progress-->%d\n", rect2.left + 6);
		SetWindowPos(temInfoWnd, HWND_TOP, rect2.left, rect2.top, 99, 30, SWP_SHOWWINDOW);
		SetWindowPos(infoWnd, HWND_TOP, rect2.left + 100, rect2.top, rect2.right - rect2.left - 300 - 50, 30, SWP_SHOWWINDOW);
		SetWindowPos(mediaWnd, HWND_TOP, rect2.left + 3, rect2.top + 31, rect2.right - rect2.left - 200 - 50, rect2.bottom - rect2.top - 60 - 30, SWP_SHOWWINDOW);
		SetWindowPos(audioWnd, HWND_TOP, rect2.right - rect2.left - 200 - 46, rect2.top + 31, 46, rect2.bottom - rect2.top - 60 - 30, SWP_SHOWWINDOW);
		SetWindowPos(progressWnd, HWND_TOP, rect2.left + 3, rect2.bottom - 60 + 3, rect2.right - rect2.left - 200 - 50, 27, SWP_SHOWWINDOW);
		SetWindowPos(playBackWnd, HWND_TOP, rect2.left + 5, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
		SetWindowPos(pauseWnd, HWND_TOP, rect2.left + 31, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
		SetWindowPos(playWnd, HWND_TOP, rect2.left + 57, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
		SetWindowPos(speedBackWnd, HWND_TOP, rect2.left + 109, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
		SetWindowPos(speedFrameComboWnd, HWND_TOP, rect2.left + 135, rect2.bottom - rect2.top - 60 + 32, 96, 200, SWP_SHOWWINDOW);
		SetWindowPos(speedWnd, HWND_TOP, rect2.left + 233, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
		SetWindowPos(playBackFrameWnd, HWND_TOP, rect2.left + 285, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
		SetWindowPos(playFrameComboWnd, HWND_TOP, rect2.left + 311, rect2.bottom - rect2.top - 60 + 32, 96, 200, SWP_SHOWWINDOW);
		SetWindowPos(playFrameWnd, HWND_TOP, rect2.left + 409, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
		if (intputAttr.mode != "1")
		{
			//SetWindowPos(dotWnd, HWND_TOP, rect2.left + 264, rect2.bottom - rect2.top - 60 + 15, 32, 32, SWP_SHOWWINDOW);
			SetWindowPos(dotLeftWnd, HWND_TOP, rect2.left + 461, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
			SetWindowPos(dotRightWnd, HWND_TOP, rect2.left + 487, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
			SetWindowPos(dotAddWnd, HWND_TOP, rect2.left + 513, rect2.bottom - rect2.top - 60 + 32, 24, 24, SWP_SHOWWINDOW);
			//SetWindowPos(dotDelWnd, HWND_TOP, rect2.left + 239, rect2.bottom - rect2.top - 60 + 15, 32, 32, SWP_SHOWWINDOW);
			SetWindowPos(okWnd, HWND_TOP, rect2.right - rect2.left - 80, rect2.bottom - rect2.top - 60 + 32, 72, 24, SWP_SHOWWINDOW);
			SetWindowPos(hwndListView, HWND_TOP, rect2.right - rect2.left - 199, rect2.top, 195, rect2.bottom - rect2.top - 60, SWP_SHOWWINDOW);

		}
		freshPlayInfo(hWnd);
		drawProgressBar(hWnd);
		drawAudioRegin(hWnd);
		break;
	}
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

/*
BOOL createButton()
{
	HICON hicon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_PLAY));
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(toolWnd, &ps);
	DrawIcon(hdc, 15, 15, hicon);
	EndPaint(toolWnd, &ps);

	return TRUE;
}*/

BOOL freshPlayInfo(HWND hWnd)
{
	WCHAR buf1[30];
	int tem_current_time = media_instance->progress_current_time / 1000;
	//wsprintfW(buf1, L"%d", media_instance->progress_current_time / 1000);
	swprintf(buf1,30,L"%.2d:%.2d:%.2d", tem_current_time / 3600, (tem_current_time - (tem_current_time / 3600 * 3600)) / 60, tem_current_time - (tem_current_time / 3600 * 3600) - (tem_current_time - (tem_current_time / 3600 * 3600)) / 60 * 60);
	WCHAR buf2[30];
	int tem_duration_time = media_instance->progress_duration_time / 1000;
	//wsprintfW(buf2, L"%d", media_instance->progress_duration_time / 1000);
	swprintf(buf2, 30, L"%.2d:%.2d:%.2d", tem_duration_time / 3600, (tem_duration_time - (tem_duration_time / 3600 * 3600)) / 60, tem_duration_time - (tem_duration_time / 3600 * 3600) - (tem_duration_time - (tem_duration_time / 3600 * 3600)) / 60 * 60);
	WCHAR buf3[32];
	wsprintfW(buf3, L"%d", media_instance->progress_current_frame_v);
	/*WCHAR buf4[32];
	wsprintfW(buf4, L"%d", media_instance->progress_total_frame_v);*/
	WCHAR buf5[32];
	wsprintfW(buf5, L"%I64d", media_instance->progress_current_pts);
	WCHAR buf6[32];
	//wsprintfW(buf6, L"%2f", speedNum);
	float tem = media_instance->getPlaySpeed();
	int tem1 = media_instance->getPlaySpeed() / 1;
	int tem2 = 1 / media_instance->getPlaySpeed();
	if (media_instance->getPlaySpeed() < 1 && media_instance->getPlaySpeed() > 0)
	{
		swprintf(buf6, L"1/%d", (int)(1 / media_instance->getPlaySpeed()));
	}
	else 
	{
		swprintf(buf6, L"%d", (int)media_instance->getPlaySpeed());
	}


	WCHAR wcs[200];
	wcscpy(wcs, L"frame:");//这句很重要，否则wcs会是乱码。
	wcscat(wcs, buf3);
	/*wcscat(wcs, L"/");
	wcscat(wcs, buf4);*/
	wcscat(wcs, L"   time:");
	wcscat(wcs, buf1); 
	wcscat(wcs, L"/");
	wcscat(wcs, buf2);
	wcscat(wcs, L"   pts:");
	wcscat(wcs, buf5);
	wcscat(wcs, L"   speed:");
	wcscat(wcs, buf6);
	//strcpy(szTitle, info.c_str());
	//SetWindowText(infoWnd, wcs);
	//SendMessage(infoWnd, WM_SETTEXT, NULL, (LPARAM)&wcs); 
	////SetWindowText和SendMessage方式都有二次字符比之前短时覆盖不全的问题,可以通过设置隐藏再显示的方式解决,
	////但对于频繁更新会有闪屏问题,所以对于高频度刷新建议采用TextOut/DrawText的方式
	//ShowWindow(infoWnd,SW_HIDE);
	//ShowWindow(infoWnd,SW_SHOW);
	

	RECT rect1;
	GetClientRect(infoWnd, &rect1);

	LOGFONT logfont; //改变输出字体
	ZeroMemory(&logfont, sizeof(LOGFONT));
	//logfont.lfCharSet = GB2312_CHARSET;
	logfont.lfHeight = 14; //设置字体的大小
	HFONT hFont = CreateFontIndirect(&logfont);

	HDC hdc = GetDC(infoWnd);
	SetTextColor(hdc,RGB(0,0,0));
	//SetBkColor(hdc,RGB(200,200,0));
	SetBkMode(hdc,TRANSPARENT);
	SelectObject(hdc, hFont);
	
	FillRect(hdc, &rect1, infoWnd_hbrBkgnd);
	size_t lenChar=wcslen(wcs);
	//TextOut(hdc,0,0,wcs,lenChar);
	DrawText(hdc,wcs,lenChar,&rect1,DT_SINGLELINE|DT_RIGHT|DT_VCENTER);


	//RECT rect2;
	//GetClientRect(temInfoWnd, &rect2);

	//HDC hdc0 = GetDC(temInfoWnd);
	//SetTextColor(hdc0, RGB(250, 0, 0));
	//SetBkMode(hdc0, TRANSPARENT);
	//SelectObject(hdc0, hFont);
	//FillRect(hdc0, &rect2, tem_infoWnd_hbrBkgnd);

	//POINT curPoint;
	//if (GetCursorPos(&curPoint))
	//{
	//	//char tem[100] = {};
	//	//sprintf(tem, "curPoint1->x=%d,y=%d", curPoint.x, curPoint.y);
	//	//logger.information(tem);
	//	ScreenToClient(progressWnd, &curPoint);
	//	//sprintf(tem, "curPoint2->x=%d,y=%d", curPoint.x, curPoint.y);
	//	//logger.information(tem);
	//	if (curPoint.x >= 0 && curPoint.y >= 0 && media_instance->getMediaStatus() == MEDIA_STATUS::RUNNING)
	//	{
	//		int tem_time = media_instance->progress_duration_time * ((double)curPoint.x / progress_width) / 1000;
	//		WCHAR wcs[50] = {};
	//		swprintf(wcs, 50, L" %.2d:%.2d:%.2d", tem_time / 3600, (tem_time - (tem_time / 3600 * 3600)) / 60, tem_time - (tem_time / 3600 * 3600) - (tem_time - (tem_time / 3600 * 3600)) / 60 * 60);
	//		size_t lenChar = wcslen(wcs);
	//		DrawText(hdc0, wcs, lenChar, &rect2, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
	//	}
	//}	

	//ReleaseDC(temInfoWnd, hdc0);
	ReleaseDC(infoWnd, hdc);
	DeleteObject(hFont);

	return TRUE;
}

BOOL drawProgressBar(HWND hWnd)
{
	RECT rect1;
	GetClientRect(progressWnd, &rect1);

	//PAINTSTRUCT ps1;
	//HDC hdc1 = BeginPaint(progressWnd, &ps1);
	HDC hdc1 = GetDC(progressWnd);

	int progress_y = 23;//进度条的y坐标
	progress_width = rect1.right;//进度条总宽度(不包含0点)
	int progress1_width = progress_width * ((double)media_instance->progress_current_time / media_instance->progress_duration_time);//进度条(已播放)宽度(不包含0点)
	int dotstart = progress_width * ((double)media_instance->dot_start_time / media_instance->progress_duration_time);
	int dotend = progress_width * ((double)media_instance->dot_end_time / media_instance->progress_duration_time);

	int progress1_x = 0;//进度条(已播放)起始x坐标
	int progress1_x1 = progress1_x + progress1_width;//进度条(已播放)结束x坐标

	//HPEN hpen1; // 创建画笔-left
	//hpen1 = CreatePen(PS_SOLID, 3, RGB(109, 109, 109));
	//HPEN hpen2; // 创建画笔-right
	//hpen2 = CreatePen(PS_SOLID, 1, RGB(183, 183, 183));
	//HPEN hpen3; // 创建画笔-dot
	//hpen3 = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));

	//擦除progress区域
	//COLORREF crBkgnd = GetBkColor(hdc1);
	//HBRUSH hbrBkgnd = CreateSolidBrush(crBkgnd);
	FillRect(hdc1, &rect1, progress_Bkgnd);

	//绘制总进度条
	//DC选择画笔
	SelectObject(hdc1, hpen2);
	//MoveToEx(hdc1, progress1_x, progress_y + 6, NULL);
	//LineTo(hdc1, progress1_x + 5, progress_y + 6);
	// (画笔)移动
	MoveToEx(hdc1, progress1_x, progress_y, NULL);
	// (画笔)画线
	LineTo(hdc1, progress1_x + progress_width, progress_y);

	//绘制已播放进度条
	// DC 选择画笔
	SelectObject(hdc1, hpen1);
	// (画笔)移动
	MoveToEx(hdc1, progress1_x, progress_y, NULL);
	// (画笔)画线
	LineTo(hdc1, progress1_x1, progress_y);

	//绘制打点点位
	if (media_instance->dot_start_time != 0)
	{
		// DC 选择画笔
		SelectObject(hdc1, hpen3);
		// (画笔)移动
		MoveToEx(hdc1, progress1_x + dotstart, progress_y - 3, NULL);
		// (画笔)画线
		LineTo(hdc1, progress1_x + dotstart, progress_y + 3);
	}
	if (media_instance->dot_end_time != 0)
	{
		// DC 选择画笔
		SelectObject(hdc1, hpen3);
		// (画笔)移动
		MoveToEx(hdc1, progress1_x + dotend, progress_y - 3, NULL);
		// (画笔)画线
		LineTo(hdc1, progress1_x + dotend, progress_y + 3);
	}
	//EndPaint(progressWnd, &ps1);

	//绘制标识
	//todo 判断标识最小时间间隔，单位s
	int min_scale = 0, scale_group = 0;
	int total_time = media_instance->progress_duration_time / 1000;
	float tem = 1.0 * progress_width / total_time;
	if (tem >= 5)
	{
		min_scale = 1;
		scale_group = 10;
	}
	else if(tem < 5 && tem >= 2)
	{
		min_scale = 2;
		scale_group = 10;
	}
	else if (tem < 2 && tem >= 1)
	{
		min_scale = 5;
		scale_group = 10;
	}
	else if (tem < 1 && tem >= 0.5)
	{
		min_scale = 10;
		scale_group = 10;
	}
	else if (tem < 0.5 && tem >= 0.2)
	{
		min_scale = 20;
		scale_group = 10;
	}
	else if (tem < 0.2 && tem >= 0.1)
	{
		min_scale = 50;
		scale_group = 10;
	}
	else if (tem < 0.1 && tem >= 0.05)
	{
		min_scale = 100;
		scale_group = 10;
	}
	else if (tem < 0.05 && tem >= 0.025)
	{
		min_scale = 200;
		scale_group = 10;
	}
	else if (tem < 0.025 && tem >= 0.0125)
	{
		min_scale = 500;
		scale_group = 10;
	}
	//todo 按最小间隔绘制刻度
	int tem_time = 0;
	int tem_group = 0;
	RECT rect2;

	//设置时间font
	LOGFONT logfont; //改变输出字体
	ZeroMemory(&logfont, sizeof(LOGFONT));
	//logfont.lfCharSet = GB2312_CHARSET;
	logfont.lfHeight = 10; //设置字体的大小
	HFONT hFont = CreateFontIndirect(&logfont);
	// DC 选择画笔
	SelectObject(hdc1, hpen_scale);
	MoveToEx(hdc1, 0, 0, NULL);
	LineTo(hdc1, progress_width, 0);
	while (tem_time < total_time)
	{
		int x = progress_width * ((double)tem_time / total_time);
		// (画笔)移动
		MoveToEx(hdc1, x, 0, NULL);
		// (画笔)画线
		if (tem_group < scale_group)
		{
			LineTo(hdc1, x, 5);
			tem_group++;
		}
		else
		{
			LineTo(hdc1, x, 10);
			tem_group = 0;

			//显示出时间
			SelectObject(hdc1, hFont);
			rect2.left = x - 50;
			rect2.right = x + 50;
			rect2.top = 12;
			rect2.bottom = 18;
			WCHAR wcs[200] = {};
			swprintf(wcs,200,L"%.2d:%.2d:%.2d", tem_time / 3600, (tem_time - (tem_time / 3600 * 3600)) / 60, tem_time - (tem_time / 3600 * 3600) - (tem_time - (tem_time / 3600 * 3600)) / 60 * 60);
			size_t lenChar = wcslen(wcs);
			DrawText(hdc1, wcs, lenChar, &rect2, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		}
		
		tem_time += min_scale;
	}


	//绘制鼠标追踪
	POINT curPoint;
	if (!GetCursorPos(&curPoint))
	{
		ReleaseDC(progressWnd, hdc1);
		DeleteObject(hFont);
		return TRUE;
	}
	else
	{
		//RECT rect2;
		//GetClientRect(temInfoWnd, &rect2);

		//HDC hdc0 = GetDC(temInfoWnd);
		//SetTextColor(hdc0, RGB(250, 0, 0));
		//SetBkMode(hdc0, TRANSPARENT);
		//SelectObject(hdc0, hFont);
		//FillRect(hdc0, &rect2, tem_infoWnd_hbrBkgnd);


		ScreenToClient(progressWnd, &curPoint);
		if (curPoint.x >= 0 && curPoint.y >= 0 && curPoint.x <= rect1.right && curPoint.y <= rect1.bottom)
		{
			// DC 选择画笔
			SelectObject(hdc1, hpen_scale);
			// (画笔)移动
			MoveToEx(hdc1, curPoint.x, 0, NULL);
			// (画笔)画线
			LineTo(hdc1, curPoint.x, rect1.bottom - 7);

			int tem_time = media_instance->progress_duration_time * ((double)curPoint.x / progress_width) / 1000;
			WCHAR wcs[50] = {};
			swprintf(wcs, 50, L" %.2d:%.2d:%.2d", tem_time / 3600, (tem_time - (tem_time / 3600 * 3600)) / 60, tem_time - (tem_time / 3600 * 3600) - (tem_time - (tem_time / 3600 * 3600)) / 60 * 60);
			size_t lenChar = wcslen(wcs);

			LOGFONT logfont; //改变输出字体
			ZeroMemory(&logfont, sizeof(LOGFONT));
			//logfont.lfCharSet = GB2312_CHARSET;
			logfont.lfHeight = 14; //设置字体的大小
			logfont.lfWeight = FW_BOLD;
			HFONT hFont = CreateFontIndirect(&logfont);
			SetTextColor(hdc1, RGB(250, 0, 0));
			SelectObject(hdc1, hFont);
			if (curPoint.x > 50)
			{
				rect2.left = curPoint.x - 50;
				rect2.right = curPoint.x;
			}
			else
			{
				rect2.left = curPoint.x;
				rect2.right = curPoint.x + 50;
			}

			if (curPoint.y > 10)
			{

				rect2.top = curPoint.y - 10;
				rect2.bottom = curPoint.y;
			}
			else
			{
				rect2.top = curPoint.y;
				rect2.bottom = curPoint.y + 10;
			}

			DrawText(hdc1, wcs, lenChar, &rect2, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
		}

		ReleaseDC(progressWnd, hdc1);
		//ReleaseDC(temInfoWnd, hdc0);
		DeleteObject(hFont);
		return TRUE;
	}
}

BOOL drawAudioRegin(HWND hWnd)
{
	RECT rect1;
	GetClientRect(audioWnd, &rect1);

	HDC hdc1 = GetDC(audioWnd);
	FillRect(hdc1, &rect1, audioRegin_Bkgnd);

	//计算dB数值
	int max_db = (int) 20.0 * log10(32767);
	int left_db = 0;
	int right_db = 0;
	if (media_instance->left_audio > 0)
	{
		left_db = (int) 20.0 * log10(media_instance->left_audio);
	}
	if (media_instance->right_audio > 0)
	{
		right_db = (int) 20.0 * log10(media_instance->right_audio);
	}
	

	RECT rect2;
	rect2.left = rect1.left + 5;
	rect2.top = rect1.top;
	rect2.bottom = rect1.bottom;
	rect2.right = rect1.left + 20;
	FillRect(hdc1, &rect2, audio_column_Bkgnd);
	rect2.top = rect2.bottom - (rect2.bottom * left_db / max_db);
	FillRect(hdc1, &rect2, audio_valid_Bkgnd);

	RECT rect3;
	rect3.left = rect1.right - 20;
	rect3.top = rect1.top;
	rect3.bottom = rect1.bottom;
	rect3.right = rect1.right - 5;
	FillRect(hdc1, &rect3, audio_column_Bkgnd);
	rect3.top = rect3.bottom - (rect3.bottom * right_db / max_db);
	FillRect(hdc1, &rect3, audio_valid_Bkgnd);

	ReleaseDC(audioWnd, hdc1);
	return TRUE;
}

/*
dotAddWnd的dialog关联Proc
*/
BOOL CALLBACK MyDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SYSCOMMAND:
		if (SC_CLOSE == wParam)
		{
			EndDialog(hwndDlg, 1);
		}
		break;
	case WM_INITDIALOG:
		//MessageBox(NULL, _T("对话框创建完成，显示之前调用"), _T("提示"), MB_OK);
		break;
	case WM_COMMAND:		//按钮发送的是WM_COMMAND消息
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			//关闭模式对话框
			int itemCount = ListView_GetItemCount(hwndListView);
			WCHAR wcs_start[200] = {};
			UtilTool::getInstance()->convertSecToWTime(media_instance->dot_start_time / 1000, wcs_start,200);
			WCHAR wcs_end[200] = {};
			UtilTool::getInstance()->convertSecToWTime(media_instance->dot_end_time / 1000, wcs_end, 200);

			//dotMap.insert(std::pair<std::wstring, std::string>(wcs_start, std::to_string(media_instance->dot_start_frame)));
			//dotMap.insert(std::pair<std::wstring, std::string>(wcs_end, std::to_string(media_instance->dot_end_frame)));
			dotMap[wcs_start] = std::to_string(media_instance->dot_start_frame);
			dotMap[wcs_end] = std::to_string(media_instance->dot_end_frame);
			//wchar_t dot_start_frame_str[10];
			//swprintf(dot_start_frame_str, L"%d", media_instance->dot_start_frame);
			//wchar_t dot_end_frame_str[10];
			//swprintf(dot_end_frame_str, L"%d", media_instance->dot_end_frame);

			wchar_t outBuffer[512];
			GetDlgItemText(hwndDlg, IDC_EDIT, (LPTSTR)outBuffer, 512);

			LV_ITEM     lvItem;
			lvItem.mask = LVIF_TEXT;       // 文字
			lvItem.cchTextMax = MAX_PATH;       // 文字长度
			lvItem.iItem = itemCount;
			lvItem.iSubItem = 0;
			lvItem.pszText = outBuffer;
			SendMessage(hwndListView, LVM_INSERTITEM, 0, (LPARAM)&lvItem);
			lvItem.iSubItem = 1;
			lvItem.pszText = wcs_start;
			SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);
			lvItem.iSubItem = 2;
			lvItem.pszText = wcs_end;
			SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);

			EndDialog(hwndDlg, 2);
			break;
		}
		case IDCANCEL:
			//关闭模式对话框
			EndDialog(hwndDlg, 3);
			break;
		}
	}
	}

	return FALSE;
}

/*
dotAddWnd的dialog关联Proc
*/
BOOL CALLBACK EditDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SYSCOMMAND:
		if (SC_CLOSE == wParam)
		{
			EndDialog(hwndDlg, 1);
		}
		break;
	case WM_INITDIALOG:
	{
		wchar_t outBuffer[512];
		ListView_GetItemText(hwndListView, listview_rclick_select_iItem, 0, outBuffer, sizeof(outBuffer));
		SetDlgItemText(hwndDlg, IDC_EDIT, outBuffer);
		break;
	}
	case WM_COMMAND:		//按钮发送的是WM_COMMAND消息
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			//关闭模式对话框
			//int itemCount = ListView_GetItemCount(hwndListView);

			wchar_t outBuffer[512];
			GetDlgItemText(hwndDlg, IDC_EDIT, (LPTSTR)outBuffer, 512);

			LV_ITEM     lvItem;
			lvItem.mask = LVIF_TEXT;       // 文字
			lvItem.cchTextMax = MAX_PATH;       // 文字长度
			lvItem.iItem = listview_rclick_select_iItem;
			lvItem.iSubItem = 0;
			lvItem.pszText = outBuffer;
			SendMessage(hwndListView, LVM_SETITEM, 0, (LPARAM)&lvItem);

			EndDialog(hwndDlg, 2);
			break;
		}
		case IDCANCEL:
			//关闭模式对话框
			EndDialog(hwndDlg, 3);
			break;
		}
	}
	}

	return FALSE;
}

BOOL InitializeOpenFilename(HWND hWnd)
{
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"All\0*.*\0ts\0*.ts\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	return TRUE;
}


Logger& initLog()
{
	Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> pConf = new Poco::Util::PropertyFileConfiguration("log_conf.properties");
	Poco::Util::LoggingConfigurator log_configurator;
	log_configurator.configure(pConf);
	/*
	AutoPtr<SimpleFileChannel> pChannel(new SimpleFileChannel);
	pChannel->setProperty("path", "test.log");
	pChannel->setProperty("rotation", "never");
	AutoPtr<PatternFormatter> pPF(new PatternFormatter);
	pPF->setProperty("pattern", "%Y-%m-%d %H:%M:%S %s: %t");
	AutoPtr<FormattingChannel> pFC(new FormattingChannel(pPF, pChannel));
	Logger::root().setChannel(pFC);*/

	//return Logger::get("cipPlayer");
	return Logger::get(Logger::ROOT);
}

//void media_callback_fucn(int eventCode)
//{
//	if (eventCode == 0)
//	{
//		logger.information("the file is end!!!");
//		HBITMAP bitmap = LoadBitmapW(hInst, MAKEINTRESOURCE(IDB_PLAY));
//		SendMessage(playWnd, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bitmap);
//		media_instance->pause();
//		ShowWindow(speedWnd, SW_HIDE);
//		ShowWindow(speedFrameComboWnd, SW_HIDE);
//		ShowWindow(speedBackWnd, SW_HIDE);
//
//		ShowWindow(playFrameWnd, SW_HIDE); 
//		ShowWindow(playFrameComboWnd, SW_HIDE);
//		ShowWindow(playBackFrameWnd, SW_HIDE);
//
//		ShowWindow(dotLeftWnd, SW_HIDE);
//		ShowWindow(dotRightWnd, SW_HIDE);
//		ShowWindow(dotAddWnd, SW_HIDE);
//	}
//	else if (eventCode == 1)
//	{
//		logger.information("eventCode=%d", eventCode);
//		ShowWindow(speedWnd, SW_SHOW);
//		ShowWindow(speedFrameComboWnd, SW_SHOW);
//		ShowWindow(speedBackWnd, SW_SHOW);
//
//		if (intputAttr.mode != "1")
//		{
//			ShowWindow(playFrameWnd, SW_SHOW);
//			ShowWindow(playFrameComboWnd, SW_SHOW);
//			ShowWindow(playBackFrameWnd, SW_SHOW);
//
//			ShowWindow(dotLeftWnd, SW_SHOW);
//			ShowWindow(dotRightWnd, SW_SHOW);
//			ShowWindow(dotAddWnd, SW_SHOW);
//		}
//	}
//	
//}

