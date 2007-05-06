
	//////////////////////
	// Name: eda_dde.cc	//
	//////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
	#pragma hdrstop
#endif

// for all others, include the necessary headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "fctsys.h"
#include "eda_dde.h"
#include "wxstruct.h"
#include "id.h"

#include "common.h"

#define ID_CONN "CAO_COM"

wxString HOSTNAME(wxT("localhost"));

/* variables locales */
#define IPC_BUF_SIZE 4000
char client_ipc_buffer[IPC_BUF_SIZE];
char server_ipc_buffer[IPC_BUF_SIZE];

wxServer * server;

void (* RemoteFct)(char * cmd);

char buffcar[1024];

void SetupServerFunction(void (* remotefct)(char * remotecmd) )
{
	RemoteFct = remotefct;
}


	/*****************************/
	/* Routines liees au SERVEUR */
	/*****************************/
/* Fonction d'initialisation d'un serveur socket
*/
WinEDA_Server * CreateServer(wxWindow * window, int service)
{
wxIPV4address addr;

	// Create a new server
	addr.Service(service);

	server = new wxServer(addr);
	if(server)
	  {
	    server->SetNotify(wxSOCKET_CONNECTION_FLAG);
	    server->SetEventHandler(*window, ID_EDA_SOCKET_EVENT_SERV);
	    server->Notify(TRUE);
	  }

	return server;
}


/********************************************************/
void WinEDA_DrawFrame::OnSockRequest(wxSocketEvent& evt)
/********************************************************/
/* Fonction appelee a chaque demande d'un client
*/
{
size_t len;
wxSocketBase *sock = evt.GetSocket();

  switch (evt.GetSocketEvent())
     {
     case wxSOCKET_INPUT:
       sock->Read(server_ipc_buffer,1);
       len = sock->Read(server_ipc_buffer+1,IPC_BUF_SIZE-2).LastCount();
       server_ipc_buffer[len+1] = 0;
       if(RemoteFct ) RemoteFct(server_ipc_buffer);
       break;

     case wxSOCKET_LOST:
       return;
       break;

     default:
       wxPrintf( wxT("WinEDA_DrawFrame::OnSockRequest() error: Invalid event !"));
       break;
     }
}

/**************************************************************/
void WinEDA_DrawFrame::OnSockRequestServer(wxSocketEvent& evt)
/**************************************************************/
/* fonction appelée lors d'une demande de connexion d'un client
*/
{
wxSocketBase *sock2;
wxSocketServer *server = (wxSocketServer *) evt.GetSocket();

    sock2 = server->Accept();
    if (sock2 == NULL) return;

    sock2->Notify(TRUE);
	sock2->SetEventHandler(*this, ID_EDA_SOCKET_EVENT);
    sock2->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
}


	/****************************/
	/* Routines liees au CLIENT */
	/*****************************/

/********************************************/
bool SendCommand( int service, char * cmdline)
/********************************************/
/* Fonction utilisee par un client pour envoyer une information a un serveur.
	- Etablit une connection Socket Client
    - envoie le contenu du buffer cmdline
    - ferme la connexion

    service contient le numéro de service en ascii.
*/
{
wxSocketClient * sock_client;
bool success = FALSE;
wxIPV4address addr;

	// Create a connexion
	addr.Hostname(HOSTNAME);
	addr.Service(service);

 // Mini-tutorial for Connect() :-)	(JP CHARRAS Note: see wxWidgets: sockets/client.cpp sample)
  // ---------------------------
  //
  // There are two ways to use Connect(): blocking and non-blocking,
  // depending on the value passed as the 'wait' (2nd) parameter.
  //
  // Connect(addr, true) will wait until the connection completes,
  // returning true on success and false on failure. This call blocks
  // the GUI (this might be changed in future releases to honour the
  // wxSOCKET_BLOCK flag).
  //
  // Connect(addr, false) will issue a nonblocking connection request
  // and return immediately. If the return value is true, then the
  // connection has been already successfully established. If it is
  // false, you must wait for the request to complete, either with
  // WaitOnConnect() or by watching wxSOCKET_CONNECTION / LOST
  // events (please read the documentation).
  //
  // WaitOnConnect() itself never blocks the GUI (this might change
  // in the future to honour the wxSOCKET_BLOCK flag). This call will
  // return false on timeout, or true if the connection request
  // completes, which in turn might mean:
  //
  //   a) That the connection was successfully established
  //   b) That the connection request failed (for example, because
  //      it was refused by the peer.
  //
  // Use IsConnected() to distinguish between these two.
  //
  // So, in a brief, you should do one of the following things:
  //
  // For blocking Connect:
  //
  //   bool success = client->Connect(addr, true);
  //
  // For nonblocking Connect:
  //
  //   client->Connect(addr, false);
  //
  //   bool waitmore = true;
  //   while (! client->WaitOnConnect(seconds, millis) && waitmore )
  //   {
  //     // possibly give some feedback to the user,
  //     // update waitmore if needed.
  //   }
  //   bool success = client->IsConnected();
  //
  // And that's all :-)
	sock_client = new wxSocketClient();
	sock_client->SetTimeout(2);	// Time out in Seconds
	sock_client->Connect(addr, FALSE);
	sock_client->WaitOnConnect(0, 100);

	if (sock_client->Ok() && sock_client->IsConnected())
	{
		success = TRUE;
		sock_client->SetFlags(wxSOCKET_NOWAIT /*wxSOCKET_WAITALL*/);
		sock_client->Write(cmdline, strlen(cmdline));
	}

	sock_client->Close();
	sock_client->Destroy();
	return success;
}

