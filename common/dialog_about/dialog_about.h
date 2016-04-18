/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2010-2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include "aboutinfo.h"
#include "dialog_about_base.h"

/**
 * About dialog to show application specific information.
 * Needs an <code>AboutAppInfo</code> object that contains the data to be displayed.
 */
class dialog_about : public dialog_about_base
{
private:

    // Icons for the various tabs of wxAuiNotebook
    wxBitmap     picInformation;
    wxBitmap     picDevelopers;
    wxBitmap     picDocWriters;
    wxBitmap     picArtists;
    wxBitmap     picTranslators;
    wxBitmap     picPackagers;
    wxBitmap     picLicense;

    AboutAppInfo info;

public:
    dialog_about( wxWindow* dlg, AboutAppInfo& appInfo );
    ~dialog_about();

private:
    void             initDialog();
    virtual void     OnClose( wxCloseEvent& event );
    virtual void     OnOkClick( wxCommandEvent& event );
    virtual void     OnHtmlLinkClicked( wxHtmlLinkEvent& event );
    virtual void     OnCopyVersionInfo( wxCommandEvent &event );

    // Notebook pages
    wxFlexGridSizer* CreateFlexGridSizer();
    void             DeleteNotebooks();
    void             CreateNotebooks();
    void             CreateNotebookPage( wxAuiNotebook*      aParent,
                                         const wxString&     aCaption,
                                         const wxBitmap&     aIcon,
                                         const Contributors& aContributors );
    void             CreateNotebookPageByCategory( wxAuiNotebook*      aParent,
                                                   const wxString&     aCaption,
                                                   const wxBitmap&     aIcon,
                                                   const Contributors& aContributors );
    void             CreateNotebookHtmlPage( wxAuiNotebook*  aParent,
                                             const wxString& aCaption,
                                             const wxBitmap& aIcon,
                                             const wxString& aHtmlMessage );

    wxHyperlinkCtrl* CreateHyperlink( wxScrolledWindow* aParent, const wxString& email );
    wxStaticBitmap*  CreateStaticBitmap( wxScrolledWindow* aParent, wxBitmap* icon );
};

#endif // DIALOG_ABOUT_H
