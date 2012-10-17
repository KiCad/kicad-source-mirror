/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <dialog_fp_lib_table_base.h>
#include <fp_lib_table.h>
#include <wx/grid.h>
#include <wx/grid.h>


class FP_TBL_MODEL : public wxGridTableBase, public FP_LIB_TABLE
{
public:

    /**
     * Constructor FP_TBL_MODEL
     * builds a wxGridTableBase (table model) by wrapping an FP_LIB_TABLE.
     * @a aFallBackTable.  Loading of this table fragment is done by using Parse().
     *
     * @param aFallBackTable is another FP_LIB_TABLE which is searched only when
     *                       a record is not found in this table.  No ownership is
     *                       taken of aFallBackTable.
     */
    FP_TBL_MODEL( const FP_LIB_TABLE& aTableToEdit ) :
        FP_LIB_TABLE( aTableToEdit )    // copy constructor
    {
    }

    ~FP_TBL_MODEL()
    {
        D(printf("%s\n", __func__ );)
    }

    //-----<wxGridTableBase overloads>-------------------------------------------

    int         GetNumberRows () { return rows.size(); }
    int         GetNumberCols () { return 4; }

    wxString    GetValue( int aRow, int aCol )
    {
        if( unsigned( aRow ) < rows.size() )
        {
            const ROW&  r  = rows[aRow];

            switch( aCol )
            {
            case 0:     return r.GetNickName();
            case 1:     return r.GetFullURI();
            case 2:     return r.GetType();
            case 3:     return r.GetOptions();
            default:
                ;       // fall thru to wxEmptyString
            }
        }

        return wxEmptyString;
    }

    void    SetValue( int aRow, int aCol, const wxString &aValue )
    {
        if( unsigned( aRow ) < rows.size() )
        {
            ROW&  r  = rows[aRow];

            switch( aCol )
            {
            case 0:     r.SetNickName( aValue );    break;
            case 1:     r.SetType( aValue  );       break;
            case 2:     r.SetFullURI( aValue );     break;
            case 3:     r.SetOptions( aValue );     break;
            }
        }
    }

    bool IsEmptyCell( int aRow, int aCol )
    {
        if( unsigned( aRow ) < rows.size() )
            return false;
        return true;
    }

    bool InsertRows( size_t aPos = 0, size_t aNumRows = 1 )
    {
        if( aPos < rows.size() )
        {
            rows.insert( rows.begin() + aPos, aNumRows, ROW() );
            return true;
        }
        return false;
    }

    bool AppendRows( size_t aNumRows = 1 )
    {
        while( aNumRows-- )
            rows.push_back( ROW() );
        return true;
    }

    bool DeleteRows( size_t aPos, size_t aNumRows )
    {
        if( aPos + aNumRows <= rows.size() )
        {
            ROWS_ITER start = rows.begin() + aPos;
            rows.erase( start, start + aNumRows );
            return true;
        }
        return false;
    }

    void Clear()
    {
        rows.clear();
        nickIndex.clear();
    }

    wxString GetColLabelValue( int aCol )
    {
        switch( aCol )
        {
        case 0:     return _( "Nickname" );
        case 1:     return _( "Library Path" );
        case 2:     return _( "Plugin" );
        case 3:     return _( "Options" );
        default:    return wxEmptyString;
        }
    }

    //-----</wxGridTableBase overloads>------------------------------------------

};



/**
 * Class DIALOG_FP_LIB_TABLE
 * shows and edits the PCB library tables.  Two tables are expected, one global
 * and one project specific.
 */
class DIALOG_FP_LIB_TABLE : public DIALOG_FP_LIB_TABLE_BASE
{
    //-----<event handlers>----------------------------------

    void pageChangedHandler( wxAuiNotebookEvent& event )
    {
        int pageNdx = m_auinotebook->GetSelection();

        m_cur_grid = pageNdx ? m_global_grid : m_project_grid;
    }

    void appendRowHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void deleteRowHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void moveUpHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void moveDownHandler( wxMouseEvent& event )
    {
        D(printf("%s\n", __func__);)
    }

    void onCancelButtonClick( wxCommandEvent& event )
    {
        EndModal( wxID_CANCEL );
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        *m_global  = m_global_model;
        *m_project = m_project_model;

        // @todo reindex, or add member function for wholesale row replacement

        EndModal( wxID_OK );
    }


    //-----</event handlers>---------------------------------


    // caller's tables are modified only on OK button.
    FP_LIB_TABLE*       m_global;
    FP_LIB_TABLE*       m_project;

    // local copies which are edited, but aborted if Cancel button.
    FP_TBL_MODEL        m_global_model;
    FP_TBL_MODEL        m_project_model;

    wxGrid*             m_cur_grid;     ///< changed based on tab choice


public:
    DIALOG_FP_LIB_TABLE( wxFrame* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject ) :
        DIALOG_FP_LIB_TABLE_BASE( aParent ),
        m_global( aGlobal ),
        m_project( aProject ),
        m_global_model( *aGlobal ),
        m_project_model( *aProject )
    {
        /*
        GetSizer()->SetSizeHints( this );
        Centre();
        SetAutoLayout( true );
        Layout();
        */

#if 1 && defined(DEBUG)
        // put some dummy data into table(s)
        FP_LIB_TABLE::ROW   row;

        row.SetNickName( wxT( "passives" ) );
        row.SetType( wxT( "kicad" ) );
        row.SetFullURI( wxT( "%G/passives" ) );
        row.SetOptions( wxT( "speed=fast,purpose=testing" ) );
        m_global_model.InsertRow( row );

        row.SetNickName( wxT( "micros" ) );
        row.SetType( wxT( "legacy" ) );
        row.SetFullURI( wxT( "%P/micros" ) );
        row.SetOptions( wxT( "speed=fast,purpose=testing" ) );
        m_global_model.InsertRow( row );

        row.SetFullURI( wxT( "%P/chips" ) );
        m_project_model.InsertRow( row );

#endif

        m_global_grid->SetTable( (wxGridTableBase*) &m_global_model );
        m_project_grid->SetTable( (wxGridTableBase*) &m_project_model );

        //m_global_grid->AutoSize();
        m_global_grid->AutoSizeColumns( false );

        //m_project_grid->AutoSize();
        m_project_grid->AutoSizeColumns( false );

        //m_path_subs_grid->AutoSize();
        m_path_subs_grid->AutoSizeColumns( false );
    }

    ~DIALOG_FP_LIB_TABLE()
    {
        // Destroy the gui stuff first, with a goal of destroying the two wxGrids now,
        // since the ~wxGrid() wants the wxGridTableBase to still be non-destroyed.
        // Without this call, the wxGridTableBase objects are destroyed first
        // (i.e. destructor called) and there is a segfault since wxGridTableBase's vtable
        // is then no longer valid.
        DestroyChildren();
    }
};



int InvokePcbLibTableEditor( wxFrame* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject )
{
    DIALOG_FP_LIB_TABLE     dlg( aParent, aGlobal, aProject );

    int ret = dlg.ShowModal();
    switch( ret )
    {
    case wxID_OK:
        break;

    case wxID_CANCEL:
        break;
    }

    return 0;
}

