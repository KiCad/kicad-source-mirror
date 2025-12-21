/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <board.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <kiway.h>
#include <kiway_player.h>
#include <pcb_fields_grid_table.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <trigo.h>
#include <widgets/grid_combobox.h>

#include "grid_layer_box_helpers.h"
#include <widgets/grid_text_button_helpers.h>
#include <widgets/grid_text_helpers.h>

enum
{
    MYID_SELECT_FOOTPRINT = 991,         // must be within GRID_TRICKS' enum range
    MYID_SHOW_DATASHEET
};


wxArrayString g_menuOrientations;


PCB_FIELDS_GRID_TABLE::PCB_FIELDS_GRID_TABLE( PCB_BASE_FRAME* aFrame, DIALOG_SHIM* aDialog,
                                              std::vector<EMBEDDED_FILES*> aFilesStack ) :
        m_frame( aFrame ),
        m_dialog( aDialog ),
        m_fieldNameValidator( FIELD_T::USER ),
        m_referenceValidator( FIELD_T::REFERENCE ),
        m_valueValidator( FIELD_T::VALUE ),
        m_urlValidator( FIELD_T::USER ),
        m_nonUrlValidator( FIELD_T::USER )
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

    m_urlAttr = new wxGridCellAttr;
    GRID_CELL_URL_EDITOR* urlEditor = new GRID_CELL_URL_EDITOR( m_dialog, nullptr, aFilesStack );
    urlEditor->SetValidator( m_urlValidator );
    m_urlAttr->SetEditor( urlEditor );

    m_eval = std::make_unique<NUMERIC_EVALUATOR>( m_frame->GetUserUnits() );

    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &PCB_FIELDS_GRID_TABLE::onUnitsChanged, this );
}


PCB_FIELDS_GRID_TABLE::~PCB_FIELDS_GRID_TABLE()
{
    m_readOnlyAttr->DecRef();
    m_boolColAttr->DecRef();
    m_orientationColAttr->DecRef();
    m_layerColAttr->DecRef();
    m_referenceAttr->DecRef();
    m_valueAttr->DecRef();
    m_urlAttr->DecRef();

    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &PCB_FIELDS_GRID_TABLE::onUnitsChanged, this );
}


int PCB_FIELDS_GRID_TABLE::GetMandatoryRowCount() const
{
    int mandatoryRows = 0;

    for( const PCB_FIELD& field : *this )
    {
        if( field.IsMandatory() )
            mandatoryRows++;
    }

    return mandatoryRows;
}


void PCB_FIELDS_GRID_TABLE::onUnitsChanged( wxCommandEvent& aEvent )
{
    if( GetView() )
        GetView()->ForceRefresh();

    aEvent.Skip();
}


wxString PCB_FIELDS_GRID_TABLE::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case PFC_NAME:        return _( "Name" );
    case PFC_VALUE:       return _( "Value" );
    case PFC_SHOWN:       return _( "Show" );
    case PFC_WIDTH:       return _( "Width" );
    case PFC_HEIGHT:      return _( "Height" );
    case PFC_THICKNESS:   return _( "Thickness" );
    case PFC_ITALIC:      return _( "Italic" );
    case PFC_LAYER:       return _( "Layer" );
    case PFC_ORIENTATION: return _( "Orientation" );
    case PFC_UPRIGHT:     return _( "Keep Upright" );
    case PFC_XOFFSET:     return _( "X Offset" );
    case PFC_YOFFSET:     return _( "Y Offset" );
    case PFC_KNOCKOUT:    return _( "Knockout" );
    case PFC_MIRRORED:    return _( "Mirrored" );
    default:              wxFAIL; return wxEmptyString;
    }
}


bool PCB_FIELDS_GRID_TABLE::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    switch( aCol )
    {
    case PFC_NAME:
    case PFC_VALUE:
    case PFC_WIDTH:
    case PFC_HEIGHT:
    case PFC_THICKNESS:
    case PFC_ORIENTATION:
    case PFC_XOFFSET:
    case PFC_YOFFSET:
        return aTypeName == wxGRID_VALUE_STRING;

    case PFC_SHOWN:
    case PFC_ITALIC:
    case PFC_UPRIGHT:
    case PFC_KNOCKOUT:
    case PFC_MIRRORED:
        return aTypeName == wxGRID_VALUE_BOOL;

    case PFC_LAYER:
        return aTypeName == wxGRID_VALUE_NUMBER;

    default:
        wxFAIL;
        return false;
    }
}


bool PCB_FIELDS_GRID_TABLE::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return CanGetValueAs( aRow, aCol, aTypeName );
}


wxGridCellAttr* PCB_FIELDS_GRID_TABLE::GetAttr( int aRow, int aCol,
                                                wxGridCellAttr::wxAttrKind aKind  )
{
    const PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case PFC_NAME:
        if( field.IsMandatory() )
        {
            m_readOnlyAttr->IncRef();
            return enhanceAttr( m_readOnlyAttr, aRow, aCol, aKind );
        }

        return enhanceAttr( nullptr, aRow, aCol, aKind );

    case PFC_VALUE:
        if( field.GetId() == FIELD_T::REFERENCE )
        {
            m_referenceAttr->IncRef();
            return enhanceAttr( m_referenceAttr, aRow, aCol, aKind );
        }
        else if( field.GetId() == FIELD_T::VALUE )
        {
            m_valueAttr->IncRef();
            return enhanceAttr( m_valueAttr, aRow, aCol, aKind );
        }
        else if( field.GetId() == FIELD_T::DATASHEET || field.HasHypertext() )
        {
            m_urlAttr->IncRef();
            return enhanceAttr( m_urlAttr, aRow, aCol, aKind );
        }

        return enhanceAttr( nullptr, aRow, aCol, aKind );

    case PFC_WIDTH:
    case PFC_HEIGHT:
    case PFC_THICKNESS:
    case PFC_XOFFSET:
    case PFC_YOFFSET:
        return enhanceAttr( nullptr, aRow, aCol, aKind );

    case PFC_SHOWN:
    case PFC_ITALIC:
    case PFC_UPRIGHT:
    case PFC_KNOCKOUT:
    case PFC_MIRRORED:
        m_boolColAttr->IncRef();
        return enhanceAttr( m_boolColAttr, aRow, aCol, aKind );

    case PFC_LAYER:
        m_layerColAttr->IncRef();
        return enhanceAttr( m_layerColAttr, aRow, aCol, aKind );

    case PFC_ORIENTATION:
        m_orientationColAttr->IncRef();
        return enhanceAttr( m_orientationColAttr, aRow, aCol, aKind );

    default:
        wxFAIL;
        return enhanceAttr( nullptr, aRow, aCol, aKind );
    }
}


wxString PCB_FIELDS_GRID_TABLE::GetValue( int aRow, int aCol )
{
    wxGrid*          grid = GetView();
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
    case PFC_NAME:       return field.GetName();
    case PFC_VALUE:      return field.GetText();
    case PFC_WIDTH:      return m_frame->StringFromValue( field.GetTextWidth(), true );
    case PFC_HEIGHT:     return m_frame->StringFromValue( field.GetTextHeight(), true );
    case PFC_THICKNESS:  return m_frame->StringFromValue( field.GetTextThickness(), true );
    case PFC_LAYER:      return field.GetLayerName();

    case PFC_ORIENTATION:
    {
        EDA_ANGLE angle = field.GetTextAngle() - field.GetParentFootprint()->GetOrientation();
        return m_frame->StringFromValue( angle, true );
    }

    case PFC_XOFFSET:    return m_frame->StringFromValue( field.GetFPRelativePosition().x, true );
    case PFC_YOFFSET:    return m_frame->StringFromValue( field.GetFPRelativePosition().y, true );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


bool PCB_FIELDS_GRID_TABLE::GetValueAsBool( int aRow, int aCol )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case PFC_SHOWN:    return field.IsVisible();
    case PFC_ITALIC:   return field.IsItalic();
    case PFC_UPRIGHT:  return field.IsKeepUpright();
    case PFC_KNOCKOUT: return field.IsKnockout();
    case PFC_MIRRORED: return field.IsMirrored();

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


long PCB_FIELDS_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case PFC_LAYER: return field.GetLayer();

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        return 0;
    }
}


void PCB_FIELDS_GRID_TABLE::SetValue( int aRow, int aCol, const wxString &aValue )
{
    PCB_FIELD& field = this->at( (size_t) aRow );
    VECTOR2I  pos;
    wxString  value = aValue;

    if( aCol != PFC_VALUE )
        value.Trim( true ).Trim( false );

    if( aCol == PFC_WIDTH
     || aCol == PFC_HEIGHT
     || aCol == PFC_THICKNESS
     || aCol == PFC_XOFFSET
     || aCol == PFC_YOFFSET )
    {
        m_eval->SetDefaultUnits( m_frame->GetUserUnits() );

        if( m_eval->Process( value ) )
        {
            m_evalOriginal[ { aRow, aCol } ] = value;
            value = m_eval->Result();
        }
    }

    switch( aCol )
    {
    case PFC_NAME:       field.SetName( value );                                       break;
    case PFC_VALUE:      field.SetText( value );                                       break;
    case PFC_WIDTH:      field.SetTextWidth( m_frame->ValueFromString( value ) );      break;
    case PFC_HEIGHT:     field.SetTextHeight( m_frame->ValueFromString( value ) );     break;
    case PFC_THICKNESS:  field.SetTextThickness( m_frame->ValueFromString( value ) );  break;

    case PFC_ORIENTATION:
        field.SetTextAngle( m_frame->AngleValueFromString( value )
                            + field.GetParentFootprint()->GetOrientation() );
        break;

    case PFC_XOFFSET:
    case PFC_YOFFSET:
        pos = field.GetFPRelativePosition();

        if( aCol == PFC_XOFFSET )
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
    m_dialog->OnModify();
}


void PCB_FIELDS_GRID_TABLE::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case PFC_SHOWN:     field.SetVisible( aValue );      break;
    case PFC_ITALIC:    field.SetItalic( aValue );       break;
    case PFC_UPRIGHT:   field.SetKeepUpright( aValue );  break;
    case PFC_KNOCKOUT:  field.SetIsKnockout( aValue );   break;
    case PFC_MIRRORED:  field.SetMirrored( aValue );     break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        break;
    }

    m_dialog->OnModify();
}


void PCB_FIELDS_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    PCB_FIELD& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case PFC_LAYER:
        field.SetLayer( ToLAYER_ID( (int) aValue ) );

        if( BOARD* board = field.GetBoard() )
            field.SetMirrored( board->IsBackLayer( field.GetLayer() ) );
        else
            field.SetMirrored( IsBackLayer( field.GetLayer() ) );

        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a long value" ), aCol ) );
        break;
    }

    m_dialog->OnModify();
}

