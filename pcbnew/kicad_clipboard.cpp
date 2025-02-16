/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <build_version.h>
#include <core/ignore.h>
#include <font/fontconfig.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <pcb_text.h>
#include <pcb_table.h>
#include <zone.h>
#include <locale_io.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <kicad_clipboard.h>
#include <kidialog.h>
#include <io/kicad/kicad_io_utils.h>

CLIPBOARD_IO::CLIPBOARD_IO():
        PCB_IO_KICAD_SEXPR(CTL_FOR_CLIPBOARD ),
        m_formatter(),
        m_writer( &CLIPBOARD_IO::clipboardWriter ),
        m_reader( &CLIPBOARD_IO::clipboardReader )
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


void CLIPBOARD_IO::clipboardWriter( const wxString& aData )
{
    wxLogNull         doNotLog; // disable logging of failed clipboard actions
    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock || !clipboard->IsOpened() )
        return;

    clipboard->SetData( new wxTextDataObject( aData ) );

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


wxString CLIPBOARD_IO::clipboardReader()
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock )
        return wxEmptyString;

    if( clipboard->IsSupported( wxDF_TEXT ) || clipboard->IsSupported( wxDF_UNICODETEXT ) )
    {
        wxTextDataObject data;
        clipboard->GetData( data );
        return data.GetText();
    }

    return wxEmptyString;
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

    auto deleteUnselectedCells =
            []( PCB_TABLE* aTable )
            {
                int minCol = aTable->GetColCount();
                int maxCol = -1;
                int minRow = aTable->GetRowCount();
                int maxRow = -1;

                for( int row = 0; row < aTable->GetRowCount(); ++row )
                {
                    for( int col = 0; col < aTable->GetColCount(); ++col )
                    {
                        PCB_TABLECELL* cell = aTable->GetCell( row, col );

                        if( cell->IsSelected() )
                        {
                            minRow = std::min( minRow, row );
                            maxRow = std::max( maxRow, row );
                            minCol = std::min( minCol, col );
                            maxCol = std::max( maxCol, col );
                        }
                        else
                        {
                            cell->SetFlags( STRUCT_DELETED );
                        }
                    }
                }

                wxCHECK_MSG( maxCol >= minCol && maxRow >= minRow, /*void*/,
                             wxT( "No selected cells!" ) );

                // aTable is always a clone in the clipboard case
                int destRow = 0;

                for( int row = minRow; row <= maxRow; row++ )
                    aTable->SetRowHeight( destRow++, aTable->GetRowHeight( row ) );

                int destCol = 0;

                for( int col = minCol; col <= maxCol; col++ )
                    aTable->SetColWidth( destCol++, aTable->GetColWidth( col ) );

                aTable->DeleteMarkedCells();
                aTable->SetColCount( ( maxCol - minCol ) + 1 );
                aTable->Normalize();
            };

    std::set<PCB_TABLE*> promotedTables;

    auto parentIsPromoted =
            [&]( PCB_TABLECELL* cell ) -> bool
            {
                for( PCB_TABLE* table : promotedTables )
                {
                    if( table->m_Uuid == cell->GetParent()->m_Uuid )
                        return true;
                }

                return false;
            };

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
        newFootprint.SetParentGroup( nullptr );
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

        for( EDA_ITEM* item : aSelected )
        {
            if( !item->IsBOARD_ITEM() )
                continue;

            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
            BOARD_ITEM* copy = nullptr;

            if( PCB_FIELD* field = dynamic_cast<PCB_FIELD*>( item ) )
            {
                if( field->IsMandatory() )
                    continue;
            }

            if( boardItem->Type() == PCB_GROUP_T )
            {
                copy = static_cast<PCB_GROUP*>( boardItem )->DeepClone();
            }
            else if( boardItem->Type() == PCB_GENERATOR_T )
            {
                copy = static_cast<PCB_GENERATOR*>( boardItem )->DeepClone();
            }
            else if( item->Type() == PCB_TABLECELL_T )
            {
                if( parentIsPromoted( static_cast<PCB_TABLECELL*>( item ) ) )
                    continue;

                copy = static_cast<BOARD_ITEM*>( item->GetParent()->Clone() );
                promotedTables.insert( static_cast<PCB_TABLE*>( copy ) );
            }
            else
            {
                copy = static_cast<BOARD_ITEM*>( boardItem->Clone() );
            }

            // If it is only a footprint, clear the nets from the pads
            if( PAD* pad = dynamic_cast<PAD*>( copy ) )
               pad->SetNetCode( 0 );

            // Don't copy group membership information for the 1st level objects being copied
            // since the group they belong to isn't being copied.
            copy->SetParentGroup( nullptr );

            // Add the pad to the new footprint before moving to ensure the local coords are
            // correct
            partialFootprint.Add( copy );

            // A list of not added items, when adding items to the footprint
            // some PCB_TEXT (reference and value) cannot be added to the footprint
            std::vector<BOARD_ITEM*> skipped_items;

            // Will catch at least PCB_GROUP_T and PCB_GENERATOR_T
            if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( copy ) )
            {
                group->RunOnChildren(
                        [&]( BOARD_ITEM* descendant )
                        {
                            // One cannot add an additional mandatory field to a given footprint:
                            // only one is allowed. So add only non-mandatory fields.
                            bool can_add = true;

                            if( const PCB_FIELD* field = dynamic_cast<const PCB_FIELD*>( item ) )
                            {
                                if( field->IsMandatory() )
                                    can_add = false;
                            }

                            if( can_add )
                                partialFootprint.Add( descendant );
                            else
                                skipped_items.push_back( descendant );
                        },
                        RECURSE_MODE::RECURSE );
            }

            // locate the reference point at (0, 0) in the copied items
            copy->Move( -refPoint );

            // Now delete items, duplicated but not added:
            for( BOARD_ITEM* skipped_item : skipped_items )
            {
                static_cast<PCB_GROUP*>( copy )->RemoveItem( skipped_item );
                skipped_item->SetParentGroup( nullptr );
                delete skipped_item;
            }
        }

        // Set the new relative internal local coordinates of copied items
        FOOTPRINT* editedFootprint = m_board->Footprints().front();
        VECTOR2I   moveVector = partialFootprint.GetPosition() + editedFootprint->GetPosition();

        partialFootprint.MoveAnchorPosition( moveVector );

        for( PCB_TABLE* table : promotedTables )
            deleteUnselectedCells( table );

        Format( &partialFootprint );

        partialFootprint.SetParent( nullptr );
    }
    else
    {
        // we will fake being a .kicad_pcb to get the full parser kicking
        // This means we also need layers and nets
        LOCALE_IO io;

        m_formatter.Print( "(kicad_pcb (version %d) (generator \"pcbnew\") (generator_version %s)",
                           SEXPR_BOARD_FILE_VERSION,
                           m_formatter.Quotew( GetMajorMinorVersion() ).c_str() );

        formatBoardLayers( m_board );
        formatNetInformation( m_board );

        for( EDA_ITEM* item : aSelected )
        {
            if( !item->IsBOARD_ITEM() )
                continue;

            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
            BOARD_ITEM* copy = nullptr;

            wxCHECK2( boardItem, continue );

            if( boardItem->Type() == PCB_FIELD_T )
            {
                PCB_FIELD* field = static_cast<PCB_FIELD*>( boardItem );
                copy = new PCB_TEXT( m_board );

                PCB_TEXT* textItem = static_cast<PCB_TEXT*>( copy );
                textItem->SetPosition( field->GetPosition() );
                textItem->SetLayer( field->GetLayer() );
                textItem->SetHyperlink( field->GetHyperlink() );
                textItem->SetText( field->GetText() );
                textItem->SetAttributes( field->GetAttributes() );
                textItem->SetTextAngle( field->GetDrawRotation() );

                if ( textItem->GetText() == wxT( "${VALUE}" ) )
                    textItem->SetText( boardItem->GetParentFootprint()->GetValue() );
                else if ( textItem->GetText() == wxT( "${REFERENCE}" ) )
                    textItem->SetText( boardItem->GetParentFootprint()->GetReference() );

            }
            else if( boardItem->Type() == PCB_TEXT_T )
            {
                copy = static_cast<BOARD_ITEM*>( boardItem->Clone() );

                PCB_TEXT* textItem = static_cast<PCB_TEXT*>( copy );

                if( textItem->GetText() == wxT( "${VALUE}" ) )
                    textItem->SetText( boardItem->GetParentFootprint()->GetValue() );
                else if( textItem->GetText() == wxT( "${REFERENCE}" ) )
                    textItem->SetText( boardItem->GetParentFootprint()->GetReference() );
            }
            else if( boardItem->Type() == PCB_GROUP_T )
            {
                copy = static_cast<PCB_GROUP*>( boardItem )->DeepClone();
            }
            else if( boardItem->Type() == PCB_GENERATOR_T )
            {
                copy = static_cast<PCB_GENERATOR*>( boardItem )->DeepClone();
            }
            else if( item->Type() == PCB_TABLECELL_T )
            {
                if( parentIsPromoted( static_cast<PCB_TABLECELL*>( item ) ) )
                    continue;

                copy = static_cast<BOARD_ITEM*>( item->GetParent()->Clone() );
                promotedTables.insert( static_cast<PCB_TABLE*>( copy ) );
            }
            else
            {
                copy = static_cast<BOARD_ITEM*>( boardItem->Clone() );
            }

            if( copy )
            {
                if( copy->Type() == PCB_FIELD_T || copy->Type() == PCB_PAD_T )
                {
                    // Create a parent footprint to own the copied item
                    FOOTPRINT* footprint = new FOOTPRINT( m_board );

                    footprint->SetPosition( copy->GetPosition() );
                    footprint->Add( copy );

                    // Convert any mandatory fields to user fields.  The destination footprint
                    // will already have its own mandatory fields.
                    if( PCB_FIELD* field = dynamic_cast<PCB_FIELD*>( copy ) )
                    {
                        if( field->IsMandatory() )
                            field->SetOrdinal( footprint->GetNextFieldOrdinal() );
                    }

                    copy = footprint;
                }

                copy->SetLocked( false );
                copy->SetParent( m_board );
                copy->SetParentGroup( nullptr );

                // locate the reference point at (0, 0) in the copied items
                copy->Move( -refPoint );

                if( copy->Type() == PCB_TABLE_T )
                {
                    PCB_TABLE* table = static_cast<PCB_TABLE*>( copy );

                    if( promotedTables.count( table ) )
                        deleteUnselectedCells( table );
                }

                Format( copy );

                if( copy->Type() == PCB_GROUP_T || copy->Type() == PCB_GENERATOR_T )
                {
                    copy->RunOnChildren(
                            [&]( BOARD_ITEM* descendant )
                            {
                                descendant->SetLocked( false );
                                Format( descendant );
                            },
                            RECURSE_MODE::RECURSE );
                }

                delete copy;
            }
        }

        m_formatter.Print( ")" );
    }

    std::string prettyData = m_formatter.GetString();
    KICAD_FORMAT::Prettify( prettyData, KICAD_FORMAT::FORMAT_MODE::COMPACT_TEXT_PROPERTIES );

    // These are placed at the end to minimize the open time of the clipboard
    m_writer( wxString( prettyData.c_str(), wxConvUTF8 ) );
}


BOARD_ITEM* CLIPBOARD_IO::Parse()
{
    BOARD_ITEM* item;
    wxString result = m_reader();

    try
    {
        item = PCB_IO_KICAD_SEXPR::Parse( result );
    }
    catch (...)
    {
        item = nullptr;
    }

    return item;
}


void CLIPBOARD_IO::SaveBoard( const wxString& aFileName, BOARD* aBoard,
                              const std::map<std::string, UTF8>* aProperties )
{
    init( aProperties );

    m_board = aBoard;       // after init()

    // Prepare net mapping that assures that net codes saved in a file are consecutive integers
    m_mapping->SetBoard( aBoard );

    m_formatter.Print( "(kicad_pcb (version %d) (generator \"pcbnew\") (generator_version %s)",
                  SEXPR_BOARD_FILE_VERSION,
                  m_formatter.Quotew( GetMajorMinorVersion() ).c_str() );

    Format( aBoard );

    m_formatter.Print( ")" );

    std::string prettyData = m_formatter.GetString();
    KICAD_FORMAT::Prettify( prettyData, KICAD_FORMAT::FORMAT_MODE::COMPACT_TEXT_PROPERTIES );

    m_writer( wxString( prettyData.c_str(), wxConvUTF8 ) );
}


BOARD* CLIPBOARD_IO::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                const std::map<std::string, UTF8>* aProperties, PROJECT* aProject )
{
    std::string result( m_reader().mb_str() );

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
    PCB_IO_KICAD_SEXPR_PARSER         parser( &reader, aAppendToMe, queryUser );

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
