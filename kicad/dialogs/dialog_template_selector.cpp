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

#include "dialog_template_selector.h"

#include <wx/dir.h>
#include <wx/settings.h>


TEMPLATE_SELECTION_PANEL::TEMPLATE_SELECTION_PANEL( wxNotebookPage* aParent,
                                                    const wxString& aPath ) :
    TEMPLATE_SELECTION_PANEL_BASE( aParent )
{
    m_parent = aParent;
    m_templatesPath = aPath;
}


TEMPLATE_WIDGET::TEMPLATE_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog ) :
    TEMPLATE_WIDGET_BASE( aParent )
{
    m_parent = aParent;
    m_dialog = aDialog;

    // wxWidgets_3.xx way of doing the same...
    // Bind(wxEVT_LEFT_DOWN, &TEMPLATE_WIDGET::OnMouse, this );

    m_bitmapIcon->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), NULL, this );
    m_staticTitle->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), NULL, this );

    // We're not selected until we're clicked
    Unselect();

    // Start with template being NULL
    m_currTemplate = NULL;
}


void TEMPLATE_WIDGET::Select()
{
    m_dialog->SetWidget( this );
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNHIGHLIGHT ) );
    m_selected = true;
    Refresh();
}


void TEMPLATE_WIDGET::Unselect()
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_selected = false;
    Refresh();
}


void TEMPLATE_WIDGET::SetTemplate(PROJECT_TEMPLATE* aTemplate)
{
    m_currTemplate = aTemplate;
    m_staticTitle->SetLabel( *(aTemplate->GetTitle()) );
    m_bitmapIcon->SetBitmap( *(aTemplate->GetIcon()) );
}


void TEMPLATE_WIDGET::OnMouse( wxMouseEvent& event )
{
    // Toggle selection here
    Select();
    event.Skip();
}


void DIALOG_TEMPLATE_SELECTOR::onNotebookResize(wxSizeEvent& event)
{
    for( size_t i=0; i < m_notebook->GetPageCount(); i++ )
    {
        m_panels[i]->SetSize( m_notebook->GetSize().GetWidth() - 6, 140 );
        m_panels[i]->m_SizerBase->FitInside( m_panels[i] );
        m_panels[i]->m_scrolledWindow->SetSize( m_panels[i]->GetSize().GetWidth() - 6,
                                                 m_panels[i]->GetSize().GetHeight() - 6 );
        m_panels[i]->m_SizerChoice->FitInside( m_panels[i]->m_scrolledWindow );
    }
    m_notebook->Refresh();

    event.Skip();
}

void DIALOG_TEMPLATE_SELECTOR::OnPageChange( wxNotebookEvent& event )
{
    int page = m_notebook->GetSelection();

    if( page != wxNOT_FOUND && (unsigned)page < m_panels.size() )
        m_tcTemplatePath->SetValue( m_panels[page]->GetPath() );
}


DIALOG_TEMPLATE_SELECTOR::DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent ) :
    DIALOG_TEMPLATE_SELECTOR_BASE( aParent )
{
    m_htmlWin->SetPage( _( "<html><h1>Template Selector</h1></html>" ) );
    m_notebook->Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_TEMPLATE_SELECTOR::onNotebookResize ), NULL, this );
    m_selectedWidget = NULL;
}


void DIALOG_TEMPLATE_SELECTOR::SetWidget( TEMPLATE_WIDGET* aWidget )
{
    if( m_selectedWidget != NULL )
        m_selectedWidget->Unselect();

    m_selectedWidget = aWidget;
    SetHtml( m_selectedWidget->GetTemplate()->GetHtmlFile() );
}

void DIALOG_TEMPLATE_SELECTOR::AddTemplate( int aPage, PROJECT_TEMPLATE* aTemplate )
{
    TEMPLATE_WIDGET* w = new TEMPLATE_WIDGET( m_panels[aPage]->m_scrolledWindow, this  );
    w->SetTemplate( aTemplate );

    m_panels[aPage]->m_SizerChoice->Add( w );
    m_panels[aPage]->m_SizerChoice->Layout();
    m_panels[aPage]->SetSize( m_notebook->GetSize().GetWidth() - 6, 140 );
    m_panels[aPage]->m_SizerBase->FitInside( m_panels[aPage] );
    m_panels[aPage]->m_scrolledWindow->SetSize( m_panels[aPage]->GetSize().GetWidth() - 6,
                                                 m_panels[aPage]->GetSize().GetHeight() - 6 );
    m_panels[aPage]->m_SizerChoice->FitInside( m_panels[aPage]->m_scrolledWindow );

    m_notebook->Refresh();
}


PROJECT_TEMPLATE* DIALOG_TEMPLATE_SELECTOR::GetSelectedTemplate()
{
    return m_selectedWidget? m_selectedWidget->GetTemplate() : NULL;
}

void DIALOG_TEMPLATE_SELECTOR::AddTemplatesPage( const wxString& aTitle, wxFileName& aPath )
{
    wxNotebookPage* newPage = new wxNotebookPage( m_notebook, wxID_ANY );

    aPath.Normalize();
    wxString path = aPath.GetFullPath();    // caller ensures this ends with file separator.

    TEMPLATE_SELECTION_PANEL* tpanel = new TEMPLATE_SELECTION_PANEL( newPage, path );
    m_panels.push_back( tpanel );

    m_notebook->AddPage( newPage, aTitle );

    if( m_notebook->GetPageCount() == 1 )
        m_tcTemplatePath->SetValue( path );

    buildPageContent( path, m_notebook->GetPageCount() - 1 );
}

void DIALOG_TEMPLATE_SELECTOR::buildPageContent( const wxString& aPath, int aPage )
{
    // Get a list of files under the template path to include as choices...
    wxArrayString   files;
    wxDir           dir;

    if( dir.Open( aPath ) )
    {
        wxDir       sub_dir;
        wxString    sub_name;

        bool cont = dir.GetFirst( &sub_name, wxEmptyString, wxDIR_DIRS );
        while( cont )
        {
            wxString sub_full = aPath + sub_name;
            if( sub_dir.Open( sub_full ) )
            {
                files.Add( sub_name );

                PROJECT_TEMPLATE* pt = new PROJECT_TEMPLATE( sub_full );
                AddTemplate( aPage, pt );
            }

            cont = dir.GetNext( &sub_name );
        }
    }
}


void DIALOG_TEMPLATE_SELECTOR::onDirectoryBrowseClicked( wxCommandEvent& event )
{
    wxFileName fn;
    fn.AssignDir( m_tcTemplatePath->GetValue() );
    fn.Normalize();
    wxString currPath = fn.GetFullPath();

    wxDirDialog dirDialog( this, _( "Select Templates Directory" ),
                           currPath,
                           wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

    if( dirDialog.ShowModal() != wxID_OK )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    m_tcTemplatePath->SetValue( dirName.GetFullPath() );

    if( currPath == m_tcTemplatePath->GetValue() )
        return;     // No change

    // Rebuild the page from the new templates path:
    replaceCurrentPage();
}


void DIALOG_TEMPLATE_SELECTOR::onValidatePath( wxCommandEvent& event )
{
    int page = m_notebook->GetSelection();

    if( page < 0 )
        return;     // Should not happen

    wxString currPath = m_tcTemplatePath->GetValue();

    if( currPath == m_panels[page]->GetPath() )     // No change
        return;

    wxFileName fn;
    fn.AssignDir( m_tcTemplatePath->GetValue() );
    fn.Normalize();
    currPath = fn.GetFullPath();
    m_tcTemplatePath->SetValue( currPath );

    replaceCurrentPage();
}


void DIALOG_TEMPLATE_SELECTOR::replaceCurrentPage()
{
    // Rebuild the page from the new templates path:
    int page = m_notebook->GetSelection();

    if( page < 0 )
        return;     // Should not happen

    wxString title = m_notebook->GetPageText( page );
    wxString currPath = m_tcTemplatePath->GetValue();

    m_notebook->DeletePage( page );

    wxNotebookPage* newPage = new wxNotebookPage( m_notebook, wxID_ANY );
    TEMPLATE_SELECTION_PANEL* tpanel = new TEMPLATE_SELECTION_PANEL( newPage, currPath );
    m_panels[page] = tpanel;
    m_notebook->InsertPage( page, newPage, title, true );

    buildPageContent( m_tcTemplatePath->GetValue(), page );

    m_selectedWidget = NULL;
}


void DIALOG_TEMPLATE_SELECTOR::OnHtmlLinkActivated( wxHtmlLinkEvent& event )
{
    wxString url = event.GetLinkInfo().GetHref();
    wxLaunchDefaultBrowser( url);
}
