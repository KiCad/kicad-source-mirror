/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <project.h>
#include <fp_text_grid_table.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_combobox.h>
#include <trigo.h>
#include <pcb_base_frame.h>
#include <footprint.h>
#include "grid_layer_box_helpers.h"
#include <widgets/grid_text_button_helpers.h>
#include <widgets/grid_text_helpers.h>

enum
{
    MYID_SELECT_FOOTPRINT = 991,         // must be within GRID_TRICKS' enum range
    MYID_SHOW_DATASHEET
};


wxArrayString g_menuOrientations;


FP_TEXT_GRID_TABLE::FP_TEXT_GRID_TABLE( PCB_BASE_FRAME* aFrame, DIALOG_SHIM* aDialog ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_fieldNameValidator( FIELD_NAME ),
        m_referenceValidator( REFERENCE_FIELD ),
        m_valueValidator( VALUE_FIELD ),
        m_urlValidator( FIELD_VALUE ),
        m_nonUrlValidator( FIELD_VALUE )
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
        g_menuOrientations.push_back( "0" + EDA_UNIT_UTILS::GetText( EDA_UNITS::DEGREES ) );
        g_menuOrientations.push_back( "90" + EDA_UNIT_UTILS::GetText( EDA_UNITS::DEGREES ) );
        g_menuOrientations.push_back( "-90" + EDA_UNIT_UTILS::GetText( EDA_UNITS::DEGREES ) );
        g_menuOrientations.push_back( "180" + EDA_UNIT_UTILS::GetText( EDA_UNITS::DEGREES ) );
    }

    m_orientationColAttr = new wxGridCellAttr;
    m_orientationColAttr->SetEditor( new GRID_CELL_COMBOBOX( g_menuOrientations ) );

    m_layerColAttr = new wxGridCellAttr;
    m_layerColAttr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_frame ) );
    m_layerColAttr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_frame, {} ) );

    m_referenceAttr = new wxGridCellAttr;
    GRID_CELL_TEXT_EDITOR* referenceEditor = new GRID_CELL_TEXT_EDITOR();
    referenceEditor->SetValidator( m_referenceValidator );
    m_referenceAttr->SetEditor( referenceEditor );

    m_valueAttr = new wxGridCellAttr;
    GRID_CELL_TEXT_EDITOR* valueEditor = new GRID_CELL_TEXT_EDITOR();
    valueEditor->SetValidator( m_valueValidator );
    m_valueAttr->SetEditor( valueEditor );

    m_footprintAttr = new wxGridCellAttr;

    if( !m_frame->IsType(  FRAME_FOOTPRINT_EDITOR ) )
        m_footprintAttr->SetReadOnly( true );

    GRID_CELL_FPID_EDITOR* fpIdEditor = new GRID_CELL_FPID_EDITOR( m_dialog, "" );
    fpIdEditor->SetValidator( m_nonUrlValidator );
    m_footprintAttr->SetEditor( fpIdEditor );

    m_urlAttr = new wxGridCellAttr;
    GRID_CELL_URL_EDITOR* urlEditor = new GRID_CELL_URL_EDITOR( m_dialog );
    urlEditor->SetValidator( m_urlValidator );
    m_urlAttr->SetEditor( urlEditor );

    m_eval = std::make_unique<NUMERIC_EVALUATOR>( m_frame->GetUserUnits() );

    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &FP_TEXT_GRID_TABLE::onUnitsChanged, this );
}


FP_TEXT_GRID_TABLE::~FP_TEXT_GRID_TABLE()
{
    m_readOnlyAttr->DecRef();
    m_boolColAttr->DecRef();
    m_orientationColAttr->DecRef();
    m_layerColAttr->DecRef();
    m_referenceAttr->DecRef();
    m_valueAttr->DecRef();
    m_footprintAttr->DecRef();
    m_urlAttr->DecRef();

    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &FP_TEXT_GRID_TABLE::onUnitsChanged, this );
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
    case FPT_NAME:        return _( "Name" );
    case FPT_VALUE:       return _( "Value" );
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
    case FPT_KNOCKOUT:    return _( "Knockout" );
    default:              wxFAIL; return wxEmptyString;
    }
}


bool FP_TEXT_GRID_TABLE::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    switch( aCol )
    {
    case FPT_NAME:
    case FPT_VALUE:
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
    case FPT_KNOCKOUT:
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
    case FPT_NAME:
        if( aRow < MANDATORY_FIELDS )
        {
            m_readOnlyAttr->IncRef();
            return m_readOnlyAttr;
        }

        return nullptr;

    case FPT_VALUE:
        if( aRow == REFERENCE_FIELD )
        {
            m_referenceAttr->IncRef();
            return m_referenceAttr;
        }
        else if( aRow == VALUE_FIELD )
        {
            m_valueAttr->IncRef();
            return m_valueAttr;
        }
        else if( aRow == FOOTPRINT_FIELD )
        {
            m_footprintAttr->IncRef();
            return m_footprintAttr;
        }
        else if( aRow == DATASHEET_FIELD )
        {
            m_urlAttr->IncRef();
            return m_urlAttr;
        }

        return nullptr;

    case FPT_WIDTH:
    case FPT_HEIGHT:
    case FPT_THICKNESS:
    case FPT_XOFFSET:
    case FPT_YOFFSET:
        return nullptr;

    case FPT_SHOWN:
    case FPT_ITALIC:
    case FPT_UPRIGHT:
    case FPT_KNOCKOUT:
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
    wxGrid*         grid = GetView();
    const PCB_FIELD& field = this->at( (size_t) aRow );

    if( grid->GetGridCursorRow() == aRow && grid->GetGridCursorCol() == aCol
            && grid->IsCellEditControlShown() )
    {
        auto it = m_evalOriginal.find( { aRow, aCol } );

        if( it != m_evalOriginal.end() )
            return it->second;
    }

    switch( aCol )
    {
    case FPT_NAME: return field.GetName();

    case FPT_VALUE: return field.GetText();

    case FPT_WIDTH: return m_frame->StringFromValue( field.GetTextWidth(), true );

    case FPT_HEIGHT: return m_frame->StringFromValue( field.GetTextHeight(), true );

    case FPT_THICKNESS: return m_frame->StringFromValue( field.GetTextThickness(), true );

    case FPT_LAYER: return field.GetLayerName();

    case FPT_ORIENTATION:
    {
        EDA_ANGLE angle = field.GetTextAngle() - field.GetParentFootprint()->GetOrientation();
        return m_frame->StringFromValue( angle, true );
    }

    case FPT_XOFFSET: return m_frame->StringFromValue( field.GetFPRelativePosition().x, true );

    case FPT_YOFFSET: return m_frame->StringFromValue( field.GetFPRelativePosition().y, true );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


bool FP_TEXT_GRID_TABLE::GetValueAsBool( int aRow, int aCol )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_SHOWN:    return field.IsVisible();
    case FPT_ITALIC:   return field.IsItalic();
    case FPT_UPRIGHT:  return field.IsKeepUpright();
    case FPT_KNOCKOUT: return field.IsKnockout();

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


long FP_TEXT_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_LAYER: return field.GetLayer();

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        return 0;
    }
}


void FP_TEXT_GRID_TABLE::SetValue( int aRow, int aCol, const wxString &aValue )
{
    PCB_FIELD& field = this->at( (size_t) aRow );
    VECTOR2I  pos;
    wxString  value = aValue;

    switch( aCol )
    {
    case FPT_WIDTH:
    case FPT_HEIGHT:
    case FPT_THICKNESS:
    case FPT_XOFFSET:
    case FPT_YOFFSET:
        m_eval->SetDefaultUnits( m_frame->GetUserUnits() );

        if( m_eval->Process( value ) )
        {
            m_evalOriginal[ { aRow, aCol } ] = value;
            value = m_eval->Result();
        }

        break;

    default:
        break;
    }

    switch( aCol )
    {
    case FPT_NAME: field.SetName( value ); break;

    case FPT_VALUE: field.SetText( value ); break;

    case FPT_WIDTH: field.SetTextWidth( m_frame->ValueFromString( value ) ); break;

    case FPT_HEIGHT: field.SetTextHeight( m_frame->ValueFromString( value ) ); break;

    case FPT_THICKNESS: field.SetTextThickness( m_frame->ValueFromString( value ) ); break;

    case FPT_ORIENTATION:
        field.SetTextAngle( m_frame->AngleValueFromString( value )
                            + field.GetParentFootprint()->GetOrientation() );
        break;

    case FPT_XOFFSET:
    case FPT_YOFFSET:
        pos = field.GetFPRelativePosition();

        if( aCol == FPT_XOFFSET )
            pos.x = m_frame->ValueFromString( value );
        else
            pos.y = m_frame->ValueFromString( value );

        field.SetFPRelativePosition( pos );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
        break;
    }

    GetView()->Refresh();
}


void FP_TEXT_GRID_TABLE::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_SHOWN: field.SetVisible( aValue ); break;

    case FPT_ITALIC: field.SetItalic( aValue ); break;

    case FPT_UPRIGHT: field.SetKeepUpright( aValue ); break;

    case FPT_KNOCKOUT: field.SetIsKnockout( aValue ); break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        break;
    }
}


void FP_TEXT_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FPT_LAYER:
        field.SetLayer( ToLAYER_ID( (int) aValue ) );
        field.SetMirrored( IsBackLayer( field.GetLayer() ) );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        break;
    }
}

