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
#if wxUSE_UNICODE
    description << (_T( " Unicode" ));
#else
    description << (_T( " Ansi" ));
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
    info.AddDeveloper(_T("Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));

    /* Add document writers */
    info.AddDocWriter(_T("Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));
    info.AddDocWriter(_T("Igor Plyatov <plyatov@gmail.com>"));

    /* Add translators */
    info.AddTranslator(_T("Czech (CZ) Milan Horák <stranger@tiscali.cz>"));
    info.AddTranslator(_T("Dutch (NL) Jerry Jacobs <jerkejacobs@gmail.com>"));
    info.AddTranslator(_T("French (FR) Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));
    info.AddTranslator(_T("Polish (PL) Mateusz Skowroński <skowri@gmail.com>"));
    info.AddTranslator(_T("Russian (RU) Igor Plyatov <plyatov@gmail.com>"));

}
