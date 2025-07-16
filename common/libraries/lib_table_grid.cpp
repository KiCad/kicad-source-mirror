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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lib_table_grid.h>
#include <string_utils.h>


wxString LIB_TABLE_GRID::GetValue( int aRow, int aCol )
{
    wxCHECK( aRow >= 0, wxEmptyString );
    size_t row = static_cast<size_t>( aRow );

    if( row < size() )
    {
        const LIBRARY_TABLE_ROW& r = at( row );

        switch( aCol )
        {
        case COL_NICKNAME: return UnescapeString( r.Nickname() );
        case COL_URI:      return r.URI();
        case COL_TYPE:     return r.Type();
        case COL_OPTIONS:  return r.Options();
        case COL_DESCR:    return r.Description();
        case COL_ENABLED:  return r.Disabled() ? wxT( "0" ) : wxT( "1" );
        case COL_VISIBLE:  return r.Hidden() ? wxT( "0" ) : wxT( "1" );
        case COL_STATUS:   return r.ErrorDescription();
        default:           return wxEmptyString;
        }
    }

    return wxEmptyString;
}


bool LIB_TABLE_GRID::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    if( aRow < static_cast<int>( size() ) )
    {
        switch( aCol )
        {
        case COL_ENABLED:
        case COL_VISIBLE:
            return aTypeName == wxGRID_VALUE_BOOL;

        default:
            return aTypeName == wxGRID_VALUE_STRING;
        }
    }

    return false;
}


bool LIB_TABLE_GRID::GetValueAsBool( int aRow, int aCol )
{
    wxCHECK( aRow >= 0, false );
    size_t row = static_cast<size_t>( aRow );

    if( row < size() && aCol == COL_ENABLED )
        return !at( row ).Disabled();
    else if( row < size() && aCol == COL_VISIBLE )
        return !at( row ).Hidden();
    else
        return false;
}


void LIB_TABLE_GRID::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxCHECK( aRow >= 0, /* void */ );
    size_t row = static_cast<size_t>( aRow );

    if( row < size() )
    {
        LIBRARY_TABLE_ROW& r = at( row );

        switch( aCol )
        {
        case COL_NICKNAME: r.SetNickname( EscapeString( aValue, CTX_LIBID ) );  break;
        case COL_URI:      r.SetURI( aValue );                                  break;
        case COL_TYPE:     r.SetType( aValue );                                break;
        case COL_OPTIONS:  r.SetOptions( aValue );                              break;
        case COL_DESCR:    r.SetDescription( aValue );                          break;
        case COL_ENABLED:  r.SetDisabled( aValue == wxT( "0" ) );               break;
        case COL_VISIBLE:  r.SetHidden( aValue == wxT( "0" ) );                 break;
        case COL_STATUS: break;
        }
    }
}

void LIB_TABLE_GRID::SetValueAsBool( int aRow, int aCol, bool aValue )
{
    wxCHECK( aRow >= 0, /* void */ );
    size_t row = static_cast<size_t>( aRow );

    if( row < size() && aCol == COL_ENABLED )
        at( row ).SetDisabled( !aValue );
    else if( row < size() && aCol == COL_VISIBLE )
        at( row ).SetHidden( !aValue );
}


bool LIB_TABLE_GRID::InsertRows( size_t aPos, size_t aNumRows  )
{
    if( aPos < size() )
    {
        for( size_t i = 0; i < aNumRows; i++ )
        {
            insert( begin() + i, makeNewRow() );
        }

        // use the (wxGridStringTable) source Luke.
        if( GetView() )
        {
            wxGridTableMessage msg( this,
                                    wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                    aPos,
                                    aNumRows );

            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    return false;
}


bool LIB_TABLE_GRID::AppendRows( size_t aNumRows )
{
    // do not modify aNumRows, original value needed for wxGridTableMessage below
    for( int i = aNumRows; i; --i )
        push_back( makeNewRow() );

    if( GetView() )
    {
        wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, aNumRows );
        GetView()->ProcessTableMessage( msg );
    }

    return true;
}


bool LIB_TABLE_GRID::DeleteRows( size_t aPos, size_t aNumRows )
{
    // aPos may be a large positive, e.g. size_t(-1), and the sum of
    // aPos+aNumRows may wrap here, so both ends of the range are tested.
    if( aPos < size() && aPos + aNumRows <= size() )
    {
        LIBRARY_TABLE_ROWS_ITER start = begin() + aPos;
        erase( start, start + aNumRows );

        if( GetView() )
        {
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aPos, aNumRows );
            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    return false;
}


wxString LIB_TABLE_GRID::GetColLabelValue( int aCol )
{
    switch( aCol )
    {
    case COL_NICKNAME:  return _( "Nickname" );
    case COL_URI:       return _( "Library Path" );

    // keep this "Library Format" text fairly long so column is sized wide enough
    case COL_TYPE:      return _( "Library Format" );
    case COL_OPTIONS:   return _( "Options" );
    case COL_DESCR:     return _( "Description" );
    case COL_ENABLED:   return _( "Enable" );
    case COL_VISIBLE:   return _( "Show" );
    case COL_STATUS:    return wxEmptyString;

    default:            return wxEmptyString;
    }
}


bool LIB_TABLE_GRID::ContainsNickname( const wxString& aNickname )
{
    for( size_t i = 0; i < size(); ++i )
    {
        LIBRARY_TABLE_ROW& row = at( i );

        if( row.Nickname() == aNickname )
            return true;
    }
    return false;
}


LIBRARY_TABLE_ROW& LIB_TABLE_GRID::at( size_t aIndex )
{
    return m_table.Rows().at( aIndex );
}


size_t LIB_TABLE_GRID::size() const
{
    return m_table.Rows().size();
}


LIBRARY_TABLE_ROW LIB_TABLE_GRID::makeNewRow()
{
    return m_table.MakeRow();
}


LIBRARY_TABLE_ROWS_ITER LIB_TABLE_GRID::begin()
{
    return m_table.Rows().begin();
}


LIBRARY_TABLE_ROWS_ITER LIB_TABLE_GRID::insert( LIBRARY_TABLE_ROWS_ITER aIterator,
                                                const LIBRARY_TABLE_ROW& aRow )
{
    return m_table.Rows().insert( aIterator, aRow );
}


void LIB_TABLE_GRID::push_back( const LIBRARY_TABLE_ROW& aRow )
{
    m_table.Rows().push_back( aRow );
}


LIBRARY_TABLE_ROWS_ITER LIB_TABLE_GRID::erase( LIBRARY_TABLE_ROWS_ITER aFirst,
                                               LIBRARY_TABLE_ROWS_ITER aLast )
{
    return m_table.Rows().erase( aFirst, aLast );
}
