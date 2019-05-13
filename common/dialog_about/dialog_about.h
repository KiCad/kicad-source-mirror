/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright (C) 2010-2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * Needs a <code>ABOUT_APP_INFO</code> object that contains the data to be displayed.
 */
class DIALOG_ABOUT : public DIALOG_ABOUT_BASE
{
private:

    // Icons for the various tabs of wxAuiNotebook
    wxBitmap     m_picInformation;
    wxBitmap     m_picDevelopers;
    wxBitmap     m_picDocWriters;
    wxBitmap     m_picArtists;
    wxBitmap     m_picTranslators;
    wxBitmap     m_picPackagers;
    wxBitmap     m_picLicense;
    wxString     m_titleName;

    ABOUT_APP_INFO& m_info;

public:
    DIALOG_ABOUT( EDA_BASE_FRAME* aParent, ABOUT_APP_INFO& aAppInfo );
    ~DIALOG_ABOUT();

private:
    void         initDialog();

    /** build the version info message
     * @param aMsg is the result
     * @param aFormatHtml = true to use a minimal HTML format
     * false to use a plain text
     */
    void         buildVersionInfoData( wxString& aMsg, bool aFormatHtml );

    void         onHtmlLinkClicked( wxHtmlLinkEvent& event );

	virtual void onCopyVersionInfo( wxCommandEvent& event ) override;
	virtual void onShowVersionInfo( wxCommandEvent& event ) override;

    // Notebook pages
    wxFlexGridSizer* createFlexGridSizer();
    void             createNotebooks();
    void             createNotebookPage( wxAuiNotebook*      aParent,
                                         const wxString&     aCaption,
                                         const wxBitmap&     aIcon,
                                         const CONTRIBUTORS& aContributors );
    void             createNotebookPageByCategory( wxAuiNotebook*      aParent,
                                                   const wxString&     aCaption,
                                                   const wxBitmap&     aIcon,
                                                   const CONTRIBUTORS& aContributors );
    void             createNotebookHtmlPage( wxAuiNotebook*  aParent,
                                             const wxString& aCaption,
                                             const wxBitmap& aIcon,
                                             const wxString& aHtmlMessage );

    wxStaticText* wxStaticTextMail( wxScrolledWindow* aParent, const wxString& email );
    wxStaticBitmap*  createStaticBitmap( wxScrolledWindow* aParent, wxBitmap* icon );
};

#endif // DIALOG_ABOUT_H
