/////////////////////////////////////////////////////////////////////////////
// Name:		eda_dde.h
// Purpose:		DDE server & client
/////////////////////////////////////////////////////////////////////////////

#define wxServer		wxSocketServer
#define wxClient		wxSocketClient

#include <wx/socket.h>

#define WinEDA_Server wxSocketServer


// TCP/IP ports used by PCBNEW and EESCHEMA respectively. 
#define KICAD_PCB_PORT_SERVICE_NUMBER   4242    ///< PCBNEW listens on this port for commands from EESCHEMA
#define KICAD_SCH_PORT_SERVICE_NUMBER   4243    ///< EESCHEMA listens on this port for commands from PCBNEW


#define MSG_TO_PCB                      KICAD_PCB_PORT_SERVICE_NUMBER
#define MSG_TO_SCH                      KICAD_SCH_PORT_SERVICE_NUMBER


	/********************/
	/* autres fonctions */
	/********************/

WinEDA_Server * CreateServer( wxWindow * window, int port );
bool SendCommand( int port, const char* cmdline );
void SetupServerFunction( void (*remotefct) (const char* remotecmd) );

