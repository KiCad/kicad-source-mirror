/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 jean-pierre.charras
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_base.h>
#include <pgm_base.h>
#include <wildcards_and_files_ext.h>

#include <wx/filedlg.h>
#include <wx/msgdlg.h>


#define DEFAULT_DPI 300     // the image DPI used in formats that do not define a DPI

IMAGE_SIZE::IMAGE_SIZE()
{
    m_outputSize = 0.0;
    m_originalDPI = DEFAULT_DPI;
    m_originalSizePixels = 0;
    m_unit = EDA_UNITS::MILLIMETRES;
}


void IMAGE_SIZE::SetOutputSizeFromInitialImageSize()
{
    // Safety-check to guarantee no divide-by-zero
    m_originalDPI = std::max( 1, m_originalDPI );

    // Set the m_outputSize value from the m_originalSizePixels and the selected unit
    if( m_unit == EDA_UNITS::MILLIMETRES )
        m_outputSize = (double)GetOriginalSizePixels() / m_originalDPI * 25.4;
    else if( m_unit == EDA_UNITS::INCHES )
        m_outputSize = (double)GetOriginalSizePixels() / m_originalDPI;
    else
        m_outputSize = m_originalDPI;
}


int IMAGE_SIZE::GetOutputDPI()
{
    int outputDPI;

    if( m_unit == EDA_UNITS::MILLIMETRES )
        outputDPI = GetOriginalSizePixels() / ( m_outputSize / 25.4 );
    else if( m_unit == EDA_UNITS::INCHES )
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

    if( m_unit == EDA_UNITS::MILLIMETRES )
    {
        size_mm = m_outputSize;
    }
    else if( m_unit == EDA_UNITS::INCHES )
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
    if( aUnit == EDA_UNITS::MILLIMETRES )
    {
        m_outputSize = size_mm;
    }
    else if( aUnit == EDA_UNITS::INCHES )
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
    EVT_MENU( wxID_EXIT, BITMAP2CMP_FRAME::OnExit )
    EVT_MENU( wxID_OPEN, BITMAP2CMP_FRAME::OnLoadFile )
END_EVENT_TABLE()


BITMAP2CMP_FRAME::BITMAP2CMP_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_BM2CMP, _( "Image Converter" ), wxDefaultPosition,
                      wxDefaultSize, wxDEFAULT_FRAME_STYLE, wxT( "bitmap2cmp" ), unityScale ),
        m_panel( nullptr ),
        m_statusBar( nullptr )
{
    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_bitmap2component ) );
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

    ReCreateMenuBar();

    LoadSettings( config() );

    GetSizer()->SetSizeHints( this );

    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    if ( m_framePos == wxDefaultPosition )
        Centre();
}


BITMAP2CMP_FRAME::~BITMAP2CMP_FRAME()
{
    SaveSettings( config() );
    /*
     * This needed for OSX: avoids further OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    Freeze();
}


wxWindow* BITMAP2CMP_FRAME::GetToolCanvas() const
{
    return m_panel->GetCurrentPage();
}


void BITMAP2CMP_FRAME::doReCreateMenuBar()
{
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar    = new wxMenuBar();

    wxMenu* fileMenu = new wxMenu;

    wxMenuItem* item = new wxMenuItem( fileMenu, wxID_OPEN, _( "Open..." ) + wxT( "\tCtrl+O" ),
                                       _( "Load source image" ) );

    fileMenu->Append( item );

#ifndef __WXMAC__
    // Mac moves Quit to the App menu so we don't need a separator on Mac
    fileMenu->AppendSeparator();
#endif

    item = new wxMenuItem( fileMenu, wxID_EXIT, _( "Quit" ) + wxT( "\tCtrl+Q" ),
                           _( "Quit Image Converter" ) );

    if( Pgm().GetCommonSettings()->m_Appearance.use_icons_in_menus )
        item->SetBitmap( KiBitmap( BITMAPS::exit ) );

    fileMenu->Append( item );

    menuBar->Append( fileMenu, _( "&File" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}


void BITMAP2CMP_FRAME::OnExit( wxCommandEvent& event )
{
    Destroy();
}


void BITMAP2CMP_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    BITMAP2CMP_SETTINGS* cfg = dynamic_cast<BITMAP2CMP_SETTINGS*>( aCfg );

    if( cfg )
    {
        m_bitmapFileName = cfg->m_BitmapFileName;
        m_convertedFileName = cfg->m_ConvertedFileName;
        m_panel->LoadSettings( cfg );
    }
}


void BITMAP2CMP_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    BITMAP2CMP_SETTINGS* cfg = dynamic_cast<BITMAP2CMP_SETTINGS*>( aCfg );

    if( cfg )
    {
        cfg->m_BitmapFileName    = m_bitmapFileName;
        cfg->m_ConvertedFileName = m_convertedFileName;
        m_panel->SaveSettings( cfg );
    }
}


void BITMAP2CMP_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxFileName  fn( m_bitmapFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = m_mruPath;

    wxFileDialog fileDlg( this, _( "Choose Image" ), path, wxEmptyString,
                          _( "Image Files" ) + wxS( " " )+ wxImage::GetImageExtWildcard(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    int diag = fileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    wxString fullFilename = fileDlg.GetPath();

    if( !OpenProjectFiles( std::vector<wxString>( 1, fullFilename ) ) )
        return;

    fn = fullFilename;
    m_mruPath = fn.GetPath();
    SetStatusText( fullFilename );
    Refresh();
}


bool BITMAP2CMP_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    m_bitmapFileName = aFileSet[0];

    return m_panel->OpenProjectFiles( aFileSet, aCtl );
}


void BITMAP2CMP_FRAME::ExportLogo()
{
    wxFileName  fn( m_convertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create Logo File" ), path, wxEmptyString,
                          DrawingSheetFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = fileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( DrawingSheetFileExtension );
    m_convertedFileName = fn.GetFullPath();

    FILE*    outfile;
    outfile = wxFopen( m_convertedFileName, wxT( "w" ) );

    if( outfile == nullptr )
    {
        wxString msg;
        msg.Printf( _( "File '%s' could not be created." ), m_convertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, KICAD_WKS_LOGO );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BITMAP2CMP_FRAME::ExportPostScriptFormat()
{
    wxFileName  fn( m_convertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create PostScript File" ), path, wxEmptyString,
                          PSFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( wxT( "ps" ) );
    m_convertedFileName = fn.GetFullPath();

    FILE*    outfile;
    outfile = wxFopen( m_convertedFileName, wxT( "w" ) );

    if( outfile == nullptr )
    {
        wxString msg;
        msg.Printf( _( "File '%s' could not be created." ), m_convertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, POSTSCRIPT_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BITMAP2CMP_FRAME::ExportEeschemaFormat()
{
    wxFileName  fn( m_convertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create Symbol Library" ), path, wxEmptyString,
                          KiCadSymbolLibFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    fn = EnsureFileExtension( fileDlg.GetPath(), KiCadSymbolLibFileExtension );
    m_convertedFileName = fn.GetFullPath();

    FILE*    outfile = wxFopen( m_convertedFileName, wxT( "w" ) );

    if( outfile == nullptr )
    {
        wxString msg;
        msg.Printf( _( "File '%s' could not be created." ), m_convertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, EESCHEMA_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BITMAP2CMP_FRAME::ExportPcbnewFormat()
{
    wxFileName  fn( m_convertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = m_mruPath;

    wxFileDialog fileDlg( this, _( "Create Footprint Library" ), path, wxEmptyString,
                          KiCadFootprintLibFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( fileDlg.ShowModal() != wxID_OK )
        return;

    fn = EnsureFileExtension( fileDlg.GetPath(), KiCadFootprintFileExtension );
    m_convertedFileName = fn.GetFullPath();

    FILE* outfile = wxFopen( m_convertedFileName, wxT( "w" ) );

    if( outfile == nullptr )
    {
        wxString msg;
        msg.Printf( _( "File '%s' could not be created." ), m_convertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    m_panel->ExportToBuffer( buffer, PCBNEW_KICAD_MOD );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
    m_mruPath = fn.GetPath();
}


