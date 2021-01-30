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

#include <build_version.h>
#include <board.h>
#include <track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <fp_text.h>
#include <locale_io.h>
#include <netinfo.h>
#include <plugins/kicad/pcb_parser.h>

#include <plugins/kicad/kicad_plugin.h>
#include <kicad_clipboard.h>

CLIPBOARD_IO::CLIPBOARD_IO():
    PCB_IO( CTL_FOR_CLIPBOARD ),
    m_formatter()
{
    m_out = &m_formatter;
}


CLIPBOARD_IO::~CLIPBOARD_IO()
{
}


void CLIPBOARD_IO::SetBoard( BOARD* aBoard )
{
    m_board = aBoard;
}


void CLIPBOARD_IO::SaveSelection( const PCB_SELECTION& aSelected, bool isFootprintEditor )
{
    VECTOR2I refPoint( 0, 0 );

    // dont even start if the selection is empty
    if( aSelected.Empty() )
        return;

    if( aSelected.HasReferencePoint() )
        refPoint = aSelected.GetReferencePoint();

    // Prepare net mapping that assures that net codes saved in a file are consecutive integers
    m_mapping->SetBoard( m_board );

    if( aSelected.Size() == 1 && aSelected.Front()->Type() == PCB_FOOTPRINT_T )
    {
        // make the footprint safe to transfer to other pcbs
        const FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aSelected.Front() );
        // Do not modify existing board
        FOOTPRINT newFootprint( *footprint );

        for( PAD* pad : newFootprint.Pads() )
            pad->SetNetCode( 0 );

        // locked means "locked in place"; copied items therefore can't be locked
        newFootprint.SetLocked( false );

        // locate the reference point at (0, 0) in the copied items
        newFootprint.Move( wxPoint( -refPoint.x, -refPoint.y ) );

        Format( static_cast<BOARD_ITEM*>( &newFootprint ) );
    }
    else if( isFootprintEditor )
    {
        FOOTPRINT partialFootprint( m_board );

        // Usefull to copy the selection to the board editor (if any), and provides
        // a dummy lib id.
        // Perhaps not a good Id, but better than a empty id
        KIID dummy;
        LIB_ID id( "clipboard", dummy.AsString() );
        partialFootprint.SetFPID( id );

        for( const EDA_ITEM* item : aSelected )
        {
            const PCB_GROUP* group = dynamic_cast<const PCB_GROUP*>( item );
            BOARD_ITEM*      clone;

            if( group )
                clone = static_cast<BOARD_ITEM*>( group->DeepClone() );
            else
                clone = static_cast<BOARD_ITEM*>( item->Clone() );

            // If it is only a footprint, clear the nets from the pads
            if( PAD* pad = dyn_cast<PAD*>( clone ) )
               pad->SetNetCode( 0 );

            // Add the pad to the new footprint before moving to ensure the local coords are
            // correct
            partialFootprint.Add( clone );

            if( group )
            {
                static_cast<PCB_GROUP*>( clone )->RunOnDescendants(
                        [&]( BOARD_ITEM* descendant )
                        {
                            partialFootprint.Add( descendant );
                        } );
            }

            // locate the reference point at (0, 0) in the copied items
            clone->Move( (wxPoint) -refPoint );
        }

        // Set the new relative internal local coordinates of copied items
        FOOTPRINT* editedFootprint = m_board->Footprints().front();
        wxPoint    moveVector = partialFootprint.GetPosition() + editedFootprint->GetPosition();

        partialFootprint.MoveAnchorPosition( moveVector );

        Format( &partialFootprint, 0 );
    }
    else
    {
        // we will fake being a .kicad_pcb to get the full parser kicking
        // This means we also need layers and nets
        LOCALE_IO io;

        m_formatter.Print( 0, "(kicad_pcb (version %d) (generator pcbnew)\n",
                           SEXPR_BOARD_FILE_VERSION );

        m_formatter.Print( 0, "\n" );

        formatBoardLayers( m_board );
        formatNetInformation( m_board );

        m_formatter.Print( 0, "\n" );

        for( EDA_ITEM* i : aSelected )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( i );
            BOARD_ITEM* copy = nullptr;

            if( item->Type() == PCB_FP_SHAPE_T )
            {
                // Convert to PCB_SHAPE_T
                copy = (BOARD_ITEM*) reinterpret_cast<PCB_SHAPE*>( item )->Clone();
                copy->SetLayer( item->GetLayer() );
            }
            else if( item->Type() == PCB_FP_TEXT_T )
            {
                // Convert to PCB_TEXT_T
                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item->GetParent() );
                FP_TEXT*   fp_text = static_cast<FP_TEXT*>( item );
                PCB_TEXT*  pcb_text = new PCB_TEXT( m_board );

                if( fp_text->GetText() == "${VALUE}" )
                    pcb_text->SetText( footprint->GetValue() );
                else if( fp_text->GetText() == "${REFERENCE}" )
                    pcb_text->SetText( footprint->GetReference() );
                else
                    pcb_text->CopyText( *fp_text );

                pcb_text->SetEffects( *fp_text );
                pcb_text->SetLayer( fp_text->GetLayer() );
                copy = pcb_text;
            }
            else if( item->Type() == PCB_PAD_T )
            {
                // Create a parent to own the copied pad
                FOOTPRINT* footprint = new FOOTPRINT( m_board );
                PAD*       pad = (PAD*) item->Clone();

                footprint->SetPosition( pad->GetPosition() );
                pad->SetPos0( wxPoint() );
                footprint->Add( pad );
                copy = footprint;
            }
            else if( item->Type() == PCB_FP_ZONE_T )
            {
                // Convert to PCB_ZONE_T
                ZONE* zone = new ZONE( m_board );
                zone->InitDataFromSrcInCopyCtor( *static_cast<ZONE*>( item ) );
                copy = zone;
            }
            else if( item->Type() == PCB_GROUP_T )
            {
                copy = static_cast<PCB_GROUP*>( item )->DeepClone();
            }
            else
            {
                copy = static_cast<BOARD_ITEM*>( item->Clone() );
            }

            auto prepItem = [&]( BOARD_ITEM* aItem )
                            {
                                aItem->SetLocked( false );
                            };

            if( copy )
            {
                prepItem( copy );

                // locate the reference point at (0, 0) in the copied items
                copy->Move( (wxPoint) -refPoint );

                Format( copy, 1 );

                if( copy->Type() == PCB_GROUP_T )
                {
                    static_cast<PCB_GROUP*>( copy )->RunOnDescendants( prepItem );
                    static_cast<PCB_GROUP*>( copy )->RunOnDescendants( [&]( BOARD_ITEM* titem )
                                                                       {
                                                                           Format( titem, 1 );
                                                                       } );
                }

                delete copy;
            }
        }
        m_formatter.Print( 0, "\n)" );
    }

    // These are placed at the end to minimize the open time of the clipboard
    wxLogNull         doNotLog; // disable logging of failed clipboard actions
    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock || !clipboard->IsOpened() )
        return;

    clipboard->SetData( new wxTextDataObject( wxString( m_formatter.GetString().c_str(),
                                                        wxConvUTF8 ) ) );

    clipboard->Flush();

    #ifndef __WXOSX__
    // This section exists to return the clipboard data, ensuring it has fully
    // been processed by the system clipboard.  This appears to be needed for
    // extremely large clipboard copies on asynchronous linux clipboard managers
    // such as KDE's Klipper. However, a read back of the data on OSX before the
    // clipboard is closed seems to cause an ASAN error (heap-buffer-overflow)
    // since it uses the cached version of the clipboard data and not the system
    // clipboard data.
    if( clipboard->IsSupported( wxDF_TEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        ( void )data.GetText(); // Keep unused variable
    }
    #endif
}


BOARD_ITEM* CLIPBOARD_IO::Parse()
{
    BOARD_ITEM* item;
    wxString result;

    wxLogNull doNotLog; // disable logging of failed clipboard actions

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

    m_out->Print( 0, "(kicad_pcb (version %d) (generator pcbnew)\n", SEXPR_BOARD_FILE_VERSION );

    Format( aBoard, 1 );

    m_out->Print( 0, ")\n" );

    wxLogNull doNotLog; // disable logging of failed clipboard actions

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
    if( clipboard->IsSupported( wxDF_TEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        ( void )data.GetText(); // Keep unused variable
    }
}


BOARD* CLIPBOARD_IO::Load( const wxString& aFileName, BOARD* aAppendToMe,
                           const PROPERTIES* aProperties, PROJECT* aProject )
{
    std::string result;

    wxLogNull doNotLog; // disable logging of failed clipboard actions

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
