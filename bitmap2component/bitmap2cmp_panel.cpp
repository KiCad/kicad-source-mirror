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

#include <bitmap2cmp_frame.h>
#include <bitmap2component.h>
#include <bitmap2cmp_panel.h>
#include <bitmap2cmp_settings.h>
#include <bitmap_io.h>
#include <common.h>
#include <math/util.h>      // for KiROUND
#include <potracelib.h>
#include <vector>
#include <wx/arrstr.h>
#include <wx/clipbrd.h>
#include <wx/dnd.h>
#include <wx/rawbmp.h>
#include <wx/msgdlg.h>
#include <wx/dcclient.h>
#include <wx/log.h>
#include <wx/string.h>

#define DEFAULT_DPI 300     // the image DPI used in formats that do not define a DPI


BITMAP2CMP_PANEL::BITMAP2CMP_PANEL( BITMAP2CMP_FRAME* aParent ) :
        BITMAP2CMP_PANEL_BASE( aParent ),
        m_parentFrame( aParent ),
        m_negative( false ),
        m_aspectRatio( 1.0 )
{
    for( const wxString& unit : { _( "mm" ), _( "Inch" ), _( "DPI" ) } )
        m_PixelUnit->Append( unit );

    m_outputSizeX.SetUnit( getUnitFromSelection() );
    m_outputSizeY.SetUnit( getUnitFromSelection() );
    m_outputSizeX.SetOutputSize( 0, getUnitFromSelection() );
    m_outputSizeY.SetOutputSize( 0, getUnitFromSelection() );

    m_UnitSizeX->ChangeValue( formatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( formatOutputSize( m_outputSizeY.GetOutputSize() ) );

    m_buttonExportFile->Enable( false );
    m_buttonExportClipboard->Enable( false );

    m_InitialPicturePanel->SetDropTarget( new DROP_FILE( this ) );
    m_GreyscalePicturePanel->SetDropTarget( new DROP_FILE( this ) );
    m_BNPicturePanel->SetDropTarget( new DROP_FILE( this ) );
}

wxWindow* BITMAP2CMP_PANEL::GetCurrentPage()
{
    return m_Notebook->GetCurrentPage();
}


void BITMAP2CMP_PANEL::LoadSettings( BITMAP2CMP_SETTINGS* cfg )
{
    if( cfg->m_Units >= 0 && cfg->m_Units < (int) m_PixelUnit->GetCount() )
        m_PixelUnit->SetSelection( cfg->m_Units );

    m_sliderThreshold->SetValue( cfg->m_Threshold );

    m_negative = cfg->m_Negative;
    m_checkNegative->SetValue( cfg->m_Negative );

    m_aspectRatio = 1.0;
    m_aspectRatioCheckbox->SetValue( true );

    switch( cfg->m_LastFormat )
    {
    default:
    case FOOTPRINT_FMT:     m_rbFootprint->SetValue( true );  break;
    case SYMBOL_FMT:
    case SYMBOL_PASTE_FMT:  m_rbSymbol->SetValue( true );     break;
    case POSTSCRIPT_FMT:    m_rbPostscript->SetValue( true ); break;
    case DRAWING_SHEET_FMT: m_rbWorksheet->SetValue( true );  break;
    }

    m_layerLabel->Enable( cfg->m_LastFormat == FOOTPRINT_FMT );
    m_layerCtrl->Enable( cfg->m_LastFormat == FOOTPRINT_FMT );

    if( cfg->m_LastLayer >= 0 && cfg->m_LastLayer < (int) m_layerCtrl->GetCount() )
        m_layerCtrl->SetSelection( cfg->m_LastLayer );
}


void BITMAP2CMP_PANEL::SaveSettings( BITMAP2CMP_SETTINGS* cfg )
{
    cfg->m_Threshold = m_sliderThreshold->GetValue();
    cfg->m_Negative = m_checkNegative->IsChecked();
    cfg->m_LastFormat = getOutputFormat();
    cfg->m_LastLayer = m_layerCtrl->GetSelection();
    cfg->m_Units = m_PixelUnit->GetSelection();
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
                if( m_Pict_Image.GetRed( x, y ) == m_Pict_Image.GetMaskRed()
                        && m_Pict_Image.GetGreen( x, y ) == m_Pict_Image.GetMaskGreen()
                        && m_Pict_Image.GetBlue( x, y ) == m_Pict_Image.GetMaskBlue() )
                {
                    m_Greyscale_Image.SetRGB( x, y, 255, 255, 255 );
                }
            }
        }
    }

    if( m_negative )
        negateGreyscaleImage();

    m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );
    m_NB_Image  = m_Greyscale_Image;
    binarize( (double)m_sliderThreshold->GetValue() / m_sliderThreshold->GetMax() );

    m_buttonExportFile->Enable( true );
    m_buttonExportClipboard->Enable( true );

    m_outputSizeX.SetOutputSizeFromInitialImageSize();
    m_UnitSizeX->ChangeValue( formatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_outputSizeY.SetOutputSizeFromInitialImageSize();
    m_UnitSizeY->ChangeValue( formatOutputSize( m_outputSizeY.GetOutputSize() ) );

    return true;
}


// return a string giving the output size, according to the selected unit
wxString BITMAP2CMP_PANEL::formatOutputSize( double aSize )
{
    wxString text;

    if( getUnitFromSelection() == EDA_UNITS::MM )
        text.Printf( wxS( "%.1f" ), aSize );
    else if( getUnitFromSelection() == EDA_UNITS::INCH )
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
        m_SizeXValue->SetLabel( wxString::Format( wxT( "%d" ), m_Pict_Bitmap.GetWidth() ) );
        m_SizeYValue->SetLabel( wxString::Format( wxT( "%d" ), m_Pict_Bitmap.GetHeight() ) );
        m_BPPValue->SetLabel( wxString::Format( wxT( "%d" ), m_Pict_Bitmap.GetDepth() ) );
    }
}


EDA_UNITS BITMAP2CMP_PANEL::getUnitFromSelection()
{
    // return the EDA_UNITS from the m_PixelUnit choice
    switch( m_PixelUnit->GetSelection() )
    {
    case 1:  return EDA_UNITS::INCH;
    case 2:  return EDA_UNITS::UNSCALED;
    case 0:
    default: return EDA_UNITS::MM;
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
            m_UnitSizeY->ChangeValue( formatOutputSize( m_outputSizeY.GetOutputSize() ) );
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
            m_UnitSizeX->ChangeValue( formatOutputSize( m_outputSizeX.GetOutputSize() ) );
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

    m_UnitSizeX->ChangeValue( formatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( formatOutputSize( m_outputSizeY.GetOutputSize() ) );
}


void BITMAP2CMP_PANEL::SetOutputSize( const IMAGE_SIZE& aSizeX, const IMAGE_SIZE& aSizeY )
{
    m_outputSizeX = aSizeX;
    m_outputSizeY = aSizeY;
    updateImageInfo();

    m_UnitSizeX->ChangeValue( formatOutputSize( m_outputSizeX.GetOutputSize() ) );
    m_UnitSizeY->ChangeValue( formatOutputSize( m_outputSizeY.GetOutputSize() ) );
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


void BITMAP2CMP_PANEL::binarize( double aThreshold )
{
    unsigned char threshold = aThreshold * 255;
    unsigned char alpha_thresh = 0.7 * threshold;

    for( int y = 0; y < m_Greyscale_Image.GetHeight(); y++ )
    {
        for( int x = 0; x < m_Greyscale_Image.GetWidth(); x++ )
        {
            unsigned char pixel = m_Greyscale_Image.GetGreen( x, y );
            unsigned char alpha = m_Greyscale_Image.HasAlpha() ? m_Greyscale_Image.GetAlpha( x, y )
                                                               : wxALPHA_OPAQUE;

            if( pixel < threshold && alpha > alpha_thresh )
                pixel = 0;
            else
                pixel = 255;

            m_NB_Image.SetRGB( x, y, pixel, pixel, pixel );
        }
    }

    m_BN_Bitmap = wxBitmap( m_NB_Image );
}


void BITMAP2CMP_PANEL::negateGreyscaleImage( )
{
    for( int y = 0; y < m_Greyscale_Image.GetHeight(); y++ )
    {
        for( int x = 0; x < m_Greyscale_Image.GetWidth(); x++ )
        {
            unsigned char pixel = m_Greyscale_Image.GetGreen( x, y );
            pixel = ~pixel;
            m_Greyscale_Image.SetRGB( x, y, pixel, pixel, pixel );
        }
    }
}


void BITMAP2CMP_PANEL::OnNegativeClicked( wxCommandEvent&  )
{
    if( m_checkNegative->GetValue() != m_negative )
    {
        negateGreyscaleImage();

        m_Greyscale_Bitmap = wxBitmap( m_Greyscale_Image );
        binarize( (double)m_sliderThreshold->GetValue() / m_sliderThreshold->GetMax() );
        m_negative = m_checkNegative->GetValue();

        Refresh();
    }
}


void BITMAP2CMP_PANEL::OnThresholdChange( wxScrollEvent& event )
{
    binarize( (double)m_sliderThreshold->GetValue() / m_sliderThreshold->GetMax() );
    Refresh();
}


void BITMAP2CMP_PANEL::OnExportToFile( wxCommandEvent& event )
{
    switch( getOutputFormat() )
    {
    case SYMBOL_FMT:
    case SYMBOL_PASTE_FMT:  m_parentFrame->ExportEeschemaFormat();     break;
    case FOOTPRINT_FMT:     m_parentFrame->ExportPcbnewFormat();       break;
    case POSTSCRIPT_FMT:    m_parentFrame->ExportPostScriptFormat();   break;
    case DRAWING_SHEET_FMT: m_parentFrame->ExportDrawingSheetFormat(); break;
    }
}


OUTPUT_FMT_ID BITMAP2CMP_PANEL::getOutputFormat()
{
    if( m_rbSymbol->GetValue() )
        return SYMBOL_FMT;
    else if( m_rbPostscript->GetValue() )
        return POSTSCRIPT_FMT;
    else if( m_rbWorksheet->GetValue() )
        return DRAWING_SHEET_FMT;
    else
        return FOOTPRINT_FMT;
}


void BITMAP2CMP_PANEL::OnExportToClipboard( wxCommandEvent& event )
{
    std::string buffer;
    OUTPUT_FMT_ID format = getOutputFormat() == SYMBOL_FMT ? SYMBOL_PASTE_FMT : getOutputFormat();
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


void BITMAP2CMP_PANEL::ExportToBuffer( std::string& aOutput, OUTPUT_FMT_ID aFormat )
{
    // Create a potrace bitmap
    potrace_bitmap_t* potrace_bitmap = bm_new( m_NB_Image.GetWidth(), m_NB_Image.GetHeight() );

    if( !potrace_bitmap )
    {
        wxMessageBox( _( "Error allocating memory for potrace bitmap" ) );
        return;
    }

    /* fill the bitmap with data */
    for( int y = 0; y < m_NB_Image.GetHeight(); y++ )
    {
        for( int x = 0; x < m_NB_Image.GetWidth(); x++ )
        {
            unsigned char pixel = m_NB_Image.GetGreen( x, y );
            BM_PUT( potrace_bitmap, x, y, pixel ? 0 : 1 );
        }
    }

    wxString layer = wxT( "F.SilkS" );

    if( aFormat == FOOTPRINT_FMT )
    {
        switch( m_layerCtrl->GetSelection() )
        {
        case 0: layer = wxT( "F.Cu" );      break;
        case 1: layer = wxT( "F.SilkS" );   break;
        case 2: layer = wxT( "F.Mask" );    break;
        case 3: layer = wxT( "Dwgs.User" ); break;
        case 4: layer = wxT( "Cmts.User" ); break;
        case 5: layer = wxT( "Eco1.User" ); break;
        case 6: layer = wxT( "Eco2.User" ); break;
        case 7: layer = wxT( "F.Fab" );     break;
        }
    }



    WX_STRING_REPORTER reporter;
    BITMAPCONV_INFO    converter( aOutput, reporter );

    converter.ConvertBitmap( potrace_bitmap, aFormat, m_outputSizeX.GetOutputDPI(),
                             m_outputSizeY.GetOutputDPI(), layer );

    if( reporter.HasMessage() )
        wxMessageBox( reporter.GetMessages(), _( "Errors" ) );
}


void BITMAP2CMP_PANEL::OnFormatChange( wxCommandEvent& event )
{
    m_layerLabel->Enable( m_rbFootprint->GetValue() );
    m_layerCtrl->Enable( m_rbFootprint->GetValue() );
}


DROP_FILE::DROP_FILE( BITMAP2CMP_PANEL* panel ) :
        m_panel( panel )
{
}


bool DROP_FILE::OnDropFiles( wxCoord x, wxCoord y, const wxArrayString& filenames )
{
    m_panel->SetFocus();

    // If a file is already loaded
    if( m_panel->GetOutputSizeX().GetOriginalSizePixels() != 0 )
    {
        wxString        cap = _( "Replace Loaded File?" );
        wxString        msg = _( "There is already a file loaded. Do you want to replace it?" );
        wxMessageDialog acceptFileDlg( m_panel, msg, cap, wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );
        int             replace = acceptFileDlg.ShowModal();

        if( replace == wxID_NO )
            return false;
    }

    std::vector<wxString> fNameVec;
    fNameVec.insert( fNameVec.begin(), filenames.begin(), filenames.end() );
    m_panel->OpenProjectFiles( fNameVec );

    return true;
}
