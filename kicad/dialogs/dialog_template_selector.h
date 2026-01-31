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

#include <memory>
#include <vector>
#include <utility>
#include <wx/filename.h>
#include <wx/fswatcher.h>
#include <wx/notebook.h>
#include <wx/timer.h>
#include <wx/webview.h>

class DIALOG_TEMPLATE_SELECTOR;
class TEMPLATE_WIDGET;


/**
 * A widget displaying a recently used template with a small icon and title.
 */
class TEMPLATE_MRU_WIDGET : public wxPanel
{
public:
    TEMPLATE_MRU_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog,
                         const wxString& aPath, const wxString& aTitle, const wxBitmap& aIcon );

    wxString GetTemplatePath() const { return m_templatePath; }

protected:
    void OnClick( wxMouseEvent& event );
    void OnDoubleClick( wxMouseEvent& event );
    void OnEnter( wxMouseEvent& event );
    void OnLeave( wxMouseEvent& event );

private:
    DIALOG_TEMPLATE_SELECTOR* m_dialog;
    wxString                  m_templatePath;
};


class TEMPLATE_WIDGET : public TEMPLATE_WIDGET_BASE
{
public:
    TEMPLATE_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog );

    /**
     * Set the project template for this widget, which will determine the icon and title
     * associated with this project template widget
     */
    void SetTemplate( PROJECT_TEMPLATE* aTemplate );

    PROJECT_TEMPLATE* GetTemplate() { return m_currTemplate; }

    void Select();
    void SelectWithoutStateChange();
    void Unselect();

    void SetDescription( const wxString& aDescription );
    wxString GetDescription() const { return m_description; }

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
    void OnSize( wxSizeEvent& event );
    void onRightClick( wxMouseEvent& event );
    void onEditTemplate( wxCommandEvent& event );
    void onDuplicateTemplate( wxCommandEvent& event );

public:
    bool IsSelected() const { return m_selected; }

protected:
    DIALOG_TEMPLATE_SELECTOR* m_dialog;
    wxWindow*                 m_parent;
    wxPanel*                  m_panel;
    bool                      m_selected;
    bool                      m_isUserTemplate;
    wxString                  m_description;

    PROJECT_TEMPLATE*         m_currTemplate;
};


class DIALOG_TEMPLATE_SELECTOR : public DIALOG_TEMPLATE_SELECTOR_BASE
{
public:
    DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent, const wxPoint& aPos, const wxSize& aSize,
                              const wxString& aUserTemplatesPath,
                              const wxString& aSystemTemplatesPath,
                              const std::vector<wxString>& aRecentTemplates );

    ~DIALOG_TEMPLATE_SELECTOR();

    PROJECT_TEMPLATE* GetSelectedTemplate();
    wxString GetProjectToEdit() const { return m_projectToEdit; }

    void SetWidget( TEMPLATE_WIDGET* aWidget );
    void SelectTemplateByPath( const wxString& aPath );
    void SelectTemplateByPath( const wxString& aPath, bool aKeepMRUVisible );
    wxString GetUserTemplatesPath() const { return m_userTemplatesPath; }

    void SetProjectToEdit( const wxString& aPath ) { m_projectToEdit = aPath; }
    void RefreshTemplateList();

protected:
    void OnSearchCtrl( wxCommandEvent& event ) override;
    void OnSearchCtrlCancel( wxCommandEvent& event ) override;
    void OnFilterChanged( wxCommandEvent& event ) override;
    void OnBackClicked( wxCommandEvent& event ) override;

    void OnSearchTimer( wxTimerEvent& event );
    void OnRefreshTimer( wxTimerEvent& event );
    void OnWebViewLoaded( wxWebViewEvent& event );
    void OnScrolledTemplatesSize( wxSizeEvent& event );

    void OnFileSystemEvent( wxFileSystemWatcherEvent& event );
    void OnSysColourChanged( wxSysColourChangedEvent& event );

private:
    enum class DialogState { Initial, Preview, MRUWithPreview };

    void SetState( DialogState aState );
    void BuildMRUList();
    void BuildTemplateList();
    void ApplyFilter();
    void LoadTemplatePreview( PROJECT_TEMPLATE* aTemplate );
    void SetupFileWatcher();
    wxString ExtractDescription( const wxFileName& aHtmlFile );
    void ShowWelcomeHtml();
    void EnsureWebViewCreated();

    DialogState                                  m_state;
    TEMPLATE_WIDGET*                             m_selectedWidget;
    PROJECT_TEMPLATE*                            m_selectedTemplate;

    wxString                                     m_userTemplatesPath;
    wxString                                     m_systemTemplatesPath;
    std::vector<wxString>                        m_recentTemplates;

    std::vector<std::unique_ptr<PROJECT_TEMPLATE>> m_templates;
    std::vector<TEMPLATE_WIDGET*>                m_templateWidgets;
    std::vector<TEMPLATE_MRU_WIDGET*>            m_mruWidgets;

    wxTimer                                      m_searchTimer;
    wxTimer                                      m_refreshTimer;

    wxFileSystemWatcher*                         m_watcher;

    wxString                                     m_projectToEdit;

    WEBVIEW_PANEL*                               m_webviewPanel;
    bool                                         m_loadingExternalHtml;
};

#endif
