/////////////////////////////////////////////////////////////////////////////

// Name:        DrawSheet.cpp
// Purpose:		member functions for DrawSheetStruct and DrawSheetLabelStruct
//				header = class_screen.h
// Author:      jean-pierre Charras
// Modified by:
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:   License GNU
// Licence:
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "schframe.h"


/***********************************************************/
DrawSheetStruct::DrawSheetStruct( const wxPoint& pos ) :
    EDA_BaseStruct( DRAW_SHEET_STRUCT_TYPE )
/***********************************************************/
{
    m_Label   = NULL;
    m_NbLabel = 0;
    m_Layer   = LAYER_SHEET;
    m_Pos = pos;
    m_TimeStamp        = GetTimeStamp();
    m_SheetNameSize    = m_FileNameSize = 60;
    m_AssociatedScreen = NULL;
    m_SheetName.Printf( wxT( "Sheet%8.8lX" ), m_TimeStamp );
    m_FileName.Printf( wxT( "file%8.8lX.sch" ), m_TimeStamp );
    m_SheetNumber    = 1;
    m_NumberOfSheets = 1;
}


/**************************************/
DrawSheetStruct::~DrawSheetStruct()
/**************************************/
{
    DrawSheetLabelStruct* label = m_Label, * next_label;

    while( label )
    {
        next_label = (DrawSheetLabelStruct*) label->Pnext;
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


/***********************************************/
DrawSheetStruct* DrawSheetStruct::GenCopy()
/***********************************************/

/* creates a copy of a sheet
 *  The linked data itself (EEDrawList) is not duplicated
 */
{
    DrawSheetStruct* newitem = new DrawSheetStruct( m_Pos );


    newitem->m_Size      = m_Size;
    newitem->m_Parent    = m_Parent;
    newitem->m_TimeStamp = GetTimeStamp();

    newitem->m_FileName      = m_FileName;
    newitem->m_FileNameSize  = m_FileNameSize;
    newitem->m_SheetName     = m_SheetName;
    newitem->m_SheetNameSize = m_SheetNameSize;

    newitem->m_Label = NULL;

    DrawSheetLabelStruct* Slabel = NULL, * label = m_Label;

    if( label )
    {
        Slabel = newitem->m_Label = label->GenCopy();
        Slabel->m_Parent = newitem;
        label = (DrawSheetLabelStruct*) label->Pnext;
    }

    while( label )
    {
        Slabel->Pnext = label->GenCopy();
        Slabel = (DrawSheetLabelStruct*) Slabel->Pnext;
        Slabel->m_Parent = newitem;
        label = (DrawSheetLabelStruct*) label->Pnext;
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
}


/****************************************************************/
void DrawSheetStruct::Place( WinEDA_DrawFrame* frame, wxDC* DC )
/****************************************************************/
{
    /* Placement en liste des structures si nouveau composant:*/
    if( m_Flags & IS_NEW )
    {
        if( !( (WinEDA_SchematicFrame*) frame )->EditSheet( this, DC ) )
        {
            frame->GetScreen()->SetCurItem( NULL );
            frame->DrawPanel->ManageCurseur = NULL;
            frame->DrawPanel->ForceCloseManageCurseur = NULL;
            RedrawOneStruct( frame->DrawPanel, DC, this, g_XorMode );
            delete this;
            return;
        }
    }

    EDA_BaseStruct::Place( frame, DC ); //puts it on the EEDrawList.
}


/********************************************************************/
void DrawSheetStruct::CleanupSheet( WinEDA_SchematicFrame* frame, wxDC* DC )
/********************************************************************/

/* Delete pinsheets which are not corresponding to a hierarchal label
 *  if DC != NULL, redraw Sheet
 */
{
    DrawSheetLabelStruct* Pinsheet, * NextPinsheet;

    if( !IsOK( frame, _( "Ok to cleanup this sheet" ) ) )
        return;

    Pinsheet = m_Label;
    while( Pinsheet )
    {
        /* Search Hlabel corresponding to this Pinsheet */

        EDA_BaseStruct*      DrawStruct = m_AssociatedScreen->EEDrawList;
        DrawHierLabelStruct* HLabel = NULL;
        for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Pnext )
        {
            if( DrawStruct->Type() != DRAW_HIER_LABEL_STRUCT_TYPE )
                continue;
            HLabel = (DrawHierLabelStruct*) DrawStruct;
            if( Pinsheet->m_Text.CmpNoCase( HLabel->m_Text ) == 0 )
                break; // Found!
            HLabel = NULL;
        }

        NextPinsheet = (DrawSheetLabelStruct*) Pinsheet->Pnext;
        if( HLabel == NULL )   // Hlabel not found: delete pinsheet
        {
            frame->GetScreen()->SetModify();
            frame->DeleteSheetLabel( DC, Pinsheet );
        }
        Pinsheet = NextPinsheet;
    }
}


/**************************************************************************************/
void DrawSheetStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                            int DrawMode, int Color )
/**************************************************************************************/
/* Draw the hierarchical sheet shape */
{
    DrawSheetLabelStruct* SheetLabelStruct;
    int      txtcolor;
    wxString Text;
    int      color;
    wxPoint  pos = m_Pos + offset;
    int      LineWidth = g_DrawMinimunLineWidth;

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    GRRect( &panel->m_ClipBox, DC, pos.x, pos.y,
            pos.x + m_Size.x, pos.y + m_Size.y, LineWidth, color );

    /* Draw text : SheetName */
    if( Color > 0 )
        txtcolor = Color;
    else
        txtcolor = ReturnLayerColor( LAYER_SHEETNAME );

    Text = wxT( "Sheet: " ) + m_SheetName;
    DrawGraphicText( panel, DC,
                     wxPoint( pos.x, pos.y - 8 ), txtcolor,
                     Text, TEXT_ORIENT_HORIZ, wxSize( m_SheetNameSize, m_SheetNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM, LineWidth );

    /* Draw text : FileName */
    if( Color >= 0 )
        txtcolor = Color;
    else
        txtcolor = ReturnLayerColor( LAYER_SHEETFILENAME );
    Text = wxT( "File: " ) + m_FileName;
    DrawGraphicText( panel, DC,
                     wxPoint( pos.x, pos.y + m_Size.y + 4 ),
                     txtcolor,
                     Text, TEXT_ORIENT_HORIZ, wxSize( m_FileNameSize, m_FileNameSize ),
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP, LineWidth );


    /* Draw text : SheetLabel */
    SheetLabelStruct = m_Label;
    while( SheetLabelStruct != NULL )
    {
        SheetLabelStruct->Draw( panel, DC, offset, DrawMode, Color );
        SheetLabelStruct = (DrawSheetLabelStruct*) (SheetLabelStruct->Pnext);
    }
}


/**************************************************************************************/
void DrawSheetStruct::DeleteAnnotation( bool recurse )
/**************************************************************************************/
{
    if( recurse && m_AssociatedScreen )
    {
        EDA_BaseStruct* strct = m_AssociatedScreen->EEDrawList;
        for( ; strct; strct = strct->Pnext )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* sheet = (DrawSheetStruct*) strct;
                sheet->DeleteAnnotation( recurse );
            }
        }
    }
    EDA_BaseStruct* comp = m_AssociatedScreen->EEDrawList;
    for( ; comp; comp = comp->Pnext )
    {
        if( comp->Type() == DRAW_LIB_ITEM_STRUCT_TYPE )
        {
            ( (EDA_SchComponentStruct*) comp )->ClearAnnotation();
        }
    }
}


/*******************************************************************/
int DrawSheetStruct::ComponentCount()
/*******************************************************************/
{
    //count our own components, without the power components.

    /* Routine retournant le nombre de composants dans le schema,
     *  powers non comprises */
    int n = 0;

    if( m_AssociatedScreen )
    {
        EDA_BaseStruct* bs;
        for( bs = m_AssociatedScreen->EEDrawList; bs != NULL; bs = bs->Pnext )
        {
            if( bs->Type() == DRAW_LIB_ITEM_STRUCT_TYPE )
            {
                DrawPartStruct* Cmp = (DrawPartStruct*) bs;
                if( Cmp->m_Field[VALUE].m_Text.GetChar( 0 ) != '#' )
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
bool DrawSheetStruct::SearchHierarchy( wxString filename, SCH_SCREEN** screen )
/*******************************************************************************/
{
    //search the existing hierarchy for an instance of screen "FileName".
    if( m_AssociatedScreen )
    {
        EDA_BaseStruct* strct = m_AssociatedScreen->EEDrawList;
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* ss = (DrawSheetStruct*) strct;
                if( ss->m_AssociatedScreen &&
                    ss->m_AssociatedScreen->m_FileName.CmpNoCase( filename ) == 0 )
                {
                    *screen = ss->m_AssociatedScreen;
                    return true;
                }
                if( ss->SearchHierarchy( filename, screen ) )
                    return true;
            }
            strct = strct->Pnext;
        }
    }
    return false;
}


/*******************************************************************************/
bool DrawSheetStruct::LocatePathOfScreen( SCH_SCREEN* screen, DrawSheetPath* list )
/*******************************************************************************/
{
    //search the existing hierarchy for an instance of screen "FileName".
    //don't bother looking at the root sheet - it must be unique,
    //no other references to its m_s otherwise there would be loops
    //in the heirarchy.
    //search the existing hierarchy for an instance of screen "FileName".
    if( m_AssociatedScreen )
    {
        list->Push( this );
        if( m_AssociatedScreen == screen )
            return true;
        EDA_BaseStruct* strct = m_AssociatedScreen->EEDrawList;
        while( strct )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* ss = (DrawSheetStruct*) strct;
                if( ss->LocatePathOfScreen( screen, list ) )
                    return true;
            }
            strct = strct->Pnext;
        }

        list->Pop();
    }
    return false;
}


/*******************************************************************************/
bool DrawSheetStruct::Load( WinEDA_SchematicFrame* frame )
/*******************************************************************************/
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
            m_AssociatedScreen = new SCH_SCREEN( SCHEMATIC_FRAME );
            m_AssociatedScreen->m_RefCount++;
            success = frame->LoadOneEEFile( m_AssociatedScreen, m_FileName );
            if( success )
            {
                EDA_BaseStruct* bs = m_AssociatedScreen->EEDrawList;
                while( bs )
                {
                    if( bs->Type() ==  DRAW_SHEET_STRUCT_TYPE )
                    {
                        DrawSheetStruct* sheetstruct = (DrawSheetStruct*) bs;
                        if( !sheetstruct->Load( frame ) )
                            success = false;
                    }
                    bs = bs->Pnext;
                }
            }
        }
    }
    return success;
}


/*******************************************************************************/
int DrawSheetStruct::CountSheets()
/*******************************************************************************/
{
    int count = 1; //1 = this!!

    if( m_AssociatedScreen )
    {
        EDA_BaseStruct* strct = m_AssociatedScreen->EEDrawList;
        for( ; strct; strct = strct->Pnext )
        {
            if( strct->Type() == DRAW_SHEET_STRUCT_TYPE )
            {
                DrawSheetStruct* ss = (DrawSheetStruct*) strct;
                count += ss->CountSheets();
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


/************************************************************/
void DrawSheetStruct::SetFileName( const wxString& aFilename )
/************************************************************/
{
    m_FileName = aFilename;
}


/** Function ChangeFileName
 * Set a new filename and manage data and associated screen
 * The main difficulty is the filename change in a complex hierarchy.
 * - if new filename is not already used: change to the new name (and if an existing file is found, load it on request)
 * - if new filename is already used (a complex hierarchy) : add the sheet to the complex hierarchy.
 */

bool DrawSheetStruct::ChangeFileName( WinEDA_SchematicFrame * aFrame, const wxString& aFileName )
{
    if( (GetFileName() == aFileName) && m_AssociatedScreen )
        return true;

    SCH_SCREEN* Screen_to_use = NULL;
	wxString msg;
	bool LoadFromFile = false;


    if( g_RootSheet->SearchHierarchy( aFileName, &Screen_to_use ) ) //do we reload the data from the existing hierarchy
    {
        msg.Printf( _(
                       "A Sub Hierarchy named %s exists, Use it (The data in this sheet will be replaced)?" ),
                   aFileName.GetData() );
        if( ! IsOK( NULL, msg ) )
		{
			DisplayInfo(NULL, _("Sheet Filename Renaming Aborted"));
			return false;
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
			m_AssociatedScreen->m_RefCount--;                                       //be careful with these
			if( m_AssociatedScreen->m_RefCount == 0 )
				SAFE_DELETE( m_AssociatedScreen );
			m_AssociatedScreen = NULL;         //will be created later
		}
    }

	// if an associated screen exists, shared between this sheet and others sheets, what we do ?
    if( m_AssociatedScreen && ( m_AssociatedScreen->m_RefCount > 1 ))
	{
        msg = _("This sheet uses shared data in a complex hierarchy" ) ;
		msg << wxT("\n");
		msg << _("Do we convert it in a simple hierarchical sheet (otherwise delete current sheet data)");
        if( IsOK( NULL, msg ) )
		{
			LoadFromFile = true;
			wxString oldfilename = m_AssociatedScreen->m_FileName;
			m_AssociatedScreen->m_FileName = aFileName;
			aFrame->SaveEEFile( m_AssociatedScreen, FILE_SAVE_AS );
			m_AssociatedScreen->m_FileName = oldfilename;
		}
		m_AssociatedScreen->m_RefCount--;  //be careful with these
		m_AssociatedScreen = NULL;         //will be created later
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

    if ( LoadFromFile )
		Load( aFrame );
	else if ( Screen_to_use )
	{
		m_AssociatedScreen = Screen_to_use;
		m_AssociatedScreen->m_RefCount++;
	}


	//just make a new screen if needed.
	if( !m_AssociatedScreen )
	{
		m_AssociatedScreen = new SCH_SCREEN( SCHEMATIC_FRAME );
		m_AssociatedScreen->m_RefCount++;         //be careful with these
	}
	m_AssociatedScreen->m_FileName = aFileName;

    return true;
}


/************************/
/* DrawSheetLabelStruct */
/************************/

/*******************************************************************/
DrawSheetLabelStruct::DrawSheetLabelStruct( DrawSheetStruct* parent,
                                            const wxPoint& pos, const wxString& text ) :
    EDA_BaseStruct( DRAW_SHEETLABEL_STRUCT_TYPE )
    , EDA_TextStruct( text )
/*******************************************************************/
{
    m_Layer      = LAYER_SHEETLABEL;
    m_Parent     = parent;
    m_Pos        = pos;
    m_Edge       = 0;
    m_Shape      = NET_INPUT;
    m_IsDangling = TRUE;
}


/***********************************************************/
DrawSheetLabelStruct* DrawSheetLabelStruct::GenCopy()
/***********************************************************/
{
    DrawSheetLabelStruct* newitem =
        new DrawSheetLabelStruct( (DrawSheetStruct*) m_Parent, m_Pos, m_Text );

    newitem->m_Edge  = m_Edge;
    newitem->m_Shape = m_Shape;

    return newitem;
}


/********************************************************************************************/
void DrawSheetLabelStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                 int DrawMode, int Color )
/********************************************************************************************/
/* Routine de dessin des Labels type hierarchie */
{
    int    side, txtcolor;
    int    posx, tposx, posy, size2;
    wxSize size;
    int    NbSegm, coord[20];
    int    LineWidth = g_DrawMinimunLineWidth;

    if( Color >= 0 )
        txtcolor = Color;
    else
        txtcolor = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    posx = m_Pos.x + offset.x; posy = m_Pos.y + offset.y; size = m_Size;
    if( !m_Text.IsEmpty() )
    {
        if( m_Edge )
        {
            tposx = posx - size.x;
            side  = GR_TEXT_HJUSTIFY_RIGHT;
        }
        else
        {
            tposx = posx + size.x + (size.x / 8);
            side  = GR_TEXT_HJUSTIFY_LEFT;
        }
        DrawGraphicText( panel, DC, wxPoint( tposx, posy ), txtcolor,
                         m_Text, TEXT_ORIENT_HORIZ, size,
                         side, GR_TEXT_VJUSTIFY_CENTER, LineWidth );
    }
    /* dessin du symbole de connexion */

    if( m_Edge )
    {
        size.x = -size.x;
        size.y = -size.y;
    }

    coord[0] = posx; coord[1] = posy; size2 = size.x / 2;
    NbSegm   = 0;

    switch( m_Shape )
    {
    case 0:         /* input |> */
        coord[2]  = posx; coord[3] = posy - size2;
        coord[4]  = posx + size2; coord[5] = posy - size2;
        coord[6]  = posx + size.x; coord[7] = posy;
        coord[8]  = posx + size2; coord[9] = posy + size2;
        coord[10] = posx; coord[11] = posy + size2;
        coord[12] = coord[0]; coord[13] = coord[1];
        NbSegm    = 7;
        break;

    case 1:         /* output <| */
        coord[2]  = posx + size2; coord[3] = posy - size2;
        coord[4]  = posx + size.x; coord[5] = posy - size2;
        coord[6]  = posx + size.x; coord[7] = posy + size2;
        coord[8]  = posx + size2; coord[9] = posy + size2;
        coord[10] = coord[0]; coord[11] = coord[1];
        NbSegm    = 6;
        break;

    case 2:         /* bidi <> */
    case 3:         /* TriSt <> */
        coord[2] = posx + size2; coord[3] = posy - size2;
        coord[4] = posx + size.x; coord[5] = posy;
        coord[6] = posx + size2; coord[7] = posy + size2;
        coord[8] = coord[0];  coord[9] = coord[1];
        NbSegm   = 5;
        break;

    default:         /* unsp []*/
        coord[2]  = posx; coord[3] = posy - size2;
        coord[4]  = posx + size.x; coord[5] = posy - size2;
        coord[6]  = posx + size.x; coord[7] = posy + size2;
        coord[8]  = posx; coord[9] = posy + size2;
        coord[10] = coord[0]; coord[11] = coord[1];
        NbSegm    = 6;
        break;
    }

    int FillShape = FALSE;
    GRPoly( &panel->m_ClipBox, DC, NbSegm, coord, FillShape, LineWidth, txtcolor, txtcolor ); /* Poly Non rempli */
}


/**********************************************/
/* class to handle a series of sheets *********/
/* a 'path' so to speak.. *********************/
/**********************************************/
DrawSheetPath::DrawSheetPath()
{
    for( int i = 0; i<DSLSZ; i++ )
        m_sheets[i] = NULL;

    m_numSheets = 0;
}


int DrawSheetPath::Cmp( DrawSheetPath& d )
{
    if( m_numSheets > d.m_numSheets )
        return 1;
    if( m_numSheets < d.m_numSheets )
        return -1;

    //otherwise, same number of sheets.
    for( int i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i]->m_TimeStamp > d.m_sheets[i]->m_TimeStamp )
            return 1;
        if( m_sheets[i]->m_TimeStamp < d.m_sheets[i]->m_TimeStamp )
            return -1;
    }

    return 0;
}


DrawSheetStruct* DrawSheetPath::Last()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1];
    return NULL;
}


SCH_SCREEN* DrawSheetPath::LastScreen()
{
    if( m_numSheets )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen;
    return NULL;
}


EDA_BaseStruct* DrawSheetPath::LastDrawList()
{
    if( m_numSheets && m_sheets[m_numSheets - 1]->m_AssociatedScreen )
        return m_sheets[m_numSheets - 1]->m_AssociatedScreen->EEDrawList;
    return NULL;
}


void DrawSheetPath::Push( DrawSheetStruct* sheet )
{
    wxASSERT( m_numSheets <= DSLSZ );
    if( m_numSheets < DSLSZ )
    {
        m_sheets[m_numSheets] = sheet;
        m_numSheets++;
    }
}


DrawSheetStruct* DrawSheetPath::Pop()
{
    if( m_numSheets > 0 )
    {
        m_numSheets--;
        return m_sheets[m_numSheets];
    }
    return NULL;
}


wxString DrawSheetPath::Path()
{
    wxString s, t;

    s = wxT( "/" );

    //start at 1 to avoid the root sheet,
    //which does not need to be added to the path
    //it's timestamp changes anyway.
    for( int i = 1; i< m_numSheets; i++ )
    {
        t.Printf( _( "%8.8lX/" ), m_sheets[i]->m_TimeStamp );
        s = s + t;
    }

    return s;
}


wxString DrawSheetPath::PathHumanReadable()
{
    wxString s, t;

    s = wxT( "/" );

    //start at 1 to avoid the root sheet, as above.
    for( int i = 1; i< m_numSheets; i++ )
    {
        s = s + m_sheets[i]->m_SheetName + wxT( "/" );
    }

    return s;
}


void DrawSheetPath::UpdateAllScreenReferences()
{
    EDA_BaseStruct* t = LastDrawList();

    while( t )
    {
        if( t->Type() == DRAW_LIB_ITEM_STRUCT_TYPE )
        {
            EDA_SchComponentStruct* d = (EDA_SchComponentStruct*) t;
            d->m_Field[REFERENCE].m_Text = d->GetRef( this );
        }
        t = t->Pnext;
    }

    printf( "on sheet: %s \n", CONV_TO_UTF8( PathHumanReadable() ) );
}


bool DrawSheetPath::operator=( const DrawSheetPath& d1 )
{
    m_numSheets = d1.m_numSheets;
    int i;
    for( i = 0; i<m_numSheets; i++ )
    {
        m_sheets[i] = d1.m_sheets[i];
    }

    for( ; i<DSLSZ; i++ )
    {
        m_sheets[i] = 0;
    }

    return true;
}


bool DrawSheetPath::operator==( const DrawSheetPath& d1 )
{
    if( m_numSheets != d1.m_numSheets )
        return false;
    for( int i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
            return false;
    }

    return true;
}


bool DrawSheetPath::operator!=( const DrawSheetPath& d1 )
{
    if( m_numSheets != d1.m_numSheets )
        return true;
    for( int i = 0; i<m_numSheets; i++ )
    {
        if( m_sheets[i] != d1.m_sheets[i] )
            return true;
    }

    return false;
}
