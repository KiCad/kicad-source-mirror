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


#include <invoke_pcb_dialog.h>
#include <dialog_fp_plugin_options_base.h>
#include <fp_lib_table.h>
#include <grid_tricks.h>

using std::string;

// re-enter the dialog with the column sizes preserved from last time.
static int col_width_option;
static int col_width_value;


class DIALOG_FP_PLUGIN_OPTIONS : public DIALOG_FP_PLUGIN_OPTIONS_BASE
{

public:
    DIALOG_FP_PLUGIN_OPTIONS( wxTopLevelWindow* aParent,
            const wxString& aNickname, const wxString& aOptions, wxString* aResult ) :
        DIALOG_FP_PLUGIN_OPTIONS_BASE( aParent ),
        m_callers_options( aOptions ),
        m_result( aResult )
    {
        wxString title = wxString::Format(
                _( "Options for Library '%s'" ), GetChars( aNickname ) );

        SetTitle( title );

        // add Cut, Copy, and Paste to wxGrids
        m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

        string options = TO_UTF8( aOptions );

        PROPERTIES* props = FP_LIB_TABLE::ParseOptions( options );

        if( props )
        {
            m_grid->AppendRows( props->size() );

            int row = 0;
            for( PROPERTIES::const_iterator it = props->begin();  it != props->end();  ++it, ++row )
            {
                m_grid->SetCellValue( row, 0, FROM_UTF8( it->first.c_str() ) );
                m_grid->SetCellValue( row, 1, FROM_UTF8( it->second.c_str() ) );
            }
        }

        if( !col_width_option )
        {
            m_grid->AutoSizeColumns( false );
        }
        else
        {
            m_grid->SetColSize( 0, col_width_option );
            m_grid->SetColSize( 1, col_width_value );
        }

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

    wxString makeResult()
    {
        PROPERTIES  props;
        const int   rowCount = m_grid->GetNumberRows();

        for( int row = 0;  row<rowCount;  ++row )
        {
            string  name  = TO_UTF8( m_grid->GetCellValue( row, 0 ).Trim( false ).Trim() );
            string  value = TO_UTF8( m_grid->GetCellValue( row, 1 ).Trim( false ).Trim() );

            if( name.size() )
            {
                props[name] = value;
            }
        }

        string  options = FP_LIB_TABLE::FormatOptions( &props );

        return FROM_UTF8( options.c_str() );
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

    //-----<event handlers>------------------------------------------------------
    void onAddRow( wxCommandEvent& event )
    {
    }

    void onDeleteRow( wxCommandEvent& event )
    {
    }

    void onMoveUp( wxCommandEvent& event )
    {
    }

    void onMoveDown( wxCommandEvent& event )
    {
    }

    void onCancelButtonClick( wxCommandEvent& event )
    {
        abort();
    }

    void onCancelButtonClick( wxCloseEvent& event )
    {
        abort();
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        saveColSizes();

        *m_result = makeResult();       // change from edits
        EndModal( 1 );
    }
    //-----</event handlers>-----------------------------------------------------
};


void InvokePluginOptionsEditor( wxTopLevelWindow* aCaller,
        const wxString& aNickname, const wxString& aOptions, wxString* aResult )
{
    DIALOG_FP_PLUGIN_OPTIONS    dlg( aCaller, aNickname, aOptions, aResult );

    dlg.ShowModal();
}
