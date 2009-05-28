/////////////////////////////////////////////////////////////////////////////

// Name:        class_drawsheet.cpp
// Purpose:		member functions for DrawSheetStruct
//				header = class_drawsheet.h
// Author:      jean-pierre Charras
// Modified by:
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:
// Licence:     License GNU
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "confirm.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"


/***********************************************************/
DrawSheetStruct::DrawSheetStruct( const wxPoint& pos ) :
    SCH_ITEM( NULL, DRAW_SHEET_STRUCT_TYPE )
/***********************************************************/
{
    m_Label            = NULL;
    m_NbLabel          = 0;
    m_Layer            = LAYER_SHEET;
    m_Pos              = pos;
    m_TimeStamp        = GetTimeStamp();
    m_SheetNameSize    = m_FileNameSize = 60;
    m_AssociatedScreen = NULL;
    m_SheetName.Printf( wxT( "Sheet%8.8lX" ), m_TimeStamp );
    m_FileName.Printf( wxT( "file%8.8lX.sch" ), m_TimeStamp );
}


/**************************************/
DrawSheetStruct::~DrawSheetStruct()
/**************************************/
{
    Hierarchical_PIN_Sheet_Struct* label = m_Label, * next_label;

    while( label )
    {
        next_label = label->Next();
        delete label;
        label = next_label;
    }

    //also, look at the associated sheet & its reference count
    //perhaps it should be deleted also.
    if( m_AssociatedScreen )
    {
        m_AssociatedScreen->m_RefCount--;
        if( m_AssociatedScreen->m_RefCount == 0 )
            delete m_AssociatedScreen;
    }
}


/**********************************************/
bool DrawSheetStruct::Save( FILE* aFile ) const
/***********************************************/

/** Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
{
    bool Success = true;
    Hierarchical_PIN_Sheet_Struct* SheetLabel;

    fprintf( aFile, "$Sheet\n" );

    if( fprintf( aFile, "S %-4d %-4d %-4d %-4d\n",
                 m_Pos.x, m_Pos.y,
                 m_Size.x, m_Size.y ) == EOF )
    {
        Success = false;
        return Success;
    }

    //save the unique timestamp, like other shematic parts.
    if( fprintf( aFile, "U %8.8lX\n", m_TimeStamp )  == EOF )
    {
        Success = false; return Success;
    }

    /* Generation de la liste des 2 textes (sheetname et filename) */
    if( !m_SheetName.IsEmpty() )
    {
        if( fprintf( aFile, "F0 \"%s\" %d\n", CONV_TO_UTF8( m_SheetName ),
                     m_SheetNameSize ) == EOF )
        {
            Success = false; return Success;
        }
    }

    if( !m_FileName.IsEmpty() )
    {
        if( fprintf( aFile, "F1 \"%s\" %d\n", CONV_TO_UTF8( m_FileName ),
                     m_FileNameSize ) == EOF )
        {
            Success = false; return Success;
        }
    }

    /* Generation de la liste des labels (entrees) de la sous feuille */
    SheetLabel = m_Label;
    int l_id = 2;
    while( SheetLabel != NULL )
    {
        SheetLabel->m_Number = l_id;
        SheetLabel->Save( aFile );
        l_id++;
        SheetLabel = SheetLabel->Next();
    }

    fprintf( aFile, "$EndSheet\n" );
    return Success;
}


/***********************************************/
DrawSheetStruct* DrawSheetStruct::GenCopy()
/***********************************************/

/* creates a copy of a sheet
 *  The linked data itself (EEDrawList) is not duplicated
 */
{
    DrawSheetStruct* newitem = new DrawSheetStruct( m_Pos );


    newitem->m_Size = m_Size;
    newitem->SetParent( m_Parent );
    newitem->m_TimeStamp = GetTimeStamp();

    newitem->m_FileName      = m_FileName;
    newitem->m_FileNameSize  = m_FileNameSize;
    newitem->m_SheetName     = m_SheetName;
    newitem->m_SheetNameSize = m_SheetNameSize;

    newitem->m_Label = NULL;

    Hierarchical_PIN_Sheet_Struct* Slabel = NULL, * label = m_Label;

    if( label )
    {
        Slabel = newitem->m_Label = label->GenCopy();
        Slabel->SetParent( newitem );
        label = label->Next();
    }

    while( label )
    {
        Slabel->SetNext( label->GenCopy() );
        Slabel = Slabel->Next();
        Slabel->SetParent( newitem );
        label = label->Next();
    }

    /* don't copy screen data - just reference it. */
    newitem->m_AssociatedScreen = m_AssociatedScreen;
    if( m_AssociatedScreen )
        m_AssociatedScreen->m_RefCount++;

    return newitem;
}


/**********************************************************/
void DrawSheetStruct::SwapData( DrawSheetStruct* copyitem )
/**********************************************************/

/* Used if undo / redo command:
 *  swap data between this and copyitem
 */
{
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_SheetName, copyitem->m_SheetName );
    EXCHG( m_SheetNameSize, copyitem->m_SheetNameSize );
    EXCHG( m_FileNameSize, copyitem->m_FileNameSize );
    EXCHG( m_Label, copyitem->m_Label );
    EXCHG( m_NbLabel, copyitem->m_NbLabel );

    // Ensure sheet labels have their .m_Parent member poiuntin really on their parent, after swapping.
    Hierarchical_PIN_Sheet_Struct* label = m_Label;
    while( label )
    {
        label->SetParent( this );
        label = label->Next();
    }

    label = copyitem->m_Label;
    while( label )
    {
        label->SetParent( copyitem );
        label = label->Next();
    }
}


/********************************************************************/
void DrawSheetStruct::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
/********************************************************************/
{
    /* Placement en liste des structures si nouveau composant:*/
    bool isnew = (m_Flags & IS_NEW) ? true : false;
    if( isnew )
    {
        if( !frame->EditSheet( this, DC ) )
        {
            frame->GetScreen()->SetCurItem( NULL );
            frame->DrawPanel->ManageCurseur = NULL;
            frame->DrawPanel->ForceCloseManageCurseur = NULL;
            RedrawOneStruct( frame->DrawPanel, DC, this, g_XorMode );
            delete this;
            return;
        }
    }

    SCH_ITEM::Place( frame, DC ); //puts it on the EEDrawList.
    if ( isnew )
    {
        frame->SetSheetNumberAndCount();
    }
}


/********************************************************************/
void DrawSheetStruct::CleanupSheet( WinEDA_SchematicFrame* aFrame, bool aRedraw )
/********************************************************************/

/** Function CleanupSheet
 * Delete pinsheets which are not corresponding to a hierarchal label
 * @param aRedraw = true to redraw Sheet
 * @param aFrame = the schematic frame
 */
{
    Hierarchical_PIN_Sheet_Struct* Pinsheet, * NextPinsheet;

    if( !IsOK( aFrame, _( "Ok to cleanup this sheet" ) ) )
        return;

    Pinsheet = m_Label;
    while( Pinsheet )
    {
        /* Search Hlabel corresponding to this Pinsheet */

        EDA_BaseStruct* DrawStruct = m_AssociatedScreen->EEDrawList;
        SCH_HIERLABEL*  HLabel     = NULL;
        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != TYPE_SCH_HIERLABEL )
                continue;

            HLabel = (SCH_HIERLABEL*) DrawStruct;
            if( Pinsheet->m_Text.CmpNoCase( HLabel->m_Text ) == 0 )
                break; // Found!

            HLabel = NULL;
        }

        NextPinsheet = Pinsheet->Next();
        if( HLabel == NULL )   // Hlabel not found: delete pinsheet
        {
            aFrame->GetScreen()->SetModify();
            aFrame->DeleteSheetLabel( false, Pinsheet );
        }
        Pinsheet = NextPinsheet;
    }

    if( aRedraw )
        aFrame->DrawPanel->PostDirtyRect( GetBoundingBox() );
}


/**************************************************************************************/
void DrawSheetStruct::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                            const wxPoint& aOffset,
                            int aDrawMode, int aColor )
/**************************************************************************************/

/** Function Draw
 *  Draw the hierarchical sheet shape
 *  @param aPanel = the current DrawPanel
 *  @param aDc = the current Device Context
 *  @param aOffset = draw offset (usually wxPoint(0,0))
 *  @param aDrawMode = draw mode
 *  @param aColor = color used to draw sheet. Usually -1 to use the normal color for sheet items
 */
{
    Hierarchical_PIN_Sheet_Struct* SheetLabelStruct;
    int      txtcolor;
    wxString Text;
    int      color;
    wxPoint  pos = m_Pos + aOffset;
    int      LineWidth = g_DrawMinimunLineWidth;

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( aDC, aDrawMode );

    GRRect( &aPanel->m_ClipBox, aDC, pos.x, pos.y,
            pos.x + m_Size.x, pos.y + m_Size.y, LineWidth, color );

    /* Draw text : SheetName */
    if( aColor > 0 )
        txtcolor = aColor;
    else
        txtcolor = ReturnLayerColor( LAYER_SHEETNAME );

    Text = wxT( "Sheet: " ) + m_SheetName;
    DrawGraphicText( aPanel, aDC,
                     wxPoint( pos.x, pos.y - 8 ), (EDA_Colors) txtcolor,
                     Text, TEXT_ORIENT_HORIZ, wxSize( m_SheetNameSize, m_SheetNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM, LineWidth, false, false, false );

    /* Draw text : FileName */
    if( aColor >= 0 )
        txtcolor = aColor;
    else
        txtcolor = ReturnLayerColor( LAYER_SHEETFILENAME );
    Text = wxT( "File: " ) + m_FileName;
    DrawGraphicText( aPanel, aDC,
                     wxPoint( pos.x, pos.y + m_Size.y + 4 ),
                     (EDA_Colors) txtcolor,
                     Text, TEXT_ORIENT_HORIZ, wxSize( m_FileNameSize, m_FileNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP, LineWidth, false, false, false );


    /* Draw text : SheetLabel */
    SheetLabelStruct = m_Label;
    while( SheetLabelStruct != NULL )
    {
        if( !( SheetLabelStruct->m_Flags & IS_MOVED ) )
            SheetLabelStruct->Draw( aPanel, aDC, aOffset, aDrawMode, aColor );
        SheetLabelStruct = SheetLabelStruct->Next();
    }
}


/*****************************************/
EDA_Rect DrawSheetStruct::GetBoundingBox()
/*****************************************/

/** Function GetBoundingBox
 *  @return an EDA_Rect giving the bouding box of the sheet
 */
{
    int      dx, dy;

    // Determine length of texts
    wxString Text1    = wxT( "Sheet: " ) + m_SheetName;
    wxString Text2    = wxT( "File: " ) + m_FileName;
    int      textlen1 = 10 * Text1.Len() * m_SheetNameSize / 9;
    int      textlen2 = 10 * Text2.Len() * m_FileNameSize / 9;

    textlen1 = MAX( textlen1, textlen2 );
    dx = MAX( m_Size.x, textlen1 );
    dy = m_Size.y + m_SheetNameSize + m_FileNameSize + 16;

    EDA_Rect box( wxPoint( m_Pos.x, m_Pos.y - m_SheetNameSize - 8 ), wxSize( dx, dy ) );
    return box;
}

/************************************************/
bool DrawSheetStruct::HitTest( const wxPoint& aPosRef )
/************************************************/
/** Function HitTest
 * @return true if the point aPosRef is within item area
 * @param aPosRef = a wxPoint to test
 */
{
    EDA_Rect rect = GetBoundingBox();
    return rect.Inside( aPosRef );
}


/************************************/
int DrawSheetStruct::ComponentCount()
/************************************/

/** Function ComponentCount
 *  count our own components, without the power components.
 *  @return the copponent count.
 */
{
    int n = 0;

    if( m_AssociatedScreen )
    {
        EDA_BaseStruct* bs;
        for( bs = m_AssociatedScreen->EEDrawList; bs != NULL; bs = bs->Next() )
        {
            if( bs->Type() == TYPE_SCH_COMPONENT )
            {
                SCH_COMPONENT* Cmp = (SCH_COMPONENT*) bs;
                if( Cmp->GetField( VALUE )->m_Text.GetChar( 0 ) != '#' )
                    n++;
            }
            if( bs->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* sheet = (DrawSheetStruct*) bs;
                n += sheet->ComponentCount();
            }
        }
    }
    return n;
}


/*******************************************************************************/
bool DrawSheetStruct::SearchHierarchy( wxString aFilename, SCH_SCREEN** aScreen )
/*******************************************************************************/

/** Function SearchHierarchy
 *  search the existing hierarchy for an instance of screen "FileName".
 *  @param aFilename = the filename to find
 *  @param aFilename = a location to return a pointer to the screen (if found)
 *  @return bool if found, and a pointer to the screen
 */
{
    if( m_AssociatedScreen )
    {
        EDA_BaseStruct* strct = m_AssociatedScreen->EEDrawList;
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* ss = (DrawSheetStruct*) strct;
                if( ss->m_AssociatedScreen
                    && ss->m_AssociatedScreen->m_FileName.CmpNoCase( aFilename ) == 0 )
                {
                    *aScreen = ss->m_AssociatedScreen;
                    return true;
                }
                if( ss->SearchHierarchy( aFilename, aScreen ) )
                    return true;
            }
            strct = strct->Next();
        }
    }
    return false;
}


/*******************************************************************************/
bool DrawSheetStruct::LocatePathOfScreen( SCH_SCREEN*    aScreen,
                                          DrawSheetPath* aList )
/*******************************************************************************/

/** Function LocatePathOfScreen
 *  search the existing hierarchy for an instance of screen "FileName".
 *  don't bother looking at the root sheet - it must be unique,
 *  no other references to its m_AssociatedScreen otherwise there would be loops
 *  in the hierarchy.
 *  @param  aScreen = the SCH_SCREEN* screen that we search for
 *  @param aList = the DrawSheetPath*  that must be used
 *  @return true if found
 */
{
    if( m_AssociatedScreen )
    {
        aList->Push( this );
        if( m_AssociatedScreen == aScreen )
            return true;
        EDA_BaseStruct* strct = m_AssociatedScreen->EEDrawList;
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* ss = (DrawSheetStruct*) strct;
                if( ss->LocatePathOfScreen( aScreen, aList ) )
                    return true;
            }
            strct = strct->Next();
        }

        aList->Pop();
    }
    return false;
}


/**********************************************************/
bool DrawSheetStruct::Load( WinEDA_SchematicFrame* aFrame )
/***********************************************************/

/** Function Load.
 *  for the sheet: load the file m_FileName
 *  if a screen already exists, the file is already read.
 *  m_AssociatedScreen point on the screen, and its m_RefCount is incremented
 *  else creates a new associated screen and load the data file.
 *  @param aFrame = a WinEDA_SchematicFrame pointer to the maim schematic frame
 *  @return true if OK
 */
{
    bool success = true;

    if( !m_AssociatedScreen )
    {
        SCH_SCREEN* screen = NULL;
        g_RootSheet->SearchHierarchy( m_FileName, &screen );
        if( screen )
        {
            m_AssociatedScreen = screen;
            m_AssociatedScreen->m_RefCount++;

            //do not need to load the sub-sheets - this has already been done.
        }
        else
        {
            m_AssociatedScreen = new SCH_SCREEN();
            m_AssociatedScreen->m_RefCount++;
            success = aFrame->LoadOneEEFile( m_AssociatedScreen, m_FileName );
            if( success )
            {
                EDA_BaseStruct* bs = m_AssociatedScreen->EEDrawList;
                while( bs )
                {
                    if( bs->Type() ==  DRAW_SHEET_STRUCT_TYPE )
                    {
                        DrawSheetStruct* sheetstruct = (DrawSheetStruct*) bs;
                        if( !sheetstruct->Load( aFrame ) )
                            success = false;
                    }
                    bs = bs->Next();
                }
            }
        }
    }
    return success;
}


/**********************************/
int DrawSheetStruct::CountSheets()
/**********************************/
/** Function CountSheets
 * calculates the number of sheets found in "this"
 * this number includes the full subsheets count
 * @return the full count of sheets+subsheets contained by "this"
 */
{
    int count = 1; //1 = this!!

    if( m_AssociatedScreen )
    {
        EDA_BaseStruct* strct = m_AssociatedScreen->EEDrawList;
        for( ; strct; strct = strct->Next() )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* subsheet = (DrawSheetStruct*) strct;
                count += subsheet->CountSheets();
            }
        }
    }
    return count;
}


/******************************************/
wxString DrawSheetStruct::GetFileName( void )
/******************************************/
{
    return m_FileName;
}


/************************************************************************/
bool DrawSheetStruct::ChangeFileName( WinEDA_SchematicFrame* aFrame,
                                      const wxString&        aFileName )
/************************************************************************/
/** Function ChangeFileName
 * Set a new filename and manage data and associated screen
 * The main difficulty is the filename change in a complex hierarchy.
 * - if new filename is not already used: change to the new name (and if an existing file is found, load it on request)
 * - if new filename is already used (a complex hierarchy) : reference the sheet.
 * @param aFileName = the new filename
 * @param aFrame = the schematic frame
 */
{
    if( (GetFileName() == aFileName) && m_AssociatedScreen )
        return true;

    SCH_SCREEN* Screen_to_use = NULL;
    wxString    msg;
    bool        LoadFromFile = false;


    if( g_RootSheet->SearchHierarchy( aFileName, &Screen_to_use ) ) //do we reload the data from the existing hierarchy
    {
        if( m_AssociatedScreen )                                    //upon initial load, this will be null.
        {
            msg.Printf( _(
                           "A Sub Hierarchy named %s exists, Use it (The data in this sheet will be replaced)?" ),
                       aFileName.GetData() );
            if( !IsOK( NULL, msg ) )
            {
                DisplayInfoMessage( (wxWindow*)NULL, _( "Sheet Filename Renaming Aborted" ) );
                return false;
            }
        }
    }
    else if( wxFileExists( aFileName ) )         //do we reload the data from an existing file
    {
        msg.Printf( _(
                       "A file named %s exists, load it (otherwise keep current sheet data if possible)?" ),
                   aFileName.GetData() );
        if( IsOK( NULL, msg ) )
        {
            LoadFromFile = true;
            if( m_AssociatedScreen )                                // Can be NULL if loading a file when creating a new sheet
            {
                m_AssociatedScreen->m_RefCount--;                   //be careful with these
                if( m_AssociatedScreen->m_RefCount == 0 )
                    SAFE_DELETE( m_AssociatedScreen );
                m_AssociatedScreen = NULL;         //will be created later
            }
        }
    }

    // if an associated screen exists, shared between this sheet and others sheets, what we do ?
    if( m_AssociatedScreen && ( m_AssociatedScreen->m_RefCount > 1 ) )
    {
        msg = _( "This sheet uses shared data in a complex hierarchy" );
        msg << wxT( "\n" );
        msg << _(
            "Do we convert it in a simple hierarchical sheet (otherwise delete current sheet data)" );
        if( IsOK( NULL, msg ) )
        {
            LoadFromFile = true;
            wxString oldfilename = m_AssociatedScreen->m_FileName;
            m_AssociatedScreen->m_FileName = aFileName;
            aFrame->SaveEEFile( m_AssociatedScreen, FILE_SAVE_AS );
            m_AssociatedScreen->m_FileName = oldfilename;
        }
        m_AssociatedScreen->m_RefCount--;   //be careful with these
        m_AssociatedScreen = NULL;          //will be created later
    }


    SetFileName( aFileName );

    // if we use new data (from file or from internal hierarchy), delete the current sheet data
    if( m_AssociatedScreen && (LoadFromFile || Screen_to_use) )
    {
        m_AssociatedScreen->m_RefCount--;
        if( m_AssociatedScreen->m_RefCount == 0 )
            SAFE_DELETE( m_AssociatedScreen );
        m_AssociatedScreen = NULL;         //so that we reload..
    }

    if( LoadFromFile )
        Load( aFrame );
    else if( Screen_to_use )
    {
        m_AssociatedScreen = Screen_to_use;
        m_AssociatedScreen->m_RefCount++;
    }


    //just make a new screen if needed.
    if( !m_AssociatedScreen )
    {
        m_AssociatedScreen = new SCH_SCREEN();
        m_AssociatedScreen->m_RefCount++;         //be careful with these
    }
    m_AssociatedScreen->m_FileName = aFileName;

    return true;
}


/***********************************************************/
void DrawSheetStruct::DisplayInfo( WinEDA_DrawFrame* frame )
{
    frame->MsgPanel->EraseMsgBox();
    Affiche_1_Parametre( frame, 1, _( "Name" ), m_SheetName, CYAN );
    Affiche_1_Parametre( frame, 30, _( "FileName" ), m_FileName, BROWN );
}


#if defined(DEBUG)
void DrawSheetStruct::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">"
                                 << " sheet_name=\"" << CONV_TO_UTF8( m_SheetName ) << '"'
                                 << ">\n";

    // show all the pins, and check the linked list integrity
    Hierarchical_PIN_Sheet_Struct* label;
    for( label = m_Label;   label;   label = label->Next() )
    {
        label->Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n"
                                 << std::flush;
}


#endif
