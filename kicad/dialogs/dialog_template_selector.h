/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/webview_panel.h>
#include "project_template.h"

#include <vector>
#include <utility>
#include <wx/filename.h>

class DIALOG_TEMPLATE_SELECTOR;

class TEMPLATE_WIDGET : public TEMPLATE_WIDGET_BASE
{
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

    /**
     * Set whether this template widget represents a user template
     * @param aIsUser true if this is a user template (can be edited/duplicated)
     */
    void SetIsUserTemplate( bool aIsUser ) { m_isUserTemplate = aIsUser; }
    bool IsUserTemplate() const { return m_isUserTemplate; }

protected:
    void OnKillFocus( wxFocusEvent& event );
    void OnMouse( wxMouseEvent& event );
    void OnDoubleClick( wxMouseEvent& event );
    void onRightClick( wxMouseEvent& event );
    void onEditTemplate( wxCommandEvent& event );
    void onDuplicateTemplate( wxCommandEvent& event );

private:
    bool IsSelected() { return m_selected; }

protected:
    DIALOG_TEMPLATE_SELECTOR* m_dialog;
    wxWindow*                 m_parent;
    wxPanel*                  m_panel;
    bool                      m_selected;
    bool                      m_isUserTemplate;

    PROJECT_TEMPLATE*         m_currTemplate;
};


class TEMPLATE_SELECTION_PANEL : public TEMPLATE_SELECTION_PANEL_BASE
{
public:
    /**
     * @param aParent The window creating the dialog
     * @param aPath the path
     */
    TEMPLATE_SELECTION_PANEL( wxNotebookPage* aParent, const wxString& aPath );

    const wxString& GetPath() const { return m_templatesPath; }

    void AddTemplateWidget( TEMPLATE_WIDGET* aTemplateWidget );

    void SortAlphabetically();

    /**
     * Set whether templates in this panel are user templates (can be edited/duplicated)
     */
    void SetIsUserTemplates( bool aIsUser ) { m_isUserTemplates = aIsUser; }
    bool IsUserTemplates() const { return m_isUserTemplates; }

protected:
    wxNotebookPage* m_parent;
    wxString        m_templatesPath;   ///< the path to access to the folder
                                       ///<   containing the templates (which are also folders)
    bool            m_isUserTemplates; ///< true if this panel contains user templates
};


class DIALOG_TEMPLATE_SELECTOR : public DIALOG_TEMPLATE_SELECTOR_BASE
{
public:
    DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent, const wxPoint& aPos, const wxSize& aSize,
                              std::vector<std::pair<wxString, wxFileName>> aTitleDirList,
                              const wxFileName& aDefaultTemplate );

    /**
     * @return the selected template, or NULL
     */
    PROJECT_TEMPLATE* GetSelectedTemplate();
    PROJECT_TEMPLATE* GetDefaultTemplate();

    void SetWidget( TEMPLATE_WIDGET* aWidget );

    /**
     * @return the project path to edit (if Edit Template was selected), or empty string
     */
    wxString GetProjectToEdit() const { return m_projectToEdit; }

    /**
     * Set the project path to edit (used by template widgets)
     */
    void SetProjectToEdit( const wxString& aPath ) { m_projectToEdit = aPath; }

    /**
     * Refresh the current page to show updated template list
     */
    void replaceCurrentPage();

    /**
     * Get the path to the user templates directory (first panel marked as user templates)
     */
    wxString GetUserTemplatesPath() const;

protected:
    void AddTemplate( int aPage, PROJECT_TEMPLATE* aTemplate );

private:
    void SetHtml( const wxFileName& aFilename )
    {
        m_webviewPanel->LoadURL( aFilename.GetFullPath() );
    }

private:
    void buildPageContent( const wxString& aPath, int aPage );

    void OnPageChange( wxNotebookEvent& event ) override;
    void onDirectoryBrowseClicked( wxCommandEvent& event ) override;
	void onReload( wxCommandEvent& event ) override;

protected:
    std::vector<TEMPLATE_SELECTION_PANEL*> m_panels;
    TEMPLATE_WIDGET*                       m_selectedWidget;
    wxFileName                             m_defaultTemplatePath;
    TEMPLATE_WIDGET*                       m_defaultWidget;
    // Keep track of all template widgets so we can pick a sensible default
    std::vector<TEMPLATE_WIDGET*>          m_allWidgets;
    wxString                               m_projectToEdit;  ///< Project path to edit instead of creating new
};

#endif
