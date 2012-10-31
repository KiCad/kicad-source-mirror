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


/**
 * Class FP_TBL_MODEL
 * mixes in wxGridTableBase into FP_LIB_TABLE so that the latter can be used
 * as a table within wxGrid.
 */
class FP_TBL_MODEL : public wxGridTableBase, public FP_LIB_TABLE
{
public:

    /**
     * Constructor FP_TBL_MODEL
     * is a copy constructor that builds a wxGridTableBase (table model) by wrapping
     * an FP_LIB_TABLE.
     */
    FP_TBL_MODEL( const FP_LIB_TABLE& aTableToEdit ) :
        FP_LIB_TABLE( aTableToEdit )    // copy constructor
    {
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
            case 1:     r.SetFullURI( aValue );     break;
            case 2:     r.SetType( aValue  );       break;
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

    bool AppendRows( size_t aNumRows = 1 )
    {
        // do not modify aNumRows, original value needed for wxGridTableMessage below
        for( int i = aNumRows; i; --i )
            rows.push_back( ROW() );

        if( GetView() )
        {
            wxGridTableMessage msg( this,
                                    wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                    aNumRows );

            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    bool DeleteRows( size_t aPos, size_t aNumRows )
    {
        if( aPos + aNumRows <= rows.size() )
        {
            ROWS_ITER start = rows.begin() + aPos;
            rows.erase( start, start + aNumRows );

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
    typedef FP_LIB_TABLE::ROW   ROW;

    /* row & col "selection" acquisition, not currently used but works.
    // selected area by cell coordinate and count
    int selRowStart;
    int selColStart;
    int selRowCount;
    int selColCount;

    /// Gets the selected area into a sensible rectable of sel{Row,Col}{Start,Count} above.
    void getSelectedArea()
    {
        wxGridCellCoordsArray topLeft  = m_cur_grid->GetSelectionBlockTopLeft();
        wxGridCellCoordsArray botRight = m_cur_grid->GetSelectionBlockBottomRight();

        if( topLeft.Count() && botRight.Count() )
        {
            selRowStart = topLeft[0].GetRow();
            selColStart = topLeft[0].GetCol();

            selRowCount = botRight[0].GetRow() - selRowStart + 1;
            selColCount = botRight[0].GetCol() - selColStart + 1;
        }
        else
        {
            selRowStart = -1;
            selColStart = -1;
            selRowCount = 0;
            selColCount = 0;
        }

        D(printf("selRowStart:%d selColStart:%d selRowCount:%d selColCount:%d\n",
            selRowStart, selColStart, selRowCount, selColCount );)
    }
    */

    //-----<event handlers>----------------------------------

    void pageChangedHandler( wxAuiNotebookEvent& event )
    {
        int pageNdx = m_auinotebook->GetSelection();
        m_cur_grid = pageNdx==0 ? m_global_grid : m_project_grid;

        D(printf("%s cur_grid is %s\n", __func__, pageNdx==0 ? "global" : "project" );)
    }

    void appendRowHandler( wxMouseEvent& event )
    {
        m_cur_grid->AppendRows( 1 );
    }

    void deleteRowHandler( wxMouseEvent& event )
    {
        int curRow = m_cur_grid->GetGridCursorRow();
        m_cur_grid->DeleteRows( curRow );
    }

    void moveUpHandler( wxMouseEvent& event )
    {
        int curRow = m_cur_grid->GetGridCursorRow();
        if( curRow >= 1 )
        {
            int curCol = m_cur_grid->GetGridCursorCol();

            FP_TBL_MODEL* tbl = (FP_TBL_MODEL*) m_cur_grid->GetTable();

            ROW move_me = tbl->rows[curRow];

            tbl->rows.erase( tbl->rows.begin() + curRow );
            --curRow;
            tbl->rows.insert( tbl->rows.begin() + curRow, move_me );

            if( tbl->GetView() )
            {
                // fire a msg to cause redrawing
                wxGridTableMessage msg( tbl,
                                        wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                        curRow,
                                        0 );

                tbl->GetView()->ProcessTableMessage( msg );
            }

            m_cur_grid->SetGridCursor( curRow, curCol );
        }
    }

    void moveDownHandler( wxMouseEvent& event )
    {
        FP_TBL_MODEL* tbl = (FP_TBL_MODEL*) m_cur_grid->GetTable();

        int curRow = m_cur_grid->GetGridCursorRow();
        if( unsigned( curRow + 1 ) < tbl->rows.size() )
        {
            int curCol  = m_cur_grid->GetGridCursorCol();

            ROW move_me = tbl->rows[curRow];

            tbl->rows.erase( tbl->rows.begin() + curRow );
             ++curRow;
            tbl->rows.insert( tbl->rows.begin() + curRow, move_me );

            if( tbl->GetView() )
            {
                // fire a msg to cause redrawing
                wxGridTableMessage msg( tbl,
                                        wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                        curRow - 1,
                                        0 );

                tbl->GetView()->ProcessTableMessage( msg );
            }

            m_cur_grid->SetGridCursor( curRow, curCol );
        }
        D(printf("%s\n", __func__);)
    }

    void onCancelButtonClick( wxCommandEvent& event )
    {
        EndModal( 0 );
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        int dialogRet = 0;

        if( m_global_model != *m_global )
        {
            dialogRet |= 1;

            *m_global  = m_global_model;
            m_global->reindex();
        }

        if( m_project_model != *m_project )
        {
            dialogRet |= 2;

            *m_project = m_project_model;
            m_project->reindex();
        }

        EndModal( dialogRet );
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
        m_global_grid->SetTable( (wxGridTableBase*) &m_global_model );
        m_project_grid->SetTable( (wxGridTableBase*) &m_project_model );

        m_global_grid->AutoSizeColumns( false );

        m_project_grid->AutoSizeColumns( false );

        m_path_subs_grid->AutoSizeColumns( false );

        // fire pageChangedHandler() so m_cur_grid gets set
        wxAuiNotebookEvent uneventful;
        pageChangedHandler( uneventful );
    }

    ~DIALOG_FP_LIB_TABLE()
    {
        // Destroy the gui stuff first, with a goal of destroying the two wxGrids now,
        // since the ~wxGrid() wants the wxGridTableBase to still be non-destroyed.
        // Without this call, the wxGridTableBase objects are destroyed first
        // (i.e. destructor called) and there is a segfault since wxGridTableBase's vtable
        // is then no longer valid.  If ~wxGrid() would not examine a wxGridTableBase that
        // it does not own, then this would not be a concern.  But it is, since it does.
        DestroyChildren();
    }
};



int InvokePcbLibTableEditor( wxFrame* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject )
{
    DIALOG_FP_LIB_TABLE dlg( aParent, aGlobal, aProject );

    int dialogRet = dlg.ShowModal();    // returns value passed to EndModal() above

    return dialogRet;
}

