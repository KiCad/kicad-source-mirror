/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.TXT for contributors.
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/log.h>

#include <board.h>
#include <core/ignore.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <zone.h>
#include <locale_io.h>
#include <netinfo.h>
#include <plugins/kicad/pcb_parser.h>

#include <plugins/kicad/pcb_plugin.h>
#include <kicad_clipboard.h>
#include "confirm.h"

CLIPBOARD_IO::CLIPBOARD_IO():
        PCB_PLUGIN(CTL_FOR_CLIPBOARD ),
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
        newFootprint.Move( VECTOR2I( -refPoint.x, -refPoint.y ) );

        Format( static_cast<BOARD_ITEM*>( &newFootprint ) );

        newFootprint.SetParent( nullptr );
    }
    else if( isFootprintEditor )
    {
        FOOTPRINT partialFootprint( m_board );

        // Useful to copy the selection to the board editor (if any), and provides
        // a dummy lib id.
        // Perhaps not a good Id, but better than a empty id
        KIID dummy;
        LIB_ID id( "clipboard", dummy.AsString() );
        partialFootprint.SetFPID( id );

        for( const EDA_ITEM* item : aSelected )
        {
            const PCB_GROUP* group = dynamic_cast<const PCB_GROUP*>( item );
            BOARD_ITEM*      clone;

            if( const PCB_TEXT* text = dyn_cast<const PCB_TEXT*>( item ) )
            {
                if( text->GetType() != PCB_TEXT::TEXT_is_DIVERS )
                    continue;
            }

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

            // A list of not added items, when adding items to the footprint
            // some PCB_TEXT (reference and value) cannot be added to the footprint
            std::vector<BOARD_ITEM*> skipped_items;

            if( group )
            {
                static_cast<PCB_GROUP*>( clone )->RunOnDescendants(
                        [&]( BOARD_ITEM* descendant )
                        {
                            // One cannot add a text reference or value to a given footprint:
                            // only one is allowed. So add only PCB_TEXT::TEXT_is_DIVERS
                            bool can_add = true;

                            if( const PCB_TEXT* text = dyn_cast<const PCB_TEXT*>( descendant ) )
                            {
                                if( text->GetType() != PCB_TEXT::TEXT_is_DIVERS )
                                    can_add = false;
                            }

                            if( can_add )
                                partialFootprint.Add( descendant );
                            else
                                skipped_items.push_back( descendant );
                        } );
            }

            // locate the reference point at (0, 0) in the copied items
            clone->Move( -refPoint );

            // Now delete items, duplicated but not added:
            for( BOARD_ITEM* skp_item : skipped_items )
                delete skp_item;
        }

        // Set the new relative internal local coordinates of copied items
        FOOTPRINT* editedFootprint = m_board->Footprints().front();
        VECTOR2I   moveVector = partialFootprint.GetPosition() + editedFootprint->GetPosition();

        partialFootprint.MoveAnchorPosition( moveVector );

        Format( &partialFootprint, 0 );

        partialFootprint.SetParent( nullptr );
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

            if( item->Type() == PCB_TEXT_T )
            {
                copy = static_cast<BOARD_ITEM*>( item->Clone() );

                PCB_TEXT* textItem = static_cast<PCB_TEXT*>( copy );

                if( textItem->GetText() == wxT( "${VALUE}" ) )
                    textItem->SetText( item->GetParentFootprint()->GetValue() );
                else if( textItem->GetText() == wxT( "${REFERENCE}" ) )
                    textItem->SetText( item->GetParentFootprint()->GetReference() );
            }
            else if( item->Type() == PCB_PAD_T )
            {
                // Create a parent to own the copied pad
                FOOTPRINT* footprint = new FOOTPRINT( m_board );
                PAD*       pad = (PAD*) item->Clone();

                footprint->SetPosition( pad->GetPosition() );
                footprint->Add( pad );
                copy = footprint;
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
                copy->Move( -refPoint );

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
    if( clipboard->IsSupported( wxDF_TEXT ) || clipboard->IsSupported( wxDF_UNICODETEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        ignore_unused( data.GetText() );
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

    if( clipboard->IsSupported( wxDF_TEXT ) || clipboard->IsSupported( wxDF_UNICODETEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        result = data.GetText();
    }

    try
    {
        item = PCB_PLUGIN::Parse( result );
    }
    catch (...)
    {
        item = nullptr;
    }

    return item;
}


void CLIPBOARD_IO::Save( const wxString& aFileName, BOARD* aBoard,
                const STRING_UTF8_MAP* aProperties )
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
    if( clipboard->IsSupported( wxDF_TEXT ) || clipboard->IsSupported( wxDF_UNICODETEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        ignore_unused( data.GetText() );
    }
}


BOARD* CLIPBOARD_IO::Load( const wxString& aFileName, BOARD* aAppendToMe,
                           const STRING_UTF8_MAP* aProperties, PROJECT* aProject,
                           PROGRESS_REPORTER* aProgressReporter )
{
    std::string result;

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock )
        return nullptr;

    if( clipboard->IsSupported( wxDF_TEXT ) || clipboard->IsSupported( wxDF_UNICODETEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );

        result = data.GetText().mb_str();
    }

    std::function<bool( wxString, int, wxString, wxString )> queryUser =
            [&]( wxString aTitle, int aIcon, wxString aMessage, wxString aAction ) -> bool
            {
                KIDIALOG dlg( nullptr, aMessage, aTitle, wxOK | wxCANCEL | aIcon );

                if( !aAction.IsEmpty() )
                    dlg.SetOKLabel( aAction );

                dlg.DoNotShowCheckbox( aMessage, 0 );

                return dlg.ShowModal() == wxID_OK;
            };

    STRING_LINE_READER reader( result, wxT( "clipboard" ) );
    PCB_PARSER         parser( &reader, aAppendToMe, &queryUser );

    init( aProperties );

    BOARD_ITEM* item;
    BOARD* board;

    try
    {
        item =  parser.Parse();
    }
    catch( const FUTURE_FORMAT_ERROR& )
    {
        // Don't wrap a FUTURE_FORMAT_ERROR in another
        throw;
    }
    catch( const PARSE_ERROR& parse_error )
    {
        if( parser.IsTooRecent() )
            throw FUTURE_FORMAT_ERROR( parse_error, parser.GetRequiredVersion() );
        else
            throw;
    }

    if( item->Type() != PCB_T )
    {
        // The parser loaded something that was valid, but wasn't a board.
        THROW_PARSE_ERROR( _( "Clipboard content is not KiCad compatible" ), parser.CurSource(),
                           parser.CurLine(), parser.CurLineNumber(), parser.CurOffset() );
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
