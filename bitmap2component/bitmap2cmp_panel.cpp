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

#include <bitmap2cmp_frame.h>
#include <bitmap2component.h>
#include <bitmap2cmp_panel.h>
#include <bitmap2cmp_settings.h>
#include <bitmap_io.h>
#include <bitmaps.h>
#include <common.h>
#include <kiface_base.h>
#include <math/util.h>      // for KiROUND
#include <pgm_base.h>
#include <potracelib.h>
#include <wx/clipbrd.h>
#include <wx/rawbmp.h>
#include <wx/msgdlg.h>
#include <wx/dcclient.h>
#include <wx/log.h>

#define DEFAULT_DPI 300     // the image DPI used in formats that do not define a DPI


BITMAP2CMP_PANEL::BITMAP2CMP_PANEL( BITMAP2CMP_FRAME* aParent ) :
        BITMAP2CMP_PANEL_BASE( aParent ),
        m_parentFrame( aParent ), m_negative( false ),
        m_aspectRatio( 1.0 )
{
    for( wxString unit : { _( "mm" ), _( "Inch" ), _( "DPI" ) } )
        m_PixelUnit->Append( unit );

    m_outputSizeX.SetUnit( getUnitFromSelection() );
    m_outputSizeY.SetUnit( getUnitFromSelection() );
    m_outputSizeX.SetOutputSize( 0, getUnitFromSelection() );
    m_outputSizeY.SetOutputSize( 0, getUnitFromSelection() );

    m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );

    m_buttonExportFile->Enable( false );
    m_buttonExportClipboard->Enable( false );
}


BITMAP2CMP_PANEL::~BITMAP2CMP_PANEL()
{
}


wxWindow* BITMAP2CMP_PANEL::GetCurrentPage()
{
    return m_Notebook->GetCurrentPage();
}


void BITMAP2CMP_PANEL::LoadSettings( BITMAP2CMP_SETTINGS* cfg )
{
    int u_select = cfg->m_Units;

    if( u_select < 0 || u_select > 2 )  // Validity control
        u_select = 0;

    m_PixelUnit->SetSelection( u_select );

    m_sliderThreshold->SetValue( cfg->m_Threshold );

    m_negative = cfg->m_Negative;
    m_checkNegative->SetValue( cfg->m_Negative );

    m_aspectRatio = 1.0;
    m_aspectRatioCheckbox->SetValue( true );

    int format = cfg->m_LastFormat;

    if( format < 0 || format > FINAL_FMT )
        format = PCBNEW_KICAD_MOD;

    m_rbOutputFormat->SetSelection( format );

    bool enable = format == PCBNEW_KICAD_MOD;
    m_chPCBLayer->Enable( enable );

    int last_layer = cfg->m_LastModLayer;

    if( last_layer < 0 || last_layer > static_cast<int>( MOD_LYR_FINAL ) )   // Out of range
       last_layer = MOD_LYR_FSILKS;

    m_chPCBLayer->SetSelection( last_layer );
}


void BITMAP2CMP_PANEL::SaveSettings( BITMAP2CMP_SETTINGS* cfg )
{
    cfg->m_Threshold    = m_sliderThreshold->GetValue();
    cfg->m_Negative     = m_checkNegative->IsChecked();
    cfg->m_LastFormat   = m_rbOutputFormat->GetSelection();
    cfg->m_LastModLayer = m_chPCBLayer->GetSelection();
    cfg->m_Units        = m_PixelUnit->GetSelection();
}


void BITMAP2CMP_PANEL::OnPaintInit( wxPaintEvent& event )
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


void BITMAP2CMP_PANEL::OnPaintGreyscale( wxPaintEvent& event )
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


void BITMAP2CMP_PANEL::OnPaintBW( wxPaintEvent& event )
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


void BITMAP2CMP_PANEL::OnLoadFile( wxCommandEvent& event )
{
    m_parentFrame->OnLoadFile();
}


bool BITMAP2CMP_PANEL::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    m_Pict_Image.Destroy();

    if( !m_Pict_Image.LoadFile( aFileSet[0] ) )
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
    m_aspectRatio = (double) w / h;

    m_outputSizeX.SetOriginalDPI( imageDPIx );
    m_outputSizeX.SetOriginalSizePixels( w );
    m_outputSizeY.SetOriginalDPI( imageDPIy );
    m_outputSizeY.SetOriginalSizePixels( h );

    // Update display to keep aspect ratio
    wxCommandEvent dummy;
    OnSizeChangeX( dummy );

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

    if( m_negative )
        NegateGreyscaleImage( );

    m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );
    m_NB_Image  = m_Greyscale_Image;
    Binarize( (double) m_sliderThreshold->GetValue() / m_sliderThreshold->GetMax() );

    m_buttonExportFile->Enable( true );
    m_buttonExportClipboard->Enable( true );

    m_outputSizeX.SetOutputSizeFromInitialImageSize();
    m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_outputSizeY.SetOutputSizeFromInitialImageSize();
    m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );

    return true;
}


// return a string giving the output size, according to the selected unit
wxString BITMAP2CMP_PANEL::FormatOutputSize( double aSize )
{
    wxString text;

    if( getUnitFromSelection() == EDA_UNITS::MILLIMETRES )
        text.Printf( wxS( "%.1f" ), aSize );
    else if( getUnitFromSelection() == EDA_UNITS::INCHES )
        text.Printf( wxS( "%.2f" ), aSize );
    else
        text.Printf( wxT( "%d" ), KiROUND( aSize ) );

    return text;
}

void BITMAP2CMP_PANEL::updateImageInfo()
{
    // Note: the image resolution text controls are not modified here, to avoid a race between
    // text change when entered by user and a text change if it is modified here.

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


EDA_UNITS BITMAP2CMP_PANEL::getUnitFromSelection()
{
    // return the EDA_UNITS from the m_PixelUnit choice
    switch( m_PixelUnit->GetSelection() )
    {
    case 1:  return EDA_UNITS::INCHES;
    case 2:  return EDA_UNITS::UNSCALED;
    case 0:
    default: return EDA_UNITS::MILLIMETRES;
    }
}


void BITMAP2CMP_PANEL::OnSizeChangeX( wxCommandEvent& event )
{
    double new_size;

    if( m_UnitSizeX->GetValue().ToDouble( &new_size ) )
    {
        if( m_aspectRatioCheckbox->GetValue() )
        {
            double calculatedY = new_size / m_aspectRatio;

            if( getUnitFromSelection() == EDA_UNITS::UNSCALED )
            {
                // for units in DPI, keeping aspect ratio cannot use m_AspectRatioLocked.
                // just re-scale the other dpi
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


void BITMAP2CMP_PANEL::OnSizeChangeY( wxCommandEvent& event )
{
    double new_size;

    if( m_UnitSizeY->GetValue().ToDouble( &new_size ) )
    {
        if( m_aspectRatioCheckbox->GetValue() )
        {
            double calculatedX = new_size * m_aspectRatio;

            if( getUnitFromSelection() == EDA_UNITS::UNSCALED )
            {
                // for units in DPI, keeping aspect ratio cannot use m_AspectRatioLocked.
                // just re-scale the other dpi
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


void BITMAP2CMP_PANEL::OnSizeUnitChange( wxCommandEvent& event )
{
    m_outputSizeX.SetUnit( getUnitFromSelection() );
    m_outputSizeY.SetUnit( getUnitFromSelection() );
    updateImageInfo();

    m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );
}


void BITMAP2CMP_PANEL::SetOutputSize( const IMAGE_SIZE& aSizeX, const IMAGE_SIZE& aSizeY )
{
    m_outputSizeX = aSizeX;
    m_outputSizeY = aSizeY;
    updateImageInfo();

    m_UnitSizeX->ChangeValue( FormatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( FormatOutputSize( m_outputSizeY.GetOutputSize() ) );
}


void BITMAP2CMP_PANEL::ToggleAspectRatioLock( wxCommandEvent& event )
{
    if( m_aspectRatioCheckbox->GetValue() )
    {
        // Force display update when aspect ratio is locked
        wxCommandEvent dummy;
        OnSizeChangeX( dummy );
    }
}


void BITMAP2CMP_PANEL::Binarize( double aThreshold )
{
    int           h = m_Greyscale_Image.GetHeight();
    int           w = m_Greyscale_Image.GetWidth();
    unsigned char threshold = aThreshold * 255;
    unsigned char alpha_thresh = 0.7 * threshold;

    for( int y = 0; y < h; y++ )
    {
        for( int x = 0; x < w; x++ )
        {
            unsigned char pixout;
            unsigned char pixin = m_Greyscale_Image.GetGreen( x, y );
            unsigned char alpha = m_Greyscale_Image.HasAlpha() ? m_Greyscale_Image.GetAlpha( x, y )
                                                               : wxALPHA_OPAQUE;

            if( pixin < threshold && alpha > alpha_thresh )
                pixout = 0;
            else
                pixout = 255;

            m_NB_Image.SetRGB( x, y, pixout, pixout, pixout );
        }
    }

    m_BN_Bitmap = wxBitmap( m_NB_Image );
}


void BITMAP2CMP_PANEL::NegateGreyscaleImage( )
{
    unsigned char pix;
    int           h = m_Greyscale_Image.GetHeight();
    int           w = m_Greyscale_Image.GetWidth();

    for( int y = 0; y < h; y++ )
    {
        for( int x = 0; x < w; x++ )
        {
            pix   = m_Greyscale_Image.GetGreen( x, y );
            pix = ~pix;
            m_Greyscale_Image.SetRGB( x, y, pix, pix, pix );
        }
    }
}


void BITMAP2CMP_PANEL::OnNegativeClicked( wxCommandEvent&  )
{
    if( m_checkNegative->GetValue() != m_negative )
    {
        NegateGreyscaleImage();

        m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );
        Binarize( (double)m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );
        m_negative = m_checkNegative->GetValue();

        Refresh();
    }
}


void BITMAP2CMP_PANEL::OnThresholdChange( wxScrollEvent& event )
{
    Binarize( (double)m_sliderThreshold->GetValue()/m_sliderThreshold->GetMax() );
    Refresh();
}


void BITMAP2CMP_PANEL::OnExportToFile( wxCommandEvent& event )
{
    // choices of m_rbOutputFormat are expected to be in same order as
    // OUTPUT_FMT_ID. See bitmap2component.h
    OUTPUT_FMT_ID format = (OUTPUT_FMT_ID) m_rbOutputFormat->GetSelection();
    exportBitmap( format );
}


void BITMAP2CMP_PANEL::OnExportToClipboard( wxCommandEvent& event )
{
    // choices of m_rbOutputFormat are expected to be in same order as
    // OUTPUT_FMT_ID. See bitmap2component.h
    OUTPUT_FMT_ID format = (OUTPUT_FMT_ID) m_rbOutputFormat->GetSelection();

    std::string buffer;
    ExportToBuffer( buffer, format );

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    // Write buffer to the clipboard
    if( wxTheClipboard->Open() )
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxTheClipboard->SetData( new wxTextDataObject( buffer.c_str() ) );
        wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
        wxTheClipboard->Close();
    }
    else
    {
        wxMessageBox( _( "Unable to export to the Clipboard") );
    }
}


void BITMAP2CMP_PANEL::exportBitmap( OUTPUT_FMT_ID aFormat )
{
    switch( aFormat )
    {
    case EESCHEMA_FMT:     m_parentFrame->ExportEeschemaFormat();   break;
    case PCBNEW_KICAD_MOD: m_parentFrame->ExportPcbnewFormat();     break;
    case POSTSCRIPT_FMT:   m_parentFrame->ExportPostScriptFormat(); break;
    case KICAD_WKS_LOGO:   m_parentFrame->ExportLogo();             break;
    }
}


void BITMAP2CMP_PANEL::ExportToBuffer( std::string& aOutput, OUTPUT_FMT_ID aFormat )
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
            unsigned char pix = m_NB_Image.GetGreen( x, y );
            BM_PUT( potrace_bitmap, x, y, pix ? 0 : 1 );
        }
    }

    // choices of m_rbPCBLayer are expected to be in same order as
    // BMP2CMP_MOD_LAYER. See bitmap2component.h
    BMP2CMP_MOD_LAYER modLayer = MOD_LYR_FSILKS;

    if( aFormat == PCBNEW_KICAD_MOD )
        modLayer = (BMP2CMP_MOD_LAYER) m_chPCBLayer->GetSelection();

    BITMAPCONV_INFO converter( aOutput );
    converter.ConvertBitmap( potrace_bitmap, aFormat, m_outputSizeX.GetOutputDPI(),
                             m_outputSizeY.GetOutputDPI(), modLayer );

    if( !converter.GetErrorMessages().empty() )
        wxMessageBox( converter.GetErrorMessages().c_str(), _( "Errors" ) );
}


void BITMAP2CMP_PANEL::OnFormatChange( wxCommandEvent& event )
{
    bool enable = m_rbOutputFormat->GetSelection() == PCBNEW_KICAD_MOD;
    m_chPCBLayer->Enable( enable );
}
