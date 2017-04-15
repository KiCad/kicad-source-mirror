/**
 * @file kicad_clipboard.cpp
 * @brief Kicad clipboard plugin that piggybacks on the kicad_plugin
 *
 * @author Kristoffer Ödmark
 * @version 1.0
 * @date 2017-05-03
 *
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <wx/clipbrd.h>
#include <common.h>
#include <pcb_parser.h>
#include <class_netinfo.h>
#include <class_board.h>
#include <build_version.h>

#include <kicad_plugin.h>
#include <kicad_clipboard.h>

CLIPBOARD_IO::CLIPBOARD_IO():
    PCB_IO(),
    m_formatter(),
    m_parser( new CLIPBOARD_PARSER() )
{
    m_out = &m_formatter;
}

CLIPBOARD_IO::~CLIPBOARD_IO(){}

STRING_FORMATTER* CLIPBOARD_IO::GetFormatter()
{
    return &m_formatter;
}

void CLIPBOARD_IO::setBoard(BOARD* aBoard)
{
    m_board = aBoard;
}

void CLIPBOARD_IO::writeHeader(BOARD* aBoard)
{
    formatHeader( aBoard );
}


void CLIPBOARD_IO::SaveSelection( SELECTION& aSelected )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    // Prepare net mapping that assures that net codes saved in a file are consecutive integers
    m_mapping->SetBoard( m_board );

    // we will fake being a .kicad_pcb to get the full parser kicking
    // This means we also need layers and nets

    m_formatter.Print( 0, "(kicad_pcb (version %d) (host pcbnew %s)\n", SEXPR_BOARD_FILE_VERSION,
            m_formatter.Quotew( GetBuildVersion() ).c_str() );

    if( aSelected.Empty() )
        return;

    writeHeader( m_board );

    m_formatter.Print( 0, "\n" );

    for( auto i : aSelected )
    {

        // Dont format stuff that cannot exist standalone!
        if( ( i->Type() != PCB_MODULE_EDGE_T ) &&
                ( i->Type() != PCB_MODULE_TEXT_T ) &&
                ( i->Type() != PCB_PAD_T ) )
        {
            //std::cout <<"type "<< i->Type() << std::endl;
            auto item = static_cast<BOARD_ITEM*>( i );
            Format( item, 1 );
        }

    }
    m_formatter.Print( 0, "\n)" );


    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( wxString( m_formatter.GetString().c_str(), wxConvUTF8 ) ) );
        wxTheClipboard->Close();
    }
}

void CLIPBOARD_IO::Save( const wxString& aFileName, BOARD* aBoard,
                const PROPERTIES* aProperties )
{
    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    init( aProperties );

    m_board = aBoard;       // after init()

    // Prepare net mapping that assures that net codes saved in a file are consecutive integers
    m_mapping->SetBoard( aBoard );

    STRING_FORMATTER    formatter;

    m_out = &formatter;

    m_out->Print( 0, "(kicad_pcb (version %d) (host pcbnew %s)\n", SEXPR_BOARD_FILE_VERSION,
                  formatter.Quotew( GetBuildVersion() ).c_str() );

    Format( aBoard, 1 );

    m_out->Print( 0, ")\n" );

    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->SetData( new wxTextDataObject( wxString( m_formatter.GetString().c_str(), wxConvUTF8 ) ) );
        wxTheClipboard->Close();
    }

}

BOARD* CLIPBOARD_IO::Load( const wxString& aFileName, BOARD* aAppendToMe, const PROPERTIES* aProperties )
{
    std::string result;

    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
        {
            wxTextDataObject data;
            wxTheClipboard->GetData( data );

            result = data.GetText().mb_str();
        }

        wxTheClipboard->Close();
    }

    STRING_LINE_READER    reader(result, wxT( "clipboard" ) );

    init( aProperties );

    m_parser->SetLineReader( &reader );
    m_parser->SetBoard( aAppendToMe );

    BOARD* board;

    try
    {
        board = dynamic_cast<BOARD*>( m_parser->Parse() );
    }
    catch( const FUTURE_FORMAT_ERROR& )
    {
        // Don't wrap a FUTURE_FORMAT_ERROR in another
        throw;
    }
    catch( const PARSE_ERROR& parse_error )
    {
        if( m_parser->IsTooRecent() )
            throw FUTURE_FORMAT_ERROR( parse_error, m_parser->GetRequiredVersion() );
        else
            throw;
    }

    if( !board )
    {
        // The parser loaded something that was valid, but wasn't a board.
        THROW_PARSE_ERROR( _( "Clipboard content is not Kicad compatible" ),
                m_parser->CurSource(), m_parser->CurLine(),
                m_parser->CurLineNumber(), m_parser->CurOffset() );
    }

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        board->SetFileName( aFileName );

    return board;
}

