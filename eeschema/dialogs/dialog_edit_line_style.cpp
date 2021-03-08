/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Seth Hillbrand <hillbrand@ucdavis.edu>
 * Copyright (C) 2014-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/color_swatch.h>


struct lineTypeStruct
{
    wxString      name;
    const BITMAPS bitmap;
};


/*
 * Conversion map between PLOT_DASH_TYPE values and style names displayed
 */
const std::map<PLOT_DASH_TYPE, struct lineTypeStruct> lineTypeNames = {
    { PLOT_DASH_TYPE::SOLID, { _( "Solid" ), BITMAPS::stroke_solid } },
    { PLOT_DASH_TYPE::DASH, { _( "Dashed" ), BITMAPS::stroke_dash } },
    { PLOT_DASH_TYPE::DOT, { _( "Dotted" ), BITMAPS::stroke_dot } },
    { PLOT_DASH_TYPE::DASHDOT, { _( "Dash-Dot" ), BITMAPS::stroke_dashdot } },
};


#define DEFAULT_STYLE _( "Default" )
#define INDETERMINATE_STYLE _( "Leave unchanged" )


DIALOG_EDIT_LINE_STYLE::DIALOG_EDIT_LINE_STYLE( SCH_EDIT_FRAME* aParent,
                                                std::deque<SCH_ITEM*>& strokeItems ) :
          DIALOG_EDIT_LINE_STYLE_BASE( aParent ),
          m_frame( aParent ),
          m_strokeItems( strokeItems ),
          m_width( aParent, m_staticTextWidth, m_lineWidth, m_staticWidthUnits, true )
{
    m_sdbSizerApply->SetLabel( _( "Default" ) );

    m_colorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    SetInitialFocus( m_lineWidth );

    for( auto& typeEntry : lineTypeNames )
        m_typeCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

    m_typeCombo->Append( DEFAULT_STYLE );

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_EDIT_LINE_STYLE::TransferDataToWindow()
{
    auto first_stroke_item = m_strokeItems.front();

    if( std::all_of( m_strokeItems.begin() + 1, m_strokeItems.end(),
            [&]( const SCH_ITEM* r )
            {
                return r->GetPenWidth() == first_stroke_item->GetPenWidth();
            } ) )
    {
        m_width.SetValue( first_stroke_item->GetPenWidth() );
    }
    else
    {
        m_width.SetValue( INDETERMINATE_ACTION );
    }

    if( std::all_of( m_strokeItems.begin() + 1, m_strokeItems.end(),
            [&]( const SCH_ITEM* r )
            {
                return r->GetStroke().GetColor() == first_stroke_item->GetStroke().GetColor();
            } ) )
    {
        m_colorSwatch->SetSwatchColor( first_stroke_item->GetStroke().GetColor(), false );
    }
    else
    {
        m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );
    }

    if( std::all_of( m_strokeItems.begin() + 1, m_strokeItems.end(),
            [&]( const SCH_ITEM* r )
            {
                return r->GetStroke().GetPlotStyle() == first_stroke_item->GetStroke().GetPlotStyle();
            } ) )
    {
        int style = static_cast<int>( first_stroke_item->GetStroke().GetPlotStyle() );

        if( style == -1 )
            m_typeCombo->SetStringSelection( DEFAULT_STYLE );
        else if( style < (int) lineTypeNames.size() )
            m_typeCombo->SetSelection( style );
        else
            wxFAIL_MSG( "Line type not found in the type lookup map" );
    }
    else
    {
        m_typeCombo->Append( INDETERMINATE_STYLE );
        m_typeCombo->SetStringSelection( INDETERMINATE_STYLE );
    }

    return true;
}


void DIALOG_EDIT_LINE_STYLE::resetDefaults( wxCommandEvent& event )
{
    m_width.SetValue( 0 );
    m_colorSwatch->SetSwatchColor( COLOR4D::UNSPECIFIED, false );

    m_typeCombo->SetStringSelection( DEFAULT_STYLE );

    Refresh();
}


bool DIALOG_EDIT_LINE_STYLE::TransferDataFromWindow()
{
    PICKED_ITEMS_LIST pickedItems;
    STROKE_PARAMS stroke;

    for( SCH_ITEM* strokeItem : m_strokeItems )
        pickedItems.PushItem( ITEM_PICKER( m_frame->GetScreen(), strokeItem, UNDO_REDO::CHANGED ) );

    m_frame->SaveCopyInUndoList( pickedItems, UNDO_REDO::CHANGED, false );

    for( SCH_ITEM* strokeItem : m_strokeItems )
    {
        stroke = strokeItem->GetStroke();

        if( !m_width.IsIndeterminate() )
            stroke.SetWidth( m_width.GetValue() );

        auto it = lineTypeNames.begin();
        std::advance( it, m_typeCombo->GetSelection() );

        if( it == lineTypeNames.end() )
            stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
        else
            stroke.SetPlotStyle( it->first );

        stroke.SetColor( m_colorSwatch->GetSwatchColor() );

        strokeItem->SetStroke( stroke );
        m_frame->UpdateItem( strokeItem );
    }

    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();

    return true;
}
