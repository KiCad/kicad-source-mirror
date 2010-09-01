/***************************************************************
 * Name:      dialog_about.h
 * Purpose:   Defines Application Frame
 * Author:    Rafael Sokolowski (rafael.sokolowski@web.de)
 * Created:   2010-08-06
 * Copyright: Rafael Sokolowski ()
 * License:
 **************************************************************/

#ifndef DIALOG_ABOUT_H
#define DIALOG_ABOUT_H

#include <wx/html/htmlwin.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/hyperlink.h>

#include "aboutinfo.h"
#include "dialog_about_base.h"

/* Pixel information of icons in XPM format.
 * All KiCad icons are linked into shared library 'libbitmaps.a'.
 *  Icons:
 *  preference_xpm[];    // Icon for 'Developers' tab
 *  editor_xpm[];        // Icon for 'Doc Writers' tab
 *  palette_xpm[];       // Icon for 'Artists' tab
 *  language_xpm[];      // Icon for 'Translators' tab
 *  right_xpm[];         // Right arrow icon for list items
 *  info_xpm[];          // Bulb for description tab
 *  tools_xpm[];         // Sheet of paper icon for license info tab
 */
#include "bitmaps.h"

/**
 * About dialog to show application specific information.
 * Needs an <code>AboutAppInfo</code> object that contains the data to be displayed.
 */
class dialog_about : public dialog_about_base
{
private:

    // Icons for the various tabs of wxAuiNotebook
    wxBitmap     picInformation, picDevelopers, picDocWriters, picArtists, picTranslators,
                 picLicense;

    AboutAppInfo info;

public:
    dialog_about( wxWindow* dlg, AboutAppInfo& appInfo );
    ~dialog_about();
private:
    void             initDialog();
    virtual void     OnClose( wxCloseEvent& event );
    virtual void     OnOkClick( wxCommandEvent& event );
    virtual void     OnHtmlLinkClicked( wxHtmlLinkEvent& event );

    // Notebook pages
    wxFlexGridSizer* CreateFlexGridSizer();
    void             DeleteNotebooks();
    void             CreateNotebooks();
    void             CreateNotebookPage( wxAuiNotebook*      parent,
                                         const wxString&     caption,
                                         const wxBitmap&     icon,
                                         const Contributors& contributors );
    void             CreateNotebookPageByCategory( wxAuiNotebook*      parent,
                                                   const wxString&     caption,
                                                   const wxBitmap&     icon,
                                                   const Contributors& contributors );
    void             CreateNotebookHtmlPage( wxAuiNotebook*  parent,
                                             const wxString& caption,
                                             const wxBitmap& icon,
                                             const wxString& html );

    wxHyperlinkCtrl* CreateHyperlink( wxScrolledWindow* parent, const wxString& email );
    wxStaticBitmap*  CreateStaticBitmap( wxScrolledWindow* parent, wxBitmap* icon );
};
#endif // DIALOG_ABOUT_H
