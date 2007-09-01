/***********************************************************************/
/* Methodes de base de gestion des classes des elements de schematique */
/***********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "id.h"

#include "protos.h"


/************************/
/* class DrawTextStruct */
/* class DrawLabelStruct */
/* class DrawGlobalLabelStruct */
/************************/

/**************************************************************************/
DrawTextStruct::DrawTextStruct( const wxPoint& pos, const wxString& text, KICAD_T aType ) :
    EDA_BaseStruct( aType ), 
    EDA_TextStruct( text )
/**************************************************************************/
{
    m_Layer      = LAYER_NOTES;
    m_Pos        = pos;
    m_Shape      = 0;
    m_IsDangling = FALSE;
}


/*********************************************/
DrawTextStruct* DrawTextStruct::GenCopy()
/*********************************************/
{
    DrawTextStruct* newitem;
    
    switch( Type() )
    {
    case DRAW_TEXT_STRUCT_TYPE:
        newitem = new DrawTextStruct( m_Pos, m_Text );
        break;
    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
        newitem = new DrawGlobalLabelStruct( m_Pos, m_Text );
        break;
    case DRAW_LABEL_STRUCT_TYPE:
        newitem = new DrawLabelStruct( m_Pos, m_Text );
        break;
    default:
        newitem = 0;        // will crash below
    }

    newitem->m_Layer      = m_Layer;
    newitem->m_Shape      = m_Shape;
    newitem->m_Orient     = m_Orient;
    newitem->m_Size       = m_Size;
    newitem->m_Width      = m_Width;
    newitem->m_HJustify   = m_HJustify;
    newitem->m_VJustify   = m_VJustify;
    newitem->m_IsDangling = m_IsDangling;

    return newitem;
}


/********************************************************/
void DrawTextStruct::SwapData( DrawTextStruct* copyitem )
/********************************************************/
{
    EXCHG( m_Text, copyitem->m_Text );
    EXCHG( m_Pos, copyitem->m_Pos );
    EXCHG( m_Size, copyitem->m_Size );
    EXCHG( m_Width, copyitem->m_Width );
    EXCHG( m_Shape, copyitem->m_Shape );
    EXCHG( m_Orient, copyitem->m_Orient );

    //	EXCHG( m_StructType, copyitem->m_StructType );
    // how can you swap a type, it is what it was created as!
    // this is a very bad usage of m_StructType.
    KICAD_T aType = copyitem->Type();
    copyitem->SetType( Type() );
    SetType( aType );
    
    EXCHG( m_Layer, copyitem->m_Layer );
    EXCHG( m_HJustify, copyitem->m_HJustify );
    EXCHG( m_VJustify, copyitem->m_VJustify );
    EXCHG( m_IsDangling, copyitem->m_IsDangling );
}


/***************************************************************/
void DrawTextStruct::Place( WinEDA_DrawFrame* frame, wxDC* DC )
/***************************************************************/
{
    /* save old text in undo list */
    if( g_ItemToUndoCopy && ( (m_Flags & IS_NEW) == 0 ) )
    {
        /* restore old values and save new ones */
        SwapData( (DrawTextStruct*) g_ItemToUndoCopy );
        
        /* save in undo list */
        ( (WinEDA_SchematicFrame*) frame )->SaveCopyInUndoList( this, IS_CHANGED );
        
        /* restore new values */
        SwapData( (DrawTextStruct*) g_ItemToUndoCopy );
        
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = NULL;
    }

    EDA_BaseStruct::Place( frame, DC );
}


/****************************************************************************/
DrawLabelStruct::DrawLabelStruct( const wxPoint& pos, const wxString& text ) :
    DrawTextStruct( pos, text, DRAW_LABEL_STRUCT_TYPE )
/****************************************************************************/
{
    m_Layer      = LAYER_LOCLABEL;
    m_IsDangling = TRUE;
}


/***********************************************************************************/
DrawGlobalLabelStruct::DrawGlobalLabelStruct( const wxPoint& pos, const wxString& text ) :
    DrawTextStruct( pos, text, DRAW_GLOBAL_LABEL_STRUCT_TYPE )
/***********************************************************************************/
{
    m_Layer      = LAYER_GLOBLABEL;
    m_Shape      = NET_INPUT;
    m_IsDangling = TRUE;
}


/***************************************************************/
void DrawTextStruct::Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                           int DrawMode, int Color )
/***************************************************************/

/* Les textes type label ou notes peuvent avoir 4 directions, mais
 *  sont tj cadres par rapport a la 1ere lettre du texte
 */
{
    switch( Type() )
    {
    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
        DrawAsGlobalLabel( panel, DC, offset, DrawMode, Color );
        break;

    case DRAW_LABEL_STRUCT_TYPE:
        DrawAsLabel( panel, DC, offset, DrawMode, Color );
        break;

    default:
        DrawAsText( panel, DC, offset, DrawMode, Color );
    }
}


/*******************************************************************************************/
void DrawTextStruct::DrawAsText( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                 int DrawMode, int Color )
/*******************************************************************************************/

/* Texts type Label or Comment (text on layer "NOTE") have 4 directions, and the Text origin is the first letter
 */
{
    int color;
    int width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );
    GRSetDrawMode( DC, DrawMode );

    switch( m_Orient )
    {
    case 0:         /* Orientation horiz normale */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + offset.x, m_Pos.y - TXTMARGE + offset.y ),
                         color,
                         m_Text, m_Orient * 900, m_Size,
                         GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM, width );
        break;

    case 1:         /* Orientation vert UP */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x - TXTMARGE + offset.x, m_Pos.y + offset.y ), color,
                         m_Text, m_Orient * 900, m_Size,
                         GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_BOTTOM, width );
        break;

    case 2:         /* Orientation horiz inverse */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + offset.x, m_Pos.y + TXTMARGE + offset.y ), color,
                         m_Text, m_Orient * 900, m_Size,
                         GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_TOP, width );
        break;

    case 3:         /* Orientation vert BOTTOM */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + TXTMARGE + offset.y, m_Pos.y + offset.y ), color,
                         m_Text, m_Orient * 900, m_Size,
                         GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP, width );
        break;
    }

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + offset, color );
}


/***************************************************************/
void DrawTextStruct::DrawAsLabel( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                  int DrawMode, int Color )
/***************************************************************/
{
    DrawAsText( panel, DC, offset, DrawMode, Color );
}


/*****************************************************************************/
void DrawTextStruct::DrawAsGlobalLabel( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                                        int DrawMode, int Color )
/*****************************************************************************/

/* Texts type Global Label  have 4 directions, and the Text origin is the graphic icon
 */
{
    int*   Template;
    int    Poly[12];
    int    ii, jj, imax, color, HalfSize;
    wxSize Size  = m_Size;
    int    width = MAX( m_Width, g_DrawMinimunLineWidth );

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    HalfSize = Size.x / 2; ii = Size.x + TXTMARGE;

    switch( m_Orient )
    {
    case 0:         /* Orientation horiz normale */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x - ii + offset.x, m_Pos.y + offset.y ), color,
                         m_Text, TEXT_ORIENT_HORIZ, Size,
                         GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER, width );
        break;

    case 1:         /* Orientation vert UP */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + offset.x, m_Pos.y + ii + offset.y ), color,
                         m_Text, TEXT_ORIENT_VERT, Size,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP, width );
        break;

    case 2:         /* Orientation horiz inverse */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + ii + offset.x, m_Pos.y + offset.y ), color,
                         m_Text, TEXT_ORIENT_HORIZ, Size,
                         GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, width );
        break;

    case 3:         /* Orientation vert BOTTOM */
        DrawGraphicText( panel, DC,
                         wxPoint( m_Pos.x + offset.x, m_Pos.y - ii + offset.y ), color,
                         m_Text, TEXT_ORIENT_VERT, Size,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM, width );
        break;
    }


    Template = TemplateShape[m_Shape][m_Orient];

    imax = *Template; Template++;
    for( ii = 0, jj = 0; ii < imax; ii++ )
    {
        Poly[jj] = ( HalfSize * (*Template) ) + m_Pos.x + offset.x;
        jj++; Template++;
        Poly[jj] = ( HalfSize * (*Template) ) + m_Pos.y + offset.y;
        jj++; Template++;
    }

//	GRPoly(&panel->m_ClipBox, DC, imax,Poly,1, width, color, color );	/* Polygne Rempli */
    GRPoly( &panel->m_ClipBox, DC, imax, Poly, 0, width, color, color );   /* Polygne Non Rempli */

    if( m_IsDangling )
        DrawDanglingSymbol( panel, DC, m_Pos + offset, color );
}
