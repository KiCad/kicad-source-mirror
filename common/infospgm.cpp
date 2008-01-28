		/****************************************************/
		/* Display a generic info about kikac (copyright..) */
		/* Common tp CVPCB, EESCHEMA, PCBNEW and GERBVIEW	*/
		/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#ifdef KICAD_PYTHON
#include  <pyhandler.h>
#endif

// Import:
extern wxString g_Main_Title;

/* Program title strings used in about dialog.  They are kept hear to make
 * it easy to update the copyright dates. */
wxString g_KicadAboutTitle = wxT("** KICAD  (jul 2000 .. 2008) **");
wxString g_CvpcbAboutTitle = wxT("** CVPCB  (sept 1992 .. 2008) **");
wxString g_EeschemaAboutTitle = wxT("** EESCHEMA  (sept 1994 .. 2008) **");
wxString g_PcbnewAboutTitle = wxT("** PCBNEW  (sept 1992 .. 2008) **");
wxString g_GerbviewAboutTitle = wxT("** GERBVIEW  (jul 2001 .. 2008) **");

// Routines Locales

/*******************************************/
void Print_Kicad_Infos(wxWindow * frame, const wxString& title)
/*******************************************/
{
    wxString AboutCaption = wxT("About ");
    wxString Msg = title;

	Msg << wxT("\n\n") << _("Build Version:") << wxT("\n") ;

	Msg << g_Main_Title << wxT(" ") << GetBuildVersion();
#if wxUSE_UNICODE
	Msg << wxT(" - Unicode version");
#else
	Msg << wxT(" - Ansi version");
#endif

#ifdef KICAD_PYTHON
	Msg << wxT("\n");
	Msg << wxT( "python : " );
	Msg << wxString::FromAscii( PyHandler::GetInstance()->GetVersion() );
#endif

	Msg << wxT("\n\n") << _("Author:");
	Msg << wxT(" JP CHARRAS\n\n") << _("Based on wxWidgets ");
	Msg << wxMAJOR_VERSION << wxT(".") <<
		wxMINOR_VERSION << wxT(".") << wxRELEASE_NUMBER;
	if( wxSUBRELEASE_NUMBER )
		Msg << wxT(".") << wxSUBRELEASE_NUMBER;
	Msg << _("\n\nGPL License");
	Msg << _("\n\nAuthor's sites:\n");
	Msg << wxT("http://iut-tice.ujf-grenoble.fr/kicad/\n");
	Msg << wxT("http://www.gipsa-lab.inpg.fr/realise_au_lis/kicad/");
	Msg << _("\n\nInternational wiki:\n");
	Msg << wxT("http://kicad.sourceforge.net/\n");

	AboutCaption << g_Main_Title << wxT(" ") << GetBuildVersion();

	wxMessageBox(Msg, AboutCaption, wxICON_INFORMATION, frame);
}

