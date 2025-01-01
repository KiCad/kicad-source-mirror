/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Rafael Sokolowski <Rafael.Sokolowski@web.de>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

// Used for the notebook image list
enum class IMAGES {
    INFORMATION,
    VERSION,
    DEVELOPERS,
    DOCWRITERS,
    LIBRARIANS,
    ARTISTS,
    TRANSLATORS,
    PACKAGERS,
    LICENSE
};

/**
 * About dialog to show application specific information.
 * Needs a <code>ABOUT_APP_INFO</code> object that contains the data to be displayed.
 */
class DIALOG_ABOUT : public DIALOG_ABOUT_BASE
{
public:
    DIALOG_ABOUT( EDA_BASE_FRAME* aParent, ABOUT_APP_INFO& aAppInfo );
    ~DIALOG_ABOUT();

protected:
    void OnNotebookPageChanged( wxNotebookEvent& aEvent ) override;

private:
    void onHtmlLinkClicked( wxHtmlLinkEvent& event );

    void onCopyVersionInfo( wxCommandEvent& event ) override;

    void onReportBug( wxCommandEvent& event ) override;

    void onDonateClick( wxCommandEvent& event ) override;

    // Notebook pages
    void createNotebooks();
    void createNotebookPageByCategory( wxNotebook* aParent, const wxString& aCaption,
                                       IMAGES aIconIndex, const CONTRIBUTORS& aContributors );
    void createNotebookHtmlPage( wxNotebook* aParent, const wxString& aCaption,
                                 IMAGES aIconIndex, const wxString& aHtmlMessage,
                                 bool aSelection = false );

    wxStaticText* wxStaticTextRef( wxScrolledWindow* aParent, const wxString& aReference );
    wxStaticBitmap*  createStaticBitmap( wxScrolledWindow* aParent, wxBitmap* icon );

private:
    wxImageList*    m_images;
    wxString        m_titleName;
    wxString        m_untranslatedTitleName;

    ABOUT_APP_INFO& m_info;
};

#endif // DIALOG_ABOUT_H
