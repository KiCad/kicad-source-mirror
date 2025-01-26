/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <dialogs/dialog_plugin_options.h>
#include <grid_tricks.h>
#include <lib_table_base.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <bitmaps.h>
#include <string_utils.h>


#define INITIAL_HELP    \
    _(  "Select an <b>Option Choice</b> in the listbox above, and then click the " \
        "<b>Append Selected Option</b> button." )


DIALOG_PLUGIN_OPTIONS::DIALOG_PLUGIN_OPTIONS( wxWindow* aParent,
                                              const wxString& aNickname,
                                              const std::map<std::string, UTF8>& aPluginOptions,
                                              const wxString& aFormattedOptions,
                                              wxString* aResult ) :
        DIALOG_PLUGIN_OPTIONS_BASE( aParent ),
        m_callers_options( aFormattedOptions ),
        m_result( aResult ),
        m_choices( aPluginOptions ),
        m_initial_help( INITIAL_HELP ),
        m_grid_widths_dirty( true )
{
    SetTitle( wxString::Format( _( "Options for Library '%s'" ), aNickname ) );

    m_grid->SetSelectionMode( wxGrid::wxGridSelectionModes::wxGridSelectRows );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    // Option Choices Panel:
    if( m_choices.size() )
    {
        unsigned int row = 0;

        for( std::map<std::string, UTF8>::const_iterator it = m_choices.begin(); it != m_choices.end(); ++it, ++row )
        {
            wxString item = From_UTF8( it->first.c_str() );

            m_listbox->InsertItems( 1, &item, row );
        }
    }

    m_html->SetPage( m_initial_help );

    // Configure button logos
    m_append_button->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_delete_button->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    // initial focus on the grid please.
    SetInitialFocus( m_grid );

    SetupStandardButtons();
}


DIALOG_PLUGIN_OPTIONS ::~DIALOG_PLUGIN_OPTIONS()
{
    // destroy GRID_TRICKS before m_grid.
    m_grid->PopEventHandler( true );
}


bool DIALOG_PLUGIN_OPTIONS::TransferDataToWindow()
{
    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

    // Fill the grid with existing aOptions
    std::string options = TO_UTF8( m_callers_options );

    std::map<std::string, UTF8> props = LIB_TABLE::ParseOptions( options );

    if( !props.empty() )
    {
        if( props.size() > static_cast<size_t>( m_grid->GetNumberRows() ) )
            m_grid->AppendRows( props.size() - m_grid->GetNumberRows() );

        int row = 0;

        for( const auto& [key, value] : props )
        {
            m_grid->SetCellValue( row, 0, From_UTF8( key.c_str() ) );
            m_grid->SetCellValue( row, 1, value );
        }
    }

    return true;
}


bool DIALOG_PLUGIN_OPTIONS::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !DIALOG_SHIM::TransferDataFromWindow() )
        return false;

    std::map<std::string, UTF8> props;
    const int   rowCount = m_grid->GetNumberRows();

    for( int row = 0;  row<rowCount;  ++row )
    {
        std::string name = TO_UTF8( m_grid->GetCellValue( row, 0 ).Trim( false ).Trim() );
        UTF8        value = m_grid->GetCellValue( row, 1 ).Trim( false ).Trim();

        if( name.size() )
            props[name] = value;
    }

    *m_result = LIB_TABLE::FormatOptions( &props ).wx_str();
    return true;
}


int DIALOG_PLUGIN_OPTIONS::appendRow()
{
    m_grid->AppendRows( 1 );
    return m_grid->GetNumberRows() - 1;
}


int DIALOG_PLUGIN_OPTIONS::appendOption()
{
    int row = m_listbox->GetSelection();

    if( row != wxNOT_FOUND )
    {
        wxString option = m_listbox->GetString( row );

        for( row = 0; row < m_grid->GetNumberRows(); ++row )
        {
            wxString col0 = m_grid->GetCellValue( row, 0 );

            if( !col0 )     // empty col0
                break;
        }

        if( row == m_grid->GetNumberRows() )
            row = appendRow();

        m_grid->SetCellValue( row, 0, option );
        m_grid_widths_dirty = true;
    }

    return row;
}


//-----<event handlers>------------------------------------------------------

void DIALOG_PLUGIN_OPTIONS::onListBoxItemSelected( wxCommandEvent& event )
{
    // change the help text based on the m_listbox selection:
    if( event.IsSelection() )
    {
        std::string option = TO_UTF8( event.GetString() );

        if( auto it = m_choices.find( option ); it != m_choices.end() )
            m_html->SetPage( it->second );
        else
            m_html->SetPage( m_initial_help );
    }
}


void DIALOG_PLUGIN_OPTIONS::onListBoxItemDoubleClicked( wxCommandEvent& event )
{
    appendOption();
}


void DIALOG_PLUGIN_OPTIONS::onAppendOption( wxCommandEvent& )
{
    m_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                return { appendOption(), -1 };
            } );
}


void DIALOG_PLUGIN_OPTIONS::onAppendRow( wxCommandEvent& )
{
    m_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                return { appendRow(), 0 };
            } );
}


void DIALOG_PLUGIN_OPTIONS::onDeleteRow( wxCommandEvent& )
{
    m_grid->OnDeleteRows(
            [&]( int row )
            {
                m_grid->DeleteRows( row );
                m_grid_widths_dirty = true;
            } );
}


void DIALOG_PLUGIN_OPTIONS::onGridCellChange( wxGridEvent& aEvent )
{
    m_grid_widths_dirty = true;

    aEvent.Skip();
}


void DIALOG_PLUGIN_OPTIONS::onUpdateUI( wxUpdateUIEvent& )
{
    if( m_grid_widths_dirty && !m_grid->IsCellEditControlShown() )
    {
        int width = m_grid->GetClientRect().GetWidth();

        m_grid->AutoSizeColumn( 0 );
        m_grid->SetColSize( 0, std::max( 72, m_grid->GetColSize( 0 ) ) );

        m_grid->SetColSize( 1, std::max( 120, width - m_grid->GetColSize( 0 ) ) );

        m_grid_widths_dirty = false;
    }
}


void DIALOG_PLUGIN_OPTIONS::onSize( wxSizeEvent& aEvent )
{
    m_grid_widths_dirty = true;

    aEvent.Skip();
}
