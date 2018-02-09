/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef FIELDS_EDITOR_TABLE_COLUMN_H
#define FIELDS_EDITOR_TABLE_COLUMN_H

#include <wx/regex.h>
#include <wx/string.h>
#include <vector>

// Default column names (translated)
#define FIELDS_EDITOR_COL_TITLE_REFERENCE    _( "Reference" )
#define FIELDS_EDITOR_COL_TITLE_DESCRIPTION  _( "Description" )
#define FIELDS_EDITOR_COL_TITLE_FOOTPRINT    _( "Footprint" )
#define FIELDS_EDITOR_COL_TITLE_VALUE        _( "Value" )
#define FIELDS_EDITOR_COL_TITLE_DATASHEET    _( "Datasheet" )
#define FIELDS_EDITOR_COL_TITLE_QUANTITY     _( "Quantity" )

/**
 * Column type enumeration
 * Not currently implemented,
 * in the future the different column 'types' might
 * be used for something...
 */
enum FIELDS_EDITOR_COLUMN_TYPE
{
    FIELDS_EDITOR_COL_TYPE_KICAD = 0,     ///< Default column (editable)
    FIELDS_EDITOR_COL_TYPE_LIBRARY,       ///< Default column (non-editable)
    FIELDS_EDITOR_COL_TYPE_GENERATED,     ///< Generated column (e.g. Quantity)
    FIELDS_EDITOR_COL_TYPE_USER,          ///< User data
};


/**
 * Predefined column ID values for default columns.
 * User columns are assigned IDs of 1000 and above
 */
enum FIELDS_EDITOR_COLUMN_ID
{
    // Default component fields
    FIELDS_EDITOR_COL_ID_REFERENCE = 0,
    FIELDS_EDITOR_COL_ID_DESCRIPTION,
    FIELDS_EDITOR_COL_ID_FOOTPRINT,
    FIELDS_EDITOR_COL_ID_VALUE,
    FIELDS_EDITOR_COL_ID_DATASHEET,

    // Meta-data fields
    FIELDS_EDITOR_COL_ID_QUANTITY = 100,

    // Custom data fields
    FIELDS_EDITOR_COL_ID_USER = 1000,
};

/**
 * FIELDS_EDITOR_COLUMN class
 * Models a single column in the BOM view
 * Each column can be used to group components,
 * and can be hidden from the output BOM
 */
class FIELDS_EDITOR_COLUMN
{
protected:
    unsigned int        m_id;       ///< Unique column ID
    FIELDS_EDITOR_COLUMN_TYPE     m_Type;     ///< Column type

    wxString    m_Title;        ///< The column (field) title
    bool        m_Show;         ///< Is this column visible?
    bool        m_ReadOnly;     ///< Is this column read only?

    bool        m_sort;         ///< Is this column used for sorting?

public:
    FIELDS_EDITOR_COLUMN( unsigned int aId, FIELDS_EDITOR_COLUMN_TYPE aType, const wxString aTitle, bool aShow, bool aReadOnly = false, bool aSort = true ) :
            m_id( aId ),
            m_Type( aType ),
            m_Title( aTitle.Strip( wxString::both ) ),
            m_Show( aShow ),
            m_ReadOnly( aReadOnly ),
            m_sort( aSort )
    {
    }

    unsigned int Id() const { return m_id; }
    FIELDS_EDITOR_COLUMN_TYPE Type() const { return m_Type; }
    wxString Title() const { return m_Title; }
    bool IsVisible() const { return m_Show; }
    bool IsReadOnly() const { return m_ReadOnly; }
    bool IsUsedToSort() const { return m_sort; }

    //TODO - Should renaming of columns be allowed?
    //bool SetTitle( const wxString aTitle );
    void SetVisible( bool aShow = true ) { m_Show = aShow; }
    void SetReadOnly( bool aReadOnly = true ) { m_ReadOnly = aReadOnly; }
    void SetUsedToSort( bool aSort = true ) { m_sort = aSort; }

};

/*
 * The FIELDS_EDITOR_COLUMN_LIST class contains information
 * on all columns existing in the BOM
 */
class FIELDS_EDITOR_COLUMN_LIST
{
protected:

    unsigned int m_nextFieldId;

public:
    std::vector< FIELDS_EDITOR_COLUMN* > Columns;

    FIELDS_EDITOR_COLUMN_LIST();

    void Clear();

    unsigned int NextFieldId() const { return m_nextFieldId; }
    unsigned int ColumnCount( bool aIncludeHidden = true ) const;

    FIELDS_EDITOR_COLUMN* GetColumnByIndex( unsigned int aColIndex );
    FIELDS_EDITOR_COLUMN* GetColumnById( unsigned int aColId );
    FIELDS_EDITOR_COLUMN* GetColumnByTitle( const wxString& aColTitle ) ;

    bool ContainsColumn( unsigned int aColId );
    bool ContainsColumn( const wxString& aColTitle );

    bool AddColumn( FIELDS_EDITOR_COLUMN* aCol );

};



#endif /* FIELDS_EDITOR_TABLE_COLUMN_H */
