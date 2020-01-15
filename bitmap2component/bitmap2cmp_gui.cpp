/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 jean-pierre.charras
 * Copyright (C) 1992-2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#include "bitmap2cmp_gui.h"
#include "bitmap2component.h"
#include <bitmap_io.h>
#include <bitmaps.h>
#include <kiface_i.h>
#include <math/util.h>      // for KiROUND
#include <kiway.h>
#include <pgm_base.h>
#include <potracelib.h>
#include <wildcards_and_files_ext.h>
#include <wx/clipbrd.h>
#include <wx/rawbmp.h>

#include "bitmap2cmp_gui_base.h"


#define KEYWORD_FRAME_POSX          wxT( "Bmconverter_Pos_x" )
#define KEYWORD_FRAME_POSY          wxT( "Bmconverter_Pos_y" )
#define KEYWORD_FRAME_SIZEX         wxT( "Bmconverter_Size_x" )
#define KEYWORD_FRAME_SIZEY         wxT( "Bmconverter_Size_y" )
#define KEYWORD_LAST_INPUT_FILE     wxT( "Last_input" )
#define KEYWORD_LAST_OUTPUT_FILE    wxT( "Last_output" )
#define KEYWORD_LAST_FORMAT         wxT( "Last_format" )
#define KEYWORD_LAST_MODLAYER       wxT( "Last_modlayer" )
#define KEYWORD_BINARY_THRESHOLD    wxT( "Threshold" )
#define KEYWORD_BW_NEGATIVE         wxT( "Negative_choice" )
#define KEYWORD_UNIT_SELECTION      wxT( "Unit_selection" )

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
    {
        m_outputSize = (double)GetOriginalSizePixels() / m_originalDPI * 25.4;
    }
    else if( m_unit == EDA_UNITS::INCHES )
    {
        m_outputSize = (double)GetOriginalSizePixels() / m_originalDPI;
    }
    else
    {
        m_outputSize = m_originalDPI;
    }

}


int IMAGE_SIZE::GetOutputDPI()
{
    int outputDPI;

    if( m_unit == EDA_UNITS::MILLIMETRES )
    {
        outputDPI = GetOriginalSizePixels() / ( m_outputSize / 25.4 );
    }
    else if( m_unit == EDA_UNITS::INCHES )
    {
        outputDPI = GetOriginalSizePixels() / m_outputSize;
    }
    else
    {
        outputDPI = KiROUND( m_outputSize );
    }

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



BM2CMP_FRAME::BM2CMP_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    BM2CMP_FRAME_BASE( aParent )
{
    SetKiway( this, aKiway );
    m_config = GetNewConfig( Pgm().App().GetAppName() );

    wxString unitList[] =
    {
        _("mm"), _("Inch"), _("DPI")
    };

    for( int ii = 0; ii < 3; ii++ )
        m_PixelUnit->Append( unitList[ii] );

    LoadSettings( m_config.get() );

    m_outputSizeX.SetUnit( getUnitFromSelection() );
    m_outputSizeY.SetUnit( getUnitFromSelection() );
    m_outputSizeX.SetOutputSize( 0, getUnitFromSelection() );
    m_outputSizeY.SetOutputSize( 0, getUnitFromSelection() );

    m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );

    //Set icon for aspect ratio
    m_AspectRatioLocked = true;
    m_AspectRatio = 1;
    m_AspectRatioLockButton->SetBitmap( KiBitmap( locked_xpm ) );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( icon_bitmap2component_xpm ) );
    SetIcon( icon );

    GetSizer()->SetSizeHints( this );

    SetSize( m_framePos.x, m_framePos.y, m_frameSize.x, m_frameSize.y );

    m_buttonExportFile->Enable( false );
    m_buttonExportClipboard->Enable( false );


    if ( m_framePos == wxDefaultPosition )
        Centre();
}


BM2CMP_FRAME::~BM2CMP_FRAME()
{
    SaveSettings( m_config.get() );
    /*
     * This needed for OSX: avoids further OnDraw processing after this
     * destructor and before the native window is destroyed
     */
    this->Freeze( );
}


void BM2CMP_FRAME::LoadSettings( wxConfigBase* aCfg )
{
    int tmp;
    aCfg->Read( KEYWORD_FRAME_POSX, & m_framePos.x, -1 );
    aCfg->Read( KEYWORD_FRAME_POSY, & m_framePos.y, -1 );
    aCfg->Read( KEYWORD_FRAME_SIZEX, & m_frameSize.x, -1 );
    aCfg->Read( KEYWORD_FRAME_SIZEY, & m_frameSize.y, -1 );
    aCfg->Read( KEYWORD_LAST_INPUT_FILE, &m_BitmapFileName );
    aCfg->Read( KEYWORD_LAST_OUTPUT_FILE, &m_ConvertedFileName );

    int u_select = 0;
    aCfg->Read( KEYWORD_UNIT_SELECTION, &u_select, 0 );

    if( u_select < 0 || u_select > 2 )  // Validity control
        u_select = 0;

    m_PixelUnit->SetSelection( u_select );

    if( aCfg->Read( KEYWORD_BINARY_THRESHOLD, &tmp ) )
        m_sliderThreshold->SetValue( tmp );

    aCfg->Read( KEYWORD_BW_NEGATIVE, &tmp, 0 );
    m_Negative = tmp != 0;
    m_checkNegative->SetValue( m_Negative );
    m_exportToClipboard = false;
    m_AspectRatioLocked = false;

    if( aCfg->Read( KEYWORD_LAST_FORMAT, &tmp ) )
    {
        if( tmp < 0 || tmp > FINAL_FMT )
            tmp = PCBNEW_KICAD_MOD;

        m_rbOutputFormat->SetSelection( tmp );
    }

    if( tmp == PCBNEW_KICAD_MOD )
        m_rbPCBLayer->Enable( true );
    else
        m_rbPCBLayer->Enable( false );

    if( aCfg->Read( KEYWORD_LAST_MODLAYER, &tmp ) )
    {
        if( (unsigned) tmp > MOD_LYR_FINAL )    // Out of range
            m_rbPCBLayer->SetSelection( MOD_LYR_FSILKS );
        else
            m_rbPCBLayer->SetSelection( tmp );
    }
}


void BM2CMP_FRAME::SaveSettings( wxConfigBase* aCfg )
{
    if( !aCfg )
        return;

    m_frameSize = GetSize();
    m_framePos  = GetPosition();

    if( !IsIconized()  )
    {
        aCfg->Write( KEYWORD_FRAME_POSX, (long) m_framePos.x );
        aCfg->Write( KEYWORD_FRAME_POSY, (long) m_framePos.y );
        aCfg->Write( KEYWORD_FRAME_SIZEX, (long) m_frameSize.x );
        aCfg->Write( KEYWORD_FRAME_SIZEY, (long) m_frameSize.y );
    }

    aCfg->Write( KEYWORD_LAST_INPUT_FILE, m_BitmapFileName );
    aCfg->Write( KEYWORD_LAST_OUTPUT_FILE, m_ConvertedFileName );
    aCfg->Write( KEYWORD_BINARY_THRESHOLD, m_sliderThreshold->GetValue() );
    aCfg->Write( KEYWORD_BW_NEGATIVE, m_checkNegative->IsChecked() ? 1 : 0 );
    aCfg->Write( KEYWORD_LAST_FORMAT,  m_rbOutputFormat->GetSelection() );
    aCfg->Write( KEYWORD_LAST_MODLAYER,  m_rbPCBLayer->GetSelection() );
    aCfg->Write( KEYWORD_UNIT_SELECTION, m_PixelUnit->GetSelection() );
}


void BM2CMP_FRAME::OnPaintInit( wxPaintEvent& event )
{
#ifdef __WXMAC__
    // Otherwise fails due: using wxPaintDC without being in a native paint event
    wxClientDC pict_dc( m_InitialPicturePanel );
#else
    wxPaintDC pict_dc( m_InitialPicturePanel );
#endif

    m_InitialPicturePanel->PrepareDC( pict_dc );

    // OSX crashes with empty bitmaps (on initial refreshes)
    if( m_Pict_Bitmap.IsOk() )
        pict_dc.DrawBitmap( m_Pict_Bitmap, 0, 0, !!m_Pict_Bitmap.GetMask() );

    event.Skip();
}


void BM2CMP_FRAME::OnPaintGreyscale( wxPaintEvent& event )
{
#ifdef __WXMAC__
    // Otherwise fails due: using wxPaintDC without being in a native paint event
    wxClientDC greyscale_dc( m_GreyscalePicturePanel );
#else
    wxPaintDC greyscale_dc( m_GreyscalePicturePanel );
#endif

    m_GreyscalePicturePanel->PrepareDC( greyscale_dc );

    // OSX crashes with empty bitmaps (on initial refreshes)
    if( m_Greyscale_Bitmap.IsOk() )
        greyscale_dc.DrawBitmap( m_Greyscale_Bitmap, 0, 0, !!m_Greyscale_Bitmap.GetMask() );

    event.Skip();
}


void BM2CMP_FRAME::OnPaintBW( wxPaintEvent& event )
{
#ifdef __WXMAC__
    // Otherwise fails due: using wxPaintDC without being in a native paint event
    wxClientDC nb_dc( m_BNPicturePanel );
#else
    wxPaintDC nb_dc( m_BNPicturePanel );
#endif

    m_BNPicturePanel->PrepareDC( nb_dc );

    if( m_BN_Bitmap.IsOk() )
        nb_dc.DrawBitmap( m_BN_Bitmap, 0, 0, !!m_BN_Bitmap.GetMask() );

    event.Skip();
}


void BM2CMP_FRAME::OnLoadFile( wxCommandEvent& event )
{
    wxFileName  fn( m_BitmapFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = m_mruPath;

    wxFileDialog fileDlg( this, _( "Choose Image" ), path, wxEmptyString,
                          _( "Image Files " ) + wxImage::GetImageExtWildcard(),
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


bool BM2CMP_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    m_Pict_Image.Destroy();
    m_BitmapFileName = aFileSet[0];

    if( !m_Pict_Image.LoadFile( m_BitmapFileName ) )
    {
        // LoadFile has its own UI, no need for further failure notification here
        return false;
    }

    m_Pict_Bitmap = wxBitmap( m_Pict_Image );

    // Determine image resolution in DPI (does not existing in all formats).
    // the resolution can be given in bit per inches or bit per cm in file

    int imageDPIx = m_Pict_Image.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONX );
    int imageDPIy = m_Pict_Image.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONY );

    if( imageDPIx > 1 && imageDPIy > 1 )
    {
        if( m_Pict_Image.GetOptionInt( wxIMAGE_OPTION_RESOLUTIONUNIT ) == wxIMAGE_RESOLUTION_CM )
        {
            imageDPIx = KiROUND( imageDPIx * 2.54 );
            imageDPIy = KiROUND( imageDPIy * 2.54 );
        }
    }
    else    // fallback to a default value (DEFAULT_DPI)
    {
        imageDPIx = imageDPIy = DEFAULT_DPI;
    }

    m_InputXValueDPI->SetLabel( wxString::Format( wxT( "%d" ), imageDPIx ) );
    m_InputYValueDPI->SetLabel( wxString::Format( wxT( "%d" ), imageDPIy ) );

    int h  = m_Pict_Bitmap.GetHeight();
    int w  = m_Pict_Bitmap.GetWidth();
    m_AspectRatio = (double) w / h;

    m_outputSizeX.SetOriginalDPI( imageDPIx );
    m_outputSizeX.SetOriginalSizePixels( w );
    m_outputSizeY.SetOriginalDPI( imageDPIy );
    m_outputSizeY.SetOriginalSizePixels( h );

    // Update display to keep aspect ratio
    auto fakeEvent = wxCommandEvent();
    OnSizeChangeX( fakeEvent );

    updateImageInfo();

    m_InitialPicturePanel->SetVirtualSize( w, h );
    m_GreyscalePicturePanel->SetVirtualSize( w, h );
    m_BNPicturePanel->SetVirtualSize( w, h );

    m_Greyscale_Image.Destroy();
    m_Greyscale_Image = m_Pict_Image.ConvertToGreyscale( );

    if( m_Pict_Bitmap.GetMask() )
    {
        for( int x = 0; x < m_Pict_Bitmap.GetWidth(); x++ )
        {
            for( int y = 0; y < m_Pict_Bitmap.GetHeight(); y++ )
            {
                if( m_Pict_Image.GetRed( x, y ) == m_Pict_Image.GetMaskRed() &&
                    m_Pict_Image.GetGreen( x, y ) == m_Pict_Image.GetMaskGreen() &&
                    m_Pict_Image.GetBlue( x, y ) == m_Pict_Image.GetMaskBlue() )
                {
                    m_Greyscale_Image.SetRGB( x, y, 255, 255, 255 );
                }
            }
        }
    }

    if( m_Negative )
        NegateGreyscaleImage( );

    m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );
    m_NB_Image  = m_Greyscale_Image;
    Binarize( (double) m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );

    m_buttonExportFile->Enable( true );
    m_buttonExportClipboard->Enable( true );

    m_outputSizeX.SetOutputSizeFromInitialImageSize();
    m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_outputSizeY.SetOutputSizeFromInitialImageSize();
    m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );

    return true;
}


// return a string giving the output size, according to the selected unit
wxString BM2CMP_FRAME::FormatOutputSize( double aSize )
{
    wxString text;

    if( getUnitFromSelection() == EDA_UNITS::MILLIMETRES )
    {
        text.Printf( "%.1f", aSize );
    }
    else if( getUnitFromSelection() == EDA_UNITS::INCHES )
    {
        text.Printf( "%.2f", aSize );
    }
    else
    {
        text.Printf( "%d", KiROUND( aSize ) );
    }

    return text;
}

void BM2CMP_FRAME::updateImageInfo()
{
    // Note: the image resolution text controls are not modified
    // here, to avoid a race between text change when entered by user and
    // a text change if it is modified here.

    if( m_Pict_Bitmap.IsOk() )
    {
        int h = m_Pict_Bitmap.GetHeight();
        int w = m_Pict_Bitmap.GetWidth();
        int nb = m_Pict_Bitmap.GetDepth();

        m_SizeXValue->SetLabel( wxString::Format( wxT( "%d" ), w ) );
        m_SizeYValue->SetLabel( wxString::Format( wxT( "%d" ), h ) );
        m_BPPValue->SetLabel( wxString::Format( wxT( "%d" ), nb ) );
    }
}


EDA_UNITS BM2CMP_FRAME::getUnitFromSelection()
{
    // return the EDA_UNITS from the m_PixelUnit choice
    switch( m_PixelUnit->GetSelection() )
    {
    case 1:
        return EDA_UNITS::INCHES;

    case 2:
        return EDA_UNITS::UNSCALED;

    case 0:
    default:
        break;
    }

    return EDA_UNITS::MILLIMETRES;
}


void BM2CMP_FRAME::OnSizeChangeX( wxCommandEvent& event )
{
    double new_size;

    if( m_UnitSizeX->GetValue().ToDouble( &new_size ) )
    {
        if( m_AspectRatioLocked )
        {
            double calculatedY = new_size / m_AspectRatio;

            if( getUnitFromSelection() == EDA_UNITS::UNSCALED )
            {
                // for units in DPI, keeping aspect ratio cannot use m_AspectRatioLocked.
                // just rescale the other dpi
                double ratio = new_size / m_outputSizeX.GetOutputSize();
                calculatedY = m_outputSizeY.GetOutputSize() * ratio;
            }

            m_outputSizeY.SetOutputSize( calculatedY, getUnitFromSelection() );
            m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );
        }

        m_outputSizeX.SetOutputSize( new_size, getUnitFromSelection() );
    }

    updateImageInfo();
}


void BM2CMP_FRAME::OnSizeChangeY( wxCommandEvent& event )
{
    double new_size;

    if( m_UnitSizeY->GetValue().ToDouble( &new_size ) )
    {
        if( m_AspectRatioLocked )
        {
            double calculatedX = new_size * m_AspectRatio;

            if( getUnitFromSelection() == EDA_UNITS::UNSCALED )
            {
                // for units in DPI, keeping aspect ratio cannot use m_AspectRatioLocked.
                // just rescale the other dpi
                double ratio = new_size / m_outputSizeX.GetOutputSize();
                calculatedX = m_outputSizeX.GetOutputSize() * ratio;
            }

            m_outputSizeX.SetOutputSize( calculatedX, getUnitFromSelection() );
            m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
        }

        m_outputSizeY.SetOutputSize( new_size, getUnitFromSelection() );
    }

    updateImageInfo();
}


void BM2CMP_FRAME::OnSizeUnitChange( wxCommandEvent& event )
{
    m_outputSizeX.SetUnit( getUnitFromSelection() );
    m_outputSizeY.SetUnit( getUnitFromSelection() );
    updateImageInfo();

    m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );
}


void BM2CMP_FRAME::ToggleAspectRatioLock( wxCommandEvent& event )
{
    m_AspectRatioLocked = !m_AspectRatioLocked;

    if( m_AspectRatioLocked )
    {
        m_AspectRatioLockButton->SetBitmap( KiBitmap( locked_xpm ) );
        //Force display update when aspect ratio is locked
        auto fakeEvent = wxCommandEvent();
        OnSizeChangeX( fakeEvent );
    }

    else
    {
        m_AspectRatioLockButton->SetBitmap( KiBitmap( unlocked_xpm ) );
    }
}


void BM2CMP_FRAME::Binarize( double aThreshold )
{
    int           h = m_Greyscale_Image.GetHeight();
    int           w = m_Greyscale_Image.GetWidth();
    unsigned char threshold = aThreshold * 255;
    unsigned char alpha_thresh = 0.7 * threshold;

    for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ )
        {
            unsigned char pixout;
            auto pixin   = m_Greyscale_Image.GetGreen( x, y );
            auto alpha   = m_Greyscale_Image.HasAlpha() ?
                    m_Greyscale_Image.GetAlpha( x, y ) : wxALPHA_OPAQUE;

            if( pixin < threshold && alpha > alpha_thresh )
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
    int             h = m_Greyscale_Image.GetHeight();
    int             w = m_Greyscale_Image.GetWidth();

    for( int y = 0; y < h; y++ )
        for( int x = 0; x < w; x++ )
        {
            pix   = m_Greyscale_Image.GetGreen( x, y );
            pix = ~pix;
            m_Greyscale_Image.SetRGB( x, y, pix, pix, pix );
        }
}


void BM2CMP_FRAME::OnNegativeClicked( wxCommandEvent&  )
{
    if( m_checkNegative->GetValue() != m_Negative )
    {
        NegateGreyscaleImage();

        m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );
        Binarize( (double)m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );
        m_Negative = m_checkNegative->GetValue();

        Refresh();
    }
}


void BM2CMP_FRAME::OnThresholdChange( wxScrollEvent& event )
{
    Binarize( (double)m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );
    Refresh();
}


void BM2CMP_FRAME::OnExportToFile( wxCommandEvent& event )
{
    m_exportToClipboard = false;
    // choices of m_rbOutputFormat are expected to be in same order as
    // OUTPUT_FMT_ID. See bitmap2component.h
    OUTPUT_FMT_ID format = (OUTPUT_FMT_ID) m_rbOutputFormat->GetSelection();
    exportBitmap( format );
}


void BM2CMP_FRAME::OnExportToClipboard( wxCommandEvent& event )
{
    m_exportToClipboard = true;
    // choices of m_rbOutputFormat are expected to be in same order as
    // OUTPUT_FMT_ID. See bitmap2component.h
    OUTPUT_FMT_ID format = (OUTPUT_FMT_ID) m_rbOutputFormat->GetSelection();

    std::string buffer;
    ExportToBuffer( buffer, format );

    // Write buffer to the clipboard
    if (wxTheClipboard->Open())
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject( buffer.c_str() ) );
        wxTheClipboard->Close();
    }
    else
        wxMessageBox( _( "Unable to export to the Clipboard") );
}


void BM2CMP_FRAME::exportBitmap( OUTPUT_FMT_ID aFormat )
{
    switch( aFormat )
    {
    case EESCHEMA_FMT:
        exportEeschemaFormat();
        break;

    case PCBNEW_KICAD_MOD:
        exportPcbnewFormat();
        break;

    case POSTSCRIPT_FMT:
        exportPostScriptFormat();
        break;

    case KICAD_LOGO:
        OnExportLogo();
        break;
    }
}


void BM2CMP_FRAME::OnExportLogo()
{
    wxFileName  fn( m_ConvertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create Logo File" ), path, wxEmptyString,
                          PageLayoutDescrFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    int          diag = fileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( PageLayoutDescrFileExtension );
    m_ConvertedFileName = fn.GetFullPath();

    FILE*    outfile;
    outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File \"%s\" could not be created." ), m_ConvertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    ExportToBuffer( buffer, KICAD_LOGO );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BM2CMP_FRAME::exportPostScriptFormat()
{
    wxFileName  fn( m_ConvertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create Postscript File" ),
                          path, wxEmptyString,
                          PSFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    int          diag = fileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( wxT( "ps" ) );
    m_ConvertedFileName = fn.GetFullPath();

    FILE*    outfile;
    outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File \"%s\" could not be created." ), m_ConvertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    ExportToBuffer( buffer, POSTSCRIPT_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BM2CMP_FRAME::exportEeschemaFormat()
{
    wxFileName  fn( m_ConvertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists(path) )
        path = ::wxGetCwd();

    wxFileDialog fileDlg( this, _( "Create Symbol Library" ),
                          path, wxEmptyString,
                          SchematicLibraryFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    int          diag = fileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( SchematicLibraryFileExtension );
    m_ConvertedFileName = fn.GetFullPath();

    FILE*    outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File \"%s\" could not be created." ), m_ConvertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    ExportToBuffer( buffer, EESCHEMA_FMT );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
}


void BM2CMP_FRAME::exportPcbnewFormat()
{
    wxFileName  fn( m_ConvertedFileName );
    wxString    path = fn.GetPath();

    if( path.IsEmpty() || !wxDirExists( path ) )
        path = m_mruPath;

    wxFileDialog fileDlg( this, _( "Create Footprint Library" ),
                          path, wxEmptyString,
                          KiCadFootprintLibFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    int          diag = fileDlg.ShowModal();

    if( diag != wxID_OK )
        return;

    fn = fileDlg.GetPath();
    fn.SetExt( KiCadFootprintFileExtension );
    m_ConvertedFileName = fn.GetFullPath();

    FILE* outfile = wxFopen( m_ConvertedFileName, wxT( "w" ) );

    if( outfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File \"%s\" could not be created." ), m_ConvertedFileName );
        wxMessageBox( msg );
        return;
    }

    std::string buffer;
    ExportToBuffer( buffer, PCBNEW_KICAD_MOD );
    fputs( buffer.c_str(), outfile );
    fclose( outfile );
    m_mruPath = fn.GetPath();
}


void BM2CMP_FRAME::ExportToBuffer( std::string& aOutput, OUTPUT_FMT_ID aFormat )
{
    // Create a potrace bitmap
    int h = m_NB_Image.GetHeight();
    int w = m_NB_Image.GetWidth();
    potrace_bitmap_t* potrace_bitmap = bm_new( w, h );

    if( !potrace_bitmap )
    {
        wxString msg;
        msg.Printf( _( "Error allocating memory for potrace bitmap" ) );
        wxMessageBox( msg );
        return;
    }

    /* fill the bitmap with data */
    for( int y = 0; y < h; y++ )
    {
        for( int x = 0; x < w; x++ )
        {
            auto pix = m_NB_Image.GetGreen( x, y );
            BM_PUT( potrace_bitmap, x, y, pix ? 0 : 1 );
        }
    }

    // choices of m_rbPCBLayer are expected to be in same order as
    // BMP2CMP_MOD_LAYER. See bitmap2component.h
    BMP2CMP_MOD_LAYER modLayer = MOD_LYR_FSILKS;

    if( aFormat == PCBNEW_KICAD_MOD )
        modLayer = (BMP2CMP_MOD_LAYER) m_rbPCBLayer->GetSelection();

    BITMAPCONV_INFO converter( aOutput );
    converter.ConvertBitmap( potrace_bitmap, aFormat, m_outputSizeX.GetOutputDPI(),
            m_outputSizeY.GetOutputDPI(), modLayer );

    if( !converter.GetErrorMessages().empty() )
        wxMessageBox( converter.GetErrorMessages().c_str(), _( "Errors" ) );
}


void BM2CMP_FRAME::OnFormatChange( wxCommandEvent& event )
{
    if( m_rbOutputFormat->GetSelection() == PCBNEW_KICAD_MOD )
        m_rbPCBLayer->Enable( true );
    else
        m_rbPCBLayer->Enable( false );
}
