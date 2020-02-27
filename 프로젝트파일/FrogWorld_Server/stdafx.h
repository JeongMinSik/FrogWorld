#pragma once

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "lua\\lua53.lib")

extern "C"
{
#include "lua\include\lua.h"
#include "lua\include\lauxlib.h"
#include "lua\include\lualib.h"
}

#define		_WINSOCK_DEPRECATED_NO_WARNINGS

enum GRASS_TYPE { GRASS, BEIGE, MUD, BUSH, FLOWER, HILL };

#include <stdio.h>
#include <fstream>
#include <Ws2tcpip.h>
#include <winsock2.h>
#include <thread>
#include <map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <queue>
#include <sqlext.h>
#include <iostream>

#include "Protocol.h"

using namespace std;