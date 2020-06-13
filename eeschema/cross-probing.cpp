/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <kiface_i.h>
#include <kiway_express.h>
#include <macros.h>
#include <eda_dde.h>
#include <connection_graph.h>
#include <sch_edit_frame.h>
#include <general.h>
#include <lib_item.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <schematic.h>
#include <reporter.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <tools/ee_actions.h>
#include <tools/sch_editor_control.h>


SCH_ITEM* SCH_EDITOR_CONTROL::FindComponentAndItem( const wxString& aReference,
                                                    bool            aSearchHierarchy,
                                                    SCH_SEARCH_T    aSearchType,
                                                    const wxString& aSearchText )
{
    SCH_SHEET_PATH* sheetWithComponentFound = NULL;
    SCH_COMPONENT*  component               = NULL;
    wxPoint         pos;
    LIB_PIN*        pin = nullptr;
    SCH_SHEET_LIST  sheetList;
    SCH_ITEM*       foundItem = nullptr;

    if( !aSearchHierarchy )
        sheetList.push_back( m_frame->GetCurrentSheet() );
    else
        sheetList.BuildSheetList( &m_frame->Schematic().Root() );

    for( SCH_SHEET_PATH& sheet : sheetList )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        for( auto item : screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            SCH_COMPONENT* pSch = static_cast<SCH_COMPONENT*>( item );

            if( aReference.CmpNoCase( pSch->GetRef( &sheet ) ) == 0 )
            {
                component               = pSch;
                sheetWithComponentFound = &sheet;

                if( aSearchType == HIGHLIGHT_PIN )
                {
                    pos = pSch->GetPosition();  // temporary: will be changed if the pin is found.
                    pin = pSch->GetPin( aSearchText );

                    if( pin )
                    {
                        pos += pin->GetPosition();
                        foundItem = component;
                        break;
                    }
                }
                else
                {
                    pos = pSch->GetPosition();
                    foundItem = component;
                    break;
                }
            }
        }

        if( foundItem )
            break;
    }

    if( component )
    {
        if( *sheetWithComponentFound != m_frame->GetCurrentSheet() )
        {
            m_frame->Schematic().SetCurrentSheet( *sheetWithComponentFound );
            m_frame->DisplayCurrentSheet();
        }

        wxPoint delta;
        pos  -= component->GetPosition();
        delta = component->GetTransform().TransformCoordinate( pos );
        pos   = delta + component->GetPosition();

        m_frame->GetCanvas()->GetViewControls()->SetCrossHairCursorPosition( pos, false );
        m_frame->CenterScreen( pos, false );
    }

    /* Print diag */
    wxString msg_item;
    wxString msg;

    if( aSearchType == HIGHLIGHT_PIN )
        msg_item.Printf( _( "pin %s" ), aSearchText );
    else
        msg_item = _( "component" );

    if( component )
    {
        if( foundItem )
            msg.Printf( _( "%s %s found" ), aReference, msg_item );
        else
            msg.Printf( _( "%s found but %s not found" ), aReference, msg_item );
    }
    else
        msg.Printf( _( "Component %s not found" ), aReference );

    m_frame->SetStatusText( msg );

    m_probingPcbToSch = true;   // recursion guard
    {
        // Clear any existing highlighting
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        if( foundItem )
            m_toolMgr->RunAction( EE_ACTIONS::addItemToSel, true, foundItem );
    }
    m_probingPcbToSch = false;

    m_frame->GetCanvas()->Refresh();

    return foundItem;
}


void SCH_EDIT_FRAME::ExecuteRemoteCommand( const char* cmdline )
{
    SCH_EDITOR_CONTROL* editor = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
    char                line[1024];

    strncpy( line, cmdline, sizeof(line) - 1 );
    line[ sizeof(line) - 1 ] = '\0';

    char* idcmd = strtok( line, " \n\r" );
    char* text  = strtok( NULL, "\"\n\r" );

    if( idcmd == NULL )
        return;

    if( strcmp( idcmd, "$NET:" ) == 0 )
    {
        wxString netName = FROM_UTF8( text );

        if( auto sg = Schematic().ConnectionGraph()->FindFirstSubgraphByName( netName ) )
            m_highlightedConn = sg->m_driver_connection;

        GetToolManager()->RunAction( EE_ACTIONS::updateNetHighlighting, true );

        SetStatusText( _( "Selected net: " ) + UnescapeString( netName ) );
        return;
    }

    if( strcmp( idcmd, "$CLEAR:" ) == 0 )
    {
        if( text && strcmp( text, "HIGHLIGHTED" ) == 0 )
        {
            GetCanvas()->GetView()->HighlightItem( nullptr, nullptr );
            GetCanvas()->Refresh();
        }

        return;
    }

    if( text == NULL )
        return;

    if( strcmp( idcmd, "$PART:" ) != 0 )
        return;

    wxString part_ref = FROM_UTF8( text );

    /* look for a complement */
    idcmd = strtok( NULL, " \n\r" );

    if( idcmd == NULL )    // Highlight component only (from Cvpcb or Pcbnew)
    {
        // Highlight component part_ref, or clear Highlight, if part_ref is not existing
        editor->FindComponentAndItem( part_ref, true, HIGHLIGHT_COMPONENT, wxEmptyString );
        return;
    }

    text = strtok( NULL, "\"\n\r" );

    if( text == NULL )
        return;

    wxString msg = FROM_UTF8( text );

    if( strcmp( idcmd, "$REF:" ) == 0 )
    {
        // Highlighting the reference itself isn't actually that useful, and it's harder to
        // see.  Highlight the parent and display the message.
        editor->FindComponentAndItem( part_ref, true, HIGHLIGHT_COMPONENT, msg );
    }
    else if( strcmp( idcmd, "$VAL:" ) == 0 )
    {
        // Highlighting the value itself isn't actually that useful, and it's harder to see.
        // Highlight the parent and display the message.
        editor->FindComponentAndItem( part_ref, true, HIGHLIGHT_COMPONENT, msg );
    }
    else if( strcmp( idcmd, "$PAD:" ) == 0 )
    {
        editor->FindComponentAndItem( part_ref, true, HIGHLIGHT_PIN, msg );
    }
    else
    {
        editor->FindComponentAndItem( part_ref, true, HIGHLIGHT_COMPONENT, wxEmptyString );
    }
}


std::string FormatProbeItem( EDA_ITEM* aItem, SCH_COMPONENT* aComp )
{
    // This is a keyword followed by a quoted string.

    // Cross probing to Pcbnew if a pin or a component is found
    switch( aItem->Type() )
    {
    case SCH_FIELD_T:
        if( aComp )
            return StrPrintf( "$PART: \"%s\"", TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );
        break;

    case SCH_COMPONENT_T:
        aComp = (SCH_COMPONENT*) aItem;
        return StrPrintf( "$PART: \"%s\"", TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );

    case SCH_SHEET_T:
        {
            // For cross probing, we need the full path of the sheet, because
            // in complex hierarchies the sheet uuid of not unique
            SCH_SHEET* sheet = (SCH_SHEET*)aItem;
            wxString full_path;

            SCH_SHEET* parent = sheet;
            while( (parent = dynamic_cast<SCH_SHEET*>( parent->GetParent() ) ) )
            {
                if( parent->GetParent() )   // The root sheet has no parent and path is just "/"
                {
                    full_path.Prepend( parent->m_Uuid.AsString() );
                    full_path.Prepend( "/" );
                }
            }

            full_path += "/" + sheet->m_Uuid.AsString();

            return StrPrintf( "$SHEET: \"%s\"", TO_UTF8( full_path ) );
        }

    case SCH_PIN_T:
        {
            SCH_PIN* pin = (SCH_PIN*) aItem;
            aComp = pin->GetParentComponent();

            if( !pin->GetNumber().IsEmpty() )
            {
                return StrPrintf( "$PIN: \"%s\" $PART: \"%s\"",
                                  TO_UTF8( pin->GetNumber() ),
                                  TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );
            }
            else
            {
                return StrPrintf( "$PART: \"%s\"",
                                  TO_UTF8( aComp->GetField( REFERENCE )->GetText() ) );
            }
        }

    default:
        break;
    }

    return "";
}


void SCH_EDIT_FRAME::SendMessageToPCBNEW( EDA_ITEM* aObjectToSync, SCH_COMPONENT* aLibItem )
{
    wxASSERT( aObjectToSync );     // fix the caller

    if( !aObjectToSync )
        return;

    std::string packet = FormatProbeItem( aObjectToSync, aLibItem );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_PCB, packet.c_str() );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void SCH_EDIT_FRAME::SendCrossProbeNetName( const wxString& aNetName )
{
    // The command is a keyword followed by a quoted string.

    std::string packet = StrPrintf( "$NET: \"%s\"", TO_UTF8( aNetName ) );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_PCB, packet.c_str() );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void SCH_EDIT_FRAME::SetCrossProbeConnection( const SCH_CONNECTION* aConnection )
{
    if( !aConnection )
    {
        SendCrossProbeClearHighlight();
        return;
    }

    if( aConnection->IsNet() )
    {
        SendCrossProbeNetName( aConnection->Name() );
        return;
    }

    if( aConnection->Members().empty() )
        return;

    auto all_members = aConnection->AllMembers();

    wxString nets = all_members[0]->Name();

    if( all_members.size() == 1 )
    {
        SendCrossProbeNetName( nets );
        return;
    }

    // TODO: This could be replaced by just sending the bus name once we have bus contents
    // included as part of the netlist sent from eeschema to pcbnew (and thus pcbnew can
    // natively keep track of bus membership)

    for( size_t i = 1; i < all_members.size(); i++ )
        nets << "," << all_members[i]->Name();

    std::string packet = StrPrintf( "$NETS: \"%s\"", TO_UTF8( nets ) );

    if( !packet.empty() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_PCB, packet.c_str() );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void SCH_EDIT_FRAME::SendCrossProbeClearHighlight()
{
    std::string packet = "$CLEAR\n";

    if( Kiface().IsSingle() )
        SendCommand( MSG_TO_PCB, packet.c_str() );
    else
    {
        // Typically ExpressMail is going to be s-expression packets, but since
        // we have existing interpreter of the cross probe packet on the other
        // side in place, we use that here.
        Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_CROSS_PROBE, packet, this );
    }
}


void SCH_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_CROSS_PROBE:
        ExecuteRemoteCommand( payload.c_str() );
        break;

    case MAIL_SCH_GET_NETLIST:
        if( payload.find( "quiet-annotate" ) != std::string::npos )
        {
            Schematic().GetSheets().AnnotatePowerSymbols();
            AnnotateComponents( true, UNSORTED, INCREMENTAL_BY_REF, 0, false, false, true,
                                NULL_REPORTER::GetInstance() );
        }

        if( payload.find( "no-annotate" ) == std::string::npos )
        {
            // Ensure schematic is OK for netlist creation (especially that it is fully annotated):
            if( !ReadyToNetlist() )
                return;
        }

        {
            NETLIST_EXPORTER_KICAD exporter( &Schematic() );
            STRING_FORMATTER formatter;

            exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD );

            payload = formatter.GetString();
        }
        break;

    case MAIL_BACKANNOTATE_FOOTPRINTS:
        try
        {
            SCH_EDITOR_CONTROL* controlTool = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
            controlTool->BackAnnotateFootprints( payload );
        }
        catch( const IO_ERROR& DBG( ioe ) )
        {
            DBG( printf( "%s: ioe:%s\n", __func__, TO_UTF8( ioe.What() ) );)
        }
        break;

    case MAIL_SCH_REFRESH:
    {
        SCH_SCREENS schematic( Schematic().Root() );
        schematic.TestDanglingEnds();

        GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );
        GetCanvas()->Refresh();
        break;
    }
    case MAIL_IMPORT_FILE:
    {
        // Extract file format type and path (plugin type and path separated with \n)
        size_t split = payload.find( '\n' );
        wxCHECK( split != std::string::npos, /*void*/ );
        int importFormat;

        try
        {
            importFormat = std::stoi( payload.substr( 0, split ) );
        }
        catch( std::invalid_argument& )
        {
            wxFAIL;
            importFormat = -1;
        }

        std::string path = payload.substr( split + 1 );
        wxASSERT( !path.empty() );

        if( importFormat >= 0 )
            importFile( path, importFormat );
    }
        break;

    case MAIL_SCH_SAVE:
        if( SaveProject() )
            payload = "success";
        break;

    case MAIL_SCH_UPDATE:
        m_toolManager->RunAction( ACTIONS::updateSchematicFromPcb, true );
        break;

    default:
        ;
    }
}
