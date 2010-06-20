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
#include "wx/wx.h"
#include "wx/config.h"

#include "bitmap2cmp_gui_base.h"

#include "potracelib.h"
#include "bitmap_io.h"

//#include "bitmap2component.xpm"


#define KEYWORD_FRAME_POSX wxT( "bmconverter_Pos_x" )
#define KEYWORD_FRAME_POSY wxT( "bmconverter_Pos_y" )
#define KEYWORD_FRAME_SIZEX wxT( "bmconverter_Size_x" )
#define KEYWORD_FRAME_SIZEY wxT( "bmconverter_Size_y" )

extern int bitmap2component( potrace_bitmap_t* aPotrace_bitmap, FILE* aOutfile, int aFormat );

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
    wxString  m_ImgFileName;
    wxSize m_FrameSize;
    wxPoint m_FramePos;
    wxConfig * m_Config;

public:
    BM2CMP_FRAME();
    ~BM2CMP_FRAME();

private:

    // Event handlers
    void OnPaint( wxPaintEvent& event );
    void OnLoadFile( wxCommandEvent& event );
    void OnExportEeschema( wxCommandEvent& event );
    void OnExportPcbnew( wxCommandEvent& event );
    void Binarize( int aThreshold );
	void OnOptionsSelection( wxCommandEvent& event );
	void OnThresholdChange( wxScrollEvent& event );
    void NegateGreyscaleImage( );
    void ExportFile( FILE* aOutfile, int aFormat );
};

BM2CMP_FRAME::BM2CMP_FRAME() : BM2CMP_FRAME_BASE( NULL )
{
    m_Config = new wxConfig();
    m_Config->Read( KEYWORD_FRAME_POSX, & m_FramePos.x, -1 );
    m_Config->Read( KEYWORD_FRAME_POSY, & m_FramePos.y, -1 );
    m_Config->Read( KEYWORD_FRAME_SIZEX, & m_FrameSize.x, -1 );
    m_Config->Read( KEYWORD_FRAME_SIZEY, & m_FrameSize.y, -1 );

    SetIcon( wxICON( bitmap2component_icon ) );

    wxString msg( wxT( "  0000  " ) );
    m_gridInfo->SetCellValue( 1, 0, msg );
    m_gridInfo->SetCellValue( 2, 0, msg );
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    if ( m_FramePos == wxDefaultPosition )
        Centre();
}

BM2CMP_FRAME::~BM2CMP_FRAME()
{
    if( ( m_Config == NULL ) || IsIconized() )
        return;

    m_FrameSize = GetSize();
    m_FramePos  = GetPosition();

    m_Config->Write( KEYWORD_FRAME_POSX, (long) m_FramePos.x );
    m_Config->Write( KEYWORD_FRAME_POSY, (long) m_FramePos.y );
    m_Config->Write( KEYWORD_FRAME_SIZEX, (long) m_FrameSize.x );
    m_Config->Write( KEYWORD_FRAME_SIZEY, (long) m_FrameSize.y );

    delete m_Config;

    /* This needed for OSX: avoids furter OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    this->Freeze( );
}


void BM2CMP_FRAME::OnPaint( wxPaintEvent& event )
{
    wxPaintDC pict_dc( m_InitialPicturePanel );
    wxPaintDC greyscale_dc( m_GreyscalePicturePanel );
    wxPaintDC nb_dc( m_BNPicturePanel );

    m_InitialPicturePanel->PrepareDC( pict_dc );
    m_GreyscalePicturePanel->PrepareDC( greyscale_dc );
    m_BNPicturePanel->PrepareDC( nb_dc );
    pict_dc.DrawBitmap( m_Pict_Bitmap, 0, 0, false );
    greyscale_dc.DrawBitmap( m_Greyscale_Bitmap, 0, 0, false );
    nb_dc.DrawBitmap( m_BN_Bitmap, 0, 0, false );
}

/* Called to load a bitmap file
 */
void BM2CMP_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxFileDialog FileDlg( this, _( "Choose Image" ), ::wxGetCwd(), wxEmptyString,
                          _( "Image Files " ) + wxImage::GetImageExtWildcard(),
                          wxFD_OPEN );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;
    m_ImgFileName = FileDlg.GetPath();

    if( !m_Pict_Image.LoadFile( m_ImgFileName ) )
    {
        wxMessageBox( _( "Couldn't load image from '%s'." ), m_ImgFileName.c_str() );
        return;
    }

    m_Pict_Bitmap = wxBitmap( m_Pict_Image );

    int h  = m_Pict_Bitmap.GetHeight();
    int w  = m_Pict_Bitmap.GetWidth();
    int nb = m_Pict_Bitmap.GetDepth();

    wxString msg;
    msg.Printf( wxT( "%d" ), h );
    m_gridInfo->SetCellValue( 0, 0, msg );
    msg.Printf( wxT( "%d" ), w );
    m_gridInfo->SetCellValue( 1, 0, msg );
    msg.Printf( wxT( "%d" ), nb );
    m_gridInfo->SetCellValue( 2, 0, msg );

    m_InitialPicturePanel->SetVirtualSize( w, h );
    m_GreyscalePicturePanel->SetVirtualSize( w, h );
    m_BNPicturePanel->SetVirtualSize( w, h );

    m_Greyscale_Image.Destroy();
    m_Greyscale_Image = m_Pict_Image.ConvertToGreyscale( );
    if( m_rbOptions->GetSelection() > 0 )
        NegateGreyscaleImage( );
    m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );

    m_NB_Image  = m_Greyscale_Image;
    Binarize( m_sliderThreshold->GetValue() );
    Refresh();
}


void BM2CMP_FRAME::Binarize( int aThreshold )
{
    unsigned int  pixin;
    unsigned char pixout;
    int           h = m_Greyscale_Image.GetHeight();
    int           w = m_Greyscale_Image.GetWidth();
    unsigned int  threshold = (aThreshold * 256) / 10;

    for( int y = 1; y < h; y++ )
        for( int x = 1; x < w; x++ )
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

    for( int y = 1; y < h; y++ )
        for( int x = 1; x < w; x++ )
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
    Binarize( m_sliderThreshold->GetValue() );
    Refresh();
}

void BM2CMP_FRAME::OnThresholdChange( wxScrollEvent& event )
{
    Binarize( m_sliderThreshold->GetValue() );
    Refresh();
}


void BM2CMP_FRAME::OnExportEeschema( wxCommandEvent& event )
{
    wxString     msg = _( "Schematic lib file (*.lib)|*.lib" );
    wxFileDialog FileDlg( this, _( "Create lib file" ), ::wxGetCwd(), wxEmptyString,
                          msg,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;
    wxString filename = FileDlg.GetPath();

    FILE*    outfile;
    outfile = wxFopen( filename, wxT( "w" ) );
    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File %s could not be created" ), filename.c_str() );
        wxMessageBox( msg );
        return;
    }

    ExportFile( outfile, 1 );
    fclose( outfile );
}


void BM2CMP_FRAME::OnExportPcbnew( wxCommandEvent& event )
{
    wxString     msg = _( "Footprint export file (*.emp)|*.emp" );
    wxFileDialog FileDlg( this, _( "Create footprint export file" ), ::wxGetCwd(), wxEmptyString,
                          msg,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = FileDlg.ShowModal();

    if( diag != wxID_OK )
        return;
    wxString filename = FileDlg.GetPath();

    FILE*    outfile;
    outfile = wxFopen( filename, wxT( "w" ) );
    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File %s could not be created" ), filename.c_str() );
        wxMessageBox( msg );
        return;
    }

    ExportFile( outfile, 0 );
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
    for( int y = 0; y<h; y++ )
    {
        for( int x = 0; x<w; x++ )
        {
            unsigned char pix = m_NB_Image.GetGreen( x, y );
            BM_PUT( potrace_bitmap, x, y, pix ? 1 : 0 );
        }
    }

    bitmap2component( potrace_bitmap, aOutfile, aFormat );
}


// BM_TO_CMP_APP

class BM_TO_CMP_APP : public wxApp
{
public:
    virtual bool OnInit();
};


IMPLEMENT_APP( BM_TO_CMP_APP )

///-----------------------------------------------------------------------------
// BM_TO_CMP_APP
// main program
//-----------------------------------------------------------------------------

bool BM_TO_CMP_APP::OnInit()
{
    wxInitAllImageHandlers();

    SetVendorName( wxT( "kicad" ) );

    wxFrame* frame = new BM2CMP_FRAME();
    frame->Show( true );

    return true;
}
