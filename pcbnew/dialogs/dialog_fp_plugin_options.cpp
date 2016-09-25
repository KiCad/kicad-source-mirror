/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 CERN
 * Copyright (C) 2013 KiCad Developers, see change_log.txt for contributors.
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
#include <invoke_pcb_dialog.h>
#include <dialog_fp_plugin_options_base.h>
#include <fp_lib_table.h>
#include <grid_tricks.h>


#define INITIAL_HELP    \
    _(  "Select an <b>Option Choice</b> in the listbox above, and then click the <b>Append Selected Option</b> button." )


using std::string;

// re-enter the dialog with the column sizes preserved from last time.
static int col_width_option;
static int col_width_value;


/**
 * Class DIALOG_FP_PLUGIN_OPTIONS
 * is an options editor in the form of a two column name/value
 * spreadsheet like (table) UI.  It takes hints from a pcbnew PLUGIN as to
 * supported options.
 */
class DIALOG_FP_PLUGIN_OPTIONS : public DIALOG_FP_PLUGIN_OPTIONS_BASE
{

public:
    DIALOG_FP_PLUGIN_OPTIONS( wxTopLevelWindow* aParent,
            const wxString& aNickname, const wxString& aPluginType,
            const wxString& aOptions, wxString* aResult ) :
        DIALOG_FP_PLUGIN_OPTIONS_BASE( aParent ),
        m_callers_options( aOptions ),
        m_result( aResult ),
        m_initial_help( INITIAL_HELP )
    {
        wxString title = wxString::Format(
                _( "Options for Library '%s'" ), GetChars( aNickname ) );

        SetTitle( title );

        // add Cut, Copy, and Paste to wxGrid
        m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

        m_grid->SetColMinimalWidth( 1, 250 );

        // Fill the grid with existing aOptions
        string options = TO_UTF8( aOptions );

        PROPERTIES* props = FP_LIB_TABLE::ParseOptions( options );

        if( props )
        {
            if( (int) props->size() > m_grid->GetNumberRows() )
                m_grid->AppendRows( props->size() - m_grid->GetNumberRows() );

            int row = 0;
            for( PROPERTIES::const_iterator it = props->begin();  it != props->end();  ++it, ++row )
            {
                m_grid->SetCellValue( row, 0, FROM_UTF8( it->first.c_str() ) );
                m_grid->SetCellValue( row, 1, it->second );
            }

            delete props;
        }

        // Option Choices Panel:

        IO_MGR::PCB_FILE_T  pi_type = IO_MGR::EnumFromStr( aPluginType );
        PLUGIN::RELEASER    pi( IO_MGR::PluginFind( pi_type ) );

        pi->FootprintLibOptions( &m_choices );

        if( m_choices.size() )
        {
            int row = 0;
            for( PROPERTIES::const_iterator it = m_choices.begin();  it != m_choices.end();  ++it, ++row )
            {
                wxString item = FROM_UTF8( it->first.c_str() );

                m_listbox->InsertItems( 1, &item, row );
            }
        }

        m_html->SetPage( m_initial_help );

        if( !col_width_option )
        {
            m_grid->AutoSizeColumns( false );
        }
        else
        {
            m_grid->SetColSize( 0, col_width_option );
            m_grid->SetColSize( 1, col_width_value );
        }

        Fit();

        // initial focus on the grid please.
        m_grid->SetFocus();
    }

    ~DIALOG_FP_PLUGIN_OPTIONS()
    {
        // destroy GRID_TRICKS before m_grid.
        m_grid->PopEventHandler( true );
    }


private:
    const wxString& m_callers_options;
    wxString*       m_result;
    PROPERTIES      m_choices;
    wxString        m_initial_help;


    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based column index.
    int getCursorCol() const
    {
        return m_grid->GetGridCursorCol();
    }

    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based row index.
    int getCursorRow() const
    {
        return m_grid->GetGridCursorRow();
    }

    wxArrayString getRow( int aRow )
    {
        wxArrayString row;

        const int col_count = m_grid->GetNumberCols();
        for( int col = 0;  col < col_count;  ++col )
        {
            row.Add( m_grid->GetCellValue( aRow, col ) );
        }

        return row;
    }

    void setRow( int aRow, const wxArrayString& aPair )
    {
        const int col_count = m_grid->GetNumberCols();
        for( int col = 0;  col < col_count;  ++col )
        {
            m_grid->SetCellValue( aRow, col, aPair[col] );
        }
    }

    wxString makeResult()
    {
        PROPERTIES  props;
        const int   rowCount = m_grid->GetNumberRows();

        for( int row = 0;  row<rowCount;  ++row )
        {
            string  name  = TO_UTF8( m_grid->GetCellValue( row, 0 ).Trim( false ).Trim() );
            UTF8    value = m_grid->GetCellValue( row, 1 ).Trim( false ).Trim();

            if( name.size() )
            {
                props[name] = value;
            }
        }

        return FP_LIB_TABLE::FormatOptions( &props );
    }

    void saveColSizes()
    {
        col_width_option = m_grid->GetColSize( 0 );
        col_width_value  = m_grid->GetColSize( 1 );
    }

    void abort()
    {
        saveColSizes();

        *m_result = m_callers_options;      // tell caller "no change"
        EndModal( 0 );
    }

    int appendRow()
    {
        if( m_grid->AppendRows( 1 ) )
        {
            int last_row = m_grid->GetNumberRows() - 1;

            // wx documentation is wrong, SetGridCursor does not make visible.
            m_grid->MakeCellVisible( last_row, 0 );
            m_grid->SetGridCursor( last_row, 0 );

            return last_row;
        }

        return -1;
    }

    void appendOption()
    {
        int selected_row = m_listbox->GetSelection();
        if( selected_row != wxNOT_FOUND )
        {
            wxString    option = m_listbox->GetString( selected_row );

            int row_count = m_grid->GetNumberRows();
            int row;

            for( row=0;  row<row_count;  ++row )
            {
                wxString col0 = m_grid->GetCellValue( row, 0 );

                if( !col0 )     // empty col0
                    break;
            }

            if( row == row_count )
                row = appendRow();

            m_grid->SetCellValue( row, 0, option );
            m_grid->AutoSizeColumns( false );
        }
    }

    //-----<event handlers>------------------------------------------------------

    void onListBoxItemSelected( wxCommandEvent& event ) override
    {
        // change the help text based on the m_listbox selection:
        if( event.IsSelection() )
        {
            string  option = TO_UTF8( event.GetString() );
            UTF8    help_text;

            if( m_choices.Value( option.c_str(), &help_text ) )
            {
                wxString page = help_text;

                m_html->SetPage( page );
            }
            else
            {
                m_html->SetPage( m_initial_help );
            }
        }
    }

    void onListBoxItemDoubleClicked( wxCommandEvent& event ) override
    {
        appendOption();
    }

    void onAppendOption( wxCommandEvent& event ) override
    {
        appendOption();
    }

    void onAppendRow( wxMouseEvent& event ) override
    {
        appendRow();
    }

    void onDeleteRow( wxMouseEvent& event ) override
    {
        int rowCount = m_grid->GetNumberRows();
        int curRow   = getCursorRow();

        m_grid->DeleteRows( curRow );

        if( curRow && curRow == rowCount - 1 )
        {
            m_grid->MakeCellVisible( curRow-1, getCursorCol() );
            m_grid->SetGridCursor( curRow-1, getCursorCol() );
        }
    }

    void onMoveUp( wxMouseEvent& event ) override
    {
        int curRow = getCursorRow();
        if( curRow >= 1 )
        {
            int curCol = getCursorCol();

            wxArrayString move_me = getRow( curRow );

            m_grid->DeleteRows( curRow );
            --curRow;
            m_grid->InsertRows( curRow );

            setRow( curRow, move_me );

            wxGridTableBase* tbl = m_grid->GetTable();

            if( tbl->GetView() )
            {
                // fire a msg to cause redrawing
                wxGridTableMessage msg( tbl,
                                        wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                        curRow,
                                        0 );

                tbl->GetView()->ProcessTableMessage( msg );
            }

            m_grid->MakeCellVisible( curRow, curCol );
            m_grid->SetGridCursor( curRow, curCol );
        }
    }

    void onMoveDown( wxMouseEvent& event ) override
    {
        int curRow = getCursorRow();
        if( curRow + 1 < m_grid->GetNumberRows() )
        {
            int curCol  = getCursorCol();

            wxArrayString move_me = getRow( curRow );

            m_grid->DeleteRows( curRow );
             ++curRow;
            m_grid->InsertRows( curRow );
            setRow( curRow, move_me );

            wxGridTableBase* tbl = m_grid->GetTable();

            if( tbl->GetView() )
            {
                // fire a msg to cause redrawing
                wxGridTableMessage msg( tbl,
                                        wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                        curRow - 1,
                                        0 );

                tbl->GetView()->ProcessTableMessage( msg );
            }

            m_grid->MakeCellVisible( curRow, curCol );
            m_grid->SetGridCursor( curRow, curCol );
        }
    }

    void onCancelButtonClick( wxCommandEvent& event ) override
    {
        abort();
    }

    void onCancelCaptionButtonClick( wxCloseEvent& event ) override
    {
        abort();
    }

    void onOKButtonClick( wxCommandEvent& event ) override
    {
        saveColSizes();

        *m_result = makeResult();       // change from edits
        EndModal( 1 );
    }
    //-----</event handlers>-----------------------------------------------------
};


void InvokePluginOptionsEditor( wxTopLevelWindow* aCaller,
        const wxString& aNickname, const wxString& aPluginType,
        const wxString& aOptions, wxString* aResult )
{
    DIALOG_FP_PLUGIN_OPTIONS    dlg( aCaller, aNickname, aPluginType, aOptions, aResult );

    dlg.ShowModal();
}
