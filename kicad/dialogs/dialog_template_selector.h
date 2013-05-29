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

#include "dialogs/dialog_template_selector_base.h"
#include "project_template.h"

class TEMPLATE_WIDGET : public TEMPLATE_WIDGET_BASE
{
protected:
    wxDialog* dialog;
    wxWindow* parent;
    wxPanel* panel;

    bool selected;
    PROJECT_TEMPLATE* templ;

    void OnKillFocus( wxFocusEvent& event );
    void OnMouse( wxMouseEvent& event );

public:
    TEMPLATE_WIDGET( wxWindow* aParent, wxDialog* aDialog );
    ~TEMPLATE_WIDGET();

    /**
     * Set the project template for this widget, which will determine the icon and title
     * associated with this project template widget
     */
    void SetTemplate(PROJECT_TEMPLATE* aTemplate);
    PROJECT_TEMPLATE* GetTemplate();
    void Select();
    void Unselect();
    bool IsSelected();
};

class TEMPLATE_SELECTION_PANEL : public TEMPLATE_SELECTION_PANEL_BASE
{
protected:
    wxWindow* parent;
    wxString  m_templatesPath;

public:
    /**
     * @param aParent The window creating the dialog
     */
    TEMPLATE_SELECTION_PANEL( wxWindow* aParent, const wxString& aPath );
    ~TEMPLATE_SELECTION_PANEL();

    const wxString& GetPath() { return m_templatesPath; }
};

class DIALOG_TEMPLATE_SELECTOR : public DIALOG_TEMPLATE_SELECTOR_BASE
{
protected:
    vector<TEMPLATE_SELECTION_PANEL*> m_panels;
    void AddTemplate( int aPage, PROJECT_TEMPLATE* aTemplate );
    TEMPLATE_WIDGET* m_selectedWidget;

public:
    DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent );
    ~DIALOG_TEMPLATE_SELECTOR();

    /**
     * Add a new page with \a aTitle, populated with templates from \a aPath
     * - All directories under the path are treated as templates
     */
    void AddPage( const wxString& aTitle, wxFileName& aPath );
    void SetHtml( wxFileName aFilename );
    TEMPLATE_WIDGET* GetWidget();
    void SetWidget( TEMPLATE_WIDGET* aWidget );
    void onNotebookResize( wxSizeEvent& event );
	void OnPageChange( wxNotebookEvent& event );
};

#endif
