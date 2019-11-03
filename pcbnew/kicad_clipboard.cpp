/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see CHANGELOG.TXT for contributors.
 * @author Kristoffer Ödmark
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
#include <netinfo.h>
#include <class_board.h>
#include <build_version.h>

#include <kicad_plugin.h>
#include <kicad_clipboard.h>

CLIPBOARD_IO::CLIPBOARD_IO():
    PCB_IO( CTL_STD_LAYER_NAMES ),
    m_formatter(),
    m_parser( new CLIPBOARD_PARSER() )
{
    m_out = &m_formatter;
}


CLIPBOARD_IO::~CLIPBOARD_IO()
{
    delete m_parser;
}


STRING_FORMATTER* CLIPBOARD_IO::GetFormatter()
{
    return &m_formatter;
}


void CLIPBOARD_IO::SetBoard( BOARD* aBoard )
{
    m_board = aBoard;
}


void CLIPBOARD_IO::SaveSelection( const PCBNEW_SELECTION& aSelected )
{
    VECTOR2I refPoint( 0, 0 );

    // dont even start if the selection is empty
    if( aSelected.Empty() )
        return;

    if( aSelected.HasReferencePoint() )
        refPoint = aSelected.GetReferencePoint();

    // Prepare net mapping that assures that net codes saved in a file are consecutive integers
    m_mapping->SetBoard( m_board );

    // Differentiate how it is formatted depending on what selection contains
    bool onlyModuleParts = true;
    for( const auto i : aSelected )
    {
        // check if it not one of the module primitives
        if( ( i->Type() != PCB_MODULE_EDGE_T ) &&
                ( i->Type() != PCB_MODULE_TEXT_T ) &&
                ( i->Type() != PCB_PAD_T ) )
        {
            onlyModuleParts = false;
            break;
        }
    }

    // if there is only parts of a module selected, format it as a new module else
    // format it as an entire board
    MODULE partialModule( m_board );

    // only a module selected.
    if( aSelected.Size() == 1 && aSelected.Front()->Type() == PCB_MODULE_T )
    {
        // make the module safe to transfer to other pcbs
        const MODULE* mod = static_cast<MODULE*>( aSelected.Front() );
        // Do not modify existing board
        MODULE newModule( *mod );

        for( D_PAD* pad : newModule.Pads() )
            pad->SetNetCode( 0 );

        // locked means "locked in place"; copied items therefore can't be locked
        newModule.SetLocked( false );

        // locate the reference point at (0, 0) in the copied items
        newModule.Move( wxPoint( -refPoint.x, -refPoint.y ) );

        Format( static_cast<BOARD_ITEM*>( &newModule ) );
    }
    // partial module selected.
    else if( onlyModuleParts )
    {
        for( const auto item : aSelected )
        {
            BOARD_ITEM* clone = static_cast<BOARD_ITEM*>( item->Clone() );

            // Do not add reference/value - convert them to the common type
            if( TEXTE_MODULE* text = dyn_cast<TEXTE_MODULE*>( clone ) )
                text->SetType( TEXTE_MODULE::TEXT_is_DIVERS );

            // If it is only a module, clear the nets from the pads
            if( D_PAD* pad = dyn_cast<D_PAD*>( clone ) )
               pad->SetNetCode( 0 );

            // Add the pad to the new module before moving to ensure the local coords are correct
            partialModule.Add( clone );

            // locate the reference point at (0, 0) in the copied items
            clone->Move( (wxPoint) -refPoint );
        }

        // Set the new relative internal local coordinates of copied items
        MODULE* editedModule = m_board->Modules().front();
        wxPoint moveVector = partialModule.GetPosition() + editedModule->GetPosition();

        partialModule.MoveAnchorPosition( moveVector );

        Format( &partialModule, 0 );

    }
    // lots of stuff selected
    else
    {
        // we will fake being a .kicad_pcb to get the full parser kicking
        // This means we also need layers and nets
        LOCALE_IO io;

        m_formatter.Print( 0, "(kicad_pcb (version %d) (host pcbnew %s)\n",
                SEXPR_BOARD_FILE_VERSION, m_formatter.Quotew( GetBuildVersion() ).c_str() );


        m_formatter.Print( 0, "\n" );

        formatBoardLayers( m_board );
        formatNetInformation( m_board );

        m_formatter.Print( 0, "\n" );


        for( const auto i : aSelected )
        {
            // Dont format stuff that cannot exist standalone!
            if( ( i->Type() != PCB_MODULE_EDGE_T ) &&
                ( i->Type() != PCB_MODULE_TEXT_T ) &&
                ( i->Type() != PCB_PAD_T ) )
            {
                auto item = static_cast<BOARD_ITEM*>( i );
                std::unique_ptr<BOARD_ITEM> clone( static_cast<BOARD_ITEM*> ( item->Clone() ) );

                // locked means "locked in place"; copied items therefore can't be locked
                if( MODULE* module = dyn_cast<MODULE*>( clone.get() ) )
                    module->SetLocked( false );
                else if( TRACK* track = dyn_cast<TRACK*>( clone.get() ) )
                    track->SetLocked( false );

                // locate the reference point at (0, 0) in the copied items
                clone->Move( (wxPoint) -refPoint );

                Format( clone.get(), 1 );
            }
        }
        m_formatter.Print( 0, "\n)" );
    }

    // These are placed at the end to minimize the open time of the clipboard
    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock || !clipboard->IsOpened() )
        return;

    clipboard->SetData( new wxTextDataObject(
                wxString( m_formatter.GetString().c_str(), wxConvUTF8 ) ) );

    clipboard->Flush();

    // This section exists to return the clipboard data, ensuring it has fully
    // been processed by the system clipboard.  This appears to be needed for
    // extremely large clipboard copies on asynchronous linux clipboard managers
    // such as KDE's Klipper
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        ( void )data.GetText(); // Keep unused variable
    }
}


BOARD_ITEM* CLIPBOARD_IO::Parse()
{
    BOARD_ITEM* item;
    wxString result;

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock )
        return nullptr;


    if( clipboard->IsSupported( wxDF_TEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        result = data.GetText();
    }

    try
    {
        item = PCB_IO::Parse( result );
    }
    catch (...)
    {
        item = nullptr;
    }

    return item;
}


void CLIPBOARD_IO::Save( const wxString& aFileName, BOARD* aBoard,
                const PROPERTIES* aProperties )
{
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

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock )
        return;

    clipboard->SetData( new wxTextDataObject(
                wxString( m_formatter.GetString().c_str(), wxConvUTF8 ) ) );
    clipboard->Flush();

    // This section exists to return the clipboard data, ensuring it has fully
    // been processed by the system clipboard.  This appears to be needed for
    // extremely large clipboard copies on asynchronous linux clipboard managers
    // such as KDE's Klipper
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        ( void )data.GetText(); // Keep unused variable
    }
}


BOARD* CLIPBOARD_IO::Load( const wxString& aFileName,
        BOARD* aAppendToMe, const PROPERTIES* aProperties )
{
    std::string result;

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock )
        return nullptr;

    if( clipboard->IsSupported( wxDF_TEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );

        result = data.GetText().mb_str();
    }

    STRING_LINE_READER    reader(result, wxT( "clipboard" ) );

    init( aProperties );

    m_parser->SetLineReader( &reader );
    m_parser->SetBoard( aAppendToMe );

    BOARD_ITEM* item;
    BOARD* board;

    try
    {
        item =  m_parser->Parse();
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

    if( item->Type() != PCB_T )
    {
        // The parser loaded something that was valid, but wasn't a board.
        THROW_PARSE_ERROR( _( "Clipboard content is not KiCad compatible" ),
                m_parser->CurSource(), m_parser->CurLine(),
                m_parser->CurLineNumber(), m_parser->CurOffset() );
    }
    else
    {
        board = dynamic_cast<BOARD*>( item );
    }

    // Give the filename to the board if it's new
    if( board && !aAppendToMe )
        board->SetFileName( aFileName );

    return board;
}
