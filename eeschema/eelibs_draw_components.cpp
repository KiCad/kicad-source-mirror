/****************************************/
/* Modules to handle component drawing. */
/****************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "component_class.h"
#include "general.h"
#include "trigo.h"
#include "protos.h"

//#define DRAW_ARC_WITH_ANGLE		// Used to select function to draw arcs


/* Local functions */
/* Descr component <DUMMY> used when a component is not found in library,
 *  to draw a dummy shape
 *  This component is a 400 mils square with the text ??
 *  DEF DUMMY U 0 40 Y Y 1 0 N
 *  F0 "U" 0 -350 60 H V
 *  F1 "DUMMY" 0 350 60 H V
 *  DRAW
 *  T 0 0 0 150 0 0 0 ??
 *  S -200 200 200 -200 0 1 0
 *  ENDDRAW
 *  ENDDEF
 */
static void CreateDummyCmp();
static void DrawLibPartAux( WinEDA_DrawPanel * panel, wxDC * DC,
                            SCH_COMPONENT * Component,
                            EDA_LibComponentStruct * Entry,
                            const wxPoint &Pos,
                            int TransMat[2][2],
                            int Multi, int convert,
                            int DrawMode, int Color = -1, bool DrawPinText = TRUE );

/* Local variables */
static EDA_LibComponentStruct* DummyCmp;

/***************************************************************************/
wxPoint TransformCoordinate( int aTransformMatrix[2][2], wxPoint& aPosition )
/***************************************************************************/

/** Function TransformCoordinate
 * Calculate the wew coordinate from the old one, according to the transform matrix.
 * @param aTransformMatrix = rotation, mirror .. matrix
 * @param aPosition = the position to transform
 * @return the new coordinate
 */
{
    wxPoint new_pos;

    new_pos.x = (aTransformMatrix[0][0] * aPosition.x) + (aTransformMatrix[0][1] * aPosition.y);
    new_pos.y = (aTransformMatrix[1][0] * aPosition.x) + (aTransformMatrix[1][1] * aPosition.y);
    return new_pos;
}


/******************************/
void CreateDummyCmp()
/******************************/
{
    DummyCmp = new              EDA_LibComponentStruct( NULL );

    LibDrawSquare* Square = new LibDrawSquare();

    Square->m_Pos   = wxPoint( -200, 200 );
    Square->m_End   = wxPoint( 200, -200 );
    Square->m_Width = 4;

    LibDrawText* Text = new LibDrawText();

    Text->m_Size.x = Text->m_Size.y = 150;
    Text->m_Text   = wxT( "??" );

    DummyCmp->m_Drawings = Square;
    Square->Pnext = Text;
}


/*************************************************************/
void DrawLibEntry( WinEDA_DrawPanel* panel, wxDC* DC,
                   EDA_LibComponentStruct* LibEntry,
                   const wxPoint & aOffset,
                   int Multi, int convert,
                   int DrawMode, int Color )
/**************************************************************/

/* Routine de dessin d'un composant d'une librairie
 *  LibEntry = pointeur sur la description en librairie
 *  posX, posY = position du composant
 *  DrawMode = GrOR ..
 *  Color = 0 : dessin en vraies couleurs, sinon couleur = Color
 *
 *  Une croix symbolise le point d'accrochage (ref position) du composant
 *
 *  Le composant est toujours trace avec orientation 0
 */
{
    int           color;
    int           TransMat[2][2];
    wxString      Prefix;
    LibDrawField* Field;
    wxPoint       text_pos;

    /* Orientation normale */
    TransMat[0][0] = 1; TransMat[1][1] = -1;
    TransMat[1][0] = TransMat[0][1] = 0;

    DrawLibPartAux( panel, DC, NULL, LibEntry, aOffset,
        TransMat, Multi,
        convert, DrawMode, Color );

    /* Trace des 2 champs ref et value (Attention aux coord: la matrice
     *  de transformation change de signe les coord Y */

    GRSetDrawMode( DC, DrawMode );

    if( LibEntry->m_Prefix.m_Attributs & TEXT_NO_VISIBLE )
    {
        if( Color >= 0 )
            color = Color;
        else
            color = g_InvisibleItemColor;
    }
    else color = Color;

    if( LibEntry->m_UnitCount > 1 )
#if defined(KICAD_GOST)
        Prefix.Printf( wxT( "%s?.%c" ), LibEntry->m_Prefix.m_Text.GetData(), Multi + '1' - 1);
#else
        Prefix.Printf( wxT( "%s?%c" ), LibEntry->m_Prefix.m_Text.GetData(), Multi + 'A' - 1 );
#endif
    else
        Prefix = LibEntry->m_Prefix.m_Text + wxT( "?" );

    if( (LibEntry->m_Prefix.m_Flags & IS_MOVED) == 0 )
        LibEntry->m_Prefix.Draw( panel, DC, aOffset, color, DrawMode, &Prefix, TransMat );

    if( LibEntry->m_Name.m_Attributs & TEXT_NO_VISIBLE )
    {
        if( Color >= 0 )
            color = Color;
        else
            color = g_InvisibleItemColor;
    }
    else color = Color;

    if( (LibEntry->m_Name.m_Flags & IS_MOVED) == 0 )
       LibEntry->m_Name.Draw( panel, DC, aOffset, color, DrawMode, NULL, TransMat );

    for( Field = LibEntry->Fields; Field != NULL; Field = (LibDrawField*) Field->Pnext )
    {
        if( Field->m_Text.IsEmpty() )
            return;
        if( (Field->m_Flags & IS_MOVED) != 0 )
            continue;
        if( Field->m_Attributs & TEXT_NO_VISIBLE )
        {
            if( Color >= 0 )
                color = Color;
            else
                color = g_InvisibleItemColor;
        }
        else color = Color;
        Field->Draw( panel, DC, aOffset, color, DrawMode, NULL, TransMat );
    }

    // Trace de l'ancre
    int len = 3 * panel->GetZoom();
    GRLine( &panel->m_ClipBox, DC, aOffset.x, aOffset.y - len, aOffset.x, aOffset.y + len, 0, color );
    GRLine( &panel->m_ClipBox, DC, aOffset.x - len, aOffset.y, aOffset.x + len, aOffset.y, 0, color );
}


/*****************************************************************************
* Routine to draw the given part at given position, transformed/mirror as	 *
* specified, and in the given drawing mode. Only this one is visible...		 *
*****************************************************************************/
void SCH_COMPONENT::Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                          const wxPoint& offset, int DrawMode, int Color )
{
    EDA_LibComponentStruct* Entry;
    int  ii;
    bool dummy = FALSE;

    if( ( Entry = FindLibPart( m_ChipName.GetData(), wxEmptyString, FIND_ROOT ) ) == NULL )
    {
        /* composant non trouve, on affiche un composant "dummy" */
        dummy = TRUE;
        if( DummyCmp == NULL )
            CreateDummyCmp();
        Entry = DummyCmp;
    }

    DrawLibPartAux( panel, DC, this, Entry, m_Pos + offset,
        m_Transform,
        dummy ? 0 : m_Multi,
        dummy ? 0 : m_Convert,
        DrawMode );

    /* Trace des champs, avec placement et orientation selon orient. du
     *  composant
     */

    SCH_CMP_FIELD* field = GetField( REFERENCE );

    if( ( (field->m_Attributs & TEXT_NO_VISIBLE) == 0 )
       && !(field->m_Flags & IS_MOVED) )
    {
        if( Entry->m_UnitCount > 1 )
        {
            field->m_AddExtraText = true;
            field->Draw( panel, DC, offset, DrawMode );
        }
        else
        {
            field->m_AddExtraText = false;
            field->Draw( panel, DC, offset, DrawMode );
        }
    }

    for( ii = VALUE; ii < GetFieldCount(); ii++ )
    {
        field = GetField( ii );

        if( field->m_Flags & IS_MOVED )
            continue;

        field->Draw( panel, DC, offset, DrawMode );
    }
}


/***********************************************************/
void SCH_CMP_FIELD::Draw( WinEDA_DrawPanel* panel,
                           wxDC*             DC,
                           const wxPoint&    offset,
                           int               DrawMode,
                           int               Color )
/***********************************************************/

/* Routine de trace des textes type Field du composant.
 *  entree:
 *      DrawMode: mode de trace
 */
{
    int            orient, color;

    wxPoint        pos; /* Position des textes */
    SCH_COMPONENT* DrawLibItem = (SCH_COMPONENT*) m_Parent;
    int            hjustify, vjustify;
    int            LineWidth = MAX( m_Width, g_DrawMinimunLineWidth );

    if( m_Attributs & TEXT_NO_VISIBLE )
        return;

    if( IsVoid() )
        return;

    GRSetDrawMode( DC, DrawMode );

    /* Calcul de la position des textes, selon orientation du composant */
    orient   = m_Orient;
    hjustify = m_HJustify; vjustify = m_VJustify;
    pos = m_Pos - DrawLibItem->m_Pos;

    pos  = DrawLibItem->GetScreenCoord( pos );
    pos += DrawLibItem->m_Pos;

    /* Y a t-il rotation (pour l'orientation, la justification)*/
    if( DrawLibItem->m_Transform[0][1] )  // Rotation du composant de 90deg
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
        /* Y a t-il rotation, miroir (pour les justifications)*/
        EXCHG( hjustify, vjustify );
        if( DrawLibItem->m_Transform[1][0] < 0 )
            vjustify = -vjustify;
        if( DrawLibItem->m_Transform[1][0] > 0 )
            hjustify = -hjustify;
    }
    else
    {
        /* Texte horizontal: Y a t-il miroir (pour les justifications)*/
        if( DrawLibItem->m_Transform[0][0] < 0 )
            hjustify = -hjustify;
        if( DrawLibItem->m_Transform[1][1] > 0 )
            vjustify = -vjustify;
    }

    if( m_FieldId == REFERENCE )
        color = ReturnLayerColor( LAYER_REFERENCEPART );
    else if( m_FieldId == VALUE )
        color = ReturnLayerColor( LAYER_VALUEPART );
    else
        color = ReturnLayerColor( LAYER_FIELDS );

    if( !m_AddExtraText || (m_FieldId != REFERENCE) )
    {
        DrawGraphicText( panel, DC, pos, color, m_Text.GetData(),
            orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
            m_Size,
            hjustify, vjustify, LineWidth );
    }
    else // Si il y a plusieurs parts par boitier, ajouter a la reference l'identification de la selection ( A, B ... )
    {
        /* On ajoute alors A ou B ... a la reference */
        wxString fulltext = m_Text;
#if defined(KICAD_GOST)
        fulltext.Append( '.');
        fulltext.Append( '1' - 1 + DrawLibItem->m_Multi );
#else
        fulltext.Append( 'A' - 1 + DrawLibItem->m_Multi );
#endif

        DrawGraphicText( panel, DC, pos, color, fulltext.GetData(),
            orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
            m_Size,
            hjustify, vjustify, LineWidth );
    }
}


/********************************************************************************/
EDA_LibComponentStruct* FindLibPart( const wxChar* Name, const wxString& LibName, int Alias )
/********************************************************************************/

/*
 *  Routine to find a part in one of the libraries given its name.
 *  Name = Name of part.
 *  LibName = Name of Lib; if "": seach in all libs
 *  Alias = Flag: si flag != 0, retourne un pointeur sur une part ou un alias
 *                si flag = 0, retourne un pointeur sur une part meme si le nom
 *                correspond a un alias
 *  Alias = FIND_ROOT, ou Alias = FIND_ALIAS
 */
{
    EDA_LibComponentStruct*       Entry;

    static EDA_LibComponentStruct DummyEntry( wxEmptyString );  /* Used only to call PQFind. */

    LibraryStruct* Lib = g_LibraryList;

    DummyEntry.m_Drawings    = NULL; /* Used only to call PQFind. */
    DummyEntry.m_Name.m_Text = Name;

    PQCompFunc( (PQCompFuncType) LibraryEntryCompare );

    Entry = NULL;
    FindLibName.Empty();

    while( Lib )
    {
        if( !LibName.IsEmpty() )
        {
            if( Lib->m_Name != LibName )
            {
                Lib = Lib->m_Pnext;
                continue;
            }
        }

        if( Lib == NULL )
            break;

        Entry = (EDA_LibComponentStruct*) PQFind( Lib->m_Entries, &DummyEntry );
        if( Entry != NULL )
        {
            FindLibName = Lib->m_Name;
            break;
        }

        Lib = Lib->m_Pnext;
    }

    /* Si le nom est un alias, recherche du vrai composant */
    if( Entry )
    {
        if( (Entry->Type != ROOT ) && (Alias == FIND_ROOT) )
            Entry = FindLibPart( ( (EDA_LibCmpAliasStruct*) Entry )->m_RootName.GetData(),
                Lib->m_Name, FIND_ROOT );
    }

    return Entry;
}


/*****************************************************************************
* Routine to draw the given part at given position, transformed/mirror as
* specified, and in the given drawing mode.
* if Color < 0: Draw in normal color
* else draw  in color = Color
*****************************************************************************/
/* DrawMode  = GrXOR, GrOR ..*/
void DrawLibPartAux( WinEDA_DrawPanel* panel, wxDC* DC,
                     SCH_COMPONENT* Component,
                     EDA_LibComponentStruct* Entry,
                     const wxPoint& Pos,
                     int TransMat[2][2],
                     int Multi, int convert, int DrawMode,
                     int Color, bool DrawPinText )
{
    wxPoint            pos1, pos2;
    LibEDA_BaseStruct* DEntry;
    bool force_nofill;

    if( Entry->m_Drawings == NULL )
        return;
    GRSetDrawMode( DC, DrawMode );

    for( DEntry = Entry->m_Drawings; DEntry != NULL; DEntry = DEntry->Next() )
    {
        /* Do not draw items not attached to the current part */
        if( Multi && DEntry->m_Unit && (DEntry->m_Unit != Multi) )
            continue;

        if( convert && DEntry->m_Convert && (DEntry->m_Convert != convert) )
            continue;

        if( DEntry->m_Flags & IS_MOVED )
            continue; // Do do draw here an item while moving (the cursor handler does that)

        force_nofill = false;

        switch( DEntry->Type() )
        {
        case COMPONENT_PIN_DRAW_TYPE:
        {
            DrawPinPrms prms( Entry, DrawPinText );
            DEntry->Draw( panel, DC, Pos, Color, DrawMode, &prms, TransMat );
        }
            break;

        case COMPONENT_ARC_DRAW_TYPE:
        case COMPONENT_CIRCLE_DRAW_TYPE:
        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        case COMPONENT_RECT_DRAW_TYPE:
        case COMPONENT_POLYLINE_DRAW_TYPE:
        default:
            if( g_IsPrinting && DEntry->m_Fill == FILLED_WITH_BG_BODYCOLOR
               && GetGRForceBlackPenState() )
                force_nofill = true;
            DEntry->Draw( panel, DC, Pos, Color, DrawMode, (void*) force_nofill, TransMat );
            break;
        }
    }

    if( g_DebugLevel > 4 ) /* Draw the component boundary box */
    {
        EDA_Rect BoundaryBox;
        if( Component )
            BoundaryBox = Component->GetBoundaryBox();
        else
            BoundaryBox = Entry->GetBoundaryBox( Multi, convert );
        int x1 = BoundaryBox.GetX();
        int y1 = BoundaryBox.GetY();
        int x2 = BoundaryBox.GetRight();
        int y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
        BoundaryBox = Component->GetField( REFERENCE )->GetBoundaryBox();
        x1 = BoundaryBox.GetX();
        y1 = BoundaryBox.GetY();
        x2 = BoundaryBox.GetRight();
        y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
        BoundaryBox = Component->GetField( VALUE )->GetBoundaryBox();
        x1 = BoundaryBox.GetX();
        y1 = BoundaryBox.GetY();
        x2 = BoundaryBox.GetRight();
        y2 = BoundaryBox.GetBottom();
        GRRect( &panel->m_ClipBox, DC, x1, y1, x2, y2, BROWN );
    }
}


/*****************************************************************************
* Routine to rotate the given angular direction by the given Transformation. *
* Input (and output) angles must be as follows:								 *
*	Unit is 0.1 degre														 *
* Angle1 in [0..3600], Angle2 > Angle1 in [0..7200]. Arc is assumed to be less *
* than 180.0 degrees.															 *
* Algorithm:																 *
* Map the angles to a point on the unit circle which is mapped using the	 *
* transform (only mirror and rotate so it remains on the unit circle) to	 *
* a new point which is used to detect new angle.							 *
*****************************************************************************/
bool MapAngles( int* Angle1, int* Angle2, int TransMat[2][2] )
{
    int    Angle, Delta;
    double x, y, t;
    bool   swap = FALSE;

    Delta = *Angle2 - *Angle1;
    if( Delta >= 1800 )
    {
        *Angle1 -= 1;
        *Angle2 += 1;
    }

    x = cos( *Angle1 * M_PI / 1800.0 );
    y = sin( *Angle1 * M_PI / 1800.0 );
    t = x * TransMat[0][0] + y * TransMat[0][1];
    y = x * TransMat[1][0] + y * TransMat[1][1];
    x = t;
    *Angle1 = (int) (atan2( y, x ) * 1800.0 / M_PI + 0.5);

    x = cos( *Angle2 * M_PI / 1800.0 );
    y = sin( *Angle2 * M_PI / 1800.0 );
    t = x * TransMat[0][0] + y * TransMat[0][1];
    y = x * TransMat[1][0] + y * TransMat[1][1];
    x = t;
    *Angle2 = (int) (atan2( y, x ) * 1800.0 / M_PI + 0.5);

    NORMALIZE_ANGLE( *Angle1 );
    NORMALIZE_ANGLE( *Angle2 );
    if( *Angle2 < *Angle1 )
        *Angle2 += 3600;

    if( *Angle2 - *Angle1 > 1800 )
    {                /* Need to swap the two angles. */
        Angle   = (*Angle1);
        *Angle1 = (*Angle2);
        *Angle2 = Angle;

        NORMALIZE_ANGLE( *Angle1 );
        NORMALIZE_ANGLE( *Angle2 );
        if( *Angle2 < *Angle1 )
            *Angle2 += 3600;
        swap = TRUE;
    }

    if( Delta >= 1800 )
    {
        *Angle1 += 1;
        *Angle2 -= 1;
    }

    return swap;
}


/*****************************************************************************
* Routine to display an outline version of given library entry.				 *
* This routine is applied by the PlaceLibItem routine above.			 	 *
*****************************************************************************/
void DrawingLibInGhost( WinEDA_DrawPanel* panel, wxDC* DC,
                        EDA_LibComponentStruct* LibEntry,
                        SCH_COMPONENT* DrawLibItem, int PartX, int PartY,
                        int multi, int convert, int Color, bool DrawPinText )
{
    int DrawMode = g_XorMode;

    DrawLibPartAux( panel, DC, DrawLibItem, LibEntry, wxPoint( PartX, PartY ),
        DrawLibItem->m_Transform,
        multi, convert, DrawMode, Color, DrawPinText );
}


/************************************************************/
/* Routine to draw One LibraryDrawStruct at given position, */
/* matrice de transformation  1 0 0 -1 (normale)			*/
/* DrawMode  = GrXOR, GrOR ..								*/
/************************************************************/
/* Utilise en LibEdit et Lib Browse */
void DrawLibraryDrawStruct( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                            EDA_LibComponentStruct* aLibEntry,
                            wxPoint aPosition,
                            LibEDA_BaseStruct* aDrawItem,
                            int aDrawMode, int aColor )
{
    int  TransMat[2][2];
    bool no_fill;

    GRSetDrawMode( aDC, aDrawMode );

    TransMat[0][0] = 1;
    TransMat[0][1] = TransMat[1][0] = 0;
    TransMat[1][1] = -1;

    no_fill = false;

    switch( aDrawItem->Type() )
    {
    case COMPONENT_PIN_DRAW_TYPE:     /* Trace des Pins */
    {
        DrawPinPrms prms( aLibEntry, true );
        aDrawItem->Draw( aPanel, aDC, aPosition, aColor, aDrawMode, &prms, TransMat );
    }
        break;

    case COMPONENT_ARC_DRAW_TYPE:
    case COMPONENT_CIRCLE_DRAW_TYPE:
    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
    case COMPONENT_RECT_DRAW_TYPE:
    case COMPONENT_POLYLINE_DRAW_TYPE:
    default:
        if( g_IsPrinting && aDrawItem->m_Fill == FILLED_WITH_BG_BODYCOLOR
           && GetGRForceBlackPenState() )
            no_fill = true;
        aDrawItem->Draw( aPanel, aDC, aPosition, aColor, aDrawMode, (void*) no_fill, TransMat );
        break;
    }
}
