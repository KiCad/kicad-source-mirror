/**
 * @file eda_dde.h
 * @brief DDE server & client.
 */

#ifndef EDA_DDE_H_
#define EDA_DDE_H_

#include <wx/socket.h>


// TCP/IP ports used by Pcbnew and Eeschema respectively.

///< Pcbnew listens on this port for commands from Eeschema
#define KICAD_PCB_PORT_SERVICE_NUMBER   4242

///< Eeschema listens on this port for commands from Pcbnew
#define KICAD_SCH_PORT_SERVICE_NUMBER   4243


#define MSG_TO_PCB                      KICAD_PCB_PORT_SERVICE_NUMBER
#define MSG_TO_SCH                      KICAD_SCH_PORT_SERVICE_NUMBER

wxSocketServer* CreateServer( wxWindow * window, int port, bool local = true );
bool SendCommand( int port, const char* cmdline );
void SetupServerFunction( void (*remotefct) (const char* remotecmd) );

#endif    // EDA_DDE_H_
