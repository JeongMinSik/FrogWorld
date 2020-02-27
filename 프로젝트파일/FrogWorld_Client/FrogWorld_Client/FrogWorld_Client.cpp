// FrogWorld_Client.cpp : 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "FrogWorld_Client.h"


#define MAX_LOADSTRING 100

/////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "Player.h"
#include "NetworkManager.h"

RECT				g_clientRect;
map<UINT, Player*>	g_players;
NetworkManager		g_networkManager;

HBITMAP				g_frogBmp;
BITMAP				g_frogBmpInfo;
HBITMAP				g_attackBmp;
BITMAP				g_attackBmpInfo;
HBITMAP				g_herbBmp;
BITMAP				g_herbBmpInfo;

HBITMAP				g_itemBmp[MAX_ITEM_TYPE];
BITMAP				g_itemBmpInfo[MAX_ITEM_TYPE];

HBITMAP				g_npcBmp[MAX_NPC_TYPE];
BITMAP				g_npcBmpInfo[MAX_NPC_TYPE];

enum GRASS_TYPE {GRASS, BEIGE, MUD, BUSH, FLOWER, HILL };
#define				MAX_GRASS_TYPE		(6)
HBITMAP				g_grassBmp[MAX_GRASS_TYPE];
BITMAP				g_grassBmpInfo[MAX_GRASS_TYPE];

int					g_grassMap[BOARD_COL][BOARD_ROW];

#define	CLIENT_BOARD_COL	14
#define	CLIENT_BOARD_ROW	10

#define ID_EDIT_NAME 100
#define ID_EDIT_STORY 101
#define ID_BUTTON_ENTER 102
HWND hChatName, hChatEdit, hChatEnter, hChatBoard;
const int gItemBoxWidth = 300;
const int gNameWidth = 300;
const int gEnterWidth = 100;
const int gEditHeight = 20;
const int gBoardHeight = gEditHeight *(MAX_CHAT_LINE -1) + 5;

WNDPROC origLVEditWndProc = NULL;

bool operator == (const POINT& lhs, const POINT& rhs)
{
	return (lhs.x == rhs.x && lhs.y == rhs.y);
}

void DrawClient(HWND hWnd);

/////////////////////////////////////////////////////////////////////////////////////////////////////////



// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.


LRESULT CALLBACK LVEditWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CHAR:
		switch (wParam)
		{
		case 27:
		{
			// edit is cancelled, so no LVN_ENDLABELEDIT is sent
			DestroyWindow(hWnd);
			break;
		}break;
		}
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_RETURN:
			SendMessage(hChatEnter, WM_COMMAND, MAKEWORD(BN_CLICKED, ID_BUTTON_ENTER), (LPARAM)0);
			break;
		}
	}
	return CallWindowProc(origLVEditWndProc, hWnd, uMsg, wParam, lParam);
	}
	return true;
}

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE hPrevInstance,_In_ LPWSTR lpCmdLine,_In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_FrogWorld_Client, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 응용 프로그램 초기화를 수행합니다.
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FrogWorld_Client));

    MSG msg;

    // 기본 메시지 루프입니다.
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

#ifdef _DEBUG
	FreeConsole();
#endif

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FrogWorld_Client));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_FrogWorld_Client);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
#ifdef _DEBUG
	AllocConsole();
	FILE*	pStreamOut;
	FILE*	pStreamIn;

	system("mode con:cols=60 lines=15");
	system("title Debug Console");
	freopen_s(&pStreamOut, "CONOUT$", "wt", stdout);
	freopen_s(&pStreamIn, "CONIN$", "rt", stdin);
#endif

   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

INT_PTR CALLBACK IpCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS1), IPM_SETADDRESS, 0, MAKEIPADDRESS(127,0,0,1));
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		switch (wParam) 
		{
		case IDOK:
		{
			DWORD address;
			SendMessage(GetDlgItem(hDlg, IDC_IPADDRESS1), IPM_GETADDRESS, 0, (LPARAM)&address);
			BOOL success = false;
			int id = GetDlgItemInt(hDlg, IDC_EDIT_ID, &success, TRUE);
			printf("input db id:%d \n", id);
			g_networkManager.set_id_ipAddress(id, address);
		}
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
	{
		DialogBox(hInst, MAKEINTRESOURCE(IDD_IP), hWnd, IpCallback);

		// 리소스 설정
		g_networkManager.setPlayer(&g_players);
		g_frogBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FROG));
		GetObject(g_frogBmp, sizeof(BITMAP), &g_frogBmpInfo);
		g_attackBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ATTACK));
		GetObject(g_attackBmp, sizeof(BITMAP), &g_attackBmpInfo);
		g_herbBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_HERB));
		GetObject(g_herbBmp, sizeof(BITMAP), &g_herbBmpInfo);

		// 리소스 - NPC
		g_npcBmp[NPC_BLACK_TREE] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BLACK_TREE));
		g_npcBmp[NPC_RED_TREE] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RED_TREE));
		g_npcBmp[NPC_PIG] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PIG));
		g_npcBmp[NPC_CRO] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CRO));

		for (int i = 0; i < MAX_NPC_TYPE; ++i)
		{
			GetObject(g_npcBmp[i], sizeof(BITMAP), &g_npcBmpInfo[i]);
		}

		// 리소스 - 아이템
		g_itemBmp[ITEM_RED_POTION] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RED_POTION));
		g_itemBmp[ITEM_BLUE_POTION] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BLUE_POTION));
		g_itemBmp[ITEM_GREEN_POTION] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GREEN_POTION));

		for (int i = 0; i < MAX_ITEM_TYPE; ++i)
		{
			GetObject(g_itemBmp[i], sizeof(BITMAP), &g_itemBmpInfo[i]);
		}

		// 리소스 - 타일맵
		g_grassBmp[GRASS_TYPE::GRASS] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_GRASS));
		g_grassBmp[GRASS_TYPE::BEIGE] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_G_BEIGE));
		g_grassBmp[GRASS_TYPE::MUD] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_G_MUD));
		g_grassBmp[GRASS_TYPE::BUSH] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_G_BUSH));
		g_grassBmp[GRASS_TYPE::FLOWER] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_G_FLOWER));
		g_grassBmp[GRASS_TYPE::HILL] = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_G_HILL));

		for (int i = 0; i < MAX_GRASS_TYPE; ++i)
		{
			GetObject(g_grassBmp[i], sizeof(BITMAP), &g_grassBmpInfo[i]);
		}

		// 맵 로딩
		ifstream fin("grassMap.txt");
		for (int r = 0; r < BOARD_ROW; ++r)
		{
			for (int c = 0; c < BOARD_COL; ++c)
			{
				fin >> g_grassMap[c][r];
			}
		}
		fin.close();
		printf("load grassMap success! \n");

		// 채팅창
		GetClientRect(hWnd, &g_clientRect);
		
		hChatName = CreateWindow(L"static", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
			0, g_clientRect.bottom - gEditHeight, gNameWidth, gEditHeight, hWnd, (HMENU)ID_EDIT_NAME, NULL, NULL);
		hChatEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
			gNameWidth, g_clientRect.bottom- gEditHeight, g_clientRect.right - g_clientRect.left - gNameWidth - gEnterWidth - gItemBoxWidth, gEditHeight, hWnd, (HMENU)ID_EDIT_STORY, NULL, NULL);
		hChatEnter = CreateWindow(L"button", L"확인", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			g_clientRect.right - gEnterWidth - gItemBoxWidth, g_clientRect.bottom - gEditHeight, gEnterWidth, gEditHeight, hWnd, (HMENU)ID_BUTTON_ENTER, NULL, NULL);
		hChatBoard = CreateWindow(L"static", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
			0, g_clientRect.bottom - gEditHeight - gBoardHeight, (g_clientRect.right - g_clientRect.left - gItemBoxWidth), gBoardHeight, hWnd, NULL, NULL, NULL);
		
		// origLVEditWndProc = (WNDPROC)SetWindowLongPtr(hChatEdit, GWLP_WNDPROC, (LONG_PTR)LVEditWndProc);

		// 서버 접속
		if (g_networkManager.connectServer(hWnd, hChatName, hChatBoard) == false)
		{
			MessageBox(hWnd, L"Network Connect Error!", szWindowClass, MB_OK);
			exit(0);
		}
		break;
	}
	case WM_SIZE:
	{
		GetClientRect(hWnd, &g_clientRect);
		MoveWindow(hChatName, 0, g_clientRect.bottom - gEditHeight, gNameWidth, gEditHeight, TRUE);
		MoveWindow(hChatEdit, gNameWidth, g_clientRect.bottom - gEditHeight, g_clientRect.right - g_clientRect.left - gNameWidth - gEnterWidth - gItemBoxWidth, gEditHeight, TRUE);
		MoveWindow(hChatEnter, g_clientRect.right - gEnterWidth - gItemBoxWidth, g_clientRect.bottom - gEditHeight, gEnterWidth, gEditHeight, TRUE);
		MoveWindow(hChatBoard, 0, g_clientRect.bottom - gEditHeight - gBoardHeight, (g_clientRect.right - g_clientRect.left - gItemBoxWidth), gBoardHeight, TRUE);
		break;
	}
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다.
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case ID_BUTTON_ENTER:
				switch (HIWORD(wParam)) 
				{
				case BN_CLICKED:
					char charChat[MAX_CHAT_BUF];
					TCHAR tcharChat[MAX_CHAT_BUF] = { 0 };
					GetWindowText(hChatEdit, tcharChat, MAX_CHAT_BUF - 1);
					SetWindowText(hChatEdit, NULL);
					SetFocus(hWnd);
					WideCharToMultiByte(CP_ACP, 0, tcharChat, -1, charChat, MAX_CHAT_BUF, NULL, NULL);
					if (strnlen(charChat, MAX_CHAT_BUF) == 0) break;
					g_networkManager.sendChat(charChat);
					break;
				}
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }

        }
        break;
	case WM_KEYDOWN:
	{
		bool move = false;
		switch (wParam)
		{
		case VK_UP:
			if (g_networkManager.getMyPlayer()->action_ != ACTION_TYPE::DIR_UP)
			{
				g_networkManager.getMyPlayer()->action_ = ACTION_TYPE::DIR_UP;
				g_networkManager.getMyPlayer()->lookDir_ = ACTION_TYPE::DIR_UP;
				move = true;
			}
			break;
		case VK_DOWN:
			if (g_networkManager.getMyPlayer()->action_ != ACTION_TYPE::DIR_DOWN)
			{
				g_networkManager.getMyPlayer()->action_ = ACTION_TYPE::DIR_DOWN;
				g_networkManager.getMyPlayer()->lookDir_ = ACTION_TYPE::DIR_DOWN;
				move = true;
			}
			break;
		case VK_LEFT:
			if (g_networkManager.getMyPlayer()->action_ != ACTION_TYPE::DIR_LEFT)
			{
				g_networkManager.getMyPlayer()->action_ = ACTION_TYPE::DIR_LEFT;
				g_networkManager.getMyPlayer()->lookDir_ = ACTION_TYPE::DIR_LEFT;
				move = true;
			}
			break;
		case VK_RIGHT:
			if (g_networkManager.getMyPlayer()->action_ != ACTION_TYPE::DIR_RIGHT)
			{
				g_networkManager.getMyPlayer()->action_ = ACTION_TYPE::DIR_RIGHT;
				g_networkManager.getMyPlayer()->lookDir_ = ACTION_TYPE::DIR_RIGHT;
				move = true;
			}
			break;
		case 'a': case 'A':
			if (g_networkManager.getMyPlayer()->action_ != ACTION_TYPE::ATTACK)
			{
				g_networkManager.getMyPlayer()->action_ = ACTION_TYPE::ATTACK;
				g_networkManager.sendAttack();
			}
			break;
		case VK_F1:
			if (g_networkManager.getMyPlayer()->items_[ITEM_RED_POTION] > 0)
			{
				g_networkManager.sendUseItem(ITEM_RED_POTION);
			}
			break;
		case VK_F2:
			if (g_networkManager.getMyPlayer()->items_[ITEM_BLUE_POTION] > 0)
			{
				g_networkManager.sendUseItem(ITEM_BLUE_POTION);
			}
			break;
		case VK_F3:
			if (g_networkManager.getMyPlayer()->items_[ITEM_GREEN_POTION] > 0)
			{
				g_networkManager.sendUseItem(ITEM_GREEN_POTION);
			}
			break;
		/*case '6':
		{
			POINT pos = g_networkManager.getMyPlayer()->pos_;
			g_grassMap[pos.y][pos.x] = GRASS_TYPE::GRASS;
			InvalidateRect(hWnd, NULL, 0);
			break;
		}
		case '1':
		{
			POINT pos = g_networkManager.getMyPlayer()->pos_;
			g_grassMap[pos.y][pos.x] = GRASS_TYPE::BEIGE;
			InvalidateRect(hWnd, NULL, 0);
			break;
		}
		case '2':
		{
			POINT pos = g_networkManager.getMyPlayer()->pos_;
			g_grassMap[pos.y][pos.x] = GRASS_TYPE::MUD;
			InvalidateRect(hWnd, NULL, 0);
			break;
		}
		case '3':
		{
			POINT pos = g_networkManager.getMyPlayer()->pos_;
			g_grassMap[pos.y][pos.x] = GRASS_TYPE::BUSH;
			InvalidateRect(hWnd, NULL, 0);
			break;
		}
		case '4':
		{
			POINT pos = g_networkManager.getMyPlayer()->pos_;
			g_grassMap[pos.y][pos.x] = GRASS_TYPE::FLOWER;
			InvalidateRect(hWnd, NULL, 0);
			break;
		}
		case '5':
		{
			POINT pos = g_networkManager.getMyPlayer()->pos_;
			g_grassMap[pos.y][pos.x] = GRASS_TYPE::HILL;
			InvalidateRect(hWnd, NULL, 0);
			break;
		}
		case '0':
		{
			ofstream fout("grassMap.txt");
			for (int r = 0; r < BOARD_ROW; ++r)
			{
				for (int c = 0; c < BOARD_COL; ++c)
				{
					fout << g_grassMap[c][r] << " ";
				}
			}
			fout.close();
			printf("save grassMap success! \n");
			break;
		}*/
		}

		if (move)
		{
			//InvalidateRect(hWnd, NULL, 0);
			POINT pos= g_networkManager.getMyPlayer()->pos_;
			printf("x:%d y:%d \n", pos.x, pos.y);
			g_networkManager.sendMove();
		}
		break;
	}
	case WM_KEYUP:
	{
		bool stop = false;
		switch (wParam)
		{
		case VK_UP:
			if (g_networkManager.getMyPlayer()->action_ == ACTION_TYPE::DIR_UP)
			{
				stop = true;
			}
			break;
		case VK_DOWN:
			if (g_networkManager.getMyPlayer()->action_ == ACTION_TYPE::DIR_DOWN)
			{
				stop = true;
			}
			break;
		case VK_LEFT:
			if (g_networkManager.getMyPlayer()->action_ == ACTION_TYPE::DIR_LEFT)
			{
				stop = true;
			}
			break;
		case VK_RIGHT:
			if (g_networkManager.getMyPlayer()->action_ == ACTION_TYPE::DIR_RIGHT)
			{
				stop = true;
			}
			break;
		case 'a': case 'A':
			if (g_networkManager.getMyPlayer()->action_ == ACTION_TYPE::ATTACK)
			{
				stop = true;
			}
			break;
		}
		if (stop)
		{
			g_networkManager.getMyPlayer()->action_ = ACTION_TYPE::DIR_STOP;
			g_networkManager.sendMove();
		}
		break;
	}
    case WM_PAINT:
	{
		DrawClient(hWnd);
		break;
		
	}
	case WM_SOCKET:
		g_networkManager.processSocketMessage(lParam);
		break;
    case WM_DESTROY:
	{
		g_players.erase(g_players.begin(), g_players.end());
		PostQuitMessage(0);
	}
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void DrawClient(HWND hWnd)
{
	Player *myPlayer = g_networkManager.getMyPlayer();

	if (myPlayer == NULL)
		return;

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);

	// 더블버퍼링
	HDC backMemDC = CreateCompatibleDC(hdc);
	static HBITMAP backBitmap = NULL;
	SetStretchBltMode(backMemDC, COLORONCOLOR);

	backBitmap = CreateCompatibleBitmap(hdc, g_clientRect.right, g_clientRect.bottom);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(backMemDC, backBitmap);

	SetBkMode(backMemDC, TRANSPARENT);
	FillRect(backMemDC, &g_clientRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	// 리소스들
	HDC frogMemDC = CreateCompatibleDC(hdc);
	SelectObject(frogMemDC, g_frogBmp);
	HDC attackMemDC = CreateCompatibleDC(hdc);
	SelectObject(attackMemDC, g_attackBmp);
	HDC herbMemDC = CreateCompatibleDC(hdc);
	SelectObject(herbMemDC, g_herbBmp);

	HDC	npcMemDC[MAX_NPC_TYPE];
	for (int i = 0; i < MAX_NPC_TYPE; ++i)
	{
		npcMemDC[i] = CreateCompatibleDC(hdc);
		SelectObject(npcMemDC[i], g_npcBmp[i]);
	}

	HDC itemMemDC[MAX_ITEM_TYPE];
	for (int i = 0; i < MAX_ITEM_TYPE; ++i)
	{
		itemMemDC[i] = CreateCompatibleDC(hdc);
		SelectObject(itemMemDC[i], g_itemBmp[i]);
	}

	HDC	grassMemDC[MAX_GRASS_TYPE];
	for (int i = 0; i < MAX_GRASS_TYPE; ++i)
	{
		grassMemDC[i] = CreateCompatibleDC(hdc);
		SelectObject(grassMemDC[i], g_grassBmp[i]);
	}


	POINT myPos = myPlayer->pos_;
	POINT realPos;
	int widthSpace = int((g_clientRect.right - g_clientRect.left ) / CLIENT_BOARD_COL);
	int heightSpace = int((g_clientRect.bottom - g_clientRect.top - (gBoardHeight+gEditHeight)) / CLIENT_BOARD_ROW);

	// 월드맵
	for (int y = 0; y < CLIENT_BOARD_ROW; ++y)
	{
		realPos.y = (myPos.y - CLIENT_BOARD_ROW / 2) + y;
		if (realPos.y < 0 || BOARD_ROW <=realPos.y )
			continue;

		for (int x = 0; x < CLIENT_BOARD_COL; ++x)
		{
			realPos.x = (myPos.x - CLIENT_BOARD_COL / 2) + x;
			if (realPos.x < 0 || BOARD_COL <=realPos.x)
				continue;


			int left = g_clientRect.left + widthSpace * x;
			int top = g_clientRect.top + heightSpace * y;

			int index = g_grassMap[realPos.y][realPos.x];
			StretchBlt(backMemDC, left, top, widthSpace, heightSpace,grassMemDC[index], 0, 0, g_grassBmpInfo[index].bmWidth, g_grassBmpInfo[index].bmHeight,SRCCOPY);
			
			// 화면 구분자(허브)
			/*if (realPos.x % 8 == 0 && realPos.y % 8 == 0)
			{
				TransparentBlt(backMemDC, left, top, widthSpace, heightSpace,
					herbMemDC, 0, 0, g_herbBmpInfo.bmWidth, g_herbBmpInfo.bmHeight, RGB(255, 0, 255));
			}*/
		}
	}


	TCHAR uiStr[128];

	// 아이템 창
	int itemBox_left = g_clientRect.right - gItemBoxWidth;
	int itemBox_top = g_clientRect.bottom - gBoardHeight - gEditHeight;
	int item_space_width = gItemBoxWidth / MAX_ITEM_TYPE;
	int item_space_height = (gBoardHeight+ gEditHeight);
	
	SetTextColor(backMemDC, RGB(0, 0, 0));	
	for (int x = 0; x < MAX_ITEM_TYPE; ++x)
	{
		switch (x)
		{
		case ITEM_RED_POTION:
			wsprintf(uiStr, L" F%d: %2d개", x + 1, myPlayer->items_[x]);
			break;
		case ITEM_BLUE_POTION:
			wsprintf(uiStr, L" F%d: %2d개", x + 1, myPlayer->items_[x]);
			break;
		case ITEM_GREEN_POTION:
			wsprintf(uiStr, L" F%d: %2d개", x + 1, myPlayer->items_[x]);
			break;
		}
		Rectangle(backMemDC, itemBox_left + item_space_width * x, itemBox_top, itemBox_left + item_space_width * (x + 1), g_clientRect.bottom);
		TextOut(backMemDC, itemBox_left + item_space_width * x, itemBox_top + 5, (LPCWSTR)uiStr, lstrlen(uiStr));

		if (myPlayer->items_[x] != 0)
		{
			TransparentBlt(backMemDC, itemBox_left + item_space_width * x, itemBox_top + 20, item_space_width, item_space_height - 25,
				itemMemDC[x], 0, 0, g_itemBmpInfo[x].bmWidth, g_itemBmpInfo[x].bmHeight, RGB(255, 0, 255));
		}
	}

	// 다른 캐릭터(플레이어+NPC)
	for (auto p = g_players.begin(); p != g_players.end(); p++)
	{
		if (p->second == myPlayer) continue;

		realPos.x = (CLIENT_BOARD_COL / 2) + p->second->pos_.x - myPos.x;
		realPos.y = (CLIENT_BOARD_ROW / 2) + p->second->pos_.y - myPos.y;

		int left = g_clientRect.left + widthSpace * realPos.x;
		int top = g_clientRect.top + heightSpace * realPos.y;

		int frame = (p->second->lookDir_ - 1);

		if (p->first < MAX_PLAYER_SEED) // 다른 플레이어
		{
			TransparentBlt(backMemDC, left, top + 10, widthSpace, heightSpace - 10, frogMemDC, (int)(frame*g_frogBmpInfo.bmWidth*0.25f), 0, (int)(g_frogBmpInfo.bmWidth*0.25f), g_frogBmpInfo.bmHeight, RGB(255, 0, 255));
		}
		else //  NPC
		{
			int type = p->second->npcType_;
			if (type == NPC_TYPE::NPC_RED_TREE || type == NPC_TYPE::NPC_BLACK_TREE) // 스태틱
			{
				TransparentBlt(backMemDC, left, top + 10, widthSpace, heightSpace - 10, npcMemDC[type], 0, 0, g_npcBmpInfo[type].bmWidth, g_npcBmpInfo[type].bmHeight, RGB(255, 0, 255));
			}
			else // 다이나믹
			{
				TransparentBlt(backMemDC, left, top + 10, widthSpace, heightSpace - 10, npcMemDC[type], (int)(frame*g_npcBmpInfo[type].bmWidth*0.25f), 0, (int)(g_npcBmpInfo[type].bmWidth*0.25f), g_npcBmpInfo[type].bmHeight, RGB(255, 0, 255));
			}
		}

		// 맞은 이펙트
		if (p->second->isSuffing_)
		{
			p->second->attackFrame_ = (p->second->attackFrame_ + 1) % 4;
			TransparentBlt(backMemDC, left, top + 10, widthSpace, heightSpace - 10, attackMemDC, (int)(p->second->attackFrame_*g_attackBmpInfo.bmWidth*0.25f), 0, (int)(g_attackBmpInfo.bmWidth*0.25f), g_attackBmpInfo.bmHeight, RGB(255, 0, 255));
			p->second->isSuffing_ = false;
		}

		wsprintf(uiStr, L"[lv:%02d hp:%3d / %3d]", p->second->level_, p->second->hp_, p->second->maxHp_);
		SetTextColor(backMemDC, RGB(255, 255, 255));
		TextOut(backMemDC, left, top, (LPCWSTR)uiStr, lstrlen(uiStr));
	}

	// 화면 중앙
	int myLeft = g_clientRect.left + widthSpace * (CLIENT_BOARD_COL / 2);
	int myTop = g_clientRect.top + heightSpace * (CLIENT_BOARD_ROW / 2);

	// 주인공 플레이어 렌더링
	int frame = (myPlayer->lookDir_ - 1);
	TransparentBlt(backMemDC, myLeft, myTop + 10, widthSpace, heightSpace - 10, frogMemDC, (int)(frame*g_frogBmpInfo.bmWidth*0.25f), 0, (int)(g_frogBmpInfo.bmWidth*0.25f), g_frogBmpInfo.bmHeight, RGB(255, 0, 255));
	
	// 맞은 이펙트
	if (myPlayer->isSuffing_)
	{
		myPlayer->attackFrame_ = (myPlayer->attackFrame_ + 1) % 4;
		TransparentBlt(backMemDC, myLeft, myTop + 10, widthSpace, heightSpace - 10, attackMemDC, (int)(myPlayer->attackFrame_*g_attackBmpInfo.bmWidth*0.25f), 0, (int)(g_attackBmpInfo.bmWidth*0.25f), g_attackBmpInfo.bmHeight, RGB(255, 0, 255));
		myPlayer->isSuffing_ = false;
	}
	
	wsprintf(uiStr, L"[lv:%02d hp:%3d / %3d]", myPlayer->level_, myPlayer->hp_, myPlayer->maxHp_);
	SetTextColor(backMemDC, RGB(255, 0, 0));
	TextOut(backMemDC, myLeft, myTop, (LPCWSTR)uiStr, lstrlen(uiStr));

	// 더블버퍼링
	BitBlt(hdc, 0, 0, g_clientRect.right, g_clientRect.bottom, backMemDC, 0, 0, SRCCOPY);
	
	DeleteObject(SelectObject(backMemDC, hOldBitmap));
	for (int i = 0; i < MAX_NPC_TYPE; ++i)
	{
		DeleteDC(npcMemDC[i]);
	}
	for (int i = 0; i < MAX_ITEM_TYPE; ++i)
	{
		DeleteDC(itemMemDC[i]);
	}
	for (int i = 0; i < MAX_GRASS_TYPE; ++i)
	{
		DeleteDC(grassMemDC[i]);
	}
	DeleteDC(frogMemDC);
	DeleteDC(attackMemDC);
	DeleteDC(herbMemDC);
	DeleteDC(backMemDC);
	EndPaint(hWnd, &ps);
}


