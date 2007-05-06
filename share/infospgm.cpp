		/****************************************************/
		/* Affichage du panneau d'information (copyright..) */
		/* Commun a CVPCB, EESCHEMA et PCBNEW				*/
		/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

// Import:
extern wxString g_Main_Title;

wxString MsgInfos(
#ifdef GERBVIEW
wxT("** GERBVIEW  (jul 2001 .. 2006) **")
#else
#ifdef PCBNEW
wxT("** PCBNEW  (sept 1992 .. 2006) **")
#endif
#endif

#ifdef CVPCB
wxT("** CVPCB  (sept 1992 .. 2006) **")
#endif

#ifdef KICAD
wxT("** KICAD (jul 2000 .. 2006) **")
#endif

#ifdef EESCHEMA
wxT("** EESCHEMA  (sept 1994 .. 2006) **")
#endif
);

/* Routines Locales */

/*******************************************/
void Print_Kicad_Infos(wxWindow * frame)
/*******************************************/
{
wxString Msg = MsgInfos;
	Msg << wxT("\n\n") << _("Build Version:") << wxT("\n") ;

	Msg << g_Main_Title << wxT(" ") << GetBuildVersion();
#if wxUSE_UNICODE
	Msg << wxT(" - Unicode version");
#else
	Msg << wxT(" - Ansi version");
#endif
	Msg << wxT("\n\n") << _("Author:");
	Msg << wxT("JP CHARRAS\n\n") << _("Based on wxWidgets ");
	Msg << wxMAJOR_VERSION << wxT(".") <<
		wxMINOR_VERSION << wxT(".") << wxRELEASE_NUMBER;
	if ( wxSUBRELEASE_NUMBER )
		Msg << wxT(".") << wxSUBRELEASE_NUMBER;
	Msg << _("\n\nGPL License");
	Msg << _("\n\nWeb sites:\n");
	Msg << wxT("http://iut-tice.ujf-grenoble.fr/kicad/\n");
	Msg << wxT("http://www.lis.inpg.fr/realise_au_lis/kicad/");

	wxMessageBox(Msg, wxEmptyString, wxICON_INFORMATION, frame);
}

