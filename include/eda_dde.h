/////////////////////////////////////////////////////////////////////////////
// Name:		eda_dde.h
// Purpose:		DDE server & client
/////////////////////////////////////////////////////////////////////////////

#define wxServer		wxSocketServer
#define wxClient		wxSocketClient

#include <wx/socket.h>

#define WinEDA_Server wxSocketServer


	/********************/
	/* autres fonctions */
	/********************/

WinEDA_Server * CreateServer(wxWindow * window, int service);
bool SendCommand( int service, char * cmdline);
void SetupServerFunction(void (* remotefct)(char * remotecmd) );







