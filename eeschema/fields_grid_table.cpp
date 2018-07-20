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
#include <dialog_shim.h>
#include <fields_grid_table.h>
#include <sch_field.h>
#include <sch_validators.h>
#include <validators.h>
#include <class_library.h>
#include <template_fieldnames.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>

#include "eda_doc.h"


enum
{
    MYID_SELECT_FOOTPRINT = 991,         // must be within GRID_TRICKS' enum range
    MYID_SHOW_DATASHEET
};


template <class T>
FIELDS_GRID_TABLE<T>::FIELDS_GRID_TABLE( DIALOG_SHIM* aDialog, bool aInLibEdit, LIB_PART* aPart ) :
    m_userUnits( aDialog->GetUserUnits() ),
    m_part( aPart ),
    m_inLibEdit( aInLibEdit ),
    m_valueValidator( aInLibEdit, REFERENCE )
{
    // Build the column attributes.

    m_readOnlyAttr = new wxGridCellAttr;
    m_readOnlyAttr->SetReadOnly( true );

    m_valueColAttr = new wxGridCellAttr;
    GRID_CELL_TEXT_EDITOR* textEditor = new GRID_CELL_TEXT_EDITOR();
    textEditor->SetValidator( m_valueValidator );
    m_valueColAttr->SetEditor( textEditor );

    m_footprintAttr = new wxGridCellAttr;
    m_footprintAttr->SetEditor( new GRID_CELL_FOOTPRINT_EDITOR( aDialog ) );

    m_urlAttr = new wxGridCellAttr;
    m_urlAttr->SetEditor( new GRID_CELL_URL_EDITOR( aDialog ) );

    m_boolColAttr = new wxGridCellAttr;
    m_boolColAttr->SetRenderer( new wxGridCellBoolRenderer() );
    m_boolColAttr->SetEditor( new wxGridCellBoolEditor() );
    m_boolColAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );

    wxArrayString vAlignNames;
    vAlignNames.Add( _( "Top" ) );
    vAlignNames.Add( _( "Center" ) );
    vAlignNames.Add( _( "Bottom" ) );
    m_vAlignColAttr = new wxGridCellAttr;
    m_vAlignColAttr->SetEditor( new wxGridCellChoiceEditor( vAlignNames ) );
    m_vAlignColAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );

    wxArrayString hAlignNames;
    hAlignNames.Add( _( "Left" ) );
    hAlignNames.Add(_( "Center" ) );
    hAlignNames.Add(_( "Right" ) );
    m_hAlignColAttr = new wxGridCellAttr;
    m_hAlignColAttr->SetEditor( new wxGridCellChoiceEditor( hAlignNames ) );
    m_hAlignColAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );

    wxArrayString orientationNames;
    orientationNames.Add( _( "Horizontal" ) );
    orientationNames.Add(_( "Vertical" ) );
    m_orientationColAttr = new wxGridCellAttr;
    m_orientationColAttr->SetEditor( new wxGridCellChoiceEditor( orientationNames ) );
    m_orientationColAttr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );
}


template <class T>
FIELDS_GRID_TABLE<T>::~FIELDS_GRID_TABLE()
{
    m_readOnlyAttr->DecRef();
    m_boolColAttr->DecRef();
    m_valueColAttr->DecRef();
    m_footprintAttr->DecRef();
    m_urlAttr->DecRef();
    m_vAlignColAttr->DecRef();
    m_hAlignColAttr->DecRef();
    m_orientationColAttr->DecRef();
}


template <class T>
wxString FIELDS_GRID_TABLE<T>::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case FDC_NAME:         return _( "Name" );
    case FDC_VALUE:        return _( "Value" );
    case FDC_SHOWN:        return _( "Show" );
    case FDC_H_ALIGN:      return _( "H Align" );
    case FDC_V_ALIGN:      return _( "V Align" );
    case FDC_ITALIC:       return _( "Italic" );
    case FDC_BOLD:         return _( "Bold" );
    case FDC_TEXT_SIZE:    return _( "Text Size" );
    case FDC_ORIENTATION:  return _( "Orientation" );
    case FDC_POSX:         return _( "X Position" );
    case FDC_POSY:         return _( "Y Position" );
    default:               wxFAIL; return wxEmptyString;
    }
}


template <class T>
bool FIELDS_GRID_TABLE<T>::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    switch( aCol )
    {
    case FDC_NAME:
    case FDC_VALUE:
    case FDC_H_ALIGN:
    case FDC_V_ALIGN:
    case FDC_TEXT_SIZE:
    case FDC_ORIENTATION:
    case FDC_POSX:
    case FDC_POSY:
        return aTypeName == wxGRID_VALUE_STRING;

    case FDC_SHOWN:
    case FDC_ITALIC:
    case FDC_BOLD:
        return aTypeName == wxGRID_VALUE_BOOL;

    default:
        wxFAIL;
        return false;
    }
}


template <class T>
bool FIELDS_GRID_TABLE<T>::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return CanGetValueAs( aRow, aCol, aTypeName );
}


template <class T>
wxGridCellAttr* FIELDS_GRID_TABLE<T>::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind  )
{
    switch( aCol )
    {
    case FDC_NAME:
        if( aRow < MANDATORY_FIELDS )
        {
            m_readOnlyAttr->IncRef();
            return m_readOnlyAttr;
        }
        else
            return nullptr;

    case FDC_VALUE:
        // This field is the lib name and the default value when loading this component in
        // schematic.  The value is now not editable here (in this dialog) because changing
        // it is equivalent to create a new component or alias. This is handled in libedit,
        // not in this dialog.
        if( m_inLibEdit && aRow == VALUE )
        {
            m_readOnlyAttr->IncRef();
            return m_readOnlyAttr;
        }
        // For power symbols, the value is not editable, because value and pin
        // name must be same and can be edited only in library editor
        else if( m_part && m_part->IsPower() )
        {
            m_readOnlyAttr->IncRef();
            return m_readOnlyAttr;
        }
        else if( aRow == FOOTPRINT )
        {
            m_footprintAttr->IncRef();
            return m_footprintAttr;
        }
        else if( aRow == DATASHEET )
        {
            m_urlAttr->IncRef();
            return m_urlAttr;
        }
        else
        {
            // Some fields have different validation semantics.  Make sure the
            // validator knows what it's validating.
            m_valueValidator.SetFieldId( aRow );

            m_valueColAttr->IncRef();
            return m_valueColAttr;
        }

    case FDC_TEXT_SIZE:
    case FDC_POSX:
    case FDC_POSY:
        return nullptr;

    case FDC_H_ALIGN:
        m_hAlignColAttr->IncRef();
        return m_hAlignColAttr;

    case FDC_V_ALIGN:
        m_vAlignColAttr->IncRef();
        return m_vAlignColAttr;

    case FDC_ORIENTATION:
        m_orientationColAttr->IncRef();
        return m_orientationColAttr;

    case FDC_SHOWN:
    case FDC_ITALIC:
    case FDC_BOLD:
        m_boolColAttr->IncRef();
        return m_boolColAttr;

    default:
        wxFAIL;
        return nullptr;
    }
}


template <class T>
wxString FIELDS_GRID_TABLE<T>::GetValue( int aRow, int aCol )
{
    const T& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FDC_NAME:
        // Use default field name for mandatory fields, because they are transalted
        // according to the current locale
        if( aRow < MANDATORY_FIELDS )
            return TEMPLATE_FIELDNAME::GetDefaultFieldName( aRow );
        else
            return field.GetName( false );

    case FDC_VALUE:
        return field.GetText();

    case FDC_H_ALIGN:
        switch( field.GetHorizJustify() )
        {
        case GR_TEXT_HJUSTIFY_LEFT:   return _( "Left" );
        case GR_TEXT_HJUSTIFY_CENTER: return _( "Center" );
        case GR_TEXT_HJUSTIFY_RIGHT:  return _( "Right" );
        }

    case FDC_V_ALIGN:
        switch( field.GetVertJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP:    return _( "Top" );
        case GR_TEXT_VJUSTIFY_CENTER: return _( "Center" );
        case GR_TEXT_VJUSTIFY_BOTTOM: return _( "Bottom" );
        }

    case FDC_TEXT_SIZE:
        return StringFromValue( m_userUnits, field.GetTextSize().GetHeight(), true, true );

    case FDC_ORIENTATION:
        switch( (int) field.GetTextAngle() )
        {
        case TEXT_ANGLE_HORIZ: return _( "Horizontal" );
        case TEXT_ANGLE_VERT:  return _( "Vertical" );
        }

    case FDC_POSX:
        return StringFromValue( m_userUnits, field.GetTextPos().x, true );

    case FDC_POSY:
        return StringFromValue( m_userUnits, field.GetTextPos().y, true );

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


template <class T>
bool FIELDS_GRID_TABLE<T>::GetValueAsBool( int aRow, int aCol )
{
    const T& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FDC_SHOWN:  return field.IsVisible();
    case FDC_ITALIC: return field.IsItalic();
    case FDC_BOLD:   return field.IsBold();
    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


template <class T>
void FIELDS_GRID_TABLE<T>::SetValue( int aRow, int aCol, const wxString &aValue )
{
    T& field = this->at( (size_t) aRow );
    wxPoint    pos;

    switch( aCol )
    {
    case FDC_NAME:
        field.SetName( aValue );
        break;

    case FDC_VALUE:
        field.SetText( aValue );
        break;

    case FDC_H_ALIGN:
        if( aValue == _( "Left" ) )
            field.SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        else if( aValue == _( "Center" ) )
            field.SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        else if( aValue == _( "Right" ) )
            field.SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        else
            wxFAIL_MSG( wxT( "unknown horizontal alignment: " ) + aValue );
        break;

    case FDC_V_ALIGN:
        if( aValue == _( "Top" ) )
            field.SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        else if( aValue == _( "Center" ) )
            field.SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        else if( aValue == _( "Bottom" ) )
            field.SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        else
            wxFAIL_MSG( wxT( "unknown vertical alignment: " ) + aValue);
        break;

    case FDC_TEXT_SIZE:
        field.SetTextSize( wxSize( ValueFromString( m_userUnits, aValue ),
                                   ValueFromString( m_userUnits, aValue ) ) );
        break;

    case FDC_ORIENTATION:
        if( aValue == _( "Horizontal" ) )
            field.SetTextAngle( TEXT_ANGLE_HORIZ );
        else if( aValue == _( "Vertical" ) )
            field.SetTextAngle( TEXT_ANGLE_VERT );
        else
            wxFAIL_MSG( wxT( "unknown orientation: " ) + aValue );

    case FDC_POSX:
    case FDC_POSY:
        pos = field.GetTextPos();
        if( aCol == FDC_POSX )
            pos.x = ValueFromString( m_userUnits, aValue );
        else
            pos.y = ValueFromString( m_userUnits, aValue );
        field.SetTextPos( pos );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
        break;
    }

    GetView()->Refresh();
}

template <class T>
void FIELDS_GRID_TABLE<T>::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    T& field = this->at( (size_t) aRow );

    switch( aCol )
    {
    case FDC_SHOWN:
        field.SetVisible( aValue );
        break;
    case FDC_ITALIC:
        field.SetItalic( aValue );
        break;
    case FDC_BOLD:
        field.SetBold( aValue );
        break;
    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        break;
    }
}


// Explicit Instantiations

template class FIELDS_GRID_TABLE<SCH_FIELD>;
template class FIELDS_GRID_TABLE<LIB_FIELD>;



void FIELDS_GRID_TRICKS::showPopupMenu( wxMenu& menu )
{
    if( m_grid->GetGridCursorRow() == FOOTPRINT && m_grid->GetGridCursorCol() == FDC_VALUE )
    {
        menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ), _( "Browse for footprint" ) );
        menu.AppendSeparator();
    }
    else if( m_grid->GetGridCursorRow() == DATASHEET && m_grid->GetGridCursorCol() == FDC_VALUE )
    {
        menu.Append( MYID_SHOW_DATASHEET,   _( "Show Datasheet" ),      _( "Show datasheet in browser" ) );
        menu.AppendSeparator();
    }

    GRID_TRICKS::showPopupMenu( menu );
}


void FIELDS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    if( event.GetId() == MYID_SELECT_FOOTPRINT )
    {
        // pick a footprint using the footprint picker.
        wxString      fpid = m_grid->GetCellValue( FOOTPRINT, FDC_VALUE );
        KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true, m_dlg );

        if( frame->ShowModal( &fpid, m_dlg ) )
            m_grid->SetCellValue( FOOTPRINT, FDC_VALUE, fpid );

        frame->Destroy();
    }
    else if (event.GetId() == MYID_SHOW_DATASHEET )
    {
        wxString datasheet_uri = m_grid->GetCellValue( DATASHEET, FDC_VALUE );
        datasheet_uri = ResolveUriByEnvVars( datasheet_uri );
        GetAssociatedDocument( m_dlg, datasheet_uri );
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


