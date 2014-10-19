/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_ABOUT_H
#define DIALOG_ABOUT_H

#include <wx/html/htmlwin.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/hyperlink.h>

#include <aboutinfo.h>
#include <dialog_about_base.h>

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
#include <bitmaps.h>

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
