#include "NetworkManager.h"
#include "locale.h"

NetworkManager gNetworkManager;

BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
	case CTRL_BREAK_EVENT:
	default:
		gNetworkManager.disconnectAllClients();
		break;
	}
	return FALSE;
}

int main()
{
	// 강제종료시 처리
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);

	_wsetlocale(LC_ALL, L"korean");

	gNetworkManager.startServer();
	return 0;
}