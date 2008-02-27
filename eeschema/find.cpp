/****************************************************************/
/* EESchema: find.cpp (functions for seraching a schematic item */
/****************************************************************/

/*
 *  Search a text (text, value, reference) within a component or
 *  search a component in libraries, a marker ...,
 *  in current sheet or whole the project
 */
#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "schframe.h"

/* Variables Locales */
static int      s_ItemsCount, s_MarkerCount;
static wxString s_OldStringFound;

#include "dialog_find.cpp"


#include "protos.h"


/**************************************************************/
void WinEDA_FindFrame::FindMarker( wxCommandEvent& event )
/**************************************************************/

/*  Search markers in whole hierarchy.
 *  Mouse cursor is put on the marker
 *  search the first marker, or next marker
 */
{
    int id = event.GetId();

    if( id != FIND_NEXT_MARKER )
        m_Parent->FindMarker( 0 );
    else
        m_Parent->FindMarker( 1 );

    Close();
}


/************************************************************************/
EDA_BaseStruct* WinEDA_SchematicFrame::FindComponentAndItem(
    const wxString& component_reference, bool Find_in_hierarchy,
    int SearchType,
    const wxString& text_to_find,
    bool mouseWarp )
/************************************************************************/

/**
 * Function FindComponentAndItem
 * finds a Component in the schematic, and an item in this component.
 * @param component_reference The component reference to find.
 * @param text_to_find The text to search for, either in value, reference or elsewhere.
 * @param Find_in_hierarchy:  false => Search is made in current sheet
 *                     true => the whole hierarchy
 * @param SearchType:  0 => find component
 *                     1 => find pin
 *                     2 => find ref
 *                     3 => find value
 *                     >= 4 => unused (same as 0)
 * @param mouseWarp If true, then move the mouse cursor to the item.
 */
{
    DrawSheetList*          sheet, * SheetWithComponentFound = NULL;
    EDA_BaseStruct*         DrawList  = NULL;
    EDA_SchComponentStruct* Component = NULL;
    wxSize                  DrawAreaSize = DrawPanel->GetClientSize();
    wxPoint                 pos, curpos;
    bool                    DoCenterAndRedraw = FALSE;
    bool                    NotFound = true;
    wxString                msg;
    LibDrawPin*             pin;

    EDA_SheetList          SheetList( NULL );

    sheet = SheetList.GetFirst();
    if( !Find_in_hierarchy )
        sheet = m_CurrentSheet;

    for( ; sheet != NULL; sheet = SheetList.GetNext() )
    {
        DrawList = sheet->LastDrawList();
        for( ; (DrawList != NULL) && (NotFound == true); DrawList = DrawList->Pnext )
        {
            if( DrawList->Type() == DRAW_LIB_ITEM_STRUCT_TYPE )
            {
                EDA_SchComponentStruct* pSch;
                pSch = (EDA_SchComponentStruct*) DrawList;
                if( component_reference.CmpNoCase( pSch->GetRef(sheet) ) == 0 )
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
                        pos = pSch->m_Pos;  // temporary: will be changed if the pin is found
                        pin = LocatePinByNumber( text_to_find, pSch );
                        if( pin == NULL )
                            break;
                        NotFound = FALSE;
                        pos += pin->m_Pos;
                        break;

                    case 2:     // find reference
                        NotFound = FALSE;
                        pos = pSch->m_Field[REFERENCE].m_Pos;
                        break;

                    case 3:     // find value
                        pos = pSch->m_Pos;
                        if( text_to_find.CmpNoCase( pSch->m_Field[VALUE].m_Text ) != 0 )
                            break;
                        NotFound = FALSE;
                        pos = pSch->m_Field[VALUE].m_Pos;
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
            ActiveScreen = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
            DoCenterAndRedraw   = TRUE;
        }
        wxPoint delta;
        pos -= Component->m_Pos;

        delta.x = Component->m_Transform[0][0] * pos.x + Component->m_Transform[0][1] * pos.y;
        delta.y = Component->m_Transform[1][0] * pos.x + Component->m_Transform[1][1] * pos.y;

        pos = delta + Component->m_Pos;

        wxPoint old_cursor_position = sheet->LastScreen()->m_Curseur;
        sheet->LastScreen()->m_Curseur = pos;

        curpos = DrawPanel->CursorScreenPosition();

        DrawPanel->GetViewStart(
            &( GetScreen()->m_StartVisu.x ),
            &( GetScreen()->m_StartVisu.y ));

        // calcul des coord curseur avec origine = screen
        curpos.x -= GetScreen()->m_StartVisu.x;
        curpos.y -= GetScreen()->m_StartVisu.y;

        /* Il y a peut-etre necessite de recadrer le dessin: */
        #define MARGIN 30
        if( (curpos.x <= MARGIN) || (curpos.x >= DrawAreaSize.x - MARGIN)
           || (curpos.y <= MARGIN) || (curpos.y >= DrawAreaSize.y - MARGIN) )
        {
            DoCenterAndRedraw = true;;
        }
        #undef MARGIN

        if ( DoCenterAndRedraw )
            Recadre_Trace( mouseWarp );
        else
        {
            wxClientDC  dc( DrawPanel );

            DrawPanel->PrepareGraphicContext( &dc );

            EXCHG( old_cursor_position, sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOff( &dc );

            if( mouseWarp )
                GRMouseWarp( DrawPanel, curpos );

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


/*****************************************************************/
EDA_BaseStruct* WinEDA_SchematicFrame::FindMarker( int SearchType )
/*****************************************************************/

/* Search markers in whole the hierarchy.
 *  Mouse cursor is put on the marker
 *  SearchType = 0: search the first marker, else search next marker
 */
{
    DrawSheetList*  sheet, * FirstSheet = NULL;
    EDA_BaseStruct*   DrawList, * FirstStruct = NULL, * Struct = NULL;
    DrawMarkerStruct* Marker = NULL;
    int StartCount;
    bool NotFound;
    wxPoint           firstpos, pos;
    wxSize            DrawAreaSize = DrawPanel->GetClientSize();
    wxPoint           curpos, old_cursor_position;
    bool DoCenterAndRedraw = FALSE;
    wxString          msg, WildText;

    g_LastSearchIsMarker = TRUE;
    /* Set s_MarkerCount to 0 if we are look for the first marker */
    if( SearchType == 0 )
        s_MarkerCount = 0;

    EDA_SheetList SheetList( NULL );

    NotFound = TRUE; StartCount = 0;
    /* Search for s_MarkerCount markers */
    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        DrawList = sheet->LastDrawList();
        while( DrawList && NotFound )
        {
            if( DrawList->Type() == DRAW_MARKER_STRUCT_TYPE )
            {
                Marker   = (DrawMarkerStruct*) DrawList;
                NotFound = FALSE;
                pos = Marker->m_Pos;
                if( FirstSheet == NULL )    /* First item found */
                {
                    FirstSheet = sheet; firstpos = pos;
                    FirstStruct = DrawList;
                }

                StartCount++;
                if( s_MarkerCount >= StartCount )
                {
                    NotFound = TRUE;        /* Search for other markers */
                }
                else                        /* We have found s_MarkerCount markers -> Ok */
                {
                    Struct = DrawList; s_MarkerCount++; break;
                }
            }
            DrawList = DrawList->Pnext;
        }

        if( NotFound == FALSE )
            break;
    }

    if( NotFound && FirstSheet )       // markers are found, but we have reach the last marker */
    {                                   // After the last marker, the first marker is used */
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
            ActiveScreen = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
            DoCenterAndRedraw   = TRUE;
        }

        old_cursor_position = sheet->LastScreen()->m_Curseur;
        sheet->LastScreen()->m_Curseur   = pos;
        curpos = DrawPanel->CursorScreenPosition();

        // calcul des coord curseur avec origine = screen
        DrawPanel->GetViewStart( &m_CurrentSheet->LastScreen()->m_StartVisu.x,
                                  &m_CurrentSheet->LastScreen()->m_StartVisu.y );
        curpos.x -= m_CurrentSheet->LastScreen()->m_StartVisu.x;
        curpos.y -= m_CurrentSheet->LastScreen()->m_StartVisu.y;

        // reposition the window if the chosen marker is off screen.
        #define MARGIN 30
        if( (curpos.x <= MARGIN) || (curpos.x >= DrawAreaSize.x - MARGIN)
           || (curpos.y <= MARGIN) || (curpos.y >= DrawAreaSize.y - MARGIN) )
        {
            DoCenterAndRedraw = true;;
        }
        #undef MARGIN

        if( DoCenterAndRedraw )
            Recadre_Trace( TRUE );
        else
        {
            wxClientDC dc( DrawPanel );

            DrawPanel->PrepareGraphicContext( &dc );
            EXCHG( old_cursor_position, sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOff( &dc );
            GRMouseWarp( DrawPanel, curpos );
            EXCHG( old_cursor_position, sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOn( &dc );
        }
        wxString path = sheet->Path();
        msg.Printf( _( "Marker %d found in %s" ), s_MarkerCount, path.GetData() );
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


/**************************************************************/
void WinEDA_FindFrame::FindSchematicItem( wxCommandEvent& event )
/**************************************************************/

/* Find a string in schematic.
 *  Call to WinEDA_SchematicFrame::FindSchematicItem()
 */
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


/************************************************************************/
EDA_BaseStruct* WinEDA_SchematicFrame::FindSchematicItem(
    const wxString& pattern, int SearchType, bool mouseWarp )
/************************************************************************/

/**
 * Function FindSchematicItem
 * finds a string in the schematic.
 * @param pattern The text to search for, either in value, reference or elsewhere.
 * @param SearchType:  0 => Search is made in current sheet
 *                     1 => the whole hierarchy
 *                     2 => or for the next item
 * @param mouseWarp If true, then move the mouse cursor to the item.
 */
{
    DrawSheetList*     Sheet, * FirstSheet = NULL;
    EDA_BaseStruct* DrawList = NULL, * FirstStruct = NULL, * Struct = NULL;
    int             StartCount, ii, jj;
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

    EDA_SheetList SheetList( NULL );

    Sheet = SheetList.GetFirst();
    if( !Find_in_hierarchy )
        Sheet = m_CurrentSheet;

    for( ; Sheet != NULL; Sheet = SheetList.GetNext() )
    {
        DrawList = Sheet->LastDrawList();
        while( DrawList )
        {
            switch( DrawList->Type() )
            {
            case DRAW_LIB_ITEM_STRUCT_TYPE:
                EDA_SchComponentStruct * pSch;
                pSch = (EDA_SchComponentStruct*) DrawList;
                if( WildCompareString( WildText, pSch->GetRef(Sheet), FALSE ) )
                {
                    NotFound = FALSE;
                    pos = pSch->m_Field[REFERENCE].m_Pos;
                    break;
                }
                if( WildCompareString( WildText, pSch->m_Field[VALUE].m_Text, FALSE ) )
                {
                    NotFound = FALSE;
                    pos = pSch->m_Field[VALUE].m_Pos;
                }
                break;

            case DRAW_LABEL_STRUCT_TYPE:
            case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
            case DRAW_HIER_LABEL_STRUCT_TYPE:
            case DRAW_TEXT_STRUCT_TYPE:
                DrawTextStruct * pDraw;
                pDraw = (DrawTextStruct*) DrawList;
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
                if( FirstSheet == NULL )       /* First Item found */
                {
                    FirstSheet = Sheet;
                    firstpos    = pos;
                    FirstStruct = DrawList;
                }

                StartCount++;
                if( s_ItemsCount >= StartCount )
                {
                    NotFound = TRUE;    /* Continue recherche de l'element suivant */
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
            DrawList = DrawList->Pnext;
        }

        if( NotFound == FALSE )
            break;

        if( Find_in_hierarchy == FALSE )
            break;
    }

    if( NotFound && FirstSheet )
    {
        NotFound = FALSE;
        Sheet   = FirstSheet;
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
            ActiveScreen = m_CurrentSheet->LastScreen();
            m_CurrentSheet->UpdateAllScreenReferences();
            DoCenterAndRedraw   = TRUE;
        }

        /* the struct is a DRAW_LIB_ITEM_STRUCT_TYPE type,
         * coordinates must be computed according to its orientation matrix
         */
        if( Struct->Type() == DRAW_LIB_ITEM_STRUCT_TYPE )
        {
            EDA_SchComponentStruct* pSch = (EDA_SchComponentStruct*) Struct;

            pos.x -= pSch->m_Pos.x;
            pos.y -= pSch->m_Pos.y;

            ii = pSch->m_Transform[0][0] * pos.x + pSch->m_Transform[0][1] * pos.y;
            jj = pSch->m_Transform[1][0] * pos.x + pSch->m_Transform[1][1] * pos.y;

            pos.x = ii + pSch->m_Pos.x;
            pos.y = jj + pSch->m_Pos.y;
        }

        old_cursor_position = Sheet->LastScreen()->m_Curseur;
        Sheet->LastScreen()->m_Curseur   = pos;

        curpos = DrawPanel->CursorScreenPosition();

        DrawPanel->GetViewStart(
                &( GetScreen()->m_StartVisu.x ),
                &( GetScreen()->m_StartVisu.y ));

        // calcul des coord curseur avec origine = screen
        curpos.x -= m_CurrentSheet->LastScreen()->m_StartVisu.x;
        curpos.y -= m_CurrentSheet->LastScreen()->m_StartVisu.y;

        /* Il y a peut-etre necessite de recadrer le dessin: */
        #define MARGIN 30
        if( (curpos.x <= MARGIN) || (curpos.x >= DrawAreaSize.x - MARGIN)
           || (curpos.y <= MARGIN) || (curpos.y >= DrawAreaSize.y - MARGIN) )
        {
           DoCenterAndRedraw = true;
        }

        if ( DoCenterAndRedraw )
            Recadre_Trace( mouseWarp );
        else
        {
            wxClientDC  dc( DrawPanel );

            DrawPanel->PrepareGraphicContext( &dc );

            EXCHG( old_cursor_position, Sheet->LastScreen()->m_Curseur );
            DrawPanel->CursorOff( &dc );

            if( mouseWarp )
                GRMouseWarp( DrawPanel, curpos );

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
            // needs to be dismissed, user is in PCBNEW, and does'nt want to
            // bother with dismissing the dialog in EESCHEMA.
            msg = WildText + _( " Not Found" );
            DisplayError( this, msg, 10 );
        }
    }

    return DrawList;
}


/*************************************************************/
void WinEDA_FindFrame::LocatePartInLibs( wxCommandEvent& event )
/*************************************************************/

/* Recherche exhaustive d'un composant en librairies, meme non chargees
 */
{
    wxString                Text, FindList;
    const wxChar**          ListNames;
    LibraryStruct*          Lib = NULL;
    EDA_LibComponentStruct* LibEntry;
    bool FoundInLib = FALSE;     // True si reference trouvee ailleurs qu'en cache

    Text = m_NewTextCtrl->GetValue();
    if( Text.IsEmpty() )
    {
        Close(); return;
    }
    s_OldStringFound = Text;

    int ii, nbitems, NumOfLibs = NumOfLibraries();
    if( NumOfLibs == 0 )
    {
        DisplayError( this, _( "No libraries are loaded" ) );
        Close(); return;
    }

    ListNames = GetLibNames();

    nbitems = 0;
    for( ii = 0; ii < NumOfLibs; ii++ )    /* Recherche de la librairie */
    {
        bool IsLibCache;
        Lib = FindLibrary( ListNames[ii] );
        if( Lib == NULL )
            break;
        if( Lib->m_Name.Contains( wxT( ".cache" ) ) )
            IsLibCache = TRUE;
        else
            IsLibCache = FALSE;
        LibEntry = (EDA_LibComponentStruct*) PQFirst( &Lib->m_Entries, FALSE );
        while( LibEntry )
        {
            if( WildCompareString( Text, LibEntry->m_Name.m_Text, FALSE ) )
            {
                nbitems++;
                if( !IsLibCache )
                    FoundInLib = TRUE;
                if( !FindList.IsEmpty() )
                    FindList += wxT( "\n" );
                FindList << _( "Found " )
                + LibEntry->m_Name.m_Text
                + _( " in lib " ) + Lib->m_Name;
            }
            LibEntry = (EDA_LibComponentStruct*) PQNext( Lib->m_Entries, LibEntry, NULL );
        }
    }

    free( ListNames );

    if( !FoundInLib )
    {
        if( nbitems )
            FindList = wxT( "\n" ) + Text + _( " found only in cache" );
        else
            FindList = Text + _( " not found" );
        FindList += _( "\nExplore All Libraries?" );
        if( IsOK( this, FindList ) )
        {
            FindList.Empty();
            ExploreAllLibraries( Text, FindList );
            if( FindList.IsEmpty() )
                DisplayInfo( this, _( "Nothing found" ) );
            else
                DisplayInfo( this, FindList );
        }
    }
    else
        DisplayInfo( this, FindList );

    Close();
}


/***************************************************************************************/
int WinEDA_FindFrame::ExploreAllLibraries( const wxString& wildmask, wxString& FindList )
/***************************************************************************************/
{
    wxString FullFileName;
    FILE*    file;
    int      nbitems = 0, LineNum = 0;
    char     Line[2048], * name;

    FullFileName = MakeFileName( g_RealLibDirBuffer, wxT( "*" ), g_LibExtBuffer );

    FullFileName = wxFindFirstFile( FullFileName );
    while( !FullFileName.IsEmpty() )
    {
        file = wxFopen( FullFileName, wxT( "rt" ) );
        if( file == NULL )
            continue;

        while( GetLine( file, Line, &LineNum, sizeof(Line) ) )
        {
            if( strnicmp( Line, "DEF", 3 ) == 0 )
            {
                /* Read one DEF part from library: DEF 74LS00 U 0 30 Y Y 4 0 N */
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
                /* Read one ALIAS part from library: ALIAS 74HC00 74HCT00 7400 74LS37 */
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

    return nbitems;
}
