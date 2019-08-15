/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_edit_frame.h>
#include <general.h>
#include <eeschema_id.h>
#include <lib_item.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <sch_sheet.h>
#include <sch_view.h>
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
    SCH_ITEM*       item = NULL;
    SCH_COMPONENT*  Component = NULL;
    wxPoint         pos;
    bool            notFound = true;
    LIB_PIN*        pin = nullptr;
    SCH_SHEET_LIST  sheetList( g_RootSheet );
    EDA_ITEM*       foundItem = nullptr;

    if( !aSearchHierarchy )
        sheetList.push_back( *g_CurrentSheet );
    else
        sheetList.BuildSheetList( g_RootSheet );

    for( SCH_SHEET_PATH& sheet : sheetList)
    {
        for( item = sheet.LastDrawList(); item && notFound; item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* pSch = (SCH_COMPONENT*) item;

            if( aReference.CmpNoCase( pSch->GetRef( &sheet ) ) == 0 )
            {
                Component = pSch;
                sheetWithComponentFound = &sheet;

                if( aSearchType == HIGHLIGHT_PIN )
                {
                    pos = pSch->GetPosition();  // temporary: will be changed if the pin is found.
                    pin = pSch->GetPin( aSearchText );

                    if( pin )
                    {
                        notFound = false;
                        pos += pin->GetPosition();
                        foundItem = Component;
                    }
                }
                else
                {
                    notFound = false;
                    pos = pSch->GetPosition();
                    foundItem = Component;
                }
            }
        }

        if( notFound == false )
            break;
    }

    if( Component )
    {
        if( *sheetWithComponentFound != *g_CurrentSheet )
        {
            sheetWithComponentFound->LastScreen()->SetZoom( m_frame->GetScreen()->GetZoom() );
            *g_CurrentSheet = *sheetWithComponentFound;
            m_frame->DisplayCurrentSheet();
        }

        wxPoint delta;
        pos  -= Component->GetPosition();
        delta = Component->GetTransform().TransformCoordinate( pos );
        pos   = delta + Component->GetPosition();

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

    if( Component )
    {
        if( !notFound )
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
        m_frame->GetCanvas()->GetView()->HighlightItem( nullptr, nullptr );

        if( foundItem )
            m_frame->GetCanvas()->GetView()->HighlightItem( foundItem, pin );
    }
    m_probingPcbToSch = false;

    m_frame->GetCanvas()->Refresh();

    return item;
}


/**
 * Execute a remote command sent by Pcbnew via a socket connection.
 * <p>
 * When user selects a module or pin in Pcbnew, Eeschema shows that same
 * component or pin and moves cursor on the item.  The socket port used
 * is #KICAD_SCH_PORT_SERVICE_NUMBER which defaults to 4243.
 *
 * Valid commands are:
 * \li \c \$PART: \c "reference" Put cursor on component.
 * \li \c \$PART: \c "reference" \c \$REF: \c "ref" Put cursor on component reference.
 * \li \c \$PART: \c "reference" \c \$VAL: \c "value" Put cursor on component value.
 * \li \c \$PART: \c "reference" \c \$PAD: \c "pin name" Put cursor on the component pin.
 * \li \c \$NET: \c "netname" Highlight a specified net
 * \li \c \$CLEAR: \c "HIGHLIGHTED" Clear components highlight
 * <p>
 * They are a keyword followed by a quoted string.
 * @param cmdline = received command from Pcbnew
 */
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
        m_SelectedNetName = FROM_UTF8( text );
        GetToolManager()->RunAction( EE_ACTIONS::updateNetHighlighting, true );

        SetStatusText( _( "Selected net: " ) + UnescapeString( m_SelectedNetName ) );
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
        SCH_SHEET* sheet = (SCH_SHEET*)aItem;
        return StrPrintf( "$SHEET: \"%8.8lX\"", (unsigned long) sheet->GetTimeStamp() );
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
            Kiway().ExpressMail( FRAME_PCB, MAIL_CROSS_PROBE, packet, this );
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
            Kiway().ExpressMail( FRAME_PCB, MAIL_CROSS_PROBE, packet, this );
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
        Kiway().ExpressMail( FRAME_PCB, MAIL_CROSS_PROBE, packet, this );
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
            SCH_SCREENS schematic;
            schematic.UpdateSymbolLinks();
            SCH_SHEET_LIST sheets( g_RootSheet );
            sheets.AnnotatePowerSymbols();
            AnnotateComponents( true, UNSORTED, INCREMENTAL_BY_REF, 0, false, false, true,
                                NULL_REPORTER::GetInstance() );
        }

        if( payload.find( "no-annotate" ) == std::string::npos )
        {
            // Ensure schematic is OK for netlist creation (especially that it is fully annotated):
            if( !prepareForNetlist() )
                return;
        }

        {
            NETLIST_OBJECT_LIST* net_atoms = BuildNetListBase();
            NETLIST_EXPORTER_KICAD exporter( this, net_atoms, g_ConnectionGraph );
            STRING_FORMATTER formatter;

            exporter.Format( &formatter, GNL_ALL );

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
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks();
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

    default:
        ;
    }
}
