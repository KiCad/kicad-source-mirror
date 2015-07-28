/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PROJECT_TEMPLATE_SELECTOR_H
#define PROJECT_TEMPLATE_SELECTOR_H

#include <dialogs/dialog_template_selector_base.h>
#include "project_template.h"

class DIALOG_TEMPLATE_SELECTOR;

class TEMPLATE_WIDGET : public TEMPLATE_WIDGET_BASE
{
protected:
    DIALOG_TEMPLATE_SELECTOR* m_dialog;
    wxWindow*   m_parent;
    wxPanel*    m_panel;
    bool        m_selected;

    PROJECT_TEMPLATE* m_currTemplate;

    void OnKillFocus( wxFocusEvent& event );
    void OnMouse( wxMouseEvent& event );

public:
    TEMPLATE_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog );

    /**
     * Set the project template for this widget, which will determine the icon and title
     * associated with this project template widget
     */
    void SetTemplate(PROJECT_TEMPLATE* aTemplate);

    PROJECT_TEMPLATE* GetTemplate() { return m_currTemplate; }

    void Select();
    void Unselect();

private:
    bool IsSelected() { return m_selected; }
};


class TEMPLATE_SELECTION_PANEL : public TEMPLATE_SELECTION_PANEL_BASE
{
protected:
    wxNotebookPage* m_parent;
    wxString  m_templatesPath;      ///< the path to access to the folder
                                    ///< containing the templates (which are also folders)

public:
    /**
     * @param aParent The window creating the dialog
     * @param aPath the path
     */
    TEMPLATE_SELECTION_PANEL( wxNotebookPage* aParent, const wxString& aPath );

    const wxString& GetPath() { return m_templatesPath; }
};


class DIALOG_TEMPLATE_SELECTOR : public DIALOG_TEMPLATE_SELECTOR_BASE
{
protected:
    std::vector<TEMPLATE_SELECTION_PANEL*> m_panels;
    TEMPLATE_WIDGET* m_selectedWidget;

    void AddTemplate( int aPage, PROJECT_TEMPLATE* aTemplate );

public:
    DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent );

    /**
     * Add a new page with \a aTitle, populated with templates from \a aPath
     * - All directories under the path are treated as templates
     * @param aTitle = the title of the wxNoteBook page
     * @param aPath = the path of the main folder containing templates
     */
    void AddTemplatesPage( const wxString& aTitle, wxFileName& aPath );

    /**
     * @return the selected template, or NULL
     */
    PROJECT_TEMPLATE* GetSelectedTemplate();

private:

    void SetHtml( wxFileName aFilename )
    {
        m_htmlWin->LoadPage( aFilename.GetFullPath() );
    }

public:
    void SetWidget( TEMPLATE_WIDGET* aWidget );

private:
    void buildPageContent( const wxString& aPath, int aPage );
    void replaceCurrentPage();

    void onNotebookResize( wxSizeEvent& event );
    void OnPageChange( wxNotebookEvent& event );
    void onDirectoryBrowseClicked( wxCommandEvent& event );
	void onValidatePath( wxCommandEvent& event );
	void OnHtmlLinkActivated( wxHtmlLinkEvent& event );
};

#endif
