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

#include "bom_table_column.h"

BOM_COLUMN_LIST::BOM_COLUMN_LIST() : m_nextFieldId( BOM_COL_ID_USER )
{

}

void BOM_COLUMN_LIST::Clear()
{
    Columns.clear();
    m_nextFieldId = BOM_COL_ID_USER;
}

/**
 * Return the number of columns
 * @param aIncludeHidden - If this is false, only visible columns will be included
 */
unsigned int BOM_COLUMN_LIST::ColumnCount( bool aIncludeHidden ) const
{
    unsigned int count = 0;

    for( BOM_COLUMN* col : Columns )
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
BOM_COLUMN* BOM_COLUMN_LIST::GetColumnByIndex( unsigned int aColId )
{
    if( aColId < Columns.size() )
        return Columns[aColId];

    return nullptr;
}

/**
 * Return a column based on its unique ID
 */
BOM_COLUMN* BOM_COLUMN_LIST::GetColumnById( unsigned int aColId )
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
BOM_COLUMN* BOM_COLUMN_LIST::GetColumnByTitle( wxString aColTitle )
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
bool BOM_COLUMN_LIST::ContainsColumn( unsigned int aColId )
{
    for( BOM_COLUMN* col : Columns )
    {
        if( col && col->Id() == aColId )
            return true;
    }

    return false;
}

/**
 * Test if the list includes a column with the given title
 */
bool BOM_COLUMN_LIST::ContainsColumn( wxString aColTitle )
{
    return nullptr != GetColumnByTitle( aColTitle );
}

/**
 * Add a new column to the list
 */
bool BOM_COLUMN_LIST::AddColumn( BOM_COLUMN* aCol )
{
    if( nullptr == aCol )
        return false;

    if( ContainsColumn( aCol->Id() ) )
        return false;

    Columns.push_back( aCol );

    // If this is a user field, increment the counter
    if( aCol->Id() >= BOM_COL_ID_USER )
        m_nextFieldId++;

    return true;
}
