/****************************************/
/* Modules to handle component drawing. */
/****************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "trigo.h"
#include "protos.h"

//#define DRAW_ARC_WITH_ANGLE       // Used to select function to draw arcs


/* Local functions */


/***************************************************************************/
/** Function TransformCoordinate
 * Calculate the wew coordinate from the old one, according to the transform
 * matrix.
 * @param aTransformMatrix = rotation, mirror .. matrix
 * @param aPosition = the position to transform
 * @return the new coordinate
 */
/***************************************************************************/
wxPoint TransformCoordinate( const int      aTransformMatrix[2][2],
                             const wxPoint& aPosition )
{
    wxPoint new_pos;

    new_pos.x = ( aTransformMatrix[0][0] * aPosition.x ) +
        ( aTransformMatrix[0][1] * aPosition.y );
    new_pos.y = ( aTransformMatrix[1][0] * aPosition.x ) +
        ( aTransformMatrix[1][1] * aPosition.y );

    return new_pos;
}


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
/*************************************************************/
void DrawLibEntry( WinEDA_DrawPanel* panel, wxDC* DC,
                   EDA_LibComponentStruct* LibEntry,
                   const wxPoint& aOffset,
                   int Multi, int convert,
                   int DrawMode, int Color )
{
    int           color;
    wxString      Prefix;
    LibDrawField* Field;
    wxPoint       text_pos;


    DrawLibPartAux( panel, DC, NULL, LibEntry, aOffset,
                    DefaultTransformMatrix, Multi,
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
    else
        color = Color;

    if( LibEntry->m_UnitCount > 1 )
#if defined(KICAD_GOST)

        Prefix.Printf( wxT( "%s?.%c" ),
                       LibEntry->m_Prefix.m_Text.GetData(), Multi + '1' - 1 );
#else

        Prefix.Printf( wxT( "%s?%c" ),
                       LibEntry->m_Prefix.m_Text.GetData(), Multi + 'A' - 1 );
#endif
    else
        Prefix = LibEntry->m_Prefix.m_Text + wxT( "?" );

    if( (LibEntry->m_Prefix.m_Flags & IS_MOVED) == 0 )
        LibEntry->m_Prefix.Draw( panel, DC, aOffset, color, DrawMode,
                                 &Prefix, DefaultTransformMatrix );

    if( LibEntry->m_Name.m_Attributs & TEXT_NO_VISIBLE )
    {
        if( Color >= 0 )
            color = Color;
        else
            color = g_InvisibleItemColor;
    }
    else
        color = Color;

    if( (LibEntry->m_Name.m_Flags & IS_MOVED) == 0 )
        LibEntry->m_Name.Draw( panel, DC, aOffset, color, DrawMode,  NULL,
                               DefaultTransformMatrix );

    for( Field = LibEntry->m_Fields; Field != NULL; Field = Field->Next() )
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
        else
            color = Color;
        Field->Draw( panel, DC, aOffset, color, DrawMode, NULL,
                     DefaultTransformMatrix );
    }

    // Trace de l'ancre
    int len = panel->GetScreen()->Unscale( 3 );
    GRLine( &panel->m_ClipBox, DC, aOffset.x, aOffset.y - len, aOffset.x,
            aOffset.y + len, 0, color );
    GRLine( &panel->m_ClipBox, DC, aOffset.x - len, aOffset.y, aOffset.x + len,
            aOffset.y, 0, color );

    /* Enable this to draw the bounding box around the component to validate
     * the bounding box calculations. */
#if 0
    EDA_Rect bBox = LibEntry->GetBoundaryBox( Multi, convert );
    GRRect( &panel->m_ClipBox, DC, bBox.GetOrigin().x, bBox.GetOrigin().y,
            bBox.GetEnd().x, bBox.GetEnd().y, 0, LIGHTMAGENTA );
#endif

}


/*****************************************************************************/
/*
 *  Routine to find a part in one of the libraries given its name.
 *  Name = Name of part.
 *  LibName = Name of Lib; if "": seach in all libs
 *  Alias = Flag: si flag != 0, retourne un pointeur sur une part ou un alias
 *                si flag = 0, retourne un pointeur sur une part meme si le nom
 *                correspond a un alias
 *  Alias = FIND_ROOT, ou Alias = FIND_ALIAS
 */
/*****************************************************************************/
EDA_LibComponentStruct* FindLibPart( const wxChar*   Name,
                                     const wxString& LibName,
                                     int             Alias )
{
    EDA_LibComponentStruct*       Entry;

    /* Used only to call PQFind. */
    static EDA_LibComponentStruct DummyEntry( wxEmptyString );

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
            Entry = FindLibPart(
                 ( (EDA_LibCmpAliasStruct*) Entry )->m_RootName.GetData(),
                Lib->m_Name, FIND_ROOT );
    }

    return Entry;
}


/*****************************************************************************
 * Routine to rotate the given angular direction by the given Transformation. *
 * Input (and output) angles must be as follows:                              *
 *  Unit is 0.1 degre                                                         *
 * Angle1 in [0..3600], Angle2 > Angle1 in [0..7200]. Arc is assumed to be    *
 * less  than 180.0 degrees.                                                  *
 * Algorithm:                                                                 *
 * Map the angles to a point on the unit circle which is mapped using the     *
 * transform (only mirror and rotate so it remains on the unit circle) to     *
 * a new point which is used to detect new angle.                             *
 *****************************************************************************/
bool MapAngles( int* Angle1, int* Angle2, const int TransMat[2][2] )
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
    *Angle1 = (int) ( atan2( y, x ) * 1800.0 / M_PI + 0.5 );

    x = cos( *Angle2 * M_PI / 1800.0 );
    y = sin( *Angle2 * M_PI / 1800.0 );
    t = x * TransMat[0][0] + y * TransMat[0][1];
    y = x * TransMat[1][0] + y * TransMat[1][1];
    x = t;
    *Angle2 = (int) ( atan2( y, x ) * 1800.0 / M_PI + 0.5 );

    NORMALIZE_ANGLE( *Angle1 );
    NORMALIZE_ANGLE( *Angle2 );
    if( *Angle2 < *Angle1 )
        *Angle2 += 3600;

    if( *Angle2 - *Angle1 > 1800 ) /* Need to swap the two angles. */
    {
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
* Routine to display an outline version of given library entry.              *
* This routine is applied by the PlaceLibItem routine above.                 *
*****************************************************************************/
void DrawingLibInGhost( WinEDA_DrawPanel* panel, wxDC* DC,
                        EDA_LibComponentStruct* LibEntry,
                        SCH_COMPONENT* DrawLibItem, int PartX, int PartY,
                        int multi, int convert, int Color, bool DrawPinText )
{
    int DrawMode = g_XorMode;

    DrawLibPartAux( panel, DC, DrawLibItem, LibEntry, wxPoint( PartX, PartY ),
                    DrawLibItem->m_Transform, multi, convert, DrawMode, Color,
                    DrawPinText );
}


/************************************************************/
/* Routine to draw One LibraryDrawStruct at given position, */
/* matrice de transformation  1 0 0 -1 (normale)            */
/* DrawMode  = GrXOR, GrOR ..                               */
/************************************************************/
/* Utilise en LibEdit et Lib Browse */
void DrawLibraryDrawStruct( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                            EDA_LibComponentStruct* aLibEntry,
                            wxPoint aPosition, LibEDA_BaseStruct* aDrawItem,
                            int aDrawMode, int aColor )
{
    int  TransMat[2][2];
    bool no_fill;
    BASE_SCREEN* screen = aPanel->GetScreen();

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
        aDrawItem->Draw( aPanel, aDC, aPosition, aColor, aDrawMode, &prms,
                         TransMat );
    }
    break;

    case COMPONENT_ARC_DRAW_TYPE:
    case COMPONENT_CIRCLE_DRAW_TYPE:
    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
    case COMPONENT_RECT_DRAW_TYPE:
    case COMPONENT_POLYLINE_DRAW_TYPE:
    default:
        if( screen->m_IsPrinting
            && aDrawItem->m_Fill == FILLED_WITH_BG_BODYCOLOR
            && GetGRForceBlackPenState() )
            no_fill = true;
        aDrawItem->Draw( aPanel, aDC, aPosition, aColor, aDrawMode,
                         (void*) no_fill, TransMat );
        break;
    }
}
