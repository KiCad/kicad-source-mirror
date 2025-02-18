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

#ifndef __LIB_TABLE_GRID_H__
#define __LIB_TABLE_GRID_H__

#include <libraries/library_table.h>
#include <string_utils.h>
#include <wx/grid.h>

class LIB_TABLE_GRID_TRICKS;

const wxColour COLOUR_ROW_ENABLED( 0, 0, 0 );
const wxColour COLOUR_ROW_DISABLED( 100, 100, 100 );

/// The library table grid column order is established by this sequence.
enum COL_ORDER
{
    COL_STATUS,
    COL_ENABLED,
    COL_VISIBLE,
    COL_NICKNAME,
    COL_URI,
    COL_TYPE,
    COL_OPTIONS,
    COL_DESCR,

    COL_COUNT       // keep as last
};

/**
 * This abstract base class mixes any object derived from #LIB_TABLE into wxGridTableBase
 * so the result can be used as any type of library table within wxGrid.
 */
class LIB_TABLE_GRID : public wxGridTableBase
{
    friend class LIB_TABLE_GRID_TRICKS;

public:

    //-----<wxGridTableBase overloads>-------------------------------------------

    int GetNumberRows() override { return (int) size(); }

    int GetNumberCols() override { return COL_COUNT; }

    wxString GetValue( int aRow, int aCol ) override
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

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        if( aRow < (int) size() )
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

    bool GetValueAsBool( int aRow, int aCol ) override
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

    void SetValue( int aRow, int aCol, const wxString& aValue ) override
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
            case COL_TYPE:     r.SetType( aValue  );                                break;
            case COL_OPTIONS:  r.SetOptions( aValue );                              break;
            case COL_DESCR:    r.SetDescription( aValue );                          break;
            case COL_ENABLED:  r.SetDisabled( aValue == wxT( "0" ) );               break;
            case COL_VISIBLE:  r.SetHidden( aValue == wxT( "0" ) );                 break;
            case COL_STATUS: break;
            }
        }
    }

    void SetValueAsBool( int aRow, int aCol, bool aValue ) override
    {
        wxCHECK( aRow >= 0, /* void */ );
        size_t row = static_cast<size_t>( aRow );

        if( row < size() && aCol == COL_ENABLED )
            at( row ).SetDisabled( !aValue );
        else if( row < size() && aCol == COL_VISIBLE )
            at( row ).SetHidden( !aValue );
    }

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return !GetValue( aRow, aCol );
    }

    bool InsertRows( size_t aPos = 0, size_t aNumRows = 1 ) override
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

    bool AppendRows( size_t aNumRows = 1 ) override
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

    bool DeleteRows( size_t aPos, size_t aNumRows ) override
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

    wxString GetColLabelValue( int aCol ) override
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

    bool ContainsNickname( const wxString& aNickname )
    {
        for( size_t i = 0; i < size(); ++i )
        {
            LIBRARY_TABLE_ROW& row = at( i );

            if( row.Nickname() == aNickname )
                return true;
        }
        return false;
    }

    LIBRARY_TABLE_ROW& At( size_t aIndex )
    {
        return at( aIndex );
    }

protected:
    virtual LIBRARY_TABLE_ROW& at( size_t aIndex ) = 0;

    virtual size_t size() const = 0;

    virtual LIBRARY_TABLE_ROW makeNewRow() = 0;

    virtual LIBRARY_TABLE_ROWS_ITER begin() = 0;

    virtual LIBRARY_TABLE_ROWS_ITER insert( LIBRARY_TABLE_ROWS_ITER aIterator,
                                            const LIBRARY_TABLE_ROW& aRow ) = 0;

    virtual void push_back( const LIBRARY_TABLE_ROW& aRow ) = 0;

    virtual LIBRARY_TABLE_ROWS_ITER erase( LIBRARY_TABLE_ROWS_ITER aFirst,
                                           LIBRARY_TABLE_ROWS_ITER aLast ) = 0;
};


#endif // __LIB_TABLE_GRID_H__
