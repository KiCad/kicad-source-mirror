/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 CERN
 * Copyright (C) 2013-2016 KiCad Developers, see change_log.txt for contributors.
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
#include <widgets/wx_grid.h>
#include <bitmaps.h>
#include <macros.h>


#define INITIAL_HELP    \
    _(  "Select an <b>Option Choice</b> in the listbox above, and then click the <b>Append Selected Option</b> button." )


using std::string;


/**
 * DIALOG_FP_PLUGIN_OPTIONS
 * is an options editor in the form of a two column name/value
 * spreadsheet like (table) UI.  It takes hints from a pcbnew PLUGIN as to
 * supported options.
 */
class DIALOG_FP_PLUGIN_OPTIONS : public DIALOG_FP_PLUGIN_OPTIONS_BASE
{

public:
    DIALOG_FP_PLUGIN_OPTIONS( wxWindow* aParent, const wxString& aNickname,
                              const wxString& aPluginType, const wxString& aOptions,
                              wxString* aResult ) :
        DIALOG_FP_PLUGIN_OPTIONS_BASE( aParent ),
        m_callers_options( aOptions ),
        m_result( aResult ),
        m_initial_help( INITIAL_HELP ),
        m_grid_widths_dirty( true )
    {
        SetTitle( wxString::Format( _( "Options for Library \"%s\"" ), aNickname ) );

        // Give a bit more room for combobox editors
        m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

        m_grid->SetSelectionMode( wxGrid::wxGridSelectionModes::wxGridSelectRows );

        // add Cut, Copy, and Paste to wxGrid
        m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

        // Option Choices Panel:

        IO_MGR::PCB_FILE_T  pi_type = IO_MGR::EnumFromStr( aPluginType );
        PLUGIN::RELEASER    pi( IO_MGR::PluginFind( pi_type ) );

        pi->FootprintLibOptions( &m_choices );

        if( m_choices.size() )
        {
            unsigned int row = 0;
            for( PROPERTIES::const_iterator it = m_choices.begin();  it != m_choices.end();  ++it, ++row )
            {
                wxString item = FROM_UTF8( it->first.c_str() );

                m_listbox->InsertItems( 1, &item, row );
            }
        }

        m_html->SetPage( m_initial_help );

        // Configure button logos
        m_append_button->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
        m_delete_button->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

        // initial focus on the grid please.
        SetInitialFocus( m_grid );

        m_sdbSizer1OK->SetDefault();
    }

    ~DIALOG_FP_PLUGIN_OPTIONS() override
    {
        // destroy GRID_TRICKS before m_grid.
        m_grid->PopEventHandler( true );
    }

    bool TransferDataToWindow() override
    {
        if( !DIALOG_SHIM::TransferDataToWindow() )
            return false;

        // Fill the grid with existing aOptions
        string options = TO_UTF8( m_callers_options );

        PROPERTIES* props = LIB_TABLE::ParseOptions( options );

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

        return true;
    }

    bool TransferDataFromWindow() override
    {
        if( !m_grid->CommitPendingChanges() )
            return false;

        if( !DIALOG_SHIM::TransferDataFromWindow() )
            return false;

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

        *m_result =  LIB_TABLE::FormatOptions( &props );
        return true;
    }

private:
    const wxString& m_callers_options;
    wxString*       m_result;
    PROPERTIES      m_choices;
    wxString        m_initial_help;
    bool            m_grid_widths_dirty;

    int appendRow()
    {
        int row = m_grid->GetNumberRows();

        m_grid->AppendRows( 1 );

        // wx documentation is wrong, SetGridCursor does not make visible.
        m_grid->MakeCellVisible( row, 0 );
        m_grid->SetGridCursor( row, 0 );

        return row;
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
            m_grid_widths_dirty = true;
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
                m_html->SetPage( help_text );
            else
                m_html->SetPage( m_initial_help );
        }
    }

    void onListBoxItemDoubleClicked( wxCommandEvent& event ) override
    {
        appendOption();
    }

    void onAppendOption( wxCommandEvent&  ) override
    {
        if( !m_grid->CommitPendingChanges() )
            return;

        appendOption();
    }

    void onAppendRow( wxCommandEvent&  ) override
    {
        if( !m_grid->CommitPendingChanges() )
            return;

        appendRow();
    }

    void onDeleteRow( wxCommandEvent&  ) override
    {
        if( !m_grid->CommitPendingChanges() )
            return;

        int curRow   = m_grid->GetGridCursorRow();

        m_grid->DeleteRows( curRow );
        m_grid_widths_dirty = true;

        curRow = std::max( 0, curRow - 1 );
        m_grid->MakeCellVisible( curRow, m_grid->GetGridCursorCol() );
        m_grid->SetGridCursor( curRow, m_grid->GetGridCursorCol() );
    }

    void onGridCellChange( wxGridEvent& aEvent ) override
    {
        m_grid_widths_dirty = true;

        aEvent.Skip();
    }

    void onUpdateUI( wxUpdateUIEvent&  ) override
    {
        if( m_grid_widths_dirty && !m_grid->IsCellEditControlShown() )
        {
            int width = m_grid->GetClientRect().GetWidth();

            m_grid->AutoSizeColumn( 0 );
            m_grid->SetColSize( 0, std::max( 120, m_grid->GetColSize( 0 ) ) );

            m_grid->SetColSize( 1, width - m_grid->GetColSize( 0 ) );

            m_grid_widths_dirty = false;
        }
    }

    void onSize( wxSizeEvent& aEvent ) override
    {
        m_grid_widths_dirty = true;

        aEvent.Skip();
    }
};


void InvokePluginOptionsEditor( wxWindow* aCaller, const wxString& aNickname,
                                const wxString& aPluginType, const wxString& aOptions,
                                wxString* aResult )
{
    DIALOG_FP_PLUGIN_OPTIONS dlg( aCaller, aNickname, aPluginType, aOptions, aResult );

    dlg.ShowModal();
}
