/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2018 jean-pierre.charras
 * Copyright (C) 2011-2020 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <wx/dcclient.h>
#include <confirm.h>
#include <bitmap_base.h>

#include <dialogs/dialog_image_editor.h>

#include <algorithm>


DIALOG_IMAGE_EDITOR::DIALOG_IMAGE_EDITOR( wxWindow* aParent, BITMAP_BASE* aItem )
    : DIALOG_IMAGE_EDITOR_BASE( aParent )
{
    m_workingImage = new BITMAP_BASE( *aItem );
    wxString msg;
    msg.Printf( wxT( "%f" ), m_workingImage->GetScale() );
    m_textCtrlScale->SetValue( msg );

    finishDialogSettings();
    m_sdbSizerOK->SetDefault();
}


void DIALOG_IMAGE_EDITOR::OnGreyScaleConvert( wxCommandEvent& event )
{
    wxImage& image = *m_workingImage->GetImageData();
    image = image.ConvertToGreyscale();
    m_workingImage->RebuildBitmap();
    m_panelDraw->Refresh();
}


/*
 * Test params values correctness
 * Currently scale value must give an actual image > MIN_SIZE pixels (mandatory to be able to
 * see the image) and < MAX_SIZE pixels (if bigger, a confirmation will be asked)
 * Note: The image definition is 300ppi in drawing routines.
 */
bool DIALOG_IMAGE_EDITOR::CheckValues()
{
    #define MIN_SIZE 15     // Min size in pixels after scaling (50 mils)
    #define MAX_SIZE 6000   // Max size in pixels after scaling (20 inches)
    double tmp;
    wxString msg = m_textCtrlScale->GetValue();

    // Test number correctness
    if( !msg.ToDouble( &tmp ) || tmp < 0.0 )
    {
        wxMessageBox( _( "Incorrect scale number" ) );
        return false;
    }

    // Test value correctness
    wxSize psize = m_workingImage->GetSizePixels();
    int size_min = (int)std::min( (psize.x * tmp), (psize.y * tmp) );

    if( size_min < MIN_SIZE )   // if the size is too small, the image will be hard to locate
    {
        wxMessageBox( wxString::Format( _( "This scale results in an image which is too small "
                                           "(%.2f mm or %.1f mil)." ),
                                        25.4 / 300 * size_min, 1000.0/300.0 * size_min ) );
        return false;
    }

    int size_max = (int)std::max( (psize.x * tmp), (psize.y * tmp) );

    if( size_max > MAX_SIZE )
    {
        // the actual size is 25.4/300 * size_max in mm
        if( !IsOK( this, wxString::Format( _( "This scale results in an image which is very large "
                                              "(%.1f mm or %.2f in). Are you sure?" ),
                                           25.4 / 300 * size_max, size_max /300.0 ) ) )
        {
            return false;
        }
    }

    return true;
}


bool DIALOG_IMAGE_EDITOR::TransferDataFromWindow()
{
    return CheckValues();
}


void DIALOG_IMAGE_EDITOR::OnRedrawPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelDraw );
    wxSize size = m_panelDraw->GetClientSize();
    dc.SetDeviceOrigin( size.x/2, size.y/2 );

    double scale = 1.0 / m_workingImage->GetScalingFactor();
    dc.SetUserScale( scale, scale );
    m_workingImage->DrawBitmap( &dc, wxPoint( 0, 0 ) );
}


void DIALOG_IMAGE_EDITOR::TransferToImage( BITMAP_BASE* aItem )
{
    wxString msg = m_textCtrlScale->GetValue();
    double scale = 1.0;
    msg.ToDouble( &scale );
    m_workingImage->SetScale( scale );
    aItem->ImportData( m_workingImage );
}
