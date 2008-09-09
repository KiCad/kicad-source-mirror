/* wxWidgets about dialog */
#include <wx/aboutdlg.h>
#include "wx/statline.h"
#include "wx/generic/aboutdlgg.h"

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

extern wxString g_Main_Title; // Import program title

/**********************************/
wxString SetMsg( const wxString& msg )
/**********************************/

/* add \n at the beginning of msg under Windows, and do nothing under other version of wxWidgets
 * Needed under wxWidgets 2.8 because wxGTK and wxMSW do not have the same behavior
 * AddDeveloper needs \n between names under wxGTK, and nothing under wxMSW
 * when displaying developer and others.
 * can be removed for next wxWidgets versions when this wxWidgets bug will be solved
 */
{
    wxString tmp;

    #ifdef __WINDOWS__
    tmp = wxT( "\n" );
    #endif

    tmp << msg;
    return tmp;
}


/**************************************************/
void InitKiCadAbout( wxAboutDialogInfo& info )
/**************************************************/
{
    /* Set name and title */
    info.SetName( g_Main_Title );

    /* Set description */
    wxString description;

    description << ( _T( "Build: " ) ) << GetAboutBuildVersion();

/* Check for unicode */
#if wxUSE_UNICODE
    description << ( wxT( " Unicode " ) );
#else
    description << ( wxT( " Ansi " ) );
#endif

/* Check for wxMSW */
#if defined __WINDOWS__
    description << ( wxT( "on Windows" ) );

/* Check for wxMAC */
#elif defined __WXMAC__
    description << ( wxT( "on Macintosch" ) );

/* Check for linux and arch */
#elif defined __LINUX__
    description << ( wxT( "on GNU/Linux " ) );
#endif
#ifdef _LP64
    description << ( wxT( " 64 bits" ) );
#else
    description << ( wxT( " 32 bits" ) );
#endif
    description << ( wxT( "\n with wxWidgets " ) );
    description << wxMAJOR_VERSION << wxT( "." ) << wxMINOR_VERSION << wxT( "." ) <<
    wxRELEASE_NUMBER;

    description << wxT( "\n\nWeb sites:\n" );
    description << wxT( "http://iut-tice.ujf-grenoble.fr/kicad/" );
    description << wxT( "\n" );
    description << wxT( "http://kicad.sourceforge.net/" );
    description << wxT( "\n" );


    info.SetDescription( description );

    /* Set copyright */
    info.SetCopyright( _T( "(C) 1992-2008 KiCad Developers Team" ) );

    /* Set license */
    info.SetLicence( wxString::FromAscii
                    (
                        "The complete KiCad EDA Suite is released under the following license: \n"
                        "\n"
                        "GNU General Public License version 2\n"
                        "See <http://www.gnu.org/licenses/> for more information"
                    ) );

    /* Add developers */
    info.AddDeveloper( wxT( "Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) );
    info.AddDeveloper( SetMsg( wxT( "Dick Hollenbeck <dick@softplc.com>" ) ) );
    info.AddDeveloper( SetMsg( wxT( "kbool library: http://boolean.klaasholwerda.nl/bool.html" ) ) );

    /* Add document writers */
    info.AddDocWriter( wxT( "Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) );
    info.AddDocWriter( SetMsg( wxT( "Igor Plyatov <plyatov@gmail.com>" ) ) );

    /* Add translators */
    info.AddTranslator( wxT( "Czech (CZ) Martin Kratoška <martin@ok1rr.com>" ) );
    info.AddTranslator( SetMsg( wxT( "Dutch (NL) Jerry Jacobs <jerkejacobs@gmail.com>" ) ) );
    info.AddTranslator( SetMsg( wxT(
                                   "French (FR) Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) ) );
    info.AddTranslator( SetMsg( wxT( "Polish (PL) Mateusz Skowronski <skowri@gmail.com>" ) ) );
    info.AddTranslator( SetMsg( wxT( "Portuguese (PT) Renie Marquet <reniemarquet@uol.com.br>" ) ) );
    info.AddTranslator( SetMsg( wxT( "Russian (RU) Igor Plyatov <plyatov@gmail.com>" ) ) );
    info.AddTranslator( SetMsg( wxT(
                                   " David Briscoe, Jean Dupont (Remy),Boris Barbour, Dominique Laigle, Paul Burke" ) ) );

    info.AddTranslator( SetMsg( wxT( "Pedro Martin del Valle, Inigo Zuluaga")));

    /* Add programm credits */
#if 0   // TODO
    info.AddArtist( wxT( "" ) );
#endif
}
