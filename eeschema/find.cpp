/****************************************************************/
/* EESchema: find.cpp (functions for searching a schematic item */
/****************************************************************/

/*
 *  Search a text (text, value, reference) within a component or
 *  search a component in libraries, a marker ...,
 *  in current sheet or whole the project
 */
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "lib_pin.h"
#include "sch_marker.h"
#include "sch_component.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"

#include "kicad_device_context.h"

#include <boost/foreach.hpp>

#include "dialogs/dialog_schematic_find.h"


void SCH_EDIT_FRAME::OnFindDrcMarker( wxFindDialogEvent& event )
{
    static SCH_MARKER* lastMarker = NULL;

    wxString           msg;
    SCH_SHEET_LIST     schematic;
    SCH_SHEET_PATH*    sheetFoundIn = NULL;
    bool               wrap = ( event.GetFlags() & FR_SEARCH_WRAP ) != 0;
    wxRect             clientRect( wxPoint( 0, 0 ), GetClientSize() );

    if( event.GetFlags() & FR_CURRENT_SHEET_ONLY )
    {
        sheetFoundIn = m_CurrentSheet;
        lastMarker = (SCH_MARKER*) m_CurrentSheet->FindNextItem( TYPE_SCH_MARKER,
                                                                 lastMarker, wrap );
    }
    else
    {
        lastMarker = (SCH_MARKER*) schematic.FindNextItem( TYPE_SCH_MARKER, &sheetFoundIn,
                                                           lastMarker, wrap );
    }

    if( lastMarker != NULL )
    {
        if( sheetFoundIn != GetSheet() )
        {
            sheetFoundIn->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheetFoundIn;
            ActiveScreen    = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
        }

        sheetFoundIn->LastScreen()->m_Curseur = lastMarker->m_Pos;

        Recadre_Trace( TRUE );

        wxString path = sheetFoundIn->Path();
        wxString units = GetAbbreviatedUnitsLabel();
        double x = To_User_Unit( g_UserUnit, (double) lastMarker->m_Pos.x, m_InternalUnits );
        double y = To_User_Unit( g_UserUnit, (double) lastMarker->m_Pos.y, m_InternalUnits );
        msg.Printf( _( "Design rule check marker found in sheet %s at %0.3f%s, %0.3f%s" ),
                    GetChars( path ), x, GetChars( units ), y, GetChars( units) );
        SetStatusText( msg );
    }
    else
    {
        SetStatusText( _( "No more markers were found." ) );
    }
}


/**
 * Function FindComponentAndItem
 * finds a Component in the schematic, and an item in this component.
 * @param component_reference The component reference to find.
 * @param text_to_find - The text to search for, either in value, reference
 *                       or elsewhere.
 * @param Find_in_hierarchy:  false => Search is made in current sheet
 *                     true => the whole hierarchy
 * @param SearchType:  0 => find component
 *                     1 => find pin
 *                     2 => find ref
 *                     3 => find value
 *                     >= 4 => unused (same as 0)
 * @param mouseWarp If true, then move the mouse cursor to the item.
 */
SCH_ITEM* SCH_EDIT_FRAME::FindComponentAndItem( const wxString& component_reference,
                                                bool Find_in_hierarchy,
                                                int SearchType,
                                                const wxString& text_to_find,
                                                bool mouseWarp )
{
    SCH_SHEET_PATH* sheet, * SheetWithComponentFound = NULL;
    SCH_ITEM*       DrawList     = NULL;
    SCH_COMPONENT*  Component    = NULL;
    wxSize          DrawAreaSize = DrawPanel->GetClientSize();
    wxPoint         pos, curpos;
    bool            DoCenterAndRedraw = FALSE;
    bool            NotFound = true;
    wxString        msg;
    LIB_PIN*        pin;
    SCH_SHEET_LIST  SheetList;

    sheet = SheetList.GetFirst();
    if( !Find_in_hierarchy )
        sheet = m_CurrentSheet;

    for( ; sheet != NULL; sheet = SheetList.GetNext() )
    {
        DrawList = (SCH_ITEM*) sheet->LastDrawList();
        for( ; ( DrawList != NULL ) && ( NotFound == true );
             DrawList = DrawList->Next() )
        {
            if( DrawList->Type() == TYPE_SCH_COMPONENT )
            {
                SCH_COMPONENT* pSch;
                pSch = (SCH_COMPONENT*) DrawList;
                if( component_reference.CmpNoCase( pSch->GetRef( sheet ) ) == 0 )
                {
                    Component = pSch;
                    SheetWithComponentFound = sheet;

                    switch( SearchType )
                    {
                    default:
                    case 0:             // Find component only
                        NotFound = FALSE;
                        pos = pSch->m_Pos;
                        break;

                    case 1:                 // find a pin
                        pos = pSch->m_Pos;  /* temporary: will be changed if
                                             * the pin is found */
                        pin = pSch->GetPin( text_to_find );

                        if( pin == NULL )
                            break;

                        NotFound = FALSE;
                        pos += pin->GetPosition();
                        break;

                    case 2:     // find reference
                        NotFound = FALSE;
                        pos = pSch->GetField( REFERENCE )->m_Pos;
                        break;

                    case 3:     // find value
                        pos = pSch->m_Pos;
                        if( text_to_find.CmpNoCase( pSch->GetField( VALUE )->m_Text ) != 0 )
                            break;
                        NotFound = FALSE;
                        pos = pSch->GetField( VALUE )->m_Pos;
                        break;
                    }
                }
            }
        }

        if( (Find_in_hierarchy == FALSE) || (NotFound == FALSE) )
            break;
    }

    if( Component )
    {
        sheet = SheetWithComponentFound;

        if( sheet != GetSheet() )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheet;
            ActiveScreen    = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
            DoCenterAndRedraw = TRUE;
        }
        wxPoint delta;
        pos  -= Component->m_Pos;
        delta = Component->m_Transform.TransformCoordinate( pos );
        pos   = delta + Component->m_Pos;

        wxPoint old_cursor_position    = sheet->LastScreen()->m_Curseur;
        sheet->LastScreen()->m_Curseur = pos;

        curpos = DrawPanel->CursorScreenPosition();

        DrawPanel->GetViewStart(
            &( GetScreen()->m_StartVisu.x ),
            &( GetScreen()->m_StartVisu.y ) );

        // Calculating cursor position with original screen.
        curpos -= GetScreen()->m_StartVisu;

        /* There may be need to reframe the drawing */
        #define MARGIN 30
        if( ( curpos.x <= MARGIN ) || ( curpos.x >= DrawAreaSize.x - MARGIN )
           || ( curpos.y <= MARGIN ) || ( curpos.y >= DrawAreaSize.y - MARGIN ) )
        {
            DoCenterAndRedraw = true;;
        }
        #undef MARGIN

        if( DoCenterAndRedraw )
            Recadre_Trace( mouseWarp );
        else
        {
            INSTALL_DC( dc, DrawPanel );

            EXCHG( old_cursor_position, sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOff( &dc );

            if( mouseWarp )
                DrawPanel->MouseTo( curpos );

            EXCHG( old_cursor_position, sheet->LastScreen()->m_Curseur );

            DrawPanel->CursorOn( &dc );
        }
    }


    /* Print diaq */
    wxString msg_item;
    msg = component_reference;

    switch( SearchType )
    {
    default:
    case 0:
        break;      // Find component only

    case 1:         // find a pin
        msg_item = _( "Pin " ) + text_to_find;
        break;

    case 2:     // find reference
        msg_item = _( "Ref " ) + text_to_find;
        break;

    case 3:     // find value
        msg_item = _( "Value " ) + text_to_find;
        break;

    case 4:     // find field. todo
        msg_item = _( "Field " ) + text_to_find;
        break;
    }

    if( Component )
    {
        if( !NotFound )
        {
            if( !msg_item.IsEmpty() )
                msg += wxT( " " ) + msg_item;
            msg += _( " found" );
        }
        else
        {
            msg += _( " found" );
            if( !msg_item.IsEmpty() )
            {
                msg += wxT( " but " ) + msg_item + _( " not found" );
            }
        }
    }
    else
    {
        if( !msg_item.IsEmpty() )
            msg += wxT( " " ) + msg_item;
        msg += _( " not found" );
    }

    SetStatusText( msg );

    return DrawList;
}


/**
 * Finds an item in the schematic matching the search string.
 *
 * @param event - Find dialog event containing the find parameters.
 */
void SCH_EDIT_FRAME::OnFindSchematicItem( wxFindDialogEvent& event )
{
    static SCH_ITEM*  lastItem = NULL;  /* last item found when searching a match
                                         * note: the actual matched item can be a
                                         * part of lastItem (for instance a field in a component
                                         */
    static wxPoint  lastItemPosition;   // the actual position of the matched sub item

    SCH_SHEET_LIST    schematic;
    wxString          msg;
    SCH_SHEET_PATH*   sheetFoundIn = NULL;
    wxFindReplaceData searchCriteria;

    searchCriteria.SetFlags( event.GetFlags() );
    searchCriteria.SetFindString( event.GetFindString() );
    searchCriteria.SetReplaceString( event.GetReplaceString() );

    if( event.GetFlags() & FR_CURRENT_SHEET_ONLY && g_RootSheet->CountSheets() > 1 )
    {
        sheetFoundIn = m_CurrentSheet;
        lastItem = m_CurrentSheet->MatchNextItem( searchCriteria, lastItem, &lastItemPosition );
    }
    else
    {
        lastItem = schematic.MatchNextItem( searchCriteria, &sheetFoundIn, lastItem,
                                            &lastItemPosition );
    }

    if( lastItem != NULL )
    {
        if( sheetFoundIn != GetSheet() )
        {
            sheetFoundIn->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheetFoundIn;
            ActiveScreen    = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
        }

//        sheetFoundIn->LastScreen()->m_Curseur = lastItem->GetBoundingBox().Centre();
        sheetFoundIn->LastScreen()->m_Curseur = lastItemPosition;
        Recadre_Trace( true );

        msg = event.GetFindString() + _( " found in " ) + sheetFoundIn->PathHumanReadable();
        SetStatusText( msg );
    }
    else
    {
        msg.Printf( _( "No item found matching %s." ), GetChars( event.GetFindString() ) );
        SetStatusText( msg );
    }
}
