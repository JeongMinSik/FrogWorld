// stdafx.h : ���� ��������� ���� ��������� �ʴ�
// ǥ�� �ý��� ���� ���� �Ǵ� ������Ʈ ���� ���� ������
// ��� �ִ� ���� �����Դϴ�.
//

#pragma once

#define		_WINSOCK_DEPRECATED_NO_WARNINGS


#pragma comment( lib, "msimg32.lib" )

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
// Windows ��� ����:
#include <windows.h>

// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: ���α׷��� �ʿ��� �߰� ����� ���⿡�� �����մϴ�.

#pragma comment(lib, "ws2_32.lib")

#include <fstream>
#include <stdio.h>
#include <Ws2tcpip.h>
#include <winsock2.h>
#include <commctrl.h> 
#include <map>
#include <queue>
#include <list>
#include "Protocol.h"

using namespace std;