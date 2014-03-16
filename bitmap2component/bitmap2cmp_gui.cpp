/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 jean-pierre.charras
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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
#include <fctsys.h>
#include <appl_wxstruct.h>
#include <wxstruct.h>
#include <confirm.h>
#include <gestfich.h>

#include <bitmap2cmp_gui_base.h>

#include <potracelib.h>
#include <bitmap_io.h>

#include <colors_selection.h>
#include <build_version.h>
#include <menus_helpers.h>

#define KEYWORD_FRAME_POSX wxT( "Bmconverter_Pos_x" )
#define KEYWORD_FRAME_POSY wxT( "Bmconverter_Pos_y" )
#define KEYWORD_FRAME_SIZEX wxT( "Bmconverter_Size_x" )
#define KEYWORD_FRAME_SIZEY wxT( "Bmconverter_Size_y" )
#define KEYWORD_LAST_INPUT_FILE wxT( "Last_input" )
#define KEYWORD_LAST_OUTPUT_FILE wxT( "Last_output" )
#define KEYWORD_LAST_FORMAT wxT( "Last_format" )
#define KEYWORD_BINARY_THRESHOLD wxT( "Threshold" )
#define KEYWORD_BW_NEGATIVE wxT( "Negative_choice" )

#define DEFAULT_DPI 300     // Default resolution in Bit per inches

extern int bitmap2component( potrace_bitmap_t* aPotrace_bitmap, FILE* aOutfile,
                             int aFormat, int aDpi_X, int aDpi_Y );

/* Class BM2CMP_FRAME_BASE
This is the main frame for this application
*/
class BM2CMP_FRAME : public BM2CMP_FRAME_BASE
{
private:
    wxImage   m_Pict_Image;
    wxBitmap  m_Pict_Bitmap;
    wxImage   m_Greyscale_Image;
    wxBitmap  m_Greyscale_Bitmap;
    wxImage   m_NB_Image;
    wxBitmap  m_BN_Bitmap;
    wxSize    m_imageDPI;           // The initial image resolution. When unknown,
                                    // set to DEFAULT_DPI x DEFAULT_DPI per Inch
    wxString  m_BitmapFileName;
    wxString  m_ConvertedFileName;
    wxSize    m_frameSize;
    wxPoint   m_framePos;
    wxConfig* m_config;

public:
    BM2CMP_FRAME();
    ~BM2CMP_FRAME();

private:

    // Event handlers
    void OnPaint( wxPaintEvent& event );
    void OnLoadFile( wxCommandEvent& event );
    bool LoadFile( wxString& aFullFileName );
    void OnExport( wxCommandEvent& event );

    /**
     * Generate a schematic library which comtains one component:
     * the logo
     */
    void OnExportEeschema();

    /**
     * Depending on the option:
     * Legacy format: generate a module library which comtains one component
     * New kicad_mod format: generate a module in S expr format
     */
    void OnExportPcbnew( bool aLegacyFormat );

    /**
     * Generate a postscript file
     */
    void OnExportPostScript();

    /**
     * Generate a file suitable to be copied into a page layout
     * description file (.kicad_wks file
     */
    void OnExportLogo();

    void Binarize( double aThreshold );     // aThreshold = 0.0 (black level) to 1.0 (white level)
    void OnOptionsSelection( wxCommandEvent& event );
    void OnThresholdChange( wxScrollEvent& event );
	void OnResolutionChange( wxCommandEvent& event );

    // called when texts controls which handle the image resolution
    // lose the focus, to ensure the rigyht vaules are displayed
    // because the m_imageDPI are clipped to acceptable values, and
    // the text displayed could be differ duringa text edition
    // We are using ChangeValue here to avoid generating a wxEVT_TEXT event.
	void UpdateDPITextValueX( wxMouseEvent& event )
    {
        m_DPIValueX->ChangeValue( wxString::Format( wxT( "%d" ), m_imageDPI.x ) );
    }
	void UpdateDPITextValueY( wxMouseEvent& event )
    {
        m_DPIValueY->ChangeValue( wxString::Format( wxT( "%d" ), m_imageDPI.y ) );
    }

    void NegateGreyscaleImage( );
    void ExportFile( FILE* aOutfile, int aFormat );
    void updateImageInfo();
};

BM2CMP_FRAME::BM2CMP_FRAME() : BM2CMP_FRAME_BASE( NULL )
{
    int tmp;
    m_config = new wxConfig();
    m_config->Read( KEYWORD_FRAME_POSX, & m_framePos.x, -1 );
    m_config->Read( KEYWORD_FRAME_POSY, & m_framePos.y, -1 );
    m_config->Read( KEYWORD_FRAME_SIZEX, & m_frameSize.x, -1 );
    m_config->Read( KEYWORD_FRAME_SIZEY, & m_frameSize.y, -1 );
    m_config->Read( KEYWORD_LAST_INPUT_FILE, &m_BitmapFileName );
    m_config->Read( KEYWORD_LAST_OUTPUT_FILE, &m_ConvertedFileName );
    if( m_config->Read( KEYWORD_BINARY_THRESHOLD, &tmp ) )
        m_sliderThreshold->SetValue( tmp );
    if( m_config->Read( KEYWORD_BW_NEGATIVE, &tmp ) )
        m_rbOptions->SetSelection( tmp  ? 1 : 0 );

    m_config->Read( KEYWORD_LAST_FORMAT, &tmp );
    m_radioBoxFormat->SetSelection( tmp );


    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_bitmap2component_xpm ) );
    SetIcon( icon );

    GetSizer()->SetSizeHints( this );

    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    m_buttonExport->Enable( false );

    m_imageDPI.x = m_imageDPI.y = DEFAULT_DPI;  // Default resolution in Bit per inches

    if ( m_framePos == wxDefaultPosition )
        Centre();
}


BM2CMP_FRAME::~BM2CMP_FRAME()
{
    if( ( m_config == NULL ) || IsIconized() )
        return;

    m_frameSize = GetSize();
    m_framePos  = GetPosition();

    m_config->Write( KEYWORD_FRAME_POSX, (long) m_framePos.x );
    m_config->Write( KEYWORD_FRAME_POSY, (long) m_framePos.y );
    m_config->Write( KEYWORD_FRAME_SIZEX, (long) m_frameSize.x );
    m_config->Write( KEYWORD_FRAME_SIZEY, (long) m_frameSize.y );
    m_config->Write( KEYWORD_LAST_INPUT_FILE, m_BitmapFileName );
    m_config->Write( KEYWORD_LAST_OUTPUT_FILE, m_ConvertedFileName );
    m_config->Write( KEYWORD_BINARY_THRESHOLD, m_sliderThreshold->GetValue() );
    m_config->Write( KEYWORD_BW_NEGATIVE, m_rbOptions->GetSelection() );
    m_config->Write( KEYWORD_LAST_FORMAT,  m_radioBoxFormat->GetSelection() );

    delete m_config;

    /* This needed for OSX: avoids further OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    this->Freeze( );
}


void BM2CMP_FRAME::OnPaint( wxPaintEvent& event )
{
#ifdef __WXMAC__
    // Otherwise fails due: using wxPaintDC without being in a native paint event
    wxClientDC pict_dc( m_InitialPicturePanel );
    wxClientDC greyscale_dc( m_GreyscalePicturePanel );
    wxClientDC nb_dc( m_BNPicturePanel );
#else
    wxPaintDC pict_dc( m_InitialPicturePanel );
    wxPaintDC greyscale_dc( m_GreyscalePicturePanel );
    wxPaintDC nb_dc( m_BNPicturePanel );
#endif

    m_InitialPicturePanel->PrepareDC( pict_dc );
    m_GreyscalePicturePanel->PrepareDC( greyscale_dc );
    m_BNPicturePanel->PrepareDC( nb_dc );

    // OSX crashes with empty bitmaps (on initial refreshes)
    if(m_Pict_Bitmap.IsOk() && m_Greyscale_Bitmap.IsOk() && m_BN_Bitmap.IsOk())
    {
        pict_dc.DrawBitmap( m_Pict_Bitmap, 0, 0, false );
        greyscale_dc.DrawBitmap( m_Greyscale_Bitmap, 0, 0, false );
        nb_dc.DrawBitmap( m_BN_Bitmap, 0, 0, false );
    }
}


/* Called to load a bitmap file
 */
void BM2CMP_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxFileName fn(m_BitmapFileName);
    wxString path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = wxGetCwd();

    wxFileDialog FileDlg( this, _( "Choose Image" ), path, wxEmptyString,
                          _( "Image Files " ) + wxImage::GetImageExtWildcard(),
                          wxFD_OPEN );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    wxString fullFilename = FileDlg.GetPath();

    if( ! LoadFile( fullFilename ) )
        return;

    m_buttonExport->Enable( true );
    SetStatusText( fullFilename );
    Refresh();
}


bool BM2CMP_FRAME::LoadFile( wxString& aFullFileName )
{
    m_BitmapFileName = aFullFileName;

    if( !m_Pict_Image.LoadFile( m_BitmapFileName ) )
    {
        wxMessageBox( _( "Couldn't load image from <%s>" ), m_BitmapFileName.c_str() );
        return false;
    }

    m_Pict_Bitmap = wxBitmap( m_Pict_Image );

    int h  = m_Pict_Bitmap.GetHeight();
    int w  = m_Pict_Bitmap.GetWidth();
    // Determine image resolution in DPI (does not existing in all formats).
    // the resolution can be given in bit per inches or bit per cm in file
    m_imageDPI.x = m_Pict_Image.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONX );
    m_imageDPI.y = m_Pict_Image.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONY );

    if( m_imageDPI.x > 1 && m_imageDPI.y > 1 )
    {
        if( m_Pict_Image.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONUNIT ) == wxIMAGE_RESOLUTION_CM )
        {
            // When the initial resolution is given in bits per cm,
            // experience shows adding 1.27 to the resolution converted in dpi
            // before convert to int value reduce the conversion error
            // but it is not perfect
            m_imageDPI.x = m_imageDPI.x * 2.54 + 1.27;
            m_imageDPI.y = m_imageDPI.y * 2.54 + 1.27;
        }
    }
    else    // fallback to the default value
        m_imageDPI.x = m_imageDPI.y = DEFAULT_DPI;

    // Display image info:
    // We are using ChangeValue here to avoid generating a wxEVT_TEXT event.
    m_DPIValueX->ChangeValue( wxString::Format( wxT( "%d" ), m_imageDPI.x ) );
    m_DPIValueY->ChangeValue( wxString::Format( wxT( "%d" ), m_imageDPI.y ) );
    updateImageInfo();

    m_InitialPicturePanel->SetVirtualSize( w, h );
    m_GreyscalePicturePanel->SetVirtualSize( w, h );
    m_BNPicturePanel->SetVirtualSize( w, h );

    m_Greyscale_Image.Destroy();
    m_Greyscale_Image = m_Pict_Image.ConvertToGreyscale( );
    if( m_rbOptions->GetSelection() > 0 )
        NegateGreyscaleImage( );
    m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );

    m_NB_Image  = m_Greyscale_Image;
    Binarize( (double)m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );

    return true;
}

void BM2CMP_FRAME::updateImageInfo()
{
    // Note: the image resolution text controls are not modified
    // here, to avoid a race between text change when entered by user and
    // a text change if it is modifed here.
    int h  = m_Pict_Bitmap.GetHeight();
    int w  = m_Pict_Bitmap.GetWidth();
    int nb = m_Pict_Bitmap.GetDepth();

    m_SizeXValue->SetLabel( wxString::Format( wxT( "%d" ), w ) );
    m_SizeYValue->SetLabel( wxString::Format( wxT( "%d" ), h ) );
    m_BPPValue->SetLabel( wxString::Format( wxT( "%d" ), nb ) );

    m_SizeXValue_mm->SetLabel( wxString::Format( wxT( "%.1f" ),
        (double) w / m_imageDPI.x * 25.4 ) );
    m_SizeYValue_mm->SetLabel( wxString::Format( wxT( "%.1f" ),
        (double) h / m_imageDPI.y * 25.4 ) );
}

void BM2CMP_FRAME::OnResolutionChange( wxCommandEvent& event )
{
    long tmp;

    if( m_DPIValueX->GetValue().ToLong( &tmp ) )
        m_imageDPI.x = tmp;

    if(  m_DPIValueY->GetValue().ToLong( &tmp ) )
        m_imageDPI.y = tmp;

    if( m_imageDPI.x < 32 )
        m_imageDPI.x = 32;

    if( m_imageDPI.y < 32 )
        m_imageDPI.y = 32;

    updateImageInfo();
}

void BM2CMP_FRAME::Binarize( double aThreshold )
{
    unsigned int  pixin;
    unsigned char pixout;
    int           h = m_Greyscale_Image.GetHeight();
    int           w = m_Greyscale_Image.GetWidth();
    unsigned int  threshold = (int)(aThreshold * 256);

    for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ )
        {
            pixin   = m_Greyscale_Image.GetGreen( x, y );

            if( pixin < threshold )
                pixout = 0;
            else
                pixout = 255;

            m_NB_Image.SetRGB( x, y, pixout, pixout, pixout );
        }

    m_BN_Bitmap = wxBitmap( m_NB_Image );
}

void BM2CMP_FRAME::NegateGreyscaleImage( )
{
    unsigned char  pix;
    int           h = m_Greyscale_Image.GetHeight();
    int           w = m_Greyscale_Image.GetWidth();

    for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ )
        {
            pix   = m_Greyscale_Image.GetGreen( x, y );
            pix = ~pix;
            m_Greyscale_Image.SetRGB( x, y, pix, pix, pix );
        }
}

/* Called on Normal/Negative change option */
void BM2CMP_FRAME::OnOptionsSelection( wxCommandEvent& event )
{
    NegateGreyscaleImage( );
    m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );
    Binarize( (double)m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );
    Refresh();
}

void BM2CMP_FRAME::OnThresholdChange( wxScrollEvent& event )
{
    Binarize( (double)m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );
    Refresh();
}

void BM2CMP_FRAME::OnExport( wxCommandEvent& event )
{
    int sel = m_radioBoxFormat->GetSelection();

    switch( sel )
    {
        case 0:
            OnExportEeschema();
            break;

        case 1:
            OnExportPcbnew( true );
            break;

        case 2:
            OnExportPcbnew( false );
            break;

        case 3:
            OnExportPostScript();
            break;

        case 4:
            OnExportLogo();
            break;
    }
}

void BM2CMP_FRAME::OnExportLogo()
{
    wxFileName fn(m_ConvertedFileName);
    wxString path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxString     msg = _( "Logo file (*.kicad_wks)|*.kicad_wks" );
    wxFileDialog FileDlg( this, _( "Create a logo file" ), path, wxEmptyString,
                          msg,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    m_ConvertedFileName = FileDlg.GetPath();

    FILE*    outfile;
    outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File %s could not be created" ), m_ConvertedFileName.c_str() );
        wxMessageBox( msg );
        return;
    }

    ExportFile( outfile, 4 );
    fclose( outfile );
}

void BM2CMP_FRAME::OnExportPostScript()
{
    wxFileName fn(m_ConvertedFileName);
    wxString path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxString     msg = _( "Postscript file (*.ps)|*.ps" );
    wxFileDialog FileDlg( this, _( "Create a Postscript file" ), path, wxEmptyString,
                          msg,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    m_ConvertedFileName = FileDlg.GetPath();

    FILE*    outfile;
    outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File %s could not be created" ), m_ConvertedFileName.c_str() );
        wxMessageBox( msg );
        return;
    }

    ExportFile( outfile, 3 );
    fclose( outfile );
}

void BM2CMP_FRAME::OnExportEeschema()
{
    wxFileName fn(m_ConvertedFileName);
    wxString path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxString     msg = _( "Schematic lib file (*.lib)|*.lib" );
    wxFileDialog FileDlg( this, _( "Create a lib file for Eeschema" ), path, wxEmptyString,
                          msg,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    m_ConvertedFileName = FileDlg.GetPath();

    FILE*    outfile;
    outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File %s could not be created" ), m_ConvertedFileName.c_str() );
        wxMessageBox( msg );
        return;
    }

    ExportFile( outfile, 2 );
    fclose( outfile );
}


void BM2CMP_FRAME::OnExportPcbnew( bool aLegacyFormat )
{
    wxFileName fn(m_ConvertedFileName);
    wxString path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxString msg = aLegacyFormat ?
                _( "Footprint file (*.emp)|*.emp" ) :
                _( "Footprint file (*.kicad_mod)|*.kicad_mod" );
    wxFileDialog FileDlg( this, _( "Create a footprint file for PcbNew" ),
                          path, wxEmptyString,
                          msg,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    m_ConvertedFileName = FileDlg.GetPath();


    FILE*    outfile;
    outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File %s could not be created" ), m_ConvertedFileName.c_str() );
        wxMessageBox( msg );
        return;
    }

    ExportFile( outfile, aLegacyFormat ? 0 : 1 );
    fclose( outfile );
}

void BM2CMP_FRAME::ExportFile( FILE* aOutfile, int aFormat )
{
    // Create a potrace bitmap
    int h = m_NB_Image.GetHeight();
    int w = m_NB_Image.GetWidth();
    potrace_bitmap_t* potrace_bitmap = bm_new( w, h );

    if( !potrace_bitmap )
    {
        wxString msg;
        msg.Printf( wxT( "Error allocating memory for potrace bitmap" ) );
        wxMessageBox( msg );
        return;
    }

    /* fill the bitmap with data */
    for( int y = 0; y < h; y++ )
    {
        for( int x = 0; x < w; x++ )
        {
            unsigned char pix = m_NB_Image.GetGreen( x, y );
            BM_PUT( potrace_bitmap, x, y, pix ? 1 : 0 );
        }
    }

    bitmap2component( potrace_bitmap, aOutfile, aFormat, m_imageDPI.x, m_imageDPI.y );
}


// EDA_APP

IMPLEMENT_APP( EDA_APP )

///-----------------------------------------------------------------------------
// EDA_APP
// main program
//-----------------------------------------------------------------------------

bool EDA_APP::OnInit()
{
    wxInitAllImageHandlers();

    InitEDA_Appl( wxT( "BMP2CMP" ) );

    wxFrame* frame = new BM2CMP_FRAME();
    SetTopWindow( frame );
    frame->Show( true );

    return true;
}


void EDA_APP::MacOpenFile( const wxString& aFileName )
{
}
