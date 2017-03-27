/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lib_table_base.h>

#include <wx/grid.h>

/// The library table grid column order is established by this sequence.
enum COL_ORDER
{
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
public:

    //-----<wxGridTableBase overloads>-------------------------------------------

    int GetNumberRows() override { return (int) size(); }

    int GetNumberCols() override { return COL_COUNT; }

    wxString GetValue( int aRow, int aCol ) override
    {
        if( aRow < (int) size() )
        {
            const LIB_TABLE_ROW* r  = at( (size_t) aRow );

            switch( aCol )
            {
            case COL_NICKNAME:  return r->GetNickName();
            case COL_URI:       return r->GetFullURI();
            case COL_TYPE:      return r->GetType();
            case COL_OPTIONS:   return r->GetOptions();
            case COL_DESCR:     return r->GetDescr();
            default:
                ;       // fall thru to wxEmptyString
            }
        }

        return wxEmptyString;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        if( aRow < (int) size() )
        {
            LIB_TABLE_ROW* r  = at( (size_t) aRow );

            switch( aCol )
            {
            case COL_NICKNAME:  r->SetNickName( aValue );    break;
            case COL_URI:       r->SetFullURI( aValue );     break;
            case COL_TYPE:      r->SetType( aValue  );       break;
            case COL_OPTIONS:   r->SetOptions( aValue );     break;
            case COL_DESCR:     r->SetDescr( aValue );       break;
            }
        }
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
            wxGridTableMessage msg( this,
                                    wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                    aNumRows );

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
            LIB_TABLE_ROWS_ITER start = begin() + aPos;
            erase( start, start + aNumRows );

            if( GetView() )
            {
                wxGridTableMessage msg( this,
                                        wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                                        aPos,
                                        aNumRows );

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

        // keep this "Plugin Type" text fairly long so column is sized wide enough
        case COL_TYPE:      return _( "Plugin Type" );
        case COL_OPTIONS:   return _( "Options" );
        case COL_DESCR:     return _( "Description" );
        default:            return wxEmptyString;
        }
    }

protected:
    virtual LIB_TABLE_ROW* at( size_t aIndex ) = 0;

    virtual size_t size() const = 0;

    virtual LIB_TABLE_ROW* makeNewRow() = 0;

    virtual LIB_TABLE_ROWS_ITER begin() = 0;

    virtual LIB_TABLE_ROWS_ITER insert( LIB_TABLE_ROWS_ITER aIterator, LIB_TABLE_ROW* aRow ) = 0;

    virtual void push_back( LIB_TABLE_ROW* aRow ) = 0;

    virtual LIB_TABLE_ROWS_ITER erase( LIB_TABLE_ROWS_ITER aFirst, LIB_TABLE_ROWS_ITER aLast ) = 0;
};


#endif // __LIB_TABLE_GRID_H__
