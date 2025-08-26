/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2018 jean-pierre.charras
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

#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <bitmap_base.h>
#include <pcb_base_edit_frame.h>
#include <tool/actions.h>
#include <confirm.h>
#include <units_provider.h>
#include <dialogs/panel_image_editor.h>

#include <algorithm>


PANEL_IMAGE_EDITOR::PANEL_IMAGE_EDITOR( UNITS_PROVIDER* aUnitsProvider, wxWindow* aParent, const BITMAP_BASE& aItem ) :
        PANEL_IMAGE_EDITOR_BASE( aParent ),
        m_scale( aUnitsProvider, aParent, m_staticTextScale, m_textCtrlScale, nullptr ),
        m_workingImage( std::make_unique<BITMAP_BASE>( aItem ) )
{
    m_scale.SetUnits( EDA_UNITS::UNSCALED );
}


bool PANEL_IMAGE_EDITOR::TransferDataToWindow()
{
    m_scale.SetDoubleValue( m_workingImage->GetScale() );

    m_stPPI_Value->SetLabel( wxString::Format( wxT( "%d" ), m_workingImage->GetPPI() ) );

    return true;
}


void PANEL_IMAGE_EDITOR::OnGreyScaleConvert( wxCommandEvent& event )
{
    m_workingImage->ConvertToGreyscale();
    m_panelDraw->Refresh();
}


/*
 * Test params values correctness
 * Currently scale value must give an actual image > MIN_SIZE pixels (mandatory to be able to
 * see the image) and < MAX_SIZE pixels (if bigger, a confirmation will be asked)
 * Note: The image definition is 300ppi in drawing routines.
 */
bool PANEL_IMAGE_EDITOR::CheckValues()
{
    wxWindow* host = wxGetTopLevelParent( this );

#define MIN_SIZE 15   // Min size in pixels after scaling (50 mils)
#define MAX_SIZE 6000 // Max size in pixels after scaling (20 inches)
    double tmp = m_scale.GetDoubleValue();

    // Test number correctness
    if( tmp < 0.0 )
    {
        DisplayErrorMessage( host, _( "Scale must be a positive number." ) );
        return false;
    }

    // Test value correctness
    VECTOR2I psize = m_workingImage->GetSizePixels();
    int      size_min = (int) std::min( ( psize.x * tmp ), ( psize.y * tmp ) );

    if( size_min < MIN_SIZE ) // if the size is too small, the image will be hard to locate
    {
        DisplayErrorMessage( host, wxString::Format( _( "This scale results in an image which is too small "
                                                        "(%.2f mm or %.1f mil)." ),
                                                     25.4 / 300 * size_min,
                                                     1000.0 / 300.0 * size_min ) );
        return false;
    }

    int size_max = (int) std::max( ( psize.x * tmp ), ( psize.y * tmp ) );

    if( size_max > MAX_SIZE )
    {
        // the actual size is 25.4/300 * size_max in mm
        if( !IsOK( host, wxString::Format( _( "This scale results in an image which is very large "
                                              "(%.1f mm or %.2f in). Are you sure?" ),
                                           25.4 / 300 * size_max,
                                           size_max / 300.0 ) ) )
        {
            return false;
        }
    }

    return true;
}


bool PANEL_IMAGE_EDITOR::TransferDataFromWindow()
{
    return CheckValues();
}


double PANEL_IMAGE_EDITOR::GetScale() const
{
    return m_scale.GetDoubleValue();
}


void PANEL_IMAGE_EDITOR::SetScale( double aScale )
{
    m_scale.ChangeDoubleValue( aScale );
    m_workingImage->SetScale( aScale );
    m_panelDraw->Refresh();
}


VECTOR2I PANEL_IMAGE_EDITOR::GetImageSize() const
{
    return m_workingImage->GetSize();
}


void PANEL_IMAGE_EDITOR::OnRedrawPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelDraw );
    wxSize    display_size = m_panelDraw->GetClientSize();

    double img_scale = 1.0 / m_workingImage->GetScalingFactor();
    VECTOR2I img_size_pixels = m_workingImage->GetSizePixels();

    // Adjust the display scale to use the full available display area
    double scale_X = (double)display_size.x/img_size_pixels.x;
    double scale_Y = (double)display_size.y/img_size_pixels.y;

    double display_scale = img_scale * std::min( scale_X, scale_Y );

    dc.SetUserScale( display_scale, display_scale );
    m_workingImage->DrawBitmap( &dc, VECTOR2I( m_workingImage->GetSize()/2 ) );
}


void PANEL_IMAGE_EDITOR::TransferToImage( BITMAP_BASE& aItem )
{
    wxString msg = m_textCtrlScale->GetValue();
    double   scale = 1.0;
    msg.ToDouble( &scale );
    m_workingImage->SetScale( scale );
    aItem.ImportData( *m_workingImage );
}
