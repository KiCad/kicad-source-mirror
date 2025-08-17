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

#include <kicommon.h>
#include <libraries/library_table.h>
#include <wx/grid.h>

class LIB_TABLE_GRID_TRICKS;
class LIBRARY_MANAGER_ADAPTER;

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
class KICOMMON_API LIB_TABLE_GRID : public wxGridTableBase
{
    friend class LIB_TABLE_GRID_TRICKS;

public:
    LIB_TABLE_GRID( const LIBRARY_TABLE& aTableToEdit, LIBRARY_MANAGER_ADAPTER* aAdapter = nullptr ) :
        m_table( aTableToEdit ),
        m_adapter( aAdapter )
    {}

    //-----<wxGridTableBase overloads>-------------------------------------------

    int GetNumberRows() override { return (int) size(); }

    int GetNumberCols() override { return COL_COUNT; }

    wxString GetValue( int aRow, int aCol ) override;

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;

    bool GetValueAsBool( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;

    void SetValueAsBool( int aRow, int aCol, bool aValue ) override;

    bool IsEmptyCell( int aRow, int aCol ) override
    {
        return !GetValue( aRow, aCol );
    }

    bool InsertRows( size_t aPos = 0, size_t aNumRows = 1 ) override;

    bool AppendRows( size_t aNumRows = 1 ) override;

    bool DeleteRows( size_t aPos, size_t aNumRows ) override;

    wxString GetColLabelValue( int aCol ) override;

    bool ContainsNickname( const wxString& aNickname );

    LIBRARY_TABLE_ROW& At( size_t aIndex )
    {
        return at( aIndex );
    }

    LIBRARY_TABLE& Table() { return m_table; }

    LIBRARY_MANAGER_ADAPTER* Adapter() const { return m_adapter; }

protected:
    virtual LIBRARY_TABLE_ROW& at( size_t aIndex );

    virtual size_t size() const;

    virtual LIBRARY_TABLE_ROW makeNewRow();

    virtual LIBRARY_TABLE_ROWS_ITER begin();

    virtual LIBRARY_TABLE_ROWS_ITER insert( LIBRARY_TABLE_ROWS_ITER aIterator,
                                            const LIBRARY_TABLE_ROW& aRow );

    virtual void push_back( const LIBRARY_TABLE_ROW& aRow );

    virtual LIBRARY_TABLE_ROWS_ITER erase( LIBRARY_TABLE_ROWS_ITER aFirst,
                                           LIBRARY_TABLE_ROWS_ITER aLast );

    /// Working copy of a table
    LIBRARY_TABLE m_table;

    /// Handle to the adapter for the type of table this grid represents (may be null)
    LIBRARY_MANAGER_ADAPTER* m_adapter;
};


#endif // __LIB_TABLE_GRID_H__
