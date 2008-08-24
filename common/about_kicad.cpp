/* wxWidgets about dialog */
#include <wx/aboutdlg.h>
#include "wx/statline.h"
#include "wx/generic/aboutdlgg.h"

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

extern wxString g_Main_Title; // Import program title


/**************************************************/
void InitKiCadAbout(wxAboutDialogInfo& info)
/**************************************************/
{
    /* Set name and title */
    info.SetName(g_Main_Title);

    /* Set description */
    wxString description;

    description << (_T("Build: ")) << GetAboutBuildVersion();

/* Check for unicode */
#if wxUSE_UNICODE
    description << (_(" Unicode " ));
#else
    description << (_(" Ansi "));
#endif

/* Check for wxMSW */
#if wxMSW
    description << (_("on Windows"));
#endif

/* Check for wxMAC */
#if wxMAC
    description << (_("on Macintosch"));
#endif

/* Check for linux and arch */
#if __gnu_linux__
   description << (_("on GNU/Linux "));
#ifdef _LP64
   description << (_("64 bits"));
#else
   description << (_("32 bits"));
#endif
#endif

    info.SetDescription(description);

    /* Set copyright */
    info.SetCopyright(_T("(C) 1992-2008 KiCad Developers Team"));

    /* Set license */
    info.SetLicence(wxString::FromAscii
    (
      "The complete KiCad EDA Suite is released under the following license: \n"
      "\n"
      "GNU General Public License version 2\n"
      "\n"
      "See <http://www.gnu.org/licenses/> for more information"
    ));

    /* Add developers */
    info.AddDeveloper(_T("Dick Hollenbeck <dick@softplc.com>"));
    info.AddDeveloper(_T("\nJean-Pierre Charras <jean-pierre.charras@inpg.fr>"));

    /* Add document writers */
    info.AddDocWriter(_T("Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));
    info.AddDocWriter(_T("\nIgor Plyatov <plyatov@gmail.com>"));

    /* Add translators */
    info.AddTranslator(wxT("Czech (CZ) Milan Horák <stranger@tiscali.cz>")); /* fix for translation ! */
    info.AddTranslator(_("\nDutch (NL) Jerry Jacobs <jerkejacobs@gmail.com>"));
    info.AddTranslator(_("\nFrench (FR) Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));
    info.AddTranslator(_("\nPolish (PL) Mateusz Skowronski <skowri@gmail.com>"));
    info.AddTranslator(_("\nPortuguese (PT) Renie Marquet <reniemarquet@uol.com.br>"));
    info.AddTranslator(_("\nRussian (RU) Igor Plyatov <plyatov@gmail.com>"));

}
