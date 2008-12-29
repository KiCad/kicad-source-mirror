/* wxWidgets about dialog */
#include <wx/aboutdlg.h>
#include "wx/statline.h"
#include "wx/generic/aboutdlgg.h"

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

extern wxString g_Main_Title; // Import program title

/**********************************/
wxString
SetMsg( const wxString& msg )
/**********************************/
/* add \n at the beginning of msg under Windows, and do nothing under other version of wxWidgets
 * Needed under wxWidgets 2.8 because wxGTK and wxMSW do not have the same behavior
 * Add Developer needs \n between names under wxMSW, and nothing under wxGTK
 * when displaying developer and others.
 * Perhaps depending on wxWidgets versions
 */
{
	wxString message;

#if 1	/* Windows */
	message = wxT( "\n" );
#endif
	message << msg;
	return message;
}


/**************************************************/
void
InitKiCadAbout( wxAboutDialogInfo& info )
/**************************************************/
{
	/* Set name and title */
	info.SetName( g_Main_Title );

	/* Set description */
	wxString description;

	/* KiCad build version */
	description << ( _T( "Build: " ) ) << GetAboutBuildVersion();

	/* Print for wxversion */
	description << ( wxT( "\n\nwxWidgets " ) )
		<< wxMAJOR_VERSION
		<< wxT( "." )
		<< wxMINOR_VERSION << wxT( "." )
		<< wxRELEASE_NUMBER

	/* Show Unicode or Ansi version */
#if wxUSE_UNICODE
		<< ( wxT( " Unicode\n" ) );
#		else
		<< ( wxT( " Ansi\n" ) );
#endif



#define FreeBSD

	/**************************
 	 * Check Operating System *
 	 **************************/
#if defined __WINDOWS__
    description << ( wxT( "on Windows" ) );

	/* Check for wxMAC */
#	elif defined __WXMAC__
	description << ( wxT( "on Macintosch" ) );

	/* Linux 64 bits */
#	elif defined _LP64 && __LINUX__
	description << ( wxT( "on 64 Bits GNU/Linux" ) );

	/* Linux 32 bits */
#	elif defined __LINUX__
	description << ( wxT( "on 32 Bits GNU/Linux" ) );

	/* OpenBSD */
#	elif defined __OpenBSD__
	description << ( wxT ("on OpenBSD") );

	/* FreeBSD */
#	elif defined __FreeBSD__
	description << ( wxT ("on FreeBSD") );

#endif

	/* Websites */
	description << wxT( "\n\nKiCad on the web\n\n" );
	description << wxT( "http://iut-tice.ujf-grenoble.fr/kicad \n" );
	description << wxT( "http://kicad.sourceforge.net \n" );
	description << wxT( "http://www.kicadlib.org" );

	/* Set the complete about description */
	info.SetDescription( description );

	/* Set copyright dialog */
	info.SetCopyright( _T( "(C) 1992-2009 KiCad Developers Team" ) );

	/* Set license dialog */
	info.SetLicence( wxString::FromAscii
	(	"The complete KiCad EDA Suite is released under the\n"
		"GNU General Public License version 2.\n"
		"See <http://www.gnu.org/licenses/> for more information."
	));

	/* Add developers */
	info.AddDeveloper( wxT( "Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) );
	info.AddDeveloper( SetMsg( wxT( "Jerry Jacobs <jerkejacobs@gmail.com>" ) ) );
	info.AddDeveloper( SetMsg( wxT( "Dick Hollenbeck <dick@softplc.com>" ) ) );
	info.AddDeveloper( SetMsg( wxT( "KBool Library <http://boolean.klaasholwerda.nl/bool.html>" ) ) );
	info.AddDeveloper( SetMsg( wxT( "Vesa Solonen <vesa.solonen@hut.fi>" ) ) );
	info.AddDeveloper( SetMsg( wxT( "Wayne Stambaugh <stambaughw@verizon.net>" ) ) );

	/* Add document writers*/
	info.AddDocWriter( wxT( "Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) );
	info.AddDocWriter( SetMsg( wxT( "Igor Plyatov <plyatov@gmail.com>" ) ) );

	/* Add translators */
	info.AddTranslator( wxT( "Czech (CZ) Martin Kratoška <martin@ok1rr.com>" ) );
	info.AddTranslator( SetMsg( wxT( "Dutch (NL) Jerry Jacobs <jerkejacobs@gmail.com>" ) ) );
	info.AddTranslator( SetMsg( wxT( "French (FR) Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) ) );
	info.AddTranslator( SetMsg( wxT( "Polish (PL) Mateusz Skowroński <skowri@gmail.com>" ) ) );
	info.AddTranslator( SetMsg( wxT( "Portuguese (PT) Renie Marquet <reniemarquet@uol.com.br>" ) ) );
	info.AddTranslator( SetMsg( wxT( "Russian (RU) Igor Plyatov <plyatov@gmail.com>" ) ) );
	info.AddTranslator( SetMsg( wxT( "Spanish (ES) Pedro Martin del Valle <pkicad@yahoo.es>" ) ) );
	info.AddTranslator( SetMsg( wxT( "Spanish (ES) Iñigo Zuluaga <inigo_zuluaga@yahoo.es>" ) ) );

	/* TODO are these all russian translators, placed them here now              TODO
		TODO or else align them below other language maintainer with mail adres   TODO*/
	info.AddTranslator( SetMsg( wxT( "\n\nRemy Halvick" ) ) );
	info.AddTranslator( SetMsg( wxT( "David Briscoe" ) ) );
	info.AddTranslator( SetMsg( wxT( "Dominique Laigle" ) ) );
	info.AddTranslator( SetMsg( wxT( "Paul Burke" ) ) );

	/* Add programm credits for icons */
	info.AddArtist( wxT( "Icons by Iñigo Zuluaga" ) );
	info.AddArtist( wxT( "3D modules by Renie Marquet <reniemarquet@uol.com.br>" ) );
}
