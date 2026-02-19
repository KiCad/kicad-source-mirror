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
#include <kiplatform/ui.h>
#include <pgm_base.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
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
#include <wx/regex.h>
#include <confirm.h>
#include <project_tree_traverser.h>
#include "template_default_html.h"


static const wxChar* traceTemplateSelector = wxT( "KICAD_TEMPLATE_SELECTOR" );


TEMPLATE_MRU_WIDGET::TEMPLATE_MRU_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog,
                                          const wxString& aPath, const wxString& aTitle,
                                          const wxBitmap& aIcon ) :
        wxPanel( aParent, wxID_ANY ),
        m_dialog( aDialog ),
        m_templatePath( aPath )
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );

    wxStaticBitmap* icon = new wxStaticBitmap( this, wxID_ANY, aIcon );
    icon->SetMinSize( FromDIP( wxSize( 16, 16 ) ) );
    sizer->Add( icon, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 6 );

    wxStaticText* label = new wxStaticText( this, wxID_ANY, aTitle );
    label->SetFont( KIUI::GetInfoFont( this ) );
    sizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, 6 );

    SetSizer( sizer );
    Layout();

    Bind( wxEVT_LEFT_DOWN, &TEMPLATE_MRU_WIDGET::OnClick, this );
    Bind( wxEVT_LEFT_DCLICK, &TEMPLATE_MRU_WIDGET::OnDoubleClick, this );
    Bind( wxEVT_ENTER_WINDOW, &TEMPLATE_MRU_WIDGET::OnEnter, this );
    Bind( wxEVT_LEAVE_WINDOW, &TEMPLATE_MRU_WIDGET::OnLeave, this );

    icon->Bind( wxEVT_LEFT_DOWN, &TEMPLATE_MRU_WIDGET::OnClick, this );
    icon->Bind( wxEVT_LEFT_DCLICK, &TEMPLATE_MRU_WIDGET::OnDoubleClick, this );
    label->Bind( wxEVT_LEFT_DOWN, &TEMPLATE_MRU_WIDGET::OnClick, this );
    label->Bind( wxEVT_LEFT_DCLICK, &TEMPLATE_MRU_WIDGET::OnDoubleClick, this );
}


void TEMPLATE_MRU_WIDGET::OnClick( wxMouseEvent& event )
{
    // Just select the template in the list without showing the preview pane
    m_dialog->SelectTemplateByPath( m_templatePath, true );
    event.Skip();
}


void TEMPLATE_MRU_WIDGET::OnDoubleClick( wxMouseEvent& event )
{
    m_dialog->SelectTemplateByPath( m_templatePath );
    m_dialog->EndModal( wxID_OK );
    event.Skip();
}


void TEMPLATE_MRU_WIDGET::OnEnter( wxMouseEvent& event )
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ).ChangeLightness( 150 ) );
    Refresh();
    event.Skip();
}


void TEMPLATE_MRU_WIDGET::OnLeave( wxMouseEvent& event )
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    Refresh();
    event.Skip();
}


TEMPLATE_WIDGET::TEMPLATE_WIDGET( wxWindow* aParent, DIALOG_TEMPLATE_SELECTOR* aDialog ) :
        TEMPLATE_WIDGET_BASE( aParent )
{
    m_parent = aParent;
    m_dialog = aDialog;
    m_isUserTemplate = false;

    // Set a small minimum size to allow the dialog to shrink
    // The actual size will be determined by the parent sizer
    SetMinSize( FromDIP( wxSize( 200, -1 ) ) );

    m_bitmapIcon->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), nullptr, this );
    m_titleLabel->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), nullptr, this );
    m_descLabel->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), nullptr, this );

    Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::OnMouse ), nullptr, this );

    m_bitmapIcon->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::onRightClick ), nullptr, this );
    m_titleLabel->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::onRightClick ), nullptr, this );
    m_descLabel->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::onRightClick ), nullptr, this );
    Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( TEMPLATE_WIDGET::onRightClick ), nullptr, this );

    m_bitmapIcon->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( TEMPLATE_WIDGET::OnDoubleClick ), nullptr, this );
    m_titleLabel->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( TEMPLATE_WIDGET::OnDoubleClick ), nullptr, this );
    m_descLabel->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( TEMPLATE_WIDGET::OnDoubleClick ), nullptr, this );
    Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( TEMPLATE_WIDGET::OnDoubleClick ), nullptr, this );

    // Set description text to use a smaller font
    m_descLabel->SetFont( KIUI::GetInfoFont( this ) );

#if wxCHECK_VERSION( 3, 3, 2 )
    m_descLabel->SetWindowStyle( wxST_WRAP );
#endif

    // Bind size event for dynamic text wrapping
    Bind( wxEVT_SIZE, &TEMPLATE_WIDGET::OnSize, this );

    Unselect();

    m_currTemplate = nullptr;
}


void TEMPLATE_WIDGET::Select()
{
    m_dialog->SetWidget( this );
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
    m_titleLabel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
    m_descLabel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
    m_selected = true;
    Refresh();
}


void TEMPLATE_WIDGET::SelectWithoutStateChange()
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
    m_titleLabel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
    m_descLabel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
    m_selected = true;
    Refresh();
}


void TEMPLATE_WIDGET::Unselect()
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_titleLabel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
    m_descLabel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
    m_selected = false;
    Refresh();
}


void TEMPLATE_WIDGET::SetTemplate( PROJECT_TEMPLATE* aTemplate )
{
    m_currTemplate = aTemplate;
    m_titleLabel->SetFont( KIUI::GetInfoFont( this ).Bold() );
    m_titleLabel->SetLabel( *aTemplate->GetTitle() );

    // Generate bitmap bundle from template icon bitmap (64x64)
    // so wxStaticBitmap can size itself to the closest match
    const std::vector<int> c_bitmapSizes = { 48, 64, 96, 128 };

    wxBitmapBundle     bundle;
    wxVector<wxBitmap> bitmaps;
    wxBitmap*          icon = aTemplate->GetIcon();

    if( icon && icon->IsOk() )
        bundle = *icon;
    else
        bundle = KiBitmapBundleDef( BITMAPS::icon_kicad, c_bitmapSizes[0] );

    wxSize defSize = bundle.GetDefaultSize();

    for( const int genSize : c_bitmapSizes )
    {
        double scale = std::min( (double) genSize / defSize.GetWidth(),
                                 (double) genSize / defSize.GetHeight() );

        wxSize scaledSize( wxRound( defSize.x * scale ),
                           wxRound( defSize.y * scale ) );

        wxBitmap scaled = bundle.GetBitmap( scaledSize );
        scaled.SetScaleFactor( 1.0 );

        bitmaps.push_back( scaled );
    }

    m_bitmapIcon->SetBitmap( wxBitmapBundle::FromBitmaps( bitmaps ) );
}


void TEMPLATE_WIDGET::SetDescription( const wxString& aDescription )
{
    m_description = aDescription;

    // Truncate long descriptions to approximately 3 lines worth
    wxString displayDesc = aDescription;

    if( displayDesc.Length() > 120 )
        displayDesc = displayDesc.Left( 120 ) + wxS( "..." );

    m_descLabel->SetLabel( displayDesc );
}


void TEMPLATE_WIDGET::OnSize( wxSizeEvent& event )
{
    event.Skip();

    // wx 3.3.2+ can wrap automatically
#if !wxCHECK_VERSION( 3, 3, 2 )
    // Calculate available width for description text (widget width minus icon and padding)
    int wrapWidth = GetClientSize().GetWidth() - 48 - 20;  // 48px icon + 20px padding

    if( wrapWidth > 100 )
    {
        // Recalculate the truncated description from the original stored description
        // to avoid cumulative wrapping artifacts
        wxString displayDesc = m_description;

        if( displayDesc.Length() > 120 )
            displayDesc = displayDesc.Left( 120 ) + wxS( "..." );

        m_descLabel->SetLabel( displayDesc );
        m_descLabel->Wrap( wrapWidth );
        Layout();
    }
#endif
}


void TEMPLATE_WIDGET::OnMouse( wxMouseEvent& event )
{
    Select();
    event.Skip();
}


void TEMPLATE_WIDGET::OnDoubleClick( wxMouseEvent& event )
{
    Select();
    m_dialog->EndModal( wxID_OK );
    event.Skip();
}


void TEMPLATE_WIDGET::onRightClick( wxMouseEvent& event )
{
    if( !m_currTemplate )
    {
        event.Skip();
        return;
    }

    wxMenu menu;
    menu.Append( wxID_EDIT, m_isUserTemplate ? _( "Edit Template" ) : _( "Open Template (Read-Only)" ) );
    menu.Append( wxID_OPEN, _( "Open Template Folder" ) );
    menu.Append( wxID_COPY, _( "Duplicate Template" ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED,
               [this]( wxCommandEvent& evt )
               {
                   if( evt.GetId() == wxID_EDIT )
                       onEditTemplate( evt );
                   if( evt.GetId() == wxID_OPEN )
                       onOpenFolder( evt );
                   else if( evt.GetId() == wxID_COPY )
                       onDuplicateTemplate( evt );
               } );

    PopupMenu( &menu );
}


void TEMPLATE_WIDGET::onEditTemplate( wxCommandEvent& event )
{
    if( !m_currTemplate )
        return;

    wxFileName templatePath = m_currTemplate->GetHtmlFile();
    templatePath.RemoveLastDir();

    wxDir dir( templatePath.GetPath() );

    if( !dir.IsOpened() )
    {
        DisplayErrorMessage( m_dialog, _( "Could not open template directory." ) );
        return;
    }

    wxString filename;
    bool found = dir.GetFirst( &filename, "*.kicad_pro", wxDIR_FILES );

    if( !found )
    {
        DisplayErrorMessage( m_dialog, _( "No project file found in template directory." ) );
        return;
    }

    wxFileName projectFile( templatePath.GetPath(), filename );

    m_dialog->SetProjectToEdit( projectFile.GetFullPath() );

    m_dialog->EndModal( wxID_APPLY );
}


void TEMPLATE_WIDGET::onOpenFolder( wxCommandEvent& event )
{
    if( !m_currTemplate )
        return;

    wxFileName templatePath = m_currTemplate->GetHtmlFile();
    templatePath.RemoveLastDir();

    if( !wxLaunchDefaultApplication( templatePath.GetPath() ) )
        DisplayError( this, wxString::Format( _( "Failed to open '%s'." ), templatePath.GetPath() ) );
}


void TEMPLATE_WIDGET::onDuplicateTemplate( wxCommandEvent& event )
{
    if( !m_currTemplate )
        return;

    wxFileName templatePath = m_currTemplate->GetHtmlFile();
    templatePath.RemoveLastDir();
    wxString srcTemplatePath = templatePath.GetPath();
    wxString srcTemplateName = m_currTemplate->GetPrjDirName();

    wxTextEntryDialog nameDlg( m_dialog, _( "Enter name for the new template:" ), _( "Duplicate Template" ),
                               srcTemplateName + _( "_copy" ) );

    if( nameDlg.ShowModal() != wxID_OK )
        return;

    wxString newTemplateName = nameDlg.GetValue();

    if( newTemplateName.IsEmpty() )
    {
        DisplayErrorMessage( m_dialog, _( "Template name cannot be empty." ) );
        return;
    }

    wxString userTemplatesPath = m_dialog->GetUserTemplatesPath();

    if( userTemplatesPath.IsEmpty() )
    {
        DisplayErrorMessage( m_dialog, _( "Could not find user templates directory." ) );
        return;
    }

    wxFileName destPath( userTemplatesPath, wxEmptyString );
    destPath.AppendDir( newTemplateName );
    wxString newTemplatePath = destPath.GetPath();

    if( destPath.DirExists() )
    {
        DisplayErrorMessage( m_dialog, wxString::Format( _( "Directory '%s' already exists." ), newTemplatePath ) );
        return;
    }

    if( !destPath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        DisplayErrorMessage( m_dialog, wxString::Format( _( "Could not create directory '%s'." ), newTemplatePath ) );
        return;
    }

    wxDir sourceDir( srcTemplatePath );

    if( !sourceDir.IsOpened() )
    {
        DisplayErrorMessage( m_dialog, _( "Could not open source template directory." ) );
        return;
    }

    PROJECT_TREE_TRAVERSER traverser( nullptr, srcTemplatePath, srcTemplateName, newTemplatePath, newTemplateName );

    sourceDir.Traverse( traverser );

    if( !traverser.GetErrors().empty() )
    {
        DisplayErrorMessage( m_dialog, traverser.GetErrors() );
        return;
    }

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

                if( line.Contains( wxT( "<title>" ) ) && line.Contains( wxT( "</title>" ) ) )
                {
                    int titleStart = line.Find( wxT( "<title>" ) );
                    int titleEnd = line.Find( wxT( "</title>" ) );

                    if( titleStart != wxNOT_FOUND && titleEnd != wxNOT_FOUND && titleEnd > titleStart )
                    {
                        wxString before = line.Left( titleStart + 7 );
                        wxString after = line.Mid( titleEnd );
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

    // The file copy triggers OnFileSystemEvent which starts a refresh timer. If we show a
    // modal info dialog here, the timer fires during the modal event loop and destroys this
    // TEMPLATE_WIDGET while we're still on its call stack. Defer both the message and the
    // refresh to run after the current event handler returns.
    DIALOG_TEMPLATE_SELECTOR* dlg = m_dialog;

    dlg->CallAfter(
            [dlg, newTemplatePath]()
            {
                DisplayInfoMessage( dlg, wxString::Format( _( "Template duplicated successfully to '%s'." ),
                                                           newTemplatePath ) );
                dlg->RefreshTemplateList();
            } );
}


DIALOG_TEMPLATE_SELECTOR::DIALOG_TEMPLATE_SELECTOR( wxWindow* aParent, const wxPoint& aPos,
                                                    const wxSize& aSize, const wxString& aUserTemplatesPath,
                                                    const wxString& aSystemTemplatesPath,
                                                    const std::vector<wxString>& aRecentTemplates ) :
        DIALOG_TEMPLATE_SELECTOR_BASE( aParent, wxID_ANY, _( "Project Template Selector" ), aPos, aSize ),
        m_state( DialogState::Initial ),
        m_selectedWidget( nullptr ),
        m_selectedTemplate( nullptr ),
        m_userTemplatesPath( aUserTemplatesPath ),
        m_systemTemplatesPath( aSystemTemplatesPath ),
        m_recentTemplates( aRecentTemplates ),
        m_searchTimer( this ),
        m_refreshTimer( this ),
        m_watcher( nullptr ),
        m_webviewPanel( nullptr ),
        m_loadingExternalHtml( false )
{
    // The base class now provides the UI structure via wxFormBuilder.
    // Configure the scrolled windows.
    m_scrolledMRU->SetScrollRate( 0, 25 );
    m_scrolledTemplates->SetScrollRate( 0, 25 );
    m_scrolledTemplates->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    // Override minimum sizes to allow dialog shrinking (base class Fit() sets large sizes)
    m_scrolledTemplates->SetMinSize( FromDIP( wxSize( 300, 300 ) ) );
    m_panelTemplates->SetMinSize( FromDIP( wxSize( 500, 500 ) ) );

    // Configure the search control
    m_searchCtrl->ShowSearchButton( true );
    m_searchCtrl->ShowCancelButton( true );

    Bind( wxEVT_TIMER, &DIALOG_TEMPLATE_SELECTOR::OnSearchTimer, this, m_searchTimer.GetId() );
    Bind( wxEVT_TIMER, &DIALOG_TEMPLATE_SELECTOR::OnRefreshTimer, this, m_refreshTimer.GetId() );
    Bind( wxEVT_FSWATCHER, &DIALOG_TEMPLATE_SELECTOR::OnFileSystemEvent, this );
    Bind( wxEVT_SYS_COLOUR_CHANGED, &DIALOG_TEMPLATE_SELECTOR::OnSysColourChanged, this );
    m_scrolledTemplates->Bind( wxEVT_SIZE, &DIALOG_TEMPLATE_SELECTOR::OnScrolledTemplatesSize, this );

    // Set initial filter based on saved setting
    KICAD_SETTINGS* settings = Pgm().GetSettingsManager().GetAppSettings<KICAD_SETTINGS>( "kicad" );

    if( settings )
    {
        int filterChoice = settings->m_TemplateFilterChoice;

        if( filterChoice >= 0 && filterChoice < static_cast<int>( m_filterChoice->GetCount() ) )
            m_filterChoice->SetSelection( filterChoice );
        else
            m_filterChoice->SetSelection( 0 );
    }
    else
    {
        m_filterChoice->SetSelection( 0 );
    }

    BuildMRUList();
    BuildTemplateList();
    SetupFileWatcher();

    // Auto-select the most recently used template if available
    if( !m_recentTemplates.empty() )
        SelectTemplateByPath( m_recentTemplates.front(), true );

    SetState( DialogState::Initial );

    m_sdbSizerOK->SetDefault();

    finishDialogSettings();
}


DIALOG_TEMPLATE_SELECTOR::~DIALOG_TEMPLATE_SELECTOR()
{
    m_searchTimer.Stop();
    m_refreshTimer.Stop();

    if( m_watcher )
    {
        m_watcher->RemoveAll();
        m_watcher->SetOwner( nullptr );
        delete m_watcher;
        m_watcher = nullptr;
    }
}


void DIALOG_TEMPLATE_SELECTOR::SetState( DialogState aState )
{
    m_state = aState;

    switch( aState )
    {
    case DialogState::Initial:
        m_panelMRU->Show();
        m_panelPreview->Hide();
        m_btnBack->Enable( false );
        break;

    case DialogState::Preview:
        m_panelMRU->Hide();
        m_panelPreview->Show();
        m_btnBack->Enable( true );
        break;

    case DialogState::MRUWithPreview:
        m_panelMRU->Show();
        m_panelPreview->Show();
        m_btnBack->Enable( true );
        break;
    }

    Layout();
}


void DIALOG_TEMPLATE_SELECTOR::BuildMRUList()
{
    // Clear existing MRU widgets
    for( TEMPLATE_MRU_WIDGET* widget : m_mruWidgets )
    {
        m_sizerMRU->Detach( widget );
        widget->Destroy();
    }

    m_mruWidgets.clear();

    for( const wxString& path : m_recentTemplates )
    {
        wxFileName templateDir;
        templateDir.AssignDir( path );

        if( !templateDir.DirExists() )
            continue;

        PROJECT_TEMPLATE templ( path );

        wxString* title = templ.GetTitle();
        wxBitmap* icon = templ.GetIcon();

        wxBitmap scaledIcon;

        if( icon && icon->IsOk() )
        {
            wxImage img = icon->ConvertToImage();
            img.Rescale( 16, 16, wxIMAGE_QUALITY_HIGH );
            scaledIcon = wxBitmap( img );
        }
        else
        {
            scaledIcon = KiBitmap( BITMAPS::icon_kicad );
            wxImage img = scaledIcon.ConvertToImage();
            img.Rescale( 16, 16, wxIMAGE_QUALITY_HIGH );
            scaledIcon = wxBitmap( img );
        }

        wxString displayTitle = title ? *title : templateDir.GetDirs().Last();

        TEMPLATE_MRU_WIDGET* mruWidget = new TEMPLATE_MRU_WIDGET( m_scrolledMRU, this, path, displayTitle,
                                                                  scaledIcon );
        m_sizerMRU->Add( mruWidget, 0, wxEXPAND | wxBOTTOM, 2 );
        m_mruWidgets.push_back( mruWidget );
    }

    m_scrolledMRU->FitInside();
    m_scrolledMRU->Layout();
    m_panelMRU->Layout();
}


void DIALOG_TEMPLATE_SELECTOR::BuildTemplateList()
{
    wxLogTrace( traceTemplateSelector, "BuildTemplateList() called" );

    // Clear existing template widgets
    for( TEMPLATE_WIDGET* widget : m_templateWidgets )
    {
        m_sizerTemplateList->Detach( widget );
        widget->Destroy();
    }

    m_templateWidgets.clear();
    m_templates.clear();

    auto scanDirectory =
            [this]( const wxString& aPath, bool aIsUser )
            {
                if( aPath.IsEmpty() )
                    return;

                wxDir dir;

                if( !dir.Open( aPath ) )
                    return;

                wxLogTrace( traceTemplateSelector, "Scanning directory: %s (user=%d)", aPath, aIsUser );

                if( dir.HasSubDirs( "meta" ) )
                {
                    auto templ = std::make_unique<PROJECT_TEMPLATE>( aPath );
                    wxFileName htmlFile = templ->GetHtmlFile();
                    wxString description = ExtractDescription( htmlFile );

                    TEMPLATE_WIDGET* widget = new TEMPLATE_WIDGET( m_scrolledTemplates, this );
                    widget->SetTemplate( templ.get() );
                    widget->SetDescription( description );
                    widget->SetIsUserTemplate( aIsUser );

                    m_templates.push_back( std::move( templ ) );
                    m_templateWidgets.push_back( widget );
                }
                else
                {
                    wxString subName;
                    bool cont = dir.GetFirst( &subName, wxEmptyString, wxDIR_DIRS );

                    while( cont )
                    {
                        wxString subFull = aPath + wxFileName::GetPathSeparator() + subName;
                        wxDir subDir;

                        if( subDir.Open( subFull ) )
                        {
                            auto templ = std::make_unique<PROJECT_TEMPLATE>( subFull );
                            wxFileName htmlFile = templ->GetHtmlFile();
                            wxString description = ExtractDescription( htmlFile );

                            TEMPLATE_WIDGET* widget = new TEMPLATE_WIDGET( m_scrolledTemplates, this );
                            widget->SetTemplate( templ.get() );
                            widget->SetDescription( description );
                            widget->SetIsUserTemplate( aIsUser );

                            m_templates.push_back( std::move( templ ) );
                            m_templateWidgets.push_back( widget );
                        }

                        cont = dir.GetNext( &subName );
                    }
                }
            };

    scanDirectory( m_userTemplatesPath, true );
    scanDirectory( m_systemTemplatesPath, false );

    // Sort alphabetically with "Default" first
    std::sort( m_templateWidgets.begin(), m_templateWidgets.end(),
               []( TEMPLATE_WIDGET* a, TEMPLATE_WIDGET* b )
               {
                   wxString titleA = *a->GetTemplate()->GetTitle();
                   wxString titleB = *b->GetTemplate()->GetTitle();
                   int      cmp = titleA.CmpNoCase( titleB );

                   if( cmp == 0 )
                       return false;

                   if( titleA.CmpNoCase( "default" ) == 0 )
                       return true;
                   else if( titleB.CmpNoCase( "default" ) == 0 )
                       return false;

                   return cmp < 0;
               } );

    for( TEMPLATE_WIDGET* widget : m_templateWidgets )
    {
        m_sizerTemplateList->Add( widget, 0, wxEXPAND | wxBOTTOM, 8 );
    }

    ApplyFilter();

    // Force initial sizing of widgets after layout
    CallAfter(
            [this]()
            {
                wxSizeEvent evt( m_scrolledTemplates->GetSize() );
                OnScrolledTemplatesSize( evt );
            } );

    wxLogTrace( traceTemplateSelector, "BuildTemplateList() found %zu templates", m_templateWidgets.size() );
}


void DIALOG_TEMPLATE_SELECTOR::ApplyFilter()
{
    int filterChoice = m_filterChoice->GetSelection();
    wxString searchText = m_searchCtrl->GetValue().Lower();

    for( TEMPLATE_WIDGET* widget : m_templateWidgets )
    {
        bool matchesFilter = true;

        if( filterChoice == 1 && !widget->IsUserTemplate() )
            matchesFilter = false;
        else if( filterChoice == 2 && widget->IsUserTemplate() )
            matchesFilter = false;

        bool matchesSearch = true;

        if( !searchText.IsEmpty() )
        {
            wxString title = widget->GetTemplate()->GetTitle()->Lower();
            wxString description = widget->GetDescription().Lower();

            matchesSearch = title.Contains( searchText ) || description.Contains( searchText );
        }

        widget->Show( matchesFilter && matchesSearch );
    }

    m_scrolledTemplates->FitInside();
    m_scrolledTemplates->Layout();
    Layout();
}


void DIALOG_TEMPLATE_SELECTOR::OnSearchCtrl( wxCommandEvent& event )
{
    m_searchTimer.Stop();
    m_searchTimer.StartOnce( 200 );
}


void DIALOG_TEMPLATE_SELECTOR::OnSearchCtrlCancel( wxCommandEvent& event )
{
    m_searchCtrl->Clear();
    ApplyFilter();
}


void DIALOG_TEMPLATE_SELECTOR::OnFilterChanged( wxCommandEvent& event )
{
    KICAD_SETTINGS* settings = Pgm().GetSettingsManager().GetAppSettings<KICAD_SETTINGS>( "kicad" );

    if( settings )
        settings->m_TemplateFilterChoice = m_filterChoice->GetSelection();

    ApplyFilter();
}


void DIALOG_TEMPLATE_SELECTOR::OnSearchTimer( wxTimerEvent& event )
{
    ApplyFilter();
}


void DIALOG_TEMPLATE_SELECTOR::OnRefreshTimer( wxTimerEvent& event )
{
    wxString selectedPath;

    if( m_selectedWidget && m_selectedWidget->GetTemplate() )
    {
        wxFileName htmlFile = m_selectedWidget->GetTemplate()->GetHtmlFile();
        htmlFile.RemoveLastDir();
        selectedPath = htmlFile.GetPath();
    }

    // Clear pointers before destroying widgets to avoid dangling pointers
    m_selectedWidget = nullptr;
    m_selectedTemplate = nullptr;

    BuildTemplateList();

    if( !selectedPath.IsEmpty() )
        SelectTemplateByPath( selectedPath );
}


void DIALOG_TEMPLATE_SELECTOR::OnBackClicked( wxCommandEvent& event )
{
    if( m_selectedWidget )
        m_selectedWidget->Unselect();

    m_selectedWidget = nullptr;
    m_selectedTemplate = nullptr;

    SetState( DialogState::Initial );
    ShowWelcomeHtml();
}


void DIALOG_TEMPLATE_SELECTOR::SetWidget( TEMPLATE_WIDGET* aWidget )
{
    if( m_selectedWidget != nullptr && m_selectedWidget != aWidget )
        m_selectedWidget->Unselect();

    m_selectedWidget = aWidget;
    m_selectedTemplate = aWidget ? aWidget->GetTemplate() : nullptr;

    SetState( DialogState::Preview );

    if( m_selectedTemplate )
        LoadTemplatePreview( m_selectedTemplate );
}


void DIALOG_TEMPLATE_SELECTOR::SelectTemplateByPath( const wxString& aPath )
{
    SelectTemplateByPath( aPath, false );
}


void DIALOG_TEMPLATE_SELECTOR::SelectTemplateByPath( const wxString& aPath, bool aKeepMRUVisible )
{
    for( TEMPLATE_WIDGET* widget : m_templateWidgets )
    {
        if( widget->GetTemplate() )
        {
            wxFileName htmlFile = widget->GetTemplate()->GetHtmlFile();
            htmlFile.RemoveLastDir();

            if( htmlFile.GetPath() == aPath )
            {
                if( aKeepMRUVisible )
                {
                    // Select the widget but keep MRU visible, don't show preview
                    if( m_selectedWidget != nullptr && m_selectedWidget != widget )
                        m_selectedWidget->Unselect();

                    m_selectedWidget = widget;
                    m_selectedTemplate = widget->GetTemplate();

                    widget->SelectWithoutStateChange();

                    // Scroll the widget into view
                    int y = 0;
                    int scrollRate = 0;
                    widget->GetPosition( nullptr, &y );
                    m_scrolledTemplates->GetScrollPixelsPerUnit( nullptr, &scrollRate );

                    if( scrollRate > 0 )
                        m_scrolledTemplates->Scroll( -1, y / scrollRate );
                }
                else
                {
                    widget->Select();
                }

                return;
            }
        }
    }

    wxLogTrace( traceTemplateSelector, "SelectTemplateByPath: template not found at %s", aPath );
}


void DIALOG_TEMPLATE_SELECTOR::EnsureWebViewCreated()
{
    if( m_webviewPanel )
        return;

    // Create the WEBVIEW_PANEL lazily to avoid WebKit JavaScript VM initialization issues
    // when the dialog is opened from a coroutine context.
    m_webviewPanel = new WEBVIEW_PANEL( m_webviewPlaceholder, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxTAB_TRAVERSAL );

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    sizer->Add( m_webviewPanel, 1, wxEXPAND );
    m_webviewPlaceholder->SetSizer( sizer );
    m_webviewPlaceholder->Layout();

    m_webviewPanel->BindLoadedEvent();

    if( m_webviewPanel->GetWebView() )
    {
        Bind( wxEVT_WEBVIEW_LOADED, &DIALOG_TEMPLATE_SELECTOR::OnWebViewLoaded, this,
              m_webviewPanel->GetWebView()->GetId() );
    }
}


void DIALOG_TEMPLATE_SELECTOR::LoadTemplatePreview( PROJECT_TEMPLATE* aTemplate )
{
    if( !aTemplate )
        return;

    EnsureWebViewCreated();

    wxFileName htmlFile = aTemplate->GetHtmlFile();

    if( htmlFile.FileExists() && htmlFile.IsFileReadable() )
    {
        m_loadingExternalHtml = true;
        wxString url = wxFileName::FileNameToURL( htmlFile );
        m_webviewPanel->LoadURL( url );
    }
    else
    {
        m_loadingExternalHtml = false;
        wxString html = GetTemplateInfoHtml( *aTemplate->GetTitle(), KIPLATFORM::UI::IsDarkTheme() );
        m_webviewPanel->SetPage( html );
    }
}


void DIALOG_TEMPLATE_SELECTOR::ShowWelcomeHtml()
{
    EnsureWebViewCreated();
    m_loadingExternalHtml = false;
    m_webviewPanel->SetPage( GetWelcomeHtml( KIPLATFORM::UI::IsDarkTheme() ) );
}


void DIALOG_TEMPLATE_SELECTOR::SetupFileWatcher()
{
    if( m_watcher )
    {
        m_watcher->RemoveAll();
        delete m_watcher;
        m_watcher = nullptr;
    }

    m_watcher = new wxFileSystemWatcher();
    m_watcher->SetOwner( this );

    wxLogNull logNo;

    if( !m_userTemplatesPath.IsEmpty() )
    {
        wxFileName userDir;
        userDir.AssignDir( m_userTemplatesPath );

        if( userDir.DirExists() )
        {
            m_watcher->Add( userDir );
            wxLogTrace( traceTemplateSelector, "Watching user templates: %s", m_userTemplatesPath );
        }
    }

    if( !m_systemTemplatesPath.IsEmpty() )
    {
        wxFileName systemDir;
        systemDir.AssignDir( m_systemTemplatesPath );

        if( systemDir.DirExists() )
        {
            m_watcher->Add( systemDir );
            wxLogTrace( traceTemplateSelector, "Watching system templates: %s", m_systemTemplatesPath );
        }
    }
}


void DIALOG_TEMPLATE_SELECTOR::OnFileSystemEvent( wxFileSystemWatcherEvent& event )
{
    if( !m_watcher )
        return;

    wxLogTrace( traceTemplateSelector, "File system event detected" );

    // wxFileSystemWatcher may fire events from a worker thread on some platforms.
    // Use CallAfter to marshal timer operations to the main thread.
    CallAfter(
            [this]()
            {
                m_refreshTimer.Stop();
                m_refreshTimer.StartOnce( 500 );
            } );
}


void DIALOG_TEMPLATE_SELECTOR::OnSysColourChanged( wxSysColourChangedEvent& event )
{
    event.Skip();

    // Update background colors for the templates panel
    m_scrolledTemplates->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

    // Update all template widgets
    for( TEMPLATE_WIDGET* widget : m_templateWidgets )
    {
        if( widget->IsSelected() )
            widget->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
        else
            widget->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

        widget->Refresh();
    }

    // Update all MRU widgets
    for( TEMPLATE_MRU_WIDGET* widget : m_mruWidgets )
    {
        widget->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
        widget->Refresh();
    }

    m_scrolledTemplates->Refresh();
    m_scrolledMRU->Refresh();
}


void DIALOG_TEMPLATE_SELECTOR::OnScrolledTemplatesSize( wxSizeEvent& event )
{
    event.Skip();

    // Get the client width of the scrolled window
    int clientWidth = m_scrolledTemplates->GetClientSize().GetWidth();

    if( clientWidth <= 0 )
        return;

    // Force each widget to rewrap its text at the new width
    for( TEMPLATE_WIDGET* widget : m_templateWidgets )
    {
        wxSizeEvent sizeEvt( wxSize( clientWidth, -1 ) );
        widget->GetEventHandler()->ProcessEvent( sizeEvt );
    }

    m_scrolledTemplates->FitInside();
}


void DIALOG_TEMPLATE_SELECTOR::OnWebViewLoaded( wxWebViewEvent& event )
{
    event.Skip();

    // Safety check - ensure webview panel is valid
    if( !m_webviewPanel || !m_webviewPanel->GetWebView() )
        return;

    // Add style to external documets
    if( m_loadingExternalHtml )
    {
        wxString script = wxString::Format( wxS( R"(
( function()
{
    var style = document.createElement( 'style' );
    style.textContent = `
%s
    `;
    document.head.appendChild( style );
} )();
                                                 )" ), GetCommonStyles() );

#if !defined( __MINGW32__ )     // RunScriptAsync() is not supported on MINGW build
        if( m_webviewPanel->GetBackend() != wxWebViewBackendIE )
            m_webviewPanel->RunScriptAsync( script );
#endif
    }
}


void DIALOG_TEMPLATE_SELECTOR::RefreshTemplateList()
{
    wxString selectedPath;

    if( m_selectedWidget && m_selectedWidget->GetTemplate() )
    {
        wxFileName htmlFile = m_selectedWidget->GetTemplate()->GetHtmlFile();
        htmlFile.RemoveLastDir();
        selectedPath = htmlFile.GetPath();
    }

    // Clear pointers before destroying widgets to avoid dangling pointers
    m_selectedWidget = nullptr;
    m_selectedTemplate = nullptr;

    BuildTemplateList();

    if( !selectedPath.IsEmpty() )
        SelectTemplateByPath( selectedPath );
}


wxString DIALOG_TEMPLATE_SELECTOR::ExtractDescription( const wxFileName& aHtmlFile )
{
    if( !aHtmlFile.FileExists() || !aHtmlFile.IsFileReadable() )
        return wxEmptyString;

    wxTextFile file( aHtmlFile.GetFullPath() );

    if( !file.Open() )
        return wxEmptyString;

    wxString content;

    for( wxString line = file.GetFirstLine(); !file.Eof(); line = file.GetNextLine() )
    {
        content += line + wxT( " " );
    }

    file.Close();

    // Try to extract text from the meta description tag
    wxRegEx reMetaDesc( wxT( "<meta[^>]*name=[\"']description[\"'][^>]*content=[\"']([^\"']*)[\"']" ),
                        wxRE_ICASE );

    if( reMetaDesc.Matches( content ) )
        return reMetaDesc.GetMatch( content, 1 );

    // Fallback to first paragraph tag (allowing nested HTML tags)
    wxRegEx reParagraph( wxT( "<p[^>]*>(.*?)</p>" ), wxRE_ICASE );

    if( reParagraph.Matches( content ) )
    {
        wxString desc = reParagraph.GetMatch( content, 1 );

        // Strip nested HTML tags
        wxRegEx reTags( wxT( "<[^>]*>" ) );
        reTags.ReplaceAll( &desc, wxT( " " ) );

        // Decode common HTML entities
        desc.Replace( wxT( "&nbsp;" ), wxT( " " ) );
        desc.Replace( wxT( "&amp;" ), wxT( "&" ) );
        desc.Replace( wxT( "&lt;" ), wxT( "<" ) );
        desc.Replace( wxT( "&gt;" ), wxT( ">" ) );
        desc.Replace( wxT( "&quot;" ), wxT( "\"" ) );

        // Normalize whitespace
        wxRegEx reWhitespace( wxT( "\\s+" ) );
        reWhitespace.ReplaceAll( &desc, wxT( " " ) );

        desc = desc.Trim().Trim( false );

        if( !desc.IsEmpty() )
            return desc;
    }

    // Final fallback: extract text content from body, stripping all HTML tags
    wxRegEx reBody( wxT( "<body[^>]*>(.*)</body>" ), wxRE_ICASE );

    if( reBody.Matches( content ) )
    {
        wxString body = reBody.GetMatch( content, 1 );

        // Strip all HTML tags
        wxRegEx reTags( wxT( "<[^>]*>" ) );
        reTags.ReplaceAll( &body, wxT( " " ) );

        // Decode common HTML entities
        body.Replace( wxT( "&nbsp;" ), wxT( " " ) );
        body.Replace( wxT( "&amp;" ), wxT( "&" ) );
        body.Replace( wxT( "&lt;" ), wxT( "<" ) );
        body.Replace( wxT( "&gt;" ), wxT( ">" ) );
        body.Replace( wxT( "&quot;" ), wxT( "\"" ) );

        // Normalize whitespace
        wxRegEx reWhitespace( wxT( "\\s+" ) );
        reWhitespace.ReplaceAll( &body, wxT( " " ) );

        body = body.Trim().Trim( false );

        // Return up to 250 characters
        if( body.Length() > 250 )
            return body.Left( 250 ) + wxS( "..." );

        return body;
    }

    return wxEmptyString;
}


PROJECT_TEMPLATE* DIALOG_TEMPLATE_SELECTOR::GetSelectedTemplate()
{
    return m_selectedTemplate;
}
