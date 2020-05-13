/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Seth Hillbrand <hillbrand@ucdavis.edu>
 * Copyright (C) 2014-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <sch_line.h>
#include <dialog_edit_line_style.h>
#include <dialogs/dialog_color_picker.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <sch_edit_frame.h>

const int BUTT_COLOR_MINSIZE_X = 32;
const int BUTT_COLOR_MINSIZE_Y = 20;


struct lineTypeStruct
{
    wxString             name;
    const BITMAP_OPAQUE* bitmap;
};


/*
 * Conversion map between PLOT_DASH_TYPE values and style names displayed
 */
const std::map<PLOT_DASH_TYPE, struct lineTypeStruct> lineTypeNames = {
    { PLOT_DASH_TYPE::SOLID, { _( "Solid" ), stroke_solid_xpm } },
    { PLOT_DASH_TYPE::DASH, { _( "Dashed" ), stroke_dash_xpm } },
    { PLOT_DASH_TYPE::DOT, { _( "Dotted" ), stroke_dot_xpm } },
    { PLOT_DASH_TYPE::DASHDOT, { _( "Dash-Dot" ), stroke_dashdot_xpm } },
};


DIALOG_EDIT_LINE_STYLE::DIALOG_EDIT_LINE_STYLE( SCH_EDIT_FRAME* aParent,
                                                std::deque<SCH_LINE*>& lines ) :
          DIALOG_EDIT_LINE_STYLE_BASE( aParent ),
          m_frame( aParent ),
          m_lines( lines ),
          m_width( aParent, m_staticTextWidth, m_lineWidth, m_staticWidthUnits, true )
{
    m_sdbSizerApply->SetLabel( _( "Default" ) );

    SetInitialFocus( m_lineWidth );

    for( auto& typeEntry : lineTypeNames )
    {
        m_typeCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );
    }

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_EDIT_LINE_STYLE::TransferDataToWindow()
{
    auto first_line = m_lines.front();

    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
        [&]( const SCH_LINE* r )
        {
			return r->GetPenWidth() == first_line->GetPenWidth();
		} ) )
    {
        m_width.SetValue( first_line->GetPenWidth() );
    }
    else
    {
        m_width.SetValue( INDETERMINATE_ACTION );
    }

    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
        [&]( const SCH_LINE* r )
        {
            return r->GetLineColor() == first_line->GetLineColor();
        } ) )
    {
        setColor( first_line->GetLineColor() );
    }
    else
    {
        setColor( COLOR4D::UNSPECIFIED );
    }

    if( std::all_of( m_lines.begin() + 1, m_lines.end(),
        [&]( const SCH_LINE* r )
        {
            return r->GetLineStyle() == first_line->GetLineStyle();
        } ) )
    {
        int style = static_cast<int>( first_line->GetLineStyle() );
        wxCHECK_MSG( style < (int)lineTypeNames.size(), false,
                "Line type for first line is not found in the type lookup map" );
        m_typeCombo->SetSelection( style );
    }
    else
    {
        m_typeCombo->SetSelection( wxNOT_FOUND );
    }

    return true;
}


void DIALOG_EDIT_LINE_STYLE::onColorButtonClicked( wxCommandEvent& event )
{
    COLOR4D             newColor = COLOR4D::UNSPECIFIED;
    DIALOG_COLOR_PICKER dialog( this, m_selectedColor, false );

    if( dialog.ShowModal() == wxID_OK )
        newColor = dialog.GetColor();

    if( newColor == COLOR4D::UNSPECIFIED || m_selectedColor == newColor )
        return;

    setColor( newColor );
}


void DIALOG_EDIT_LINE_STYLE::updateColorButton( COLOR4D& aColor )
{
    wxMemoryDC iconDC;


    if( aColor == COLOR4D::UNSPECIFIED )
    {
        m_colorButton->SetBitmap( KiBitmap( question_mark_xpm ) );
    }
    else
    {
        wxBitmap bitmap( std::max( m_colorButton->GetSize().x, BUTT_COLOR_MINSIZE_X ),
                std::max( m_colorButton->GetSize().y, BUTT_COLOR_MINSIZE_Y ) );

        iconDC.SelectObject( bitmap );
        iconDC.SetPen( *wxBLACK_PEN );

        wxBrush brush( aColor.ToColour() );
        iconDC.SetBrush( brush );

        // Paint the full bitmap in aColor:
        iconDC.SetBackground( brush );
        iconDC.Clear();

        m_colorButton->SetBitmap( bitmap );
    }

    m_colorButton->Refresh();

    Refresh( false );
}


void DIALOG_EDIT_LINE_STYLE::resetDefaults( wxCommandEvent& event )
{
    m_width.SetValue( 0 );
    setColor( COLOR4D::UNSPECIFIED );

    auto typeIt = lineTypeNames.find( m_lines.front()->GetDefaultStyle() );
    wxCHECK_RET( typeIt != lineTypeNames.end(),
            "Default line type not found line dialogs line type lookup map" );

    m_typeCombo->SetSelection( static_cast<int>( typeIt->first ) );
    Refresh();
}


void DIALOG_EDIT_LINE_STYLE::setColor( const COLOR4D& aColor )
{
    m_selectedColor = aColor;

    if( aColor == COLOR4D::UNSPECIFIED )
    {
        COLOR4D defaultColor = Pgm().GetSettingsManager().GetColorSettings()->GetColor(
                m_lines.front()->GetLayer() );
        updateColorButton( defaultColor );
    }
    else
        updateColorButton( m_selectedColor );
}


bool DIALOG_EDIT_LINE_STYLE::TransferDataFromWindow()
{
    for( auto& line : m_lines )
    {
        m_frame->SaveCopyInUndoList( line, UR_CHANGED );

        if( !m_width.IsIndeterminate() )
        {
            line->SetLineWidth( m_width.GetValue() );
        }

        if( m_typeCombo->GetSelection() != wxNOT_FOUND )
        {
            int selection = m_typeCombo->GetSelection();
            wxCHECK_MSG( selection < (int)lineTypeNames.size(), false,
                    "Selected line type index exceeds size of line type lookup map" );

            auto it = lineTypeNames.begin();
            std::advance( it, selection );

            line->SetLineStyle( it->first );
        }

        line->SetLineColor( m_selectedColor );

        m_frame->RefreshItem( line );
    }

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return true;
}