/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 jean-pierre.charras
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

#include <bitmap2component.h>
#include <bitmap2cmp_frame.h>
#include <bitmap2cmp_panel.h>
#include <bitmap2cmp_settings.h>
#include <bitmap_io.h>
#include <bitmaps.h>
#include <common.h>
#include <id.h>
#include <widgets/wx_menubar.h>
#include <wildcards_and_files_ext.h>
#include <file_history.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/common_control.h>
#include <tool/action_manager.h>
#include <bitmap2cmp_control.h>
#include <tool/actions.h>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>


#define DEFAULT_DPI 300     // the image DPI used in formats that do not define a DPI

IMAGE_SIZE::IMAGE_SIZE()
{
    m_outputSize = 0.0;
    m_originalDPI = DEFAULT_DPI;
    m_originalSizePixels = 0;
    m_unit = EDA_UNITS::MM;
}


void IMAGE_SIZE::SetOutputSizeFromInitialImageSize()
{
    // Safety-check to guarantee no divide-by-zero
    m_originalDPI = std::max( 1, m_originalDPI );

    // Set the m_outputSize value from the m_originalSizePixels and the selected unit
    if( m_unit == EDA_UNITS::MM )
        m_outputSize = (double)GetOriginalSizePixels() / m_originalDPI * 25.4;
    else if( m_unit == EDA_UNITS::INCH )
        m_outputSize = (double)GetOriginalSizePixels() / m_originalDPI;
    else
        m_outputSize = m_originalDPI;
}


int IMAGE_SIZE::GetOutputDPI()
{
    int outputDPI;

    if( m_unit == EDA_UNITS::MM )
        outputDPI = GetOriginalSizePixels() / ( m_outputSize / 25.4 );
    else if( m_unit == EDA_UNITS::INCH )
        outputDPI = GetOriginalSizePixels() / m_outputSize;
    else
        outputDPI = KiROUND( m_outputSize );

    // Zero is not a DPI, and may cause divide-by-zero errors...
    outputDPI = std::max( 1, outputDPI );

    return outputDPI;
}


void IMAGE_SIZE::SetUnit( EDA_UNITS aUnit )
{
    // Set the unit used for m_outputSize, and convert the old m_outputSize value
    // to the value in new unit
    if( aUnit == m_unit )
        return;

    // Convert m_outputSize to mm:
    double size_mm;

    if( m_unit == EDA_UNITS::MM )
    {
        size_mm = m_outputSize;
    }
    else if( m_unit == EDA_UNITS::INCH )
    {
        size_mm = m_outputSize * 25.4;
    }
    else
    {
        // m_outputSize is the DPI, not an image size
        // the image size is m_originalSizePixels / m_outputSize (in inches)
        if( m_outputSize )
            size_mm =  m_originalSizePixels / m_outputSize * 25.4;
        else
            size_mm = 0;
    }

    // Convert m_outputSize to new value:
    if( aUnit == EDA_UNITS::MM )
    {
        m_outputSize = size_mm;
    }
    else if( aUnit == EDA_UNITS::INCH )
    {
        m_outputSize = size_mm / 25.4;
    }
    else
    {
        if( size_mm )
            m_outputSize = m_originalSizePixels / size_mm * 25.4;
        else
            m_outputSize = 0;
    }

    m_unit = aUnit;
}


BEGIN_EVENT_TABLE( BITMAP2CMP_FRAME, KIWAY_PLAYER )
    EVT_MENU( wxID_CLOSE, BITMAP2CMP_FRAME::OnExit )
    EVT_MENU( wxID_EXIT, BITMAP2CMP_FRAME::OnExit )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, BITMAP2CMP_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, BITMAP2CMP_FRAME::OnClearFileHistory )
END_EVENT_TABLE()


BITMAP2CMP_FRAME::BITMAP2CMP_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_BM2CMP, _( "Image Converter" ), wxDefaultPosition,
                      wxDefaultSize, wxDEFAULT_FRAME_STYLE, wxT( "bitmap2cmp" ), unityScale ),
        m_panel( nullptr ),
        m_statusBar( nullptr )
{
    m_aboutTitle = _HKI( "KiCad Image Converter" );

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_bitmap2component, 48 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_bitmap2component, 128 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_bitmap2component, 256 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_bitmap2component_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_bitmap2component_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    m_panel = new BITMAP2CMP_PANEL( this );
    mainSizer->Add( m_panel, 1, wxEXPAND, 5 );

    m_statusBar = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

    LoadSettings( config() );

    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, nullptr, nullptr, config(), this );

    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new BITMAP2CMP_CONTROL );
    m_toolManager->InitTools();

    ReCreateMenuBar();
    setupUIConditions();

    GetSizer()->SetSizeHints( this );

    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    if ( m_framePos == wxDefaultPosition )
        Centre();
}


BITMAP2CMP_FRAME::~BITMAP2CMP_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    SaveSettings( config() );

    /*
     * This needed for OSX: avoids further OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    Freeze();
}


void BITMAP2CMP_FRAME::OnExit( wxCommandEvent& aEvent )
{
    // Just generate a wxCloseEvent
    Close( false );
}


wxWindow* BITMAP2CMP_FRAME::GetToolCanvas() const
{
    return m_panel->GetCurrentPage();
}


void BITMAP2CMP_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(), _( "Image files" ) );

    if( !fn.IsEmpty() )
    {
        OpenProjectFiles( std::vector<wxString>( 1, fn ) );
        Refresh();
    }
}


void BITMAP2CMP_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void BITMAP2CMP_FRAME::doReCreateMenuBar()
{
    COMMON_CONTROL* tool = m_toolManager->GetTool<COMMON_CONTROL>();
    EDA_BASE_FRAME* base_frame = dynamic_cast<EDA_BASE_FRAME*>( this );

    // base_frame == nullptr should not happen, but it makes Coverity happy
    wxCHECK( base_frame, /* void */ );

    // wxWidgets handles the OSX Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = base_frame->GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, tool );

    fileMenu->Add( ACTIONS::open );

    static ACTION_MENU* openRecentMenu;
    FILE_HISTORY&       fileHistory = GetFileHistory();

    // Create the menu if it does not exist. Adding a file to/from the history
    // will automatically refresh the menu.
    if( !openRecentMenu )
    {
        openRecentMenu = new ACTION_MENU( false, tool );
        openRecentMenu->SetIcon( BITMAPS::recent );

        fileHistory.UseMenu( openRecentMenu );
        fileHistory.AddFilesToMenu();
    }

    // Ensure the title is up to date after changing language
    openRecentMenu->SetTitle( _( "Open Recent" ) );
    fileHistory.UpdateClearText( openRecentMenu, _( "Clear Recent Files" ) );

    wxMenuItem* item = fileMenu->Add( openRecentMenu->Clone() );

    // Add the file menu condition here since it needs the item ID for the submenu
    ACTION_CONDITIONS cond;
    cond.Enable( FILE_HISTORY::FileHistoryNotEmpty( fileHistory ) );
    RegisterUIUpdateHandler( item->GetId(), cond );

    fileMenu->AppendSeparator();
    fileMenu->AddQuit( _( "Image Converter" ) );

    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, tool );

    prefsMenu->Add( ACTIONS::openPreferences );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, tool );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    base_frame->AddStandardHelpMenu( menuBar );

    base_frame->SetMenuBar( menuBar );
    delete oldMenuBar;
}


void BITMAP2CMP_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    UpdateTitle();

    SaveSettings( config() );
    IMAGE_SIZE imageSizeX = m_panel->GetOutputSizeX();
    IMAGE_SIZE imageSizeY = m_panel->GetOutputSizeY();
    Freeze();

    wxSizer* mainSizer = m_panel->GetContainingSizer();
    mainSizer->Detach( m_panel );
    m_panel->Destroy();

    m_panel = new BITMAP2CMP_PANEL( this );
    mainSizer->Add( m_panel, 1, wxEXPAND, 5 );
    Layout();

    if( !m_srcFileName.IsEmpty() )
        OpenProjectFiles( std::vector<wxString>( 1, m_srcFileName ) );

    LoadSettings( config() );
    m_panel->SetOutputSize( imageSizeX, imageSizeY );

    Thaw();
    Refresh();
}


void BITMAP2CMP_FRAME::UpdateTitle()
{
    wxString title;

    if( !m_srcFileName.IsEmpty() )
    {
        wxFileName filename( m_srcFileName );
        title = filename.GetFullName() + wxT( " \u2014 " );
    }

    title += _( "Image Converter" );

    SetTitle( title );
}


void BITMAP2CMP_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    if( BITMAP2CMP_SETTINGS* cfg = dynamic_cast<BITMAP2CMP_SETTINGS*>( aCfg ) )
    {
        m_srcFileName = cfg->m_BitmapFileName;
        m_outFileName = cfg->m_ConvertedFileName;
        m_panel->LoadSettings( cfg );
    }
}


void BITMAP2CMP_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    if( BITMAP2CMP_SETTINGS* cfg = dynamic_cast<BITMAP2CMP_SETTINGS*>( aCfg ) )
    {
        cfg->m_BitmapFileName    = m_srcFileName;
        cfg->m_ConvertedFileName = m_outFileName;
        m_panel->SaveSettings( cfg );
    }
}


void BITMAP2CMP_FRAME::OnLoadFile()
{
    wxFileName  fn( m_srcFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = m_mruPath;

    wxFileDialog fileDlg( this, _( "Choose Image" ), path, wxEmptyString,
                          _( "Image Files" ) + wxS( " " )+ wxImage::GetImageExtWildcard(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    wxString fullFilename = fileDlg.GetPath();

    if( !OpenProjectFiles( std::vector<wxString>( 1, fullFilename ) ) )
        return;

    fn = fullFilename;
    m_mruPath = fn.GetPath();
    SetStatusText( fullFilename );
    UpdateTitle();
    Refresh();
}


bool BITMAP2CMP_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    m_srcFileName = aFileSet[0];

    if( m_panel->OpenProjectFiles( aFileSet, aCtl ) )
    {
        UpdateFileHistory( m_srcFileName );
        return true;
    }

    return false;
}


void BITMAP2CMP_FRAME::ExportDrawingSheetFormat()
{
    wxFileName  fn( m_outFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create Drawing Sheet File" ), path, wxEmptyString,
                          FILEEXT::DrawingSheetFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( FILEEXT::DrawingSheetFileExtension );
    m_outFileName = fn.GetFullPath();

    FILE* outfile = wxFopen( m_outFileName, wxT( "w" ) );

    if( !outfile )
    {
        wxMessageBox( wxString::Format( _( "File '%s' could not be created." ), m_outFileName ) );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, DRAWING_SHEET_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BITMAP2CMP_FRAME::ExportPostScriptFormat()
{
    wxFileName  fn( m_outFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create PostScript File" ), path, wxEmptyString,
                          FILEEXT::PSFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( wxT( "ps" ) );
    m_outFileName = fn.GetFullPath();

    FILE* outfile = wxFopen( m_outFileName, wxT( "w" ) );

    if( !outfile )
    {
        wxMessageBox( wxString::Format( _( "File '%s' could not be created." ), m_outFileName  ) );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, POSTSCRIPT_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BITMAP2CMP_FRAME::ExportEeschemaFormat()
{
    wxFileName  fn( m_outFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create Symbol Library" ), path, wxEmptyString,
                          FILEEXT::KiCadSymbolLibFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    fn = EnsureFileExtension( fileDlg.GetPath(), FILEEXT::KiCadSymbolLibFileExtension );
    m_outFileName = fn.GetFullPath();

    FILE* outfile = wxFopen( m_outFileName, wxT( "w" ) );

    if( !outfile )
    {
        wxMessageBox( wxString::Format( _( "File '%s' could not be created." ), m_outFileName ) );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, SYMBOL_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BITMAP2CMP_FRAME::ExportPcbnewFormat()
{
    wxFileName  fn( m_outFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = m_mruPath;

    wxFileDialog fileDlg( this, _( "Create Footprint Library" ), path, wxEmptyString,
                          FILEEXT::KiCadFootprintLibFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    fn = EnsureFileExtension( fileDlg.GetPath(), FILEEXT::KiCadFootprintFileExtension );
    m_outFileName = fn.GetFullPath();

    FILE* outfile = wxFopen( m_outFileName, wxT( "w" ) );

    if( !outfile )
    {
        wxMessageBox( wxString::Format( _( "File '%s' could not be created." ), m_outFileName ) );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, FOOTPRINT_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
    m_mruPath = fn.GetPath();
}


