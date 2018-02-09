/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "fields_editor_table_column.h"

FIELDS_EDITOR_COLUMN_LIST::FIELDS_EDITOR_COLUMN_LIST() : m_nextFieldId( FIELDS_EDITOR_COL_ID_USER )
{

}

void FIELDS_EDITOR_COLUMN_LIST::Clear()
{
    Columns.clear();
    m_nextFieldId = FIELDS_EDITOR_COL_ID_USER;
}

/**
 * Return the number of columns
 * @param aIncludeHidden - If this is false, only visible columns will be included
 */
unsigned int FIELDS_EDITOR_COLUMN_LIST::ColumnCount( bool aIncludeHidden ) const
{
    unsigned int count = 0;

    for( FIELDS_EDITOR_COLUMN* col : Columns )
    {
        if( col && ( col->IsVisible() || aIncludeHidden ) )
        {
            count++;
        }
    }

    return count;
}

/**
 * Return a column based on its stored position
 */
FIELDS_EDITOR_COLUMN* FIELDS_EDITOR_COLUMN_LIST::GetColumnByIndex( unsigned int aColId )
{
    if( aColId < Columns.size() )
        return Columns[aColId];

    return nullptr;
}

/**
 * Return a column based on its unique ID
 */
FIELDS_EDITOR_COLUMN* FIELDS_EDITOR_COLUMN_LIST::GetColumnById( unsigned int aColId )
{
    for( unsigned int ii=0; ii<Columns.size(); ii++ )
    {
        if( Columns[ii] && Columns[ii]->Id() == aColId )
            return Columns[ii];
    }

    return nullptr;
}

/**
 * Return a column based on its string title
 */
FIELDS_EDITOR_COLUMN* FIELDS_EDITOR_COLUMN_LIST::GetColumnByTitle( const wxString& aColTitle )
{
    for( unsigned int ii=0; ii<Columns.size(); ii++ )
    {
        if( Columns[ii] && Columns[ii]->Title().Cmp( aColTitle ) == 0 )
            return Columns[ii];
    }

    return nullptr;
}

/**
 * Test if the list includes a column with the given unique ID
 */
bool FIELDS_EDITOR_COLUMN_LIST::ContainsColumn( unsigned int aColId )
{
    for( FIELDS_EDITOR_COLUMN* col : Columns )
    {
        if( col && col->Id() == aColId )
            return true;
    }

    return false;
}

/**
 * Test if the list includes a column with the given title
 */
bool FIELDS_EDITOR_COLUMN_LIST::ContainsColumn( const wxString& aColTitle )
{
    return nullptr != GetColumnByTitle( aColTitle );
}

/**
 * Add a new column to the list
 */
bool FIELDS_EDITOR_COLUMN_LIST::AddColumn( FIELDS_EDITOR_COLUMN* aCol )
{
    if( nullptr == aCol )
        return false;

    if( ContainsColumn( aCol->Id() ) )
        return false;

    Columns.push_back( aCol );

    // If this is a user field, increment the counter
    if( aCol->Id() >= FIELDS_EDITOR_COL_ID_USER )
        m_nextFieldId++;

    return true;
}
