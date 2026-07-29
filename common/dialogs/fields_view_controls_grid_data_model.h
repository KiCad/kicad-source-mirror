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

#pragma once

#include <widgets/wx_grid.h>
#include <settings/bom_settings.h>

// Columns for the View Fields grid
#define DISPLAY_NAME_COLUMN   0     // The field name in the data model (translated)
#define LABEL_COLUMN          1     // The field name's label for exporting (CSV, etc.)
#define SHOW_FIELD_COLUMN     2
#define GROUP_BY_COLUMN       3
#define VIEW_FIELDS_COL_COUNT 4


// Data model for the list of fields to view (and to group-by) for the Symbol Fields Table
class VIEW_CONTROLS_GRID_DATA_MODEL : public WX_GRID_TABLE_BASE
{
public:
    VIEW_CONTROLS_GRID_DATA_MODEL( bool aForBOM ) :
            m_forBOM( aForBOM )
    {}

    ~VIEW_CONTROLS_GRID_DATA_MODEL() override = default;

    int GetNumberRows() override { return (int) m_fields.size(); }
    int GetNumberCols() override { return VIEW_FIELDS_COL_COUNT; }

    wxString GetColLabelValue( int aCol ) override;

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return false; // don't allow adjacent cell overflow, even if we are actually empty
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case DISPLAY_NAME_COLUMN:
        case LABEL_COLUMN:        return aTypeName == wxGRID_VALUE_STRING;

        case SHOW_FIELD_COLUMN:
        case GROUP_BY_COLUMN:     return aTypeName == wxGRID_VALUE_BOOL;

        default:                  wxFAIL; return false;
        }
    }

    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        return CanGetValueAs( aRow, aCol, aTypeName );
    }

    wxString GetValue( int aRow, int aCol ) override;
    bool     GetValueAsBool( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;
    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;

    void AppendRow( const wxString& aFieldName, const wxString& aBOMName, bool aShow, bool aGroupBy );
    void DeleteRow( int aRow );

    wxString GetCanonicalFieldName( int aRow );
    void     SetCanonicalFieldName( int aRow, const wxString& aName );

protected:
    bool                   m_forBOM;
    std::vector<BOM_FIELD> m_fields;
};
