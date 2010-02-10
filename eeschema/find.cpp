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

#include "program.h"
#include "general.h"
#include "class_marker_sch.h"
#include "protos.h"
#include "class_library.h"

#include "kicad_device_context.h"

#include <wx/fdrepdlg.h>          // Use the wxFindReplaceDialog events, data, and enums.
#include <boost/foreach.hpp>


/* Define schematic specific find and replace dialog flags based on the enum entries
 * in wxFindReplaceFlags.   These flags are intended to be used as bit masks in the
 * wxFindReplaceData::m_Flags member variable.  The varialble is defined as a wxUint32.
 */
enum SchematicFindReplaceFlags
{
    /* The last wxFindReplaceFlag enum is wxFR_MATCHCASE. */

    /* Search the current sheet only. */
    schFR_CURRENT_SHEET_ONLY        = wxFR_MATCHCASE << 1,

    /* Search for design rule check markers. */
    schFR_DRC_MARKERS               = wxFR_MATCHCASE << 2,

    /* Search for component in all loaded libraries. */
    schFR_SEARCH_LIBS_FOR_COMPONENT = wxFR_MATCHCASE << 3
};


/* Variables Locales */
static int      s_ItemsCount, s_MarkerCount;
static wxString s_OldStringFound;

#include "dialog_find.cpp"


/*  Search markers in whole hierarchy.
 *  Mouse cursor is put on the marker
 *  search the first marker, or next marker
 */
void WinEDA_FindFrame::FindMarker( wxCommandEvent& event )
{
    int id = event.GetId();

    if( id != FIND_NEXT_MARKER )
        m_Parent->FindMarker( 0 );
    else
        m_Parent->FindMarker( 1 );

    Close();
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
SCH_ITEM* WinEDA_SchematicFrame::FindComponentAndItem(
    const wxString& component_reference, bool Find_in_hierarchy,
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
                        pos += pin->m_Pos;
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
        delta = TransformCoordinate( Component->m_Transform, pos );
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

    Affiche_Message( msg );

    return DrawList;
}


/* Search markers in whole the hierarchy.
 *  Mouse cursor is put on the marker
 *  SearchType = 0: search the first marker, else search next marker
 */
SCH_ITEM* WinEDA_SchematicFrame::FindMarker( int SearchType )
{
    SCH_SHEET_PATH* sheet, * FirstSheet = NULL;
    SCH_ITEM*       DrawList, * FirstStruct = NULL, * Struct = NULL;
    SCH_MARKER*     Marker = NULL;
    int             StartCount;
    bool            NotFound;
    wxPoint         firstpos, pos;
    wxSize          DrawAreaSize = DrawPanel->GetClientSize();
    wxPoint         curpos, old_cursor_position;
    bool            DoCenterAndRedraw = FALSE;
    wxString        msg, WildText;

    g_LastSearchIsMarker = TRUE;
    /* Set s_MarkerCount to 0 if we are look for the first marker */
    if( SearchType == 0 )
        s_MarkerCount = 0;

    SCH_SHEET_LIST SheetList;

    NotFound = TRUE; StartCount = 0;
    /* Search for s_MarkerCount markers */
    for( sheet = SheetList.GetFirst(); sheet != NULL;
         sheet = SheetList.GetNext() )
    {
        DrawList = (SCH_ITEM*) sheet->LastDrawList();
        while( DrawList && NotFound )
        {
            if( DrawList->Type() == TYPE_SCH_MARKER )
            {
                Marker   = (SCH_MARKER*) DrawList;
                NotFound = FALSE;
                pos = Marker->m_Pos;
                if( FirstSheet == NULL )    /* First item found */
                {
                    FirstSheet  = sheet; firstpos = pos;
                    FirstStruct = DrawList;
                }

                StartCount++;
                if( s_MarkerCount >= StartCount )
                {
                    NotFound = TRUE;       /* Search for other markers */
                }
                else                        /* We have found s_MarkerCount
                                             * markers -> Ok */
                {
                    Struct = DrawList; s_MarkerCount++; break;
                }
            }
            DrawList = DrawList->Next();
        }

        if( NotFound == FALSE )
            break;
    }

    if( NotFound && FirstSheet )       /* markers are found, but we have
                                        * reach the last marker */
    {
        // After the last marker, the first marker is used */
        NotFound = FALSE; sheet = FirstSheet;
        Struct   = FirstStruct;
        pos = firstpos; s_MarkerCount = 1;
    }

    if( NotFound == FALSE )
    {
        if( sheet != GetSheet() )
        {
            sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *sheet;
            ActiveScreen    = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
            DoCenterAndRedraw = TRUE;
        }

        old_cursor_position = sheet->LastScreen()->m_Curseur;
        sheet->LastScreen()->m_Curseur = pos;
        curpos = DrawPanel->CursorScreenPosition();

        DrawPanel->GetViewStart( &m_CurrentSheet->LastScreen()->m_StartVisu.x,
                                 &m_CurrentSheet->LastScreen()->m_StartVisu.y );
        curpos.x -= m_CurrentSheet->LastScreen()->m_StartVisu.x;
        curpos.y -= m_CurrentSheet->LastScreen()->m_StartVisu.y;

        // reposition the window if the chosen marker is off screen.
        #define MARGIN 30
        if( ( curpos.x <= MARGIN ) || ( curpos.x >= DrawAreaSize.x - MARGIN )
           || ( curpos.y <= MARGIN ) || ( curpos.y >= DrawAreaSize.y - MARGIN ) )
        {
            DoCenterAndRedraw = true;;
        }
        #undef MARGIN

        if( DoCenterAndRedraw )
            Recadre_Trace( TRUE );
        else
        {
            INSTALL_DC( dc, DrawPanel );
            EXCHG( old_cursor_position, sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOff( &dc );
            DrawPanel->MouseTo( curpos );
            EXCHG( old_cursor_position, sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOn( &dc );
        }
        wxString path = sheet->Path();
        msg.Printf( _( "Marker %d found in %s" ),
                    s_MarkerCount, path.GetData() );
        Affiche_Message( msg );
    }
    else
    {
        Affiche_Message( wxEmptyString );
        msg = _( "Marker Not Found" );
        DisplayError( this, msg, 10 );
    }

    return Marker;
}


/* Find a string in schematic.
 *  Call to WinEDA_SchematicFrame::FindSchematicItem()
 */
void WinEDA_FindFrame::FindSchematicItem( wxCommandEvent& event )
{
    int id = event.GetId();

    if( id == FIND_SHEET )
        m_Parent->FindSchematicItem( m_NewTextCtrl->GetValue(), 0 );
    else if( id == FIND_HIERARCHY )
        m_Parent->FindSchematicItem( m_NewTextCtrl->GetValue(), 1 );
    else if( id == FIND_NEXT )
        m_Parent->FindSchematicItem( wxEmptyString, 2 );

    Close();
}


/**
 * Function FindSchematicItem
 * finds a string in the schematic.
 * @param pattern The text to search for, either in value, reference or
 *                 elsewhere.
 * @param SearchType:  0 => Search is made in current sheet
 *                     1 => the whole hierarchy
 *                     2 => or for the next item
 * @param mouseWarp If true, then move the mouse cursor to the item.
 */
SCH_ITEM* WinEDA_SchematicFrame::FindSchematicItem( const wxString& pattern,
                                                    int             SearchType,
                                                    bool            mouseWarp )
{
    SCH_SHEET_PATH* Sheet, * FirstSheet = NULL;
    SCH_ITEM*       DrawList = NULL, * FirstStruct = NULL, * Struct = NULL;
    int             StartCount;
    bool            NotFound;
    wxPoint         firstpos, pos, old_cursor_position;
    static int      Find_in_hierarchy;
    wxSize          DrawAreaSize = DrawPanel->GetClientSize();
    wxPoint         curpos;
    bool            DoCenterAndRedraw = FALSE;
    wxString        msg, WildText;

    g_LastSearchIsMarker = FALSE;

    if( SearchType == 0 )
    {
        s_OldStringFound  = pattern;
        Find_in_hierarchy = FALSE;
    }

    if( SearchType == 1 )
    {
        s_OldStringFound  = pattern;
        Find_in_hierarchy = TRUE;
    }

    if(  SearchType != 2  )
        s_ItemsCount = 0;

    WildText   = s_OldStringFound;
    NotFound   = TRUE;
    StartCount = 0;

    SCH_SHEET_LIST SheetList;

    Sheet = SheetList.GetFirst();
    if( !Find_in_hierarchy )
        Sheet = m_CurrentSheet;

    for( ; Sheet != NULL; Sheet = SheetList.GetNext() )
    {
        DrawList = (SCH_ITEM*) Sheet->LastDrawList();
        while( DrawList )
        {
            switch( DrawList->Type() )
            {
            case TYPE_SCH_COMPONENT:
                SCH_COMPONENT * pSch;
                pSch = (SCH_COMPONENT*) DrawList;
                if( WildCompareString( WildText, pSch->GetRef( Sheet ), FALSE ) )
                {
                    NotFound = FALSE;
                    pos = pSch->GetField( REFERENCE )->m_Pos;
                    break;
                }
                if( WildCompareString( WildText,
                                       pSch->GetField( VALUE )->m_Text,
                                       false ) )
                {
                    NotFound = FALSE;
                    pos = pSch->GetField( VALUE )->m_Pos;
                }
                break;

            case TYPE_SCH_LABEL:
            case TYPE_SCH_GLOBALLABEL:
            case TYPE_SCH_HIERLABEL:
            case TYPE_SCH_TEXT:
                SCH_TEXT * pDraw;
                pDraw = (SCH_TEXT*) DrawList;
                if( WildCompareString( WildText, pDraw->m_Text, FALSE ) )
                {
                    NotFound = FALSE;
                    pos = pDraw->m_Pos;
                }
                break;

            default:
                break;
            }

            if( NotFound == FALSE )             /* Item found ! */
            {
                if( FirstSheet == NULL )        /* First Item found */
                {
                    FirstSheet  = Sheet;
                    firstpos    = pos;
                    FirstStruct = DrawList;
                }

                StartCount++;
                if( s_ItemsCount >= StartCount )
                {
                    NotFound = TRUE;    /* Continue search of the next element */
                }
                else
                {
                    Struct = DrawList;
                    s_ItemsCount++;
                    break;
                }
            }
            if( NotFound == FALSE )
                break;
            DrawList = DrawList->Next();
        }

        if( NotFound == FALSE )
            break;

        if( Find_in_hierarchy == FALSE )
            break;
    }

    if( NotFound && FirstSheet )
    {
        NotFound = FALSE;
        Sheet    = FirstSheet;
        Struct   = FirstStruct;
        pos = firstpos;
        s_ItemsCount = 1;
    }

    if( NotFound == FALSE )
    {
        if( Sheet != GetSheet() )
        {
            Sheet->LastScreen()->SetZoom( GetScreen()->GetZoom() );
            *m_CurrentSheet = *Sheet;
            ActiveScreen    = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
            DoCenterAndRedraw = TRUE;
        }

        /* the struct is a TYPE_SCH_COMPONENT type,
         * coordinates must be computed according to its orientation matrix
         */
        if( Struct->Type() == TYPE_SCH_COMPONENT )
        {
            SCH_COMPONENT* pSch = (SCH_COMPONENT*) Struct;

            pos -= pSch->m_Pos;
            pos  = TransformCoordinate( pSch->m_Transform, pos );
            pos += pSch->m_Pos;
        }

        old_cursor_position = Sheet->LastScreen()->m_Curseur;
        Sheet->LastScreen()->m_Curseur = pos;

        curpos = DrawPanel->CursorScreenPosition();

        DrawPanel->GetViewStart(
            &( GetScreen()->m_StartVisu.x ),
            &( GetScreen()->m_StartVisu.y ) );

        curpos -= m_CurrentSheet->LastScreen()->m_StartVisu;

        /* There may be need to reframe the drawing */
        #define MARGIN 30
        if( (curpos.x <= MARGIN) || (curpos.x >= DrawAreaSize.x - MARGIN)
           || (curpos.y <= MARGIN) || (curpos.y >= DrawAreaSize.y - MARGIN) )
        {
            DoCenterAndRedraw = true;
        }

        if( DoCenterAndRedraw )
            Recadre_Trace( mouseWarp );
        else
        {
            INSTALL_DC( dc, DrawPanel );

            EXCHG( old_cursor_position, Sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOff( &dc );

            if( mouseWarp )
                DrawPanel->MouseTo( curpos );

            EXCHG( old_cursor_position, Sheet->LastScreen()->m_Curseur );

            DrawPanel->CursorOn( &dc );
        }

        msg = WildText + _( " Found in " ) + Sheet->Last()->m_SheetName;
        Affiche_Message( msg );
    }
    else
    {
        Affiche_Message( wxEmptyString );

        if( !mouseWarp )
        {
            // if called from RemoteCommand() don't popup the dialog which
            // needs to be dismissed, user is in PCBNEW, and doesn't want to
            // bother with dismissing the dialog in EESCHEMA.
            msg = WildText + _( " Not Found" );
            DisplayError( this, msg, 10 );
        }
    }

    return DrawList;
}


/*
 * Search for a given component.
 *
 * The search is made in loaded libraries, and if not found in all libraries
 * found in lib paths.
 */
void WinEDA_FindFrame::LocatePartInLibs( wxCommandEvent& event )
{
    wxArrayString nameList;
    wxString      Text, FindList;
    bool          FoundInLib = false;

    Text = m_NewTextCtrl->GetValue();
    if( Text.IsEmpty() )
    {
        Close();
        return;
    }

    s_OldStringFound = Text;

    if( CMP_LIBRARY::GetLibraryCount() == 0 )
    {
        DisplayError( this, _( "No component libraries are loaded." ) );
        Close();
        return;
    }

    int nbitemsFound = 0;

    BOOST_FOREACH( CMP_LIBRARY& lib, CMP_LIBRARY::GetLibraryList() )
    {
        nameList.Clear();
        lib.SearchEntryNames( nameList, Text );

        if( nameList.IsEmpty() )
            continue;

        nbitemsFound += nameList.GetCount();

        if( !lib.IsCache() )
            FoundInLib = true;

        for( size_t i = 0; i < nameList.GetCount(); i++ )
        {
            if( !FindList.IsEmpty() )
                FindList += wxT( "\n" );
            FindList << _( "Found " ) + nameList[i] + _( " in library " )
                + lib.GetName();
        }
    }

    if( !FoundInLib )
    {
        if( nbitemsFound )
            FindList = wxT( "\n" ) + Text + _( " found only in cache" );
        else
            FindList = Text + _( " not found" );
        FindList += _( "\nExplore All Libraries?" );
        if( IsOK( this, FindList ) )
        {
            FindList.Empty();
            ExploreAllLibraries( Text, FindList );
            if( FindList.IsEmpty() )
                DisplayInfoMessage( this, _( "Nothing found" ) );
            else
                DisplayInfoMessage( this, FindList );
        }
    }
    else
    {
        DisplayInfoMessage( this, FindList );
    }

    Close();
}


int WinEDA_FindFrame::ExploreAllLibraries( const wxString& wildmask,
                                           wxString&       FindList )
{
    wxString FullFileName;
    FILE*    file;
    int      nbitems = 0, LineNum = 0;
    char     Line[2048], * name;
    wxString path;

    for( unsigned ii = 0; ii < wxGetApp().GetLibraryPathList().GetCount(); ii++ )
    {
        path = wxGetApp().GetLibraryPathList()[ii] + STRING_DIR_SEP;
        FullFileName = wxFindFirstFile( path + wxT( "*." ) +
                                        CompLibFileExtension );

        while( !FullFileName.IsEmpty() )
        {
            file = wxFopen( FullFileName, wxT( "rt" ) );
            if( file == NULL )
                continue;

            while( GetLine( file, Line, &LineNum, sizeof(Line) ) )
            {
                if( strnicmp( Line, "DEF", 3 ) == 0 )
                {
                    /* Read one DEF part from library:
                     * DEF 74LS00 U 0 30 Y Y 4 0 N */
                    strtok( Line, " \t\r\n" );
                    name = strtok( NULL, " \t\r\n" );
                    wxString st_name = CONV_FROM_UTF8( name );
                    if( WildCompareString( wildmask, st_name, FALSE ) )
                    {
                        nbitems++;
                        if( !FindList.IsEmpty() )
                            FindList += wxT( "\n" );
                        FindList << _( "Found " ) << CONV_FROM_UTF8( name )
                                 << _( " in lib " ) << FullFileName;
                    }
                }
                else if( strnicmp( Line, "ALIAS", 5 ) == 0 )
                {
                    /* Read one ALIAS part from library:
                     * ALIAS 74HC00 74HCT00 7400 74LS37 */
                    strtok( Line, " \t\r\n" );
                    while( ( name = strtok( NULL, " \t\r\n" ) ) != NULL )
                    {
                        wxString st_name = CONV_FROM_UTF8( name );
                        if( WildCompareString( wildmask, st_name, FALSE ) )
                        {
                            nbitems++;
                            if( !FindList.IsEmpty() )
                                FindList += wxT( "\n" );
                            FindList << _( "Found " ) << CONV_FROM_UTF8( name )
                                     << _( " in lib " ) << FullFileName;
                        }
                    }
                }
            }

            fclose( file );
            FullFileName = wxFindNextFile();
        }
    }

    return nbitems;
}
