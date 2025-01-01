/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Brian Sidebotham <brian.sidebotham@gmail.com>
 * Copyright (C) 1992-2025 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/ui_common.h>
#include <wx_filename.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/settings.h>


TEMPLATE_SELECTION_PANEL::TEMPLATE_SELECTION_PANEL( wxNotebookPage* aParent,
                                                    const wxString& aPath ) :
    TEMPLATE_SELECTION_PANEL_BASE( aParent )
{
    m_parent = aParent;
    m_templatesPath = aPath;
}


void TEMPLATE_SELECTION_PANEL::AddTemplateWidget( TEMPLATE_WIDGET* aTemplateWidget )
{
    m_SizerChoice->Add( aTemplateWidget );
    Layout();
}


TEMPLATE_WIDGET::TEMPLATE_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog ) :
    TEMPLATE_WIDGET_BASE( aParent )
{
    m_parent = aParent;
    m_dialog = aDialog;

    // wxWidgets_3.xx way of doing the same...
    // Bind(wxEVT_LEFT_DOWN, &TEMPLATE_WIDGET::OnMouse, this );

    m_bitmapIcon->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ),
                           nullptr, this );
    m_staticTitle->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ),
                            nullptr, this );

    // We're not selected until we're clicked
    Unselect();

    // Start with template being NULL
    m_currTemplate = nullptr;
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


void TEMPLATE_WIDGET::SetTemplate( PROJECT_TEMPLATE* aTemplate )
{
    m_currTemplate = aTemplate;
    m_staticTitle->SetFont( KIUI::GetInfoFont( this ) );
    m_staticTitle->SetLabel( *aTemplate->GetTitle() );
    m_staticTitle->Wrap( 100 );
    m_bitmapIcon->SetBitmap( *aTemplate->GetIcon() );
}


void TEMPLATE_WIDGET::OnMouse( wxMouseEvent& event )
{
    // Toggle selection here
    Select();
    event.Skip();
}


void DIALOG_TEMPLATE_SELECTOR::OnPageChange( wxNotebookEvent& event )
{
    int page = event.GetSelection();

    if( page != wxNOT_FOUND && (unsigned)page < m_panels.size() )
        m_tcTemplatePath->SetValue( m_panels[page]->GetPath() );

    event.Skip();
}


DIALOG_TEMPLATE_SELECTOR::DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent, const wxPoint& aPos,
                                                    const wxSize&                  aSize,
                                                    std::map<wxString, wxFileName> aTitleDirMap ) :
        DIALOG_TEMPLATE_SELECTOR_BASE( aParent, wxID_ANY, _( "Project Template Selector" ), aPos,
                                       aSize )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_reloadButton->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );

    m_htmlWin->SetPage( _( "<h1>Template Selector</h1>" ) );
    m_selectedWidget = nullptr;

    for( auto& [title, pathFname] : aTitleDirMap )
    {
        pathFname.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
        wxString path = pathFname.GetFullPath(); // caller ensures this ends with file separator.

        TEMPLATE_SELECTION_PANEL* tpanel = new TEMPLATE_SELECTION_PANEL( m_notebook, path );
        m_panels.push_back( tpanel );

        m_notebook->AddPage( tpanel, title );

        if( m_notebook->GetPageCount() == 1 )
            m_tcTemplatePath->SetValue( path );

        buildPageContent( path, m_notebook->GetPageCount() - 1 );
    }

    // When all widgets have the size fixed, call finishDialogSettings to update sizers
    finishDialogSettings();
}


void DIALOG_TEMPLATE_SELECTOR::SetWidget( TEMPLATE_WIDGET* aWidget )
{
    if( m_selectedWidget != nullptr )
        m_selectedWidget->Unselect();

    m_selectedWidget = aWidget;
    SetHtml( m_selectedWidget->GetTemplate()->GetHtmlFile() );
}


void DIALOG_TEMPLATE_SELECTOR::AddTemplate( int aPage, PROJECT_TEMPLATE* aTemplate )
{
    TEMPLATE_WIDGET* w = new TEMPLATE_WIDGET( m_panels[aPage]->m_scrolledWindow, this  );
    w->SetTemplate( aTemplate );
    m_panels[aPage]->AddTemplateWidget( w );
}


PROJECT_TEMPLATE* DIALOG_TEMPLATE_SELECTOR::GetSelectedTemplate()
{
    return m_selectedWidget? m_selectedWidget->GetTemplate() : nullptr;
}


void DIALOG_TEMPLATE_SELECTOR::buildPageContent( const wxString& aPath, int aPage )
{
    // Get a list of files under the template path to include as choices...
    wxDir dir;

    if( dir.Open( aPath ) )
    {
        if( dir.HasSubDirs( "meta" ) )
        {
            AddTemplate( aPage, new PROJECT_TEMPLATE( aPath ) );
        }
        else
        {
            wxString      sub_name;
            wxArrayString subdirs;

            bool cont = dir.GetFirst( &sub_name, wxEmptyString, wxDIR_DIRS );

            while( cont )
            {
                subdirs.Add( wxString( sub_name ) );
                cont = dir.GetNext( &sub_name );
            }

            if( !subdirs.IsEmpty() )
                subdirs.Sort();

            for( const wxString& dir_name : subdirs )
            {
                wxDir    sub_dir;
                wxString sub_full = aPath + dir_name;

                if( sub_dir.Open( sub_full ) )
                    AddTemplate( aPage, new PROJECT_TEMPLATE( sub_full ) );
            }
        }
    }
    
    Layout();
}


void DIALOG_TEMPLATE_SELECTOR::onDirectoryBrowseClicked( wxCommandEvent& event )
{
    wxFileName fn;
    fn.AssignDir( m_tcTemplatePath->GetValue() );
    fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
    wxString currPath = fn.GetFullPath();

    wxDirDialog dirDialog( this, _( "Select Templates Directory" ), currPath,
                           wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

    if( dirDialog.ShowModal() != wxID_OK )
        return;

    wxFileName dirName = wxFileName::DirName( dirDialog.GetPath() );

    m_tcTemplatePath->SetValue( dirName.GetFullPath() );

    // Rebuild the page from the new templates path:
    replaceCurrentPage();
}


void DIALOG_TEMPLATE_SELECTOR::onReload( wxCommandEvent& event )
{
    int page = m_notebook->GetSelection();

    if( page < 0 )
        return;     // Should not happen

    wxString currPath = m_tcTemplatePath->GetValue();

    wxFileName fn;
    fn.AssignDir( m_tcTemplatePath->GetValue() );
    fn.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
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

    TEMPLATE_SELECTION_PANEL* tpanel = new TEMPLATE_SELECTION_PANEL( m_notebook, currPath );
    m_panels[page] = tpanel;
    m_notebook->InsertPage( page, tpanel, title, true );

    buildPageContent( m_tcTemplatePath->GetValue(), page );

    m_selectedWidget = nullptr;

    Layout();
}


void DIALOG_TEMPLATE_SELECTOR::OnHtmlLinkActivated( wxHtmlLinkEvent& event )
{
    wxString url = event.GetLinkInfo().GetHref();
    wxLaunchDefaultBrowser( url );
}
