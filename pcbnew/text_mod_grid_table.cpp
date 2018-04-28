/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiway.h>
#include <kiway_player.h>
#include <text_mod_grid_table.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include "grid_layer_box_helpers.h"

enum
{
    MYID_SELECT_FOOTPRINT = 991,         // must be within GRID_TRICKS' enum range
    MYID_SHOW_DATASHEET
};


wxArrayString g_menuOrientations;


TEXT_MOD_GRID_TABLE::TEXT_MOD_GRID_TABLE( EDA_UNITS_T aUserUnits, PCB_BASE_FRAME* aFrame ) :
    m_userUnits( aUserUnits ),
    m_frame( aFrame )
{
    // Build the column attributes.

    m_readOnlyAttr = new wxGridCellAttr;
    m_readOnlyAttr->SetReadOnly( true );

    m_boolColAttr = new wxGridCellAttr;
    m_boolColAttr->SetRenderer( new wxGridCellBoolRenderer() );
    m_boolColAttr->SetEditor( new wxGridCellBoolEditor() );
    m_boolColAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );

    if( g_menuOrientations.IsEmpty() )
    {
        g_menuOrientations.push_back( wxT( "0 " ) + GetAbbreviatedUnitsLabel( DEGREES ) );
        g_menuOrientations.push_back( wxT( "90 " ) + GetAbbreviatedUnitsLabel( DEGREES ) );
        g_menuOrientations.push_back( wxT( "-90 " ) + GetAbbreviatedUnitsLabel( DEGREES ) );
        g_menuOrientations.push_back( wxT( "180 " ) + GetAbbreviatedUnitsLabel( DEGREES ) );
    }

    m_orientationColAttr = new wxGridCellAttr;
    m_orientationColAttr->SetEditor( new GRID_CELL_COMBOBOX( g_menuOrientations ) );

    m_layerColAttr = new wxGridCellAttr;
    m_layerColAttr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_frame ) );
    m_layerColAttr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_frame, LSET::ForbiddenTextLayers() ) );
}


TEXT_MOD_GRID_TABLE::~TEXT_MOD_GRID_TABLE()
{
    m_readOnlyAttr->DecRef();
    m_boolColAttr->DecRef();
    m_orientationColAttr->DecRef();
    m_layerColAttr->DecRef();
}


wxString TEXT_MOD_GRID_TABLE::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case TMC_TEXT:        return _( "Text Items" );
    case TMC_SHOWN:       return _( "Show" );
    case TMC_WIDTH:       return _( "Width" );
    case TMC_HEIGHT:      return _( "Height" );
    case TMC_THICKNESS:   return _( "Thickness" );
    case TMC_ITALIC:      return _( "Italic" );
    case TMC_LAYER:       return _( "Layer" );
    case TMC_ORIENTATION: return _( "Orientation" );
    case TMC_UPRIGHT:     return _( "Keep Upright" );
    case TMC_XOFFSET:     return _( "X Offset" );
    case TMC_YOFFSET:     return _( "Y Offset" );
    default:              wxFAIL; return wxEmptyString;
    }
}


wxString TEXT_MOD_GRID_TABLE::GetRowLabelValue( int aRow )
{
    switch( aRow )
    {
    case 0:   return _( "Reference" );
    case 1:   return _( "Value" );
    default:  return wxEmptyString;
    }
}


bool TEXT_MOD_GRID_TABLE::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    switch( aCol )
    {
    case TMC_TEXT:
    case TMC_WIDTH:
    case TMC_HEIGHT:
    case TMC_THICKNESS:
    case TMC_ORIENTATION:
    case TMC_XOFFSET:
    case TMC_YOFFSET:
        return aTypeName == wxGRID_VALUE_STRING;

    case TMC_SHOWN:
    case TMC_ITALIC:
    case TMC_UPRIGHT:
        return aTypeName == wxGRID_VALUE_BOOL;

    case TMC_LAYER:
        return aTypeName == wxGRID_VALUE_NUMBER;

    default:
        wxFAIL;
        return false;
    }
}


bool TEXT_MOD_GRID_TABLE::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return CanGetValueAs( aRow, aCol, aTypeName );
}


wxGridCellAttr* TEXT_MOD_GRID_TABLE::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind  )
{
    switch( aCol )
    {
    case TMC_TEXT:
    case TMC_WIDTH:
    case TMC_HEIGHT:
    case TMC_THICKNESS:
    case TMC_XOFFSET:
    case TMC_YOFFSET:
        return nullptr;

    case TMC_SHOWN:
    case TMC_ITALIC:
    case TMC_UPRIGHT:
        m_boolColAttr->IncRef();
        return m_boolColAttr;

    case TMC_LAYER:
        m_layerColAttr->IncRef();
        return m_layerColAttr;

    case TMC_ORIENTATION:
        m_orientationColAttr->IncRef();
        return m_orientationColAttr;

    default:
        wxFAIL;
        return nullptr;
    }
}


wxString TEXT_MOD_GRID_TABLE::GetValue( int aRow, int aCol )
{
    const TEXTE_MODULE& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case TMC_TEXT:
        return text.GetText();

    case TMC_WIDTH:
        return StringFromValue( m_userUnits, text.GetTextWidth(), true, true );

    case TMC_HEIGHT:
        return StringFromValue( m_userUnits, text.GetTextHeight(), true, true );

    case TMC_THICKNESS:
        return StringFromValue( m_userUnits, text.GetThickness(), true, true );

    case TMC_LAYER:
        return text.GetLayerName();

    case TMC_ORIENTATION:
        return StringFromValue( DEGREES, (int) NormalizeAnglePos( text.GetTextAngle() ), true );

    case TMC_XOFFSET:
        return StringFromValue( m_userUnits, text.GetPos0().x, true );

    case TMC_YOFFSET:
        return StringFromValue( m_userUnits, text.GetPos0().y, true );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


bool TEXT_MOD_GRID_TABLE::GetValueAsBool( int aRow, int aCol )
{
    TEXTE_MODULE& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case TMC_SHOWN:    return text.IsVisible();
    case TMC_ITALIC:   return text.IsItalic();
    case TMC_UPRIGHT:  return text.IsKeepUpright();
    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


long TEXT_MOD_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    TEXTE_MODULE& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case TMC_LAYER:    return text.GetLayer();
    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        return 0;
    }
}


void TEXT_MOD_GRID_TABLE::SetValue( int aRow, int aCol, const wxString &aValue )
{
    TEXTE_MODULE& text = this->at( (size_t) aRow );
    wxPoint       pos;

    switch( aCol )
    {
    case TMC_TEXT:
        text.SetText( aValue );
        break;

    case TMC_WIDTH:
        text.SetTextWidth( ValueFromString( m_userUnits, aValue, true ) );
        break;

    case TMC_HEIGHT:
        text.SetTextHeight( ValueFromString( m_userUnits, aValue, true ) );
        break;

    case TMC_THICKNESS:
        text.SetThickness( ValueFromString( m_userUnits, aValue, true ) );
        break;

    case TMC_ORIENTATION:
        text.SetTextAngle( DoubleValueFromString( DEGREES, aValue ) );
        text.SetDrawCoord();
        break;

    case TMC_XOFFSET:
    case TMC_YOFFSET:
        pos = text.GetPos0();

        if( aCol == TMC_XOFFSET )
            pos.x = ValueFromString( m_userUnits, aValue );
        else
            pos.y = ValueFromString( m_userUnits, aValue );

        text.SetPos0( pos );
        text.SetDrawCoord();
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
        break;
    }

    GetView()->Refresh();
}


void TEXT_MOD_GRID_TABLE::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    TEXTE_MODULE& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case TMC_SHOWN:
        text.SetVisible( aValue );
        break;

    case TMC_ITALIC:
        text.SetItalic( aValue );
        break;

    case TMC_UPRIGHT:text.SetKeepUpright( aValue );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        break;
    }
}


void TEXT_MOD_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    TEXTE_MODULE& text = this->at( (size_t) aRow );

    switch( aCol )
    {
    case TMC_LAYER:
        text.SetLayer( ToLAYER_ID( (int) aValue ) );
        text.SetMirrored( IsBackLayer( text.GetLayer() ) );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        break;
    }
}

