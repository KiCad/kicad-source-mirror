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
      "GNU GPLv3"
    ));

    /* Add developers */
    info.AddDeveloper(_T("Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));

    /* Add document writers */
    info.AddDocWriter(_T("Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));

    /* Add translators */
    info.AddTranslator(_T("Dutch (NL) Jerry Jacobs <jerkejacobs@gmail.com>"));
    info.AddTranslator(_T("French (FR) Jean-Pierre Charras <jean-pierre.charras@inpg.fr>"));
}
