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
 * Add Developer needs \n between names under wxMSW, and nothing under wxGTK
 * when displaying developer and others.
 * Perhaps depending on wxWidgets versions
 */
{
    wxString tmp;

//    #ifdef __WINDOWS__
#if 1
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
    description << ( wxT( "\n\nUsing  " ) );
    description << ( wxT( "wxWidgets " ) );
    description << wxMAJOR_VERSION << wxT( "." ) << wxMINOR_VERSION << wxT( "." ) <<
    wxRELEASE_NUMBER;
#if wxUSE_UNICODE
    description << ( wxT( "  Unicode " ) );
#else
    description << ( wxT( "  Ansi " ) );
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
    info.AddDeveloper( SetMsg( wxT( "Jerry Jacobs <jerkejacobs@gmail.com>" ) ) );
    info.AddDeveloper( SetMsg( wxT( "Dick Hollenbeck <dick@softplc.com>" ) ) );
    info.AddDeveloper( SetMsg( wxT( "KBool Library <http://boolean.klaasholwerda.nl/bool.html>" ) ) );

    /* Add document writers */
    info.AddDocWriter( wxT( "Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) );
    info.AddDocWriter( SetMsg( wxT( "Igor Plyatov <plyatov@gmail.com>" ) ) );

    /* Add translators */
    info.AddTranslator( wxT( "Czech (CZ) Martin Kratoška <martin@ok1rr.com>" ) );
    info.AddTranslator( SetMsg( wxT( "Dutch (NL) Jerry Jacobs <jerkejacobs@gmail.com>" ) ) );
    info.AddTranslator( SetMsg( wxT(
                                   "French (FR) Jean-Pierre Charras <jean-pierre.charras@inpg.fr>" ) ) );
    info.AddTranslator( SetMsg( wxT( "Polish (PL) Mateusz Skowroński <skowri@gmail.com>" ) ) );
    info.AddTranslator( SetMsg( wxT( "Portuguese (PT) Renie Marquet <reniemarquet@uol.com.br>" ) ) );
    info.AddTranslator( SetMsg( wxT( "Russian (RU) Igor Plyatov <plyatov@gmail.com>" ) ) );
    info.AddTranslator( SetMsg( wxT(
                                   " David Briscoe, Remy Halvick, Boris Barbour, Dominique Laigle, Paul Burke" ) ) );

    info.AddTranslator( SetMsg( wxT( "Pedro Martin del Valle, Inigo Zuluaga")));

    /* Add programm credits for icons */
    info.AddArtist( wxT( "Icons: Inigo Zuluaga" ) );
}
