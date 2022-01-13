/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiway_player.h>
#include <fp_text_grid_table.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <trigo.h>
#include <pcb_base_frame.h>
#include "grid_layer_box_helpers.h"

enum
{
    MYID_SELECT_FOOTPRINT = 991,         // must be within GRID_TRICKS' enum range
    MYID_SHOW_DATASHEET
};


wxArrayString g_menuOrientations;


FP_TEXT_GRID_TABLE::FP_TEXT_GRID_TABLE( PCB_BASE_FRAME* aFrame ) :
        m_frame( aFrame )
{
    // Build the column attributes.

    m_readOnlyAttr = new wxGridCellAttr;
    m_readOnlyAttr->SetReadOnly( true );

    m_boolColAttr = new wxGridCellAttr;
    m_boolColAttr->SetRenderer( new wxGridCellBoolRenderer() );
    m_boolColAttr->SetEditor( new wxGridCellBoolEditor() );
    m_boolColAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

    if( g_menuOrientations.IsEmpty() )
    {
        g_menuOrientations.push_back( "0" + GetAbbreviatedUnitsLabel( EDA_UNITS::DEGREES ) );
        g_menuOrientations.push_back( "90" + GetAbbreviatedUnitsLabel( EDA_UNITS::DEGREES ) );
        g_menuOrientations.push_back( "-90" + GetAbbreviatedUnitsLabel( EDA_UNITS::DEGREES ) );
        g_menuOrientations.push_back( "180" + GetAbbreviatedUnitsLabel( EDA_UNITS::DEGREES ) );
    }

    m_orientationColAttr = new wxGridCellAttr;
    m_orientationColAttr->SetEditor( new GRID_CELL_COMBOBOX( g_menuOrientations ) );

    m_layerColAttr = new wxGridCellAttr;
    m_layerColAttr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_frame ) );
    m_layerColAttr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_frame, {} ) );

    m_frame->Bind( UNITS_CHANGED, &FP_TEXT_GRID_TABLE::onUnitsChanged, this );
}


FP_TEXT_GRID_TABLE::~FP_TEXT_GRID_TABLE()
{
    m_readOnlyAttr->DecRef();
    m_boolColAttr->DecRef();
    m_orientationColAttr->DecRef();
    m_layerColAttr->DecRef();

    m_frame->Unbind( UNITS_CHANGED, &FP_TEXT_GRID_TABLE::onUnitsChanged, this );
}


void FP_TEXT_GRID_TABLE::onUnitsChanged( wxCommandEvent& aEvent )
{
    if( GetView() )
        GetView()->ForceRefresh();

    aEvent.Skip();
}


wxString FP_TEXT_GRID_TABLE::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case FPT_TEXT:        return _( "Text Items" );
    case FPT_SHOWN:       return _( "Show" );
    case FPT_WIDTH:       return _( "Width" );
    case FPT_HEIGHT:      return _( "Height" );
    case FPT_THICKNESS:   return _( "Thickness" );
    case FPT_ITALIC:      return _( "Italic" );
    case FPT_LAYER:       return _( "Layer" );
    case FPT_ORIENTATION: return _( "Orientation" );
    case FPT_UPRIGHT:     return _( "Keep Upright" );
    case FPT_XOFFSET:     return _( "X Offset" );
    case FPT_YOFFSET:     return _( "Y Offset" );
    default:              wxFAIL; return wxEmptyString;
    }
}


wxString FP_TEXT_GRID_TABLE::GetRowLabelValue( int aRow )
{
    switch( aRow )
    {
    case 0:   return _( "Reference designator" );
    case 1:   return _( "Value" );
    default:  return wxEmptyString;
    }
}


bool FP_TEXT_GRID_TABLE::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    switch( aCol )
    {
    case FPT_TEXT:
    case FPT_WIDTH:
    case FPT_HEIGHT:
    case FPT_THICKNESS:
    case FPT_ORIENTATION:
    case FPT_XOFFSET:
    case FPT_YOFFSET:
        return aTypeName == wxGRID_VALUE_STRING;

    case FPT_SHOWN:
    case FPT_ITALIC:
    case FPT_UPRIGHT:
        return aTypeName == wxGRID_VALUE_BOOL;

    case FPT_LAYER:
        return aTypeName == wxGRID_VALUE_NUMBER;

    default:
        wxFAIL;
        return false;
    }
}


bool FP_TEXT_GRID_TABLE::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return CanGetValueAs( aRow, aCol, aTypeName );
}


wxGridCellAttr* FP_TEXT_GRID_TABLE::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind  )
{
    switch( aCol )
    {
    case FPT_TEXT:
    case FPT_WIDTH:
    case FPT_HEIGHT:
    case FPT_THICKNESS:
    case FPT_XOFFSET:
    case FPT_YOFFSET:
        return nullptr;

    case FPT_SHOWN:
    case FPT_ITALIC:
    case FPT_UPRIGHT:
        m_boolColAttr->IncRef();
        return m_boolColAttr;

    case FPT_LAYER:
        m_layerColAttr->IncRef();
        return m_layerColAttr;

    case FPT_ORIENTATION:
        m_orientationColAttr->IncRef();
        return m_orientationColAttr;

    default:
        wxFAIL;
        return nullptr;
    }
}


wxString FP_TEXT_GRID_TABLE::GetValue( int aRow, int aCol )
{
    const FP_TEXT& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_TEXT:
        return text.GetText();

    case FPT_WIDTH:
        return StringFromValue( m_frame->GetUserUnits(), text.GetTextWidth(), true );

    case FPT_HEIGHT:
        return StringFromValue( m_frame->GetUserUnits(), text.GetTextHeight(), true );

    case FPT_THICKNESS:
        return StringFromValue( m_frame->GetUserUnits(), text.GetTextThickness(), true );

    case FPT_LAYER:
        return text.GetLayerName();

    case FPT_ORIENTATION:
        return StringFromValue( EDA_UNITS::DEGREES,
                                NormalizeAnglePos( text.GetTextAngle().AsTenthsOfADegree() ),
                                true );

    case FPT_XOFFSET:
        return StringFromValue( m_frame->GetUserUnits(), text.GetPos0().x, true );

    case FPT_YOFFSET:
        return StringFromValue( m_frame->GetUserUnits(), text.GetPos0().y, true );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


bool FP_TEXT_GRID_TABLE::GetValueAsBool( int aRow, int aCol )
{
    FP_TEXT& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_SHOWN:    return text.IsVisible();
    case FPT_ITALIC:   return text.IsItalic();
    case FPT_UPRIGHT:  return text.IsKeepUpright();

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


long FP_TEXT_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    FP_TEXT& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_LAYER:    return text.GetLayer();

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        return 0;
    }
}


void FP_TEXT_GRID_TABLE::SetValue( int aRow, int aCol, const wxString &aValue )
{
    FP_TEXT& text = this->at( (size_t) aRow );
    VECTOR2I pos;

    switch( aCol )
    {
    case FPT_TEXT:
        text.SetText( aValue );
        break;

    case FPT_WIDTH:
        text.SetTextWidth( ValueFromString( m_frame->GetUserUnits(), aValue ) );
        break;

    case FPT_HEIGHT:
        text.SetTextHeight( ValueFromString( m_frame->GetUserUnits(), aValue ) );
        break;

    case FPT_THICKNESS:text.SetTextThickness( ValueFromString( m_frame->GetUserUnits(), aValue ) );
        break;

    case FPT_ORIENTATION:
        text.SetTextAngle( EDA_ANGLE( DoubleValueFromString( EDA_UNITS::UNSCALED, aValue ),
                                      DEGREES_T ) );
        text.SetDrawCoord();
        break;

    case FPT_XOFFSET:
    case FPT_YOFFSET:
        pos = text.GetPos0();

        if( aCol == FPT_XOFFSET )
            pos.x = ValueFromString( m_frame->GetUserUnits(), aValue );
        else
            pos.y = ValueFromString( m_frame->GetUserUnits(), aValue );

        text.SetPos0( pos );
        text.SetDrawCoord();
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
        break;
    }

    GetView()->Refresh();
}


void FP_TEXT_GRID_TABLE::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    FP_TEXT& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_SHOWN:
        text.SetVisible( aValue );
        break;

    case FPT_ITALIC:
        text.SetItalic( aValue );
        break;

    case FPT_UPRIGHT:text.SetKeepUpright( aValue );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        break;
    }
}


void FP_TEXT_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    FP_TEXT& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_LAYER:
        text.SetLayer( ToLAYER_ID( (int) aValue ) );
        text.SetMirrored( IsBackLayer( text.GetLayer() ) );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        break;
    }
}

