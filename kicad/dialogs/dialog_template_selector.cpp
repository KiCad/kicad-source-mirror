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

#include "dialog_template_selector.h"
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/ui_common.h>
#include <algorithm>
#include <wx_filename.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/settings.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/math.h>
#include <wx/menu.h>
#include <wx/textdlg.h>
#include <wx/textfile.h>
#include <confirm.h>
#include <project_tree_traverser.h>
#include "template_default_html.h"

// Welcome / fallback HTML now provided by template_default_html.h

TEMPLATE_SELECTION_PANEL::TEMPLATE_SELECTION_PANEL( wxNotebookPage* aParent,
                                                    const wxString& aPath ) :
    TEMPLATE_SELECTION_PANEL_BASE( aParent )
{
    m_parent = aParent;
    m_templatesPath = aPath;
    m_isUserTemplates = false;
}


void TEMPLATE_SELECTION_PANEL::AddTemplateWidget( TEMPLATE_WIDGET* aTemplateWidget )
{
    m_SizerChoice->Add( aTemplateWidget );
    Layout();
}


// Sort the widgets alphabetically, leaving Default at the top
void TEMPLATE_SELECTION_PANEL::SortAlphabetically()
{
    std::vector<TEMPLATE_WIDGET*> sortedList;
    TEMPLATE_WIDGET*              default_temp = nullptr;
    size_t                        count = m_SizerChoice->GetItemCount();

    if( count <= 1 )
        return;

    for( size_t idx = 0; idx < count; idx++ )
    {
        wxSizerItem* item = m_SizerChoice->GetItem( idx );
        if( item && item->IsWindow() )
        {
            TEMPLATE_WIDGET* temp = static_cast<TEMPLATE_WIDGET*>( item->GetWindow() );

            const wxString title = *temp->GetTemplate()->GetTitle();

            if( default_temp == nullptr && title.CmpNoCase( "default" ) == 0 )
                default_temp = temp;
            else
                sortedList.push_back( temp );
        }
    }

    std::sort(
            sortedList.begin(), sortedList.end(),
            []( TEMPLATE_WIDGET* aWidgetA, TEMPLATE_WIDGET* aWidgetB ) -> bool
    {
                   const wxString* a = aWidgetA->GetTemplate()->GetTitle();
                   const wxString* b = aWidgetB->GetTemplate()->GetTitle();

                   return ( *a ).CmpNoCase( *b ) < 0;
    });

    m_SizerChoice->Clear( false );

    if( default_temp != nullptr )
        m_SizerChoice->Add( default_temp );

    for (TEMPLATE_WIDGET* temp : sortedList)
    {
        m_SizerChoice->Add( temp );
    }

    Layout();
}


TEMPLATE_WIDGET::TEMPLATE_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog ) :
    TEMPLATE_WIDGET_BASE( aParent )
{
    m_parent = aParent;
    m_dialog = aDialog;
    m_isUserTemplate = false;

    // wxWidgets_3.xx way of doing the same...
    // Bind(wxEVT_LEFT_DOWN, &TEMPLATE_WIDGET::OnMouse, this );

    m_bitmapIcon->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ),
                           nullptr, this );
    m_staticTitle->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ),
                            nullptr, this );

    // Add right-click handler
    m_bitmapIcon->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::onRightClick ),
                           nullptr, this );
    m_staticTitle->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::onRightClick ),
                            nullptr, this );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::onRightClick ),
             nullptr, this );

    // Add double-click handler to activate the template (like OK button)
    m_bitmapIcon->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( TEMPLATE_WIDGET::OnDoubleClick ),
                           nullptr, this );
    m_staticTitle->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( TEMPLATE_WIDGET::OnDoubleClick ),
                            nullptr, this );
    Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( TEMPLATE_WIDGET::OnDoubleClick ),
             nullptr, this );

    // We're not selected until we're clicked
    Unselect();

    // Start with template being NULL
    m_currTemplate = nullptr;
}


void TEMPLATE_WIDGET::Select()
{
    m_dialog->SetWidget( this );
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
    m_staticTitle->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
    m_selected = true;
    Refresh();
}


void TEMPLATE_WIDGET::Unselect()
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_staticTitle->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
    m_selected = false;
    Refresh();
}


void TEMPLATE_WIDGET::SetTemplate( PROJECT_TEMPLATE* aTemplate )
{
    m_currTemplate = aTemplate;
    m_staticTitle->SetFont( KIUI::GetInfoFont( this ) );
    m_staticTitle->SetLabel( *aTemplate->GetTitle() );
    m_staticTitle->Wrap( 100 );
    wxBitmap* icon = aTemplate->GetIcon();

    if( icon && icon->IsOk() )
    {
        wxSize maxSize = m_bitmapIcon->GetSize();

        if( icon->GetWidth() > maxSize.x || icon->GetHeight() > maxSize.y )
        {
            double   scale = std::min( (double) maxSize.x / icon->GetWidth(),
                                       (double) maxSize.y / icon->GetHeight() );
            wxImage  image = icon->ConvertToImage();
            int      w = wxRound( icon->GetWidth() * scale );
            int      h = wxRound( icon->GetHeight() * scale );
            image.Rescale( w, h, wxIMAGE_QUALITY_HIGH );
            m_bitmapIcon->SetBitmap( wxBitmap( image ) );
        }
        else
        {
            m_bitmapIcon->SetBitmap( *icon );
        }
    }
    else
        m_bitmapIcon->SetBitmap( KiBitmap( BITMAPS::icon_kicad ) );
}


void TEMPLATE_WIDGET::OnMouse( wxMouseEvent& event )
{
    // Toggle selection here
    Select();
    event.Skip();
}


void TEMPLATE_WIDGET::OnDoubleClick( wxMouseEvent& event )
{
    // Double-click acts like pressing OK button
    Select();
    m_dialog->EndModal( wxID_OK );
    event.Skip();
}


void TEMPLATE_WIDGET::onRightClick( wxMouseEvent& event )
{
    // Only show context menu for user templates
    if( !m_isUserTemplate || !m_currTemplate )
    {
        event.Skip();
        return;
    }

    wxMenu menu;
    menu.Append( wxID_EDIT, _( "Edit Template" ) );
    menu.Append( wxID_COPY, _( "Duplicate Template" ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
               [this]( wxCommandEvent& evt )
               {
                   if( evt.GetId() == wxID_EDIT )
                       onEditTemplate( evt );
                   else if( evt.GetId() == wxID_COPY )
                       onDuplicateTemplate( evt );
               } );

    PopupMenu( &menu );
}


void TEMPLATE_WIDGET::onEditTemplate( wxCommandEvent& event )
{
    if( !m_currTemplate )
        return;

    // Get the template's base path
    wxFileName templatePath = m_currTemplate->GetHtmlFile();
    templatePath.RemoveLastDir();  // Remove "meta" dir

    // Find a .kicad_pro file in the template directory
    wxDir dir( templatePath.GetPath() );

    if( !dir.IsOpened() )
    {
        DisplayErrorMessage( m_dialog,
                            _( "Could not open template directory." ) );
        return;
    }

    wxString filename;
    bool found = dir.GetFirst( &filename, "*.kicad_pro", wxDIR_FILES );

    if( !found )
    {
        DisplayErrorMessage( m_dialog,
                            _( "No project file found in template directory." ) );
        return;
    }

    wxFileName projectFile( templatePath.GetPath(), filename );

    // Store the project path in the dialog so the caller can handle it
    m_dialog->SetProjectToEdit( projectFile.GetFullPath() );

    // Close with wxID_APPLY to indicate we want to edit, not create
    m_dialog->EndModal( wxID_APPLY );
}



void TEMPLATE_WIDGET::onDuplicateTemplate( wxCommandEvent& event )
{
    if( !m_currTemplate )
        return;

    // Get the template's base path
    wxFileName templatePath = m_currTemplate->GetHtmlFile();
    templatePath.RemoveLastDir();  // Remove "meta" dir
    wxString srcTemplatePath = templatePath.GetPath();
    wxString srcTemplateName = m_currTemplate->GetPrjDirName();

    // Ask for new template name
    wxTextEntryDialog nameDlg( m_dialog,
                               _( "Enter name for the new template:" ),
                               _( "Duplicate Template" ),
                               srcTemplateName + _( "_copy" ) );

    if( nameDlg.ShowModal() != wxID_OK )
        return;

    wxString newTemplateName = nameDlg.GetValue();

    if( newTemplateName.IsEmpty() )
    {
        DisplayErrorMessage( m_dialog, _( "Template name cannot be empty." ) );
        return;
    }

    // Get the user templates directory from the dialog
    wxString userTemplatesPath = m_dialog->GetUserTemplatesPath();

    if( userTemplatesPath.IsEmpty() )
    {
        DisplayErrorMessage( m_dialog, _( "Could not find user templates directory." ) );
        return;
    }

    // Create destination directory in user templates folder
    wxFileName destPath( userTemplatesPath, wxEmptyString );
    destPath.AppendDir( newTemplateName );
    wxString newTemplatePath = destPath.GetPath();

    if( destPath.DirExists() )
    {
        DisplayErrorMessage( m_dialog,
                            wxString::Format( _( "Directory '%s' already exists." ),
                                            newTemplatePath ) );
        return;
    }

    if( !destPath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        DisplayErrorMessage( m_dialog,
                            wxString::Format( _( "Could not create directory '%s'." ),
                                            newTemplatePath ) );
        return;
    }

    // Use shared traverser to copy all files with proper renaming
    // Pass nullptr for frame to enable simple copy mode (no KIFACE handling)
    wxDir sourceDir( srcTemplatePath );

    if( !sourceDir.IsOpened() )
    {
        DisplayErrorMessage( m_dialog, _( "Could not open source template directory." ) );
        return;
    }

    PROJECT_TREE_TRAVERSER traverser( nullptr, srcTemplatePath, srcTemplateName,
                                     newTemplatePath, newTemplateName );

    sourceDir.Traverse( traverser );

    if( !traverser.GetErrors().empty() )
    {
        DisplayErrorMessage( m_dialog, traverser.GetErrors() );
        return;
    }

    // Update the title in meta/info.html if it exists
    wxFileName metaHtmlFile( newTemplatePath, "info.html" );
    metaHtmlFile.AppendDir( "meta" );

    if( metaHtmlFile.FileExists() )
    {
        wxTextFile htmlFile( metaHtmlFile.GetFullPath() );

        if( htmlFile.Open() )
        {
            bool modified = false;

            for( size_t i = 0; i < htmlFile.GetLineCount(); i++ )
            {
                wxString line = htmlFile.GetLine( i );

                // Update the title tag - replace content between <title> and </title>
                if( line.Contains( wxT( "<title>" ) ) && line.Contains( wxT( "</title>" ) ) )
                {
                    int titleStart = line.Find( wxT( "<title>" ) );
                    int titleEnd = line.Find( wxT( "</title>" ) );

                    if( titleStart != wxNOT_FOUND && titleEnd != wxNOT_FOUND && titleEnd > titleStart )
                    {
                        wxString before = line.Left( titleStart + 7 );  // Include "<title>"
                        wxString after = line.Mid( titleEnd );          // Include "</title>" onwards
                        line = before + newTemplateName + after;
                        htmlFile[i] = line;
                        modified = true;
                    }
                }
            }

            if( modified )
                htmlFile.Write();

            htmlFile.Close();
        }
    }

    DisplayInfoMessage( m_dialog, wxString::Format( _( "Template duplicated successfully to '%s'." ),
                                                    newTemplatePath ) );

    // Refresh the widget list to show the new template
    m_dialog->replaceCurrentPage();
}



void DIALOG_TEMPLATE_SELECTOR::OnPageChange( wxNotebookEvent& event )
{
    int newPage = event.GetSelection();
    int oldPage = event.GetOldSelection();

    // Move webview panel from old page to new page
    if( oldPage != wxNOT_FOUND && (unsigned)oldPage < m_panels.size() )
    {
        // Detach webview from old panel
        m_panels[oldPage]->m_SizerBase->Detach( m_webviewPanel );
        m_panels[oldPage]->Layout();
    }

    if( newPage != wxNOT_FOUND && (unsigned)newPage < m_panels.size() )
    {
        // Reparent the webview to the new panel
        m_webviewPanel->Reparent( m_panels[newPage] );

        // Attach webview to new panel
        m_panels[newPage]->m_SizerBase->Add( m_webviewPanel, 1, wxEXPAND | wxALL, 5 );
        m_panels[newPage]->Layout();

        // Update template path
        m_tcTemplatePath->SetValue( m_panels[newPage]->GetPath() );

        // Reset to welcome page when switching tabs if no template selected
        m_webviewPanel->SetPage( GetWelcomeHtml() );
    }

    event.Skip();
}


DIALOG_TEMPLATE_SELECTOR::DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent, const wxPoint& aPos, const wxSize& aSize,
                                                    std::vector<std::pair<wxString, wxFileName>> aTitleDirList,
                                                    const wxFileName& aDefaultTemplate ) :
        DIALOG_TEMPLATE_SELECTOR_BASE( aParent, wxID_ANY, _( "Project Template Selector" ), aPos, aSize )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_reloadButton->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );

    m_webviewPanel->BindLoadedEvent();

    m_selectedWidget = nullptr;
    m_defaultTemplatePath = aDefaultTemplate;
    m_defaultWidget = nullptr;

    for( auto& [title, pathFname] : aTitleDirList )
    {
        pathFname.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
        wxString path = pathFname.GetFullPath(); // caller ensures this ends with file separator.

        TEMPLATE_SELECTION_PANEL* tpanel = new TEMPLATE_SELECTION_PANEL( m_notebook, path );

        // Mark the first panel as "User Templates" if the title matches
        if( title == _( "User Templates" ) )
            tpanel->SetIsUserTemplates( true );

        m_panels.push_back( tpanel );

        m_notebook->AddPage( tpanel, title );

        if( m_notebook->GetPageCount() == 1 )
            m_tcTemplatePath->SetValue( path );

        buildPageContent( path, m_notebook->GetPageCount() - 1 );
    }

    // Move webview panel from dialog to first template selection panel
    if( !m_panels.empty() )
    {
        // Find the sizer containing the webview and detach it
        wxSizer* parentSizer = m_webviewPanel->GetContainingSizer();

        if( parentSizer )
        {
            parentSizer->Detach( m_webviewPanel );
        }

        // Reparent the webview to the first panel
        m_webviewPanel->Reparent( m_panels[0] );

        // Add webview to first panel
        m_panels[0]->m_SizerBase->Add( m_webviewPanel, 1, wxEXPAND | wxALL, 5 );
        m_panels[0]->Layout();
    }

    if( m_defaultWidget )
        m_defaultWidget->Select();

    // Make OK button the default so Enter triggers it
    m_sdbSizerOK->SetDefault();

    // Handle Enter key in the template path text control to trigger OK
    m_tcTemplatePath->Bind( wxEVT_TEXT_ENTER,
                            [this]( wxCommandEvent& )
                            {
                                EndModal( wxID_OK );
                            } );

    // Set welcome HTML after dialog is fully constructed
    CallAfter(
            [this]()
            {
                #if defined (_WIN32)
                wxSafeYield();
                m_tcTemplatePath->SelectNone();
                #endif

                if( m_selectedWidget )
                {
                    wxFileName htmlFile = m_selectedWidget->GetTemplate()->GetHtmlFile();

                    if( htmlFile.FileExists() && htmlFile.IsFileReadable() && htmlFile.GetSize() > 100 /* Basic HTML */ )
                        m_webviewPanel->LoadURL( wxFileName::FileNameToURL( htmlFile ) );
                    else
                        m_webviewPanel->SetPage( GetWelcomeHtml() );
                }
                else
                {
                    m_webviewPanel->SetPage( GetWelcomeHtml() );
                }
            } );

    // When all widgets have the size fixed, call finishDialogSettings to update sizers
    finishDialogSettings();
}


void DIALOG_TEMPLATE_SELECTOR::SetWidget( TEMPLATE_WIDGET* aWidget )
{
    if( m_selectedWidget != nullptr )
        m_selectedWidget->Unselect();

    m_selectedWidget = aWidget;
    wxFileName htmlFile = aWidget->GetTemplate()->GetHtmlFile();

    if( htmlFile.FileExists() && htmlFile.IsFileReadable() )
        m_webviewPanel->LoadURL( wxFileName::FileNameToURL( htmlFile ) );
    else
        m_webviewPanel->SetPage( GetTemplateInfoHtml( *aWidget->GetTemplate()->GetTitle() ) );
}


void DIALOG_TEMPLATE_SELECTOR::AddTemplate( int aPage, PROJECT_TEMPLATE* aTemplate )
{
    TEMPLATE_WIDGET* w = new TEMPLATE_WIDGET( m_panels[aPage]->m_scrolledWindow, this  );
    w->SetTemplate( aTemplate );
    w->SetIsUserTemplate( m_panels[aPage]->IsUserTemplates() );
    m_panels[aPage]->AddTemplateWidget( w );
    m_allWidgets.push_back( w );

    wxFileName base = aTemplate->GetHtmlFile();
    base.RemoveLastDir();

    if( !m_defaultWidget || (m_defaultTemplatePath.IsOk() && base == m_defaultTemplatePath) )
        m_defaultWidget = w;

    wxString dirName = base.GetDirs().IsEmpty() ? wxString() : base.GetDirs().back();

    if( dirName.CmpNoCase( "default" ) == 0 )
    {
        // Prefer a directory literally named 'default'
        m_defaultWidget = w;
    }
}


PROJECT_TEMPLATE* DIALOG_TEMPLATE_SELECTOR::GetSelectedTemplate()
{
    return m_selectedWidget? m_selectedWidget->GetTemplate() : nullptr;
}

PROJECT_TEMPLATE* DIALOG_TEMPLATE_SELECTOR::GetDefaultTemplate()
{
    return m_defaultWidget? m_defaultWidget->GetTemplate() : nullptr;
}


wxString DIALOG_TEMPLATE_SELECTOR::GetUserTemplatesPath() const
{
    // Find the first panel marked as user templates
    for( const TEMPLATE_SELECTION_PANEL* panel : m_panels )
    {
        if( panel->IsUserTemplates() )
            return panel->GetPath();
    }

    // If no user templates panel found, return empty string
    return wxEmptyString;
}


void DIALOG_TEMPLATE_SELECTOR::buildPageContent( const wxString& aPath, int aPage )
{
    // Track initial template count to detect if any templates were added
    size_t initialTemplateCount = m_panels[aPage]->m_SizerChoice->GetItemCount();

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

    m_panels[aPage]->SortAlphabetically();

    // Check if any templates were added; if not, display "No templates found" message
    size_t finalTemplateCount = m_panels[aPage]->m_SizerChoice->GetItemCount();

    if( finalTemplateCount == initialTemplateCount )
    {
        // No templates found in this directory - show message in webview
        if( (unsigned)aPage < m_panels.size() )
        {
            // Get the panel's webview if it exists (it may not be directly accessible)
            // Instead, we'll set the message on the main webview if it's associated with this panel
            if( m_selectedWidget == nullptr && aPage == m_notebook->GetSelection() )
            {
                m_webviewPanel->SetPage( GetNoTemplatesHtml() );
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

    // Save the user template flag before deleting
    bool wasUserTemplates = false;

    if( (unsigned)page < m_panels.size() )
        wasUserTemplates = m_panels[page]->IsUserTemplates();

    // Block all events to the notebook and its children
    wxEventBlocker blocker( m_notebook );

    // Detach webview from current panel before deleting it
    if( (unsigned)page < m_panels.size() )
    {
        m_panels[page]->m_SizerBase->Detach( m_webviewPanel );
        m_webviewPanel->Reparent( this ); // Reparent to dialog
        m_webviewPanel->Hide(); // Hide webview panel temporarily
    }

    m_notebook->DeletePage( page );

    TEMPLATE_SELECTION_PANEL* tpanel = new TEMPLATE_SELECTION_PANEL( m_notebook, currPath );
    tpanel->SetIsUserTemplates( wasUserTemplates );  // Restore the flag
    m_panels[page] = tpanel;
    m_notebook->InsertPage( page, tpanel, title, true );

    // Reparent and add webview back to the new panel
    m_webviewPanel->Reparent( tpanel );
    m_webviewPanel->Show();
    tpanel->m_SizerBase->Add( m_webviewPanel, 1, wxEXPAND | wxALL, 5 );

    buildPageContent( m_tcTemplatePath->GetValue(), page );

    m_selectedWidget = nullptr;
    // Reset to welcome page after rebuilding
    m_webviewPanel->SetPage( GetWelcomeHtml() );

    Layout();
}