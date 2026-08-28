/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <template_fieldnames.h>

#include "fields_view_controls_grid_data_model.h"

wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN: return _( "Field" );
    case LABEL_COLUMN:        return m_forBOM ? _( "BOM Name" ) : _( "Label" );
    case SHOW_FIELD_COLUMN:   return _( "Include" );
    case GROUP_BY_COLUMN:     return _( "Group By" );
    default:                  return wxT( "unknown column" );
    };
}


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetValue( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), wxT( "bad row!" ) );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN:
        for( FIELD_T fieldId : MANDATORY_FIELDS )
        {
            if( GetDefaultFieldName( fieldId, UNTRANSLATED ) == rowData.name )
                return GetDefaultFieldName( fieldId, TRANSLATED );
        }

        return rowData.name;

    case LABEL_COLUMN:
        return rowData.label;

    default:
        // we can't assert here because wxWidgets sometimes calls this without checking
        // the column type when trying to see if there's an overflow
        return wxT( "bad wxWidgets!" );
    }
}


bool VIEW_CONTROLS_GRID_DATA_MODEL::GetValueAsBool( int aRow, int aCol )
{
    wxCHECK( aRow < GetNumberRows(), false );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case SHOW_FIELD_COLUMN: return rowData.show;
    case GROUP_BY_COLUMN:   return rowData.groupBy;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
        return false;
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case DISPLAY_NAME_COLUMN:
        // Not editable
        break;

    case LABEL_COLUMN:
        rowData.label = aValue;
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
    }

    GetView()->Refresh();
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    wxCHECK( aRow < GetNumberRows(), /*void*/ );

    BOM_FIELD& rowData = m_fields[aRow];

    switch( aCol )
    {
    case SHOW_FIELD_COLUMN: rowData.show = aValue;    break;
    case GROUP_BY_COLUMN:   rowData.groupBy = aValue; break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a bool value" ), aCol ) );
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::AppendRow( const wxString& aFieldName, const wxString& aBOMName,
                                               bool aShow, bool aGroupBy )
{
    m_fields.emplace_back( BOM_FIELD{ aFieldName, aBOMName, aShow, aGroupBy } );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
        grid->ProcessTableMessage( msg );
    }
}


void VIEW_CONTROLS_GRID_DATA_MODEL::DeleteRow( int aRow )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), /* void */ );

    m_fields.erase( m_fields.begin() + aRow );

    if( wxGrid* grid = GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
        grid->ProcessTableMessage( msg );
    }
}


wxString VIEW_CONTROLS_GRID_DATA_MODEL::GetUntranslatedFieldName( int aRow )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), wxEmptyString );

    BOM_FIELD& rowData = m_fields[aRow];

    return rowData.name;
}


void VIEW_CONTROLS_GRID_DATA_MODEL::SetUntranslatedFieldName( int aRow, const wxString& aName )
{
    wxCHECK( aRow >= 0 && aRow < GetNumberRows(), /* void */ );

    BOM_FIELD& rowData = m_fields[aRow];

    rowData.name = aName;
}
