/////////////////////////////////////////////////////////////////////////////
// Name:        sch_sheet.cpp
// Purpose:     member functions for SCH_SHEET
//              header = sch_sheet.h
// Author:      jean-pierre Charras
// Modified by:
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:
// License:     License GNU
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "confirm.h"
#include "trigo.h"
#include "richio.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"
#include "sch_component.h"


SCH_SHEET::SCH_SHEET( const wxPoint& pos ) : SCH_ITEM( NULL, SCH_SHEET_T )
{
    m_Layer = LAYER_SHEET;
    m_Pos = pos;
    m_TimeStamp = GetTimeStamp();
    m_SheetNameSize    = m_FileNameSize = 60;
    m_AssociatedScreen = NULL;
    m_SheetName.Printf( wxT( "Sheet%8.8lX" ), m_TimeStamp );
    m_FileName.Printf( wxT( "file%8.8lX.sch" ), m_TimeStamp );
}


SCH_SHEET::~SCH_SHEET()
{
    // also, look at the associated sheet & its reference count
    // perhaps it should be deleted also.
    if( m_AssociatedScreen )
    {
        m_AssociatedScreen->m_RefCount--;

        if( m_AssociatedScreen->m_RefCount == 0 )
            delete m_AssociatedScreen;
    }
}


/**
 * Function Save
 * writes the data structures for this object out to a FILE in "*.brd" format.
 * @param aFile The FILE to write to.
 * @return bool - true if success writing else false.
 */
bool SCH_SHEET::Save( FILE* aFile ) const
{
    if( fprintf( aFile, "$Sheet\n" ) == EOF
        || fprintf( aFile, "S %-4d %-4d %-4d %-4d\n",
                    m_Pos.x, m_Pos.y, m_Size.x, m_Size.y ) == EOF )
        return false;

    //save the unique timestamp, like other schematic parts.
    if( fprintf( aFile, "U %8.8lX\n", m_TimeStamp ) == EOF )
        return false;

    /* Save schematic sheetname and filename. */
    if( !m_SheetName.IsEmpty() )
    {
        if( fprintf( aFile, "F0 \"%s\" %d\n", CONV_TO_UTF8( m_SheetName ),
                     m_SheetNameSize ) == EOF )
            return false;
    }

    if( !m_FileName.IsEmpty() )
    {
        if( fprintf( aFile, "F1 \"%s\" %d\n", CONV_TO_UTF8( m_FileName ),
                     m_FileNameSize ) == EOF )
            return false;
    }

    /* Save the list of labels in the sheet. */

    BOOST_FOREACH( const SCH_SHEET_PIN& label, m_labels )
    {
        if( !label.Save( aFile ) )
            return false;
    }

    if( fprintf( aFile, "$EndSheet\n" ) == EOF )
        return false;

    return true;
}


bool SCH_SHEET::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    int              ii, fieldNdx, size;
    char             Name1[256];
    SCH_SHEET_PIN*   SheetLabel;
    char*            ptcar;

    m_TimeStamp = GetTimeStamp();

    // sheets are added to the GetDrawItems() like other schematic components.
    // however, in order to preserve the hierarchy (through m_Parent pointers),
    // a duplicate of the sheet is added to m_SubSheet array.
    // must be a duplicate, references just work for a two-layer structure.
    // this is accomplished through the Sync() function.

    if( ((char*)aLine)[0] == '$' )   // line should be "$Sheet"
    {
        if( !aLine.ReadLine() )
        {
            aErrorMsg.Printf( wxT( "Read File Errror" ) );
            return false;
        }
    }

    /* Next line: must be "S xx yy nn mm" with xx, yy = sheet position
     *  ( upper left corner  ) et nn,mm = sheet size */
    if( ( sscanf( &((char*)aLine)[1], "%d %d %d %d",
                  &m_Pos.x, &m_Pos.y, &m_Size.x, &m_Size.y ) != 4 )
        || ( ((char*)aLine)[0] != 'S' ) )
    {
        aErrorMsg.Printf( wxT( " ** EESchema file sheet struct error at line %d, aborted\n" ),
                          aLine.LineNumber() );

        aErrorMsg << CONV_FROM_UTF8( ((char*)aLine) );
        return false;
    }

    /* Read fields */
    for( ; ; ) /* Analysis of lines "Fn" text. */
    {
        if( !aLine.ReadLine() )
            return false;

        if( ((char*)aLine)[0] == 'U' )
        {
            sscanf( ((char*)aLine) + 1, "%lX", &m_TimeStamp );
            if( m_TimeStamp == 0 )  // zero is not unique!
                m_TimeStamp = GetTimeStamp();
            continue;
        }

        if( ((char*)aLine)[0] != 'F' )
            break;

        sscanf( ((char*)aLine) + 1, "%d", &fieldNdx );

        /* Read the field:
         * If fieldNdx> = 2: Fn "text" t s posx posy
         * If F0 "text" for SheetName
         * F1 and "text" for filename
         */
        ptcar = ((char*)aLine);
        while( *ptcar && ( *ptcar != '"' ) )
            ptcar++;

        if( *ptcar != '"' )
        {
            aErrorMsg.Printf( wxT( "EESchema file sheet label F%d at line %d, aborted\n" ),
                              fieldNdx, aLine.LineNumber() );
            aErrorMsg << CONV_FROM_UTF8( (char*) aLine );
            return false;
        }

        for( ptcar++, ii = 0; ; ii++, ptcar++ )
        {
            Name1[ii] = *ptcar;

            if( *ptcar == 0 )
            {
                aErrorMsg.Printf( wxT( "EESchema file sheet field F at line %d, aborted\n" ),
                                  aLine.LineNumber() );
                aErrorMsg << CONV_FROM_UTF8( (char*) aLine );
                return false;
            }

            if( *ptcar == '"' )
            {
                Name1[ii] = 0;
                ptcar++;
                break;
            }
        }

        if( ( fieldNdx == 0 ) || ( fieldNdx == 1 ) )
        {
            if( sscanf( ptcar, "%d", &size ) != 1 )
            {
                aErrorMsg.Printf( wxT( "EESchema file sheet Label error line %d, aborted\n" ),
                                  aLine.LineNumber() );

                aErrorMsg << CONV_FROM_UTF8( (char*) aLine );
            }
            if( size == 0 )
                size = DEFAULT_SIZE_TEXT;

            if( fieldNdx == 0 )
            {
                m_SheetName     = CONV_FROM_UTF8( Name1 );
                m_SheetNameSize = size;
            }
            else
            {
                SetFileName( CONV_FROM_UTF8( Name1 ) );

                //printf( "in ReadSheetDescr : m_FileName = %s \n", Name1 );
                m_FileNameSize = size;
            }
        }

        if( fieldNdx > 1 )
        {
            SheetLabel = new SCH_SHEET_PIN( this );

            if( !SheetLabel->Load( aLine, aErrorMsg ) )
            {
                delete SheetLabel;
                SheetLabel = NULL;
                return false;
            }

            AddLabel( SheetLabel );
        }
    }

    if( strnicmp( "$End", ((char*)aLine), 4 ) != 0 )
    {
        aErrorMsg.Printf( wxT( "**EESchema file end_sheet struct error at line %d, aborted\n" ),
                          aLine.LineNumber() );
        aErrorMsg << CONV_FROM_UTF8( ((char*)aLine) );
        return false;
    }

    return true;
}


/* creates a copy of a sheet
 *  The linked data itself (GetDrawItems()) is not duplicated
 */
SCH_SHEET* SCH_SHEET::GenCopy()
{
    SCH_SHEET* newitem = new SCH_SHEET( m_Pos );

    newitem->m_Size = m_Size;
    newitem->SetParent( m_Parent );
    newitem->m_TimeStamp = GetTimeStamp();

    newitem->m_FileName     = m_FileName;
    newitem->m_FileNameSize = m_FileNameSize;

/*    newitem->m_SheetName     = m_SheetName;   m_SheetName must be unique for
 * all sub sheets in a given sheet
 *                                               so we no not duplicate sheet
 * name
 */
    newitem->m_SheetNameSize = m_SheetNameSize;

    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_labels )
    {
        SCH_SHEET_PIN* newSheetPin = sheetPin.GenCopy();
        newSheetPin->SetParent( newitem );
        newitem->GetSheetPins().push_back( newSheetPin );
    }

    newitem->renumberLabels();

    /* don't copy screen data - just reference it. */
    newitem->m_AssociatedScreen = m_AssociatedScreen;

    if( m_AssociatedScreen )
        m_AssociatedScreen->m_RefCount++;

    return newitem;
}


/* Used if undo / redo command:
 *  swap data between this and copyitem
 */
void SCH_SHEET::SwapData( SCH_SHEET* copyitem )
{
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_SheetName, copyitem->m_SheetName );
    EXCHG( m_SheetNameSize, copyitem->m_SheetNameSize );
    EXCHG( m_FileNameSize, copyitem->m_FileNameSize );
    m_labels.swap( copyitem->m_labels );

    // Ensure sheet labels have their .m_Parent member pointing really on their
    // parent, after swapping.
    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_labels )
    {
        sheetPin.SetParent( this );
    }

    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, copyitem->m_labels )
    {
        sheetPin.SetParent( copyitem );
    }
}


void SCH_SHEET::AddLabel( SCH_SHEET_PIN* aLabel )
{
    wxASSERT( aLabel != NULL );
    wxASSERT( aLabel->Type() == SCH_SHEET_LABEL_T );

    m_labels.push_back( aLabel );
    renumberLabels();
}


void SCH_SHEET::RemoveLabel( SCH_SHEET_PIN* aLabel )
{
    wxASSERT( aLabel != NULL );
    wxASSERT( aLabel->Type() == SCH_SHEET_LABEL_T );

    SCH_SHEET_PIN_LIST::iterator i;

    for( i = m_labels.begin();  i < m_labels.end();  ++i )
    {
        if( *i == aLabel )
        {
            m_labels.erase( i );
            renumberLabels();
            return;
        }
    }

    wxLogDebug( wxT( "Fix me: attempt to remove label %s which is not in sheet %s." ),
                GetChars( aLabel->m_Text ), GetChars( m_SheetName ) );
}


bool SCH_SHEET::HasLabel( const wxString& aName )
{
    BOOST_FOREACH( SCH_SHEET_PIN label, m_labels )
    {
        if( label.m_Text.CmpNoCase( aName ) == 0 )
            return true;
    }

    return false;
}


bool SCH_SHEET::IsVerticalOrientation()
{
    BOOST_FOREACH( SCH_SHEET_PIN label, m_labels )
    {
        if( label.GetEdge() > 1 )
            return true;
    }
    return false;
}


bool SCH_SHEET::HasUndefinedLabels()
{
    BOOST_FOREACH( SCH_SHEET_PIN label, m_labels )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        EDA_ITEM* DrawStruct  = m_AssociatedScreen->GetDrawItems();
        SCH_HIERLABEL* HLabel = NULL;

        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != SCH_HIERARCHICAL_LABEL_T )
                continue;

            HLabel = (SCH_HIERLABEL*) DrawStruct;

            if( label.m_Text.CmpNoCase( HLabel->m_Text ) == 0 )
                break;  // Found!

            HLabel = NULL;
        }

        if( HLabel == NULL )   // Corresponding hierarchical label not found.
            return true;
    }

    return false;
}


void SCH_SHEET::Place( SCH_EDIT_FRAME* frame, wxDC* DC )
{
    /* Place list structures for new sheet. */
    bool isnew = ( m_Flags & IS_NEW ) ? true : false;

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
    else    /* save old text in undo list */
    {
        if( g_ItemToUndoCopy && ( g_ItemToUndoCopy->Type() == Type() ) )
        {
            /* restore old values and save new ones */
            SwapData( (SCH_SHEET*) g_ItemToUndoCopy );

            /* save in undo list */
            frame->SaveCopyInUndoList( this, UR_CHANGED );

            /* restore new values */
            SwapData( (SCH_SHEET*) g_ItemToUndoCopy );

            SAFE_DELETE( g_ItemToUndoCopy );
        }
    }

    SCH_ITEM::Place( frame, DC ); //puts it on the GetDrawItems().

    if( isnew )
    {
        frame->SetSheetNumberAndCount();
    }
}


/**
 * Delete sheet labels which do not have corresponding hierarchical label.
 */
void SCH_SHEET::CleanupSheet()
{
    SCH_SHEET_PIN_LIST::iterator i = m_labels.begin();

    while( i != m_labels.end() )
    {
        /* Search the schematic for a hierarchical label corresponding to this sheet label. */
        EDA_ITEM* DrawStruct = m_AssociatedScreen->GetDrawItems();
        SCH_HIERLABEL* HLabel = NULL;

        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
        {
            if( DrawStruct->Type() != SCH_HIERARCHICAL_LABEL_T )
                continue;

            HLabel = (SCH_HIERLABEL*) DrawStruct;

            if( i->m_Text.CmpNoCase( HLabel->m_Text ) == 0 )
                break;  // Found!

            HLabel = NULL;
        }

        if( HLabel == NULL )   // Hlabel not found: delete sheet label.
            m_labels.erase( i );
        else
            ++i;
    }
}


SCH_SHEET_PIN* SCH_SHEET::GetLabel( const wxPoint& aPosition )
{
    BOOST_FOREACH( SCH_SHEET_PIN& label, m_labels )
    {
        if( label.HitTest( aPosition ) )
            return &label;
    }

    return NULL;
}


/**
 * Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int SCH_SHEET::GetPenSize() const
{
    return g_DrawDefaultLineThickness;
}


/**
 * Function GetSheetNamePosition
 * @return the position of the anchor of sheet name text
 */
wxPoint SCH_SHEET::GetSheetNamePosition()
{
    wxPoint  pos = m_Pos;
    if( IsVerticalOrientation() )
    {
        pos.x -= 8;
        pos.y += m_Size.y;
    }
    else
    {
        pos.y -= 8;
    }

    return pos;
}

/**
 * Function GetFileNamePosition
 * @return the position of the anchor of filename text
 */
wxPoint SCH_SHEET::GetFileNamePosition()
{
    wxPoint  pos = m_Pos;
    if( IsVerticalOrientation() )
    {
        pos.x += m_Size.x+4;
        pos.y += m_Size.y;
    }
    else
    {
        pos.y += m_Size.y + 4;
    }

    return pos;
}


/**
 * Function Draw
 *  Draw the hierarchical sheet shape
 *  @param aPanel = the current DrawPanel
 *  @param aDc = the current Device Context
 *  @param aOffset = draw offset (usually wxPoint(0,0))
 *  @param aDrawMode = draw mode
 *  @param aColor = color used to draw sheet. Usually -1 to use the normal
 * color for sheet items
 */
void SCH_SHEET::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                      const wxPoint& aOffset, int aDrawMode, int aColor )
{
    int      txtcolor;
    wxString Text;
    int      color;
    int      name_orientation;
    wxPoint  pos_sheetname,pos_filename;
    wxPoint  pos = m_Pos + aOffset;
    int      LineWidth = g_DrawDefaultLineThickness;

    if( aColor >= 0 )
        color = aColor;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    GRRect( &aPanel->m_ClipBox, aDC, pos.x, pos.y,
            pos.x + m_Size.x, pos.y + m_Size.y, LineWidth, color );

    pos_sheetname = GetSheetNamePosition() + aOffset;
    pos_filename = GetFileNamePosition() + aOffset;

    if( IsVerticalOrientation() )
        name_orientation = TEXT_ORIENT_VERT;
    else
        name_orientation = TEXT_ORIENT_HORIZ;

    /* Draw text : SheetName */
    if( aColor > 0 )
        txtcolor = aColor;
    else
        txtcolor = ReturnLayerColor( LAYER_SHEETNAME );

    Text = wxT( "Sheet: " ) + m_SheetName;
    DrawGraphicText( aPanel, aDC, pos_sheetname,
                     (EDA_Colors) txtcolor, Text, name_orientation,
                     wxSize( m_SheetNameSize, m_SheetNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM, LineWidth,
                     false, false );

    /* Draw text : FileName */
    if( aColor >= 0 )
        txtcolor = aColor;
    else
        txtcolor = ReturnLayerColor( LAYER_SHEETFILENAME );

    Text = wxT( "File: " ) + m_FileName;
    DrawGraphicText( aPanel, aDC, pos_filename,
                     (EDA_Colors) txtcolor, Text, name_orientation,
                     wxSize( m_FileNameSize, m_FileNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP, LineWidth,
                     false, false );


    /* Draw text : SheetLabel */
    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_labels )
    {
        if( !( sheetPin.m_Flags & IS_MOVED ) )
            sheetPin.Draw( aPanel, aDC, aOffset, aDrawMode, aColor );
    }
}


/**
 * Function GetBoundingBox
 *  @return an EDA_Rect giving the bounding box of the sheet
 */
EDA_Rect SCH_SHEET::GetBoundingBox() const
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


/**
 * Function ComponentCount
 *  count our own components, without the power components.
 *  @return the component count.
 */
int SCH_SHEET::ComponentCount()
{
    int n = 0;

    if( m_AssociatedScreen )
    {
        EDA_ITEM* bs;

        for( bs = m_AssociatedScreen->GetDrawItems(); bs != NULL; bs = bs->Next() )
        {
            if( bs->Type() == SCH_COMPONENT_T )
            {
                SCH_COMPONENT* Cmp = (SCH_COMPONENT*) bs;

                if( Cmp->GetField( VALUE )->m_Text.GetChar( 0 ) != '#' )
                    n++;
            }

            if( bs->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* sheet = (SCH_SHEET*) bs;
                n += sheet->ComponentCount();
            }
        }
    }

    return n;
}


/**
 * Function SearchHierarchy
 *  search the existing hierarchy for an instance of screen "FileName".
 *  @param aFilename = the filename to find
 *  @param aFilename = a location to return a pointer to the screen (if found)
 *  @return bool if found, and a pointer to the screen
 */
bool SCH_SHEET::SearchHierarchy( wxString aFilename, SCH_SCREEN** aScreen )
{
    if( m_AssociatedScreen )
    {
        EDA_ITEM* strct = m_AssociatedScreen->GetDrawItems();

        while( strct )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* ss = (SCH_SHEET*) strct;
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


/**
 * Function LocatePathOfScreen
 *  search the existing hierarchy for an instance of screen "FileName".
 *  don't bother looking at the root sheet - it must be unique,
 *  no other references to its m_AssociatedScreen otherwise there would be
 * loops
 *  in the hierarchy.
 *  @param  aScreen = the SCH_SCREEN* screen that we search for
 *  @param aList = the SCH_SHEET_PATH*  that must be used
 *  @return true if found
 */
bool SCH_SHEET::LocatePathOfScreen( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aList )
{
    if( m_AssociatedScreen )
    {
        aList->Push( this );

        if( m_AssociatedScreen == aScreen )
            return true;

        EDA_ITEM* strct = m_AssociatedScreen->GetDrawItems();

        while( strct )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* ss = (SCH_SHEET*) strct;

                if( ss->LocatePathOfScreen( aScreen, aList ) )
                    return true;
            }

            strct = strct->Next();
        }

        aList->Pop();
    }
    return false;
}


/**
 * Function Load.
 *  for the sheet: load the file m_FileName
 *  if a screen already exists, the file is already read.
 *  m_AssociatedScreen point on the screen, and its m_RefCount is incremented
 *  else creates a new associated screen and load the data file.
 *  @param aFrame = a SCH_EDIT_FRAME pointer to the maim schematic frame
 *  @return true if OK
 */
bool SCH_SHEET::Load( SCH_EDIT_FRAME* aFrame )
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
                EDA_ITEM* bs = m_AssociatedScreen->GetDrawItems();

                while( bs )
                {
                    if( bs->Type() ==  SCH_SHEET_T )
                    {
                        SCH_SHEET* sheetstruct = (SCH_SHEET*) bs;

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


/**
 * Function CountSheets
 * calculates the number of sheets found in "this"
 * this number includes the full subsheets count
 * @return the full count of sheets+subsheets contained by "this"
 */
int SCH_SHEET::CountSheets()
{
    int count = 1; //1 = this!!

    if( m_AssociatedScreen )
    {
        EDA_ITEM* strct = m_AssociatedScreen->GetDrawItems();

        for( ; strct; strct = strct->Next() )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                SCH_SHEET* subsheet = (SCH_SHEET*) strct;
                count += subsheet->CountSheets();
            }
        }
    }
    return count;
}


wxString SCH_SHEET::GetFileName( void )
{
    return m_FileName;
}


/**
 * Function ChangeFileName
 * Set a new filename and manage data and associated screen
 * The main difficulty is the filename change in a complex hierarchy.
 * - if new filename is not already used: change to the new name (and if an
 * existing file is found, load it on request)
 * - if new filename is already used (a complex hierarchy) : reference the
 * sheet.
 * @param aFileName = the new filename
 * @param aFrame = the schematic frame
 */
bool SCH_SHEET::ChangeFileName( SCH_EDIT_FRAME* aFrame, const wxString& aFileName )
{
    if( ( GetFileName() == aFileName ) && m_AssociatedScreen )
        return true;

    SCH_SCREEN* Screen_to_use = NULL;
    wxString    msg;
    bool        LoadFromFile = false;

    // do we reload the data from the existing hierarchy
    if( g_RootSheet->SearchHierarchy( aFileName, &Screen_to_use ) )
    {
        if( m_AssociatedScreen ) // upon initial load, this will be null.
        {
            msg.Printf( _( "A Sub Hierarchy named %s exists, Use it (The \
data in this sheet will be replaced)?" ),
                        GetChars( aFileName ) );
            if( !IsOK( NULL, msg ) )
            {
                DisplayInfoMessage( (wxWindow*) NULL, _( "Sheet Filename Renaming Aborted" ) );
                return false;
            }
        }
    }
    else if( wxFileExists( aFileName ) ) // do we reload the data from an existing file
    {
        msg.Printf( _( "A file named %s exists, load it (otherwise keep \
current sheet data if possible)?" ),
                    GetChars( aFileName ) );

        if( IsOK( NULL, msg ) )
        {
            LoadFromFile = true;

            // Can be NULL if loading a file when creating a new sheet.
            if( m_AssociatedScreen )
            {
                m_AssociatedScreen->m_RefCount--;  // be careful with these

                if( m_AssociatedScreen->m_RefCount == 0 )
                    SAFE_DELETE( m_AssociatedScreen );

               m_AssociatedScreen = NULL;         // will be created later
            }
        }
    }

    // if an associated screen exists, shared between this sheet and others
    // sheets, what we do ?
    if( m_AssociatedScreen && ( m_AssociatedScreen->m_RefCount > 1 ) )
    {
        msg = _( "This sheet uses shared data in a complex hierarchy" );
        msg << wxT( "\n" );
        msg << _( "Do we convert it in a simple hierarchical sheet (\
otherwise delete current sheet data)" );

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

    // if we use new data (from file or from internal hierarchy), delete the
    // current sheet data
    if( m_AssociatedScreen && (LoadFromFile || Screen_to_use) )
    {
        m_AssociatedScreen->m_RefCount--;

        if( m_AssociatedScreen->m_RefCount == 0 )
            SAFE_DELETE( m_AssociatedScreen );

        m_AssociatedScreen = NULL;         // so that we reload..
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
        m_AssociatedScreen->m_RefCount++;         // be careful with these
    }

    m_AssociatedScreen->m_FileName = aFileName;

    return true;
}


void SCH_SHEET::DisplayInfo( WinEDA_DrawFrame* frame )
{
    frame->ClearMsgPanel();
    frame->AppendMsgPanel( _( "Sheet name" ), m_SheetName, CYAN );
    frame->AppendMsgPanel( _( "File name" ), m_FileName, BROWN );
}


void SCH_SHEET::Rotate(wxPoint rotationPoint)
{
    RotatePoint( &m_Pos, rotationPoint, 900 );
    RotatePoint( &m_Size.x, &m_Size.y, 900 );

    if( m_Size.x < 0 )
    {
        m_Pos.x += m_Size.x;
        NEGATE( m_Size.x );
    }

    if( m_Size.y < 0 )
    {
        m_Pos.y += m_Size.y;
        NEGATE( m_Size.y );
    }

    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_labels )
    {
        sheetPin.Rotate( rotationPoint );
    }
}


void SCH_SHEET::Mirror_X( int aXaxis_position )
{
    m_Pos.y -= aXaxis_position;
    NEGATE( m_Pos.y );
    m_Pos.y += aXaxis_position;
    m_Pos.y -= m_Size.y;

    BOOST_FOREACH( SCH_SHEET_PIN& sheetPin, m_labels )
    {
        sheetPin.Mirror_X( aXaxis_position );
    }
}


/**
 * Function Mirror_Y (virtual)
 * mirror item relative to an Y axis
 * @param aYaxis_position = the y axis position
 */
void SCH_SHEET::Mirror_Y( int aYaxis_position )
{
    m_Pos.x -= aYaxis_position;
    NEGATE( m_Pos.x );
    m_Pos.x += aYaxis_position;
    m_Pos.x -= m_Size.x;

    BOOST_FOREACH( SCH_SHEET_PIN& label, m_labels )
    {
        label.Mirror_Y( aYaxis_position );
    }
}


void SCH_SHEET::Resize( const wxSize& aSize )
{
    if( aSize == m_Size )
        return;

    m_Size = aSize;

    /* Move the sheet labels according to the new sheet size. */
    BOOST_FOREACH( SCH_SHEET_PIN& label, m_labels )
    {
        label.ConstraintOnEdge( label.m_Pos );
    }
}


/** Compare schematic sheet entry (filename and sheetname) against search string.
 * @param aSearchData - Criteria to search against.
 * @param aAuxData - a pointer on auxiliary data, not used here.
 * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
 * @return True if this item matches the search criteria.
 */
bool SCH_SHEET::Matches( wxFindReplaceData& aSearchData,
                         void* aAuxData, wxPoint * aFindLocation )
{
    if( SCH_ITEM::Matches( m_FileName, aSearchData ) )
    {
        if( aFindLocation )
            *aFindLocation = GetFileNamePosition();

        return true;
    }

    if( SCH_ITEM::Matches( m_SheetName, aSearchData ) )
    {
        if( aFindLocation )
            *aFindLocation = GetSheetNamePosition();

        return true;
    }

    return false;
}


void SCH_SHEET::renumberLabels()
{
    int labelId = 2;

    BOOST_FOREACH( SCH_SHEET_PIN& label, m_labels )
    {
        label.SetNumber( labelId );
        labelId++;
    }
}


void SCH_SHEET::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    // Using BOOST_FOREACH here creates problems (bad pointer value to pinsheet).
    // I do not know why.
    for( unsigned ii = 0; ii < GetSheetPins().size(); ii++ )
    {
        SCH_SHEET_PIN &pinsheet = GetSheetPins()[ii];

        wxCHECK2_MSG( pinsheet.Type() == SCH_SHEET_LABEL_T, continue,
                      wxT( "Invalid item in schematic sheet pin list.  Bad programmer!" ) );

        pinsheet.GetEndPoints( aItemList );
    }
}


bool SCH_SHEET::IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList )
{
    bool currentState = IsDangling();

    BOOST_FOREACH( SCH_SHEET_PIN& pinsheet, GetSheetPins() )
    {
        pinsheet.IsDanglingStateChanged( aItemList );
    }

    return currentState != IsDangling();
}


bool SCH_SHEET::IsDangling() const
{
    // If any hierarchical label in the sheet is dangling, then the sheet is dangling.
    for( size_t i = 0; i < GetSheetPins().size(); i++ )
    {
        if( GetSheetPins()[i].IsDangling() )
            return true;
    }

    return false;
}


bool SCH_SHEET::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    EDA_Rect boundingBox = GetBoundingBox();

    if( aRect.Intersects( boundingBox ) )
        m_Flags |= SELECTED;
    else
        m_Flags &= ~SELECTED;

    return previousState != IsSelected();
}


void SCH_SHEET::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    for( size_t i = 0; i < GetSheetPins().size(); i++ )
        aPoints.push_back( GetSheetPins()[i].m_Pos );
}


bool SCH_SHEET::DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const
{
    if( !( aFilter & SHEET_T ) )
        return false;

    EDA_Rect rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPoint );
}


bool SCH_SHEET::DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const
{
    EDA_Rect rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


#if defined(DEBUG)

void SCH_SHEET::Show( int nestLevel, std::ostream& os )
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << ">" << " sheet_name=\""
                                 << CONV_TO_UTF8( m_SheetName ) << '"' << ">\n";

    // show all the pins, and check the linked list integrity
    BOOST_FOREACH( SCH_SHEET_PIN& label, m_labels )
    {
        label.Show( nestLevel + 1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << s.Lower().mb_str() << ">\n" << std::flush;
}

#endif

