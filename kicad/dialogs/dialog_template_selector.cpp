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
#include <wx_filename.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/settings.h>

static const wxString GetWelcomeHtml()
{
    return wxString(
    "<!DOCTYPE html>"
    "<html lang=\"en\">"
    "<head>"
    "<meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
    "<title>KiCad Project Template Selector</title>"
    "<style>"
    "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #333; min-height: 100vh; box-sizing: border-box; }"
    ".container { max-width: 800px; margin: 0 auto; background: rgba(255, 255, 255, 0.95); border-radius: 12px; padding: 30px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); backdrop-filter: blur(10px); }"
    ".header { text-align: center; margin-bottom: 30px; }"
    ".logo { font-size: 2.5rem; font-weight: bold; color: #4a5568; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1); }"
    ".subtitle { font-size: 1.2rem; color: #666; margin-bottom: 20px; }"
    ".welcome-card { background: linear-gradient(135deg, #4299e1, #3182ce); color: white; padding: 25px; border-radius: 10px; margin-bottom: 25px; box-shadow: 0 4px 15px rgba(66, 153, 225, 0.3); }"
    ".welcome-card h2 { margin-top: 0; font-size: 1.8rem; margin-bottom: 15px; }"
    ".instructions { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 25px; }"
    ".instruction-card { background: #f7fafc; border: 2px solid #e2e8f0; border-radius: 8px; padding: 20px; transition: all 0.3s ease; position: relative; overflow: hidden; }"
    ".instruction-card:hover { transform: translateY(-2px); box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1); border-color: #4299e1; }"
    ".instruction-card::before { content: ''; position: absolute; top: 0; left: 0; width: 4px; height: 100%; background: linear-gradient(135deg, #4299e1, #3182ce); }"
    ".instruction-card h3 { color: #2d3748; margin-top: 0; margin-bottom: 10px; font-size: 1.3rem; }"
    ".instruction-card p { color: #4a5568; line-height: 1.6; margin: 0; }"
    ".features { background: #f0fff4; border: 2px solid #9ae6b4; border-radius: 8px; padding: 20px; margin-bottom: 25px; }"
    ".features h3 { color: #22543d; margin-top: 0; margin-bottom: 15px; font-size: 1.4rem; }"
    ".features ul { color: #2f855a; line-height: 1.8; margin: 0; padding-left: 20px; }"
    ".features li { margin-bottom: 8px; }"
    ".tips { background: #fffaf0; border: 2px solid #fbd38d; border-radius: 8px; padding: 20px; }"
    ".tips h3 { color: #c05621; margin-top: 0; margin-bottom: 15px; font-size: 1.4rem; }"
    ".tips p { color: #c05621; line-height: 1.6; margin: 0 0 10px 0; }"
    ".highlight { background: linear-gradient(120deg, #a8edea 0%, #fed6e3 100%); padding: 2px 6px; border-radius: 4px; font-weight: 600; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class=\"container\">"
    "<div class=\"header\">"
    "<div class=\"logo\">KiCad 📑</div>"
    "<div class=\"subtitle\">" + _( "Project Template Selector" ) + "</div>"
    "</div>"
    "<div class=\"welcome-card\">"
    "<h2>" + _( "Welcome to Template Selection!" ) + "</h2>"
    "<p>" + _( "Choose from a variety of pre-configured project templates to jumpstart your PCB design. Templates provide ready-to-use project structures with common components, libraries, and design rules." ) + "</p>"
    "</div>"
    "<div class=\"instructions\">"
    "<div class=\"instruction-card\">"
    "<h3>→ " + _( "Browse Templates" ) + "</h3>"
    "<p>" + _( "Navigate through the template tabs above to explore different categories of project templates. Each tab contains templates organized by type or complexity." ) + "</p>"
    "</div>"
    "<div class=\"instruction-card\">"
    "<h3>→ " + _( "Select a Template" ) + "</h3>"
    "<p>" + _( "Click on any template in the list to " ) + "<span class=\"highlight\">" + _( "preview its details" ) + "</span>. " + _( "The template information will appear in this panel, showing descriptions, included components, and project structure." ) + "</p>"
    "</div>"
    "<div class=\"instruction-card\">"
    "<h3>→ " + _( "Customize Path" ) + "</h3>"
    "<p>" + _( "Use the " ) + "<span class=\"highlight\">" + _( "folder path field" ) + "</span> " + _( "above to browse custom template directories. Click the folder icon to browse, or the refresh icon to reload templates." ) + "</p>"
    "</div>"
    "<div class=\"instruction-card\">"
    "<h3>→ " + _( "Create Project" ) + "</h3>"
    "<p>" + _( "Once you've found the right template, click " ) + "<span class=\"highlight\">" + _( "OK" ) + "</span> " + _( "to create a new project based on the selected template. Your project will inherit all template settings and files." ) + "</p>"
    "</div>"
    "</div>"
    "<div class=\"features\">"
    "<h3>" + _( "What You Get with Templates" ) + "</h3>"
    "<ul>"
    "<li><strong>" + _( "Pre-configured libraries" ) + "</strong> " + _( "- Common components and footprints already linked" ) + "</li>"
    "<li><strong>" + _( "Design rules" ) + "</strong> " + _( "- Appropriate electrical and mechanical constraints" ) + "</li>"
    "<li><strong>" + _( "Layer stackups" ) + "</strong> " + _( "- Optimized for the intended application" ) + "</li>"
    "<li><strong>" + _( "Component placement" ) + "</strong> " + _( "- Basic layout and routing guidelines" ) + "</li>"
    "<li><strong>" + _( "Documentation" ) + "</strong> " + _( "- README files and design notes" ) + "</li>"
    "<li><strong>" + _( "Manufacturing files" ) + "</strong> " + _( "- Gerber and drill file configurations" ) + "</li>"
    "</ul>"
    "</div>"
    "<div class=\"tips\">"
    "<h3>" + _( "Pro Tips" ) + "</h3>"
    "<p><strong>" + _( "Start Simple:" ) + "</strong> " + _( "Begin with basic templates and add more elements as you go." ) + "</p>"
    "<p><strong>" + _( "Customize Later:" ) + "</strong> " + _( "Templates are starting points - you can modify libraries, rules, and layouts after project creation." ) + "</p>"
    "<p><strong>" + _( "Save Your Own:" ) + "</strong> " + _( "Once you develop preferred settings, create a custom template for future projects." ) + "</p>"
    "</div>"
    "</div>"
    "</body>"
    "</html>"
);
}

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


DIALOG_TEMPLATE_SELECTOR::DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent, const wxPoint& aPos,
                                                    const wxSize&                  aSize,
                                                    std::map<wxString, wxFileName> aTitleDirMap ) :
        DIALOG_TEMPLATE_SELECTOR_BASE( aParent, wxID_ANY, _( "Project Template Selector" ), aPos,
                                       aSize )
{
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_reloadButton->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );

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

    // Set welcome HTML after dialog is fully constructed
    CallAfter( [this]() {
        m_webviewPanel->SetPage( GetWelcomeHtml() );
    });

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
    {
        m_webviewPanel->LoadURL( wxFileName::FileNameToURL( htmlFile ) );
    }
    else
    {
        // Fallback to a simple template info page if no HTML file exists
        wxString templateHtml = wxString::Format(
            "<!DOCTYPE html>"
            "<html><head><meta charset='UTF-8'><style>"
            "body { font-family: Arial, sans-serif; margin: 20px; }"
            ".template-info { background: #f0f8ff; padding: 20px; border-radius: 8px; }"
            "h1 { color: #333; margin-top: 0; }"
            "</style></head><body>"
            "<div class='template-info'>"
            "<h1>%s</h1>"
            "<p>Template selected. Click OK to create a new project based on this template.</p>"
            "</div></body></html>",
            *aWidget->GetTemplate()->GetTitle()
        );
        m_webviewPanel->SetPage( templateHtml );
    }
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

    // Detach webview from current panel before deleting it
    if( (unsigned)page < m_panels.size() )
    {
        m_panels[page]->m_SizerBase->Detach( m_webviewPanel );
        m_webviewPanel->Reparent( this ); // Reparent to dialog
        m_webviewPanel->Hide(); // Hide webview panel temporarily
    }

    m_notebook->DeletePage( page );

    TEMPLATE_SELECTION_PANEL* tpanel = new TEMPLATE_SELECTION_PANEL( m_notebook, currPath );
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