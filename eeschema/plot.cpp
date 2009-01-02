/***************************************************/
/* Plot functions common to different plot formats */
/***************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "plot_common.h"
#include "worksheet.h"

#include "protos.h"

/* Local Variables : */
static void PlotSheetLabelStruct( Hierarchical_PIN_Sheet_Struct* Struct );
static void PlotTextField( SCH_COMPONENT* DrawLibItem,
                           int FieldNumber, int IsMulti, int DrawMode );
static void PlotPinSymbol( const wxPoint & pos, int len, int orient, int Shape );

/***/

/* Defines for filling polygons in plot polygon functions */
#define FILL   true
#define NOFILL false

/* routine de lever ou baisser de plume.
  * si plume = 'U' les traces suivants se feront plume levee
  * si plume = 'D' les traces suivants se feront plume levee
 */
void Plume( int plume )
{
    if( g_PlotFormat  == PLOT_FORMAT_HPGL )
        Plume_HPGL( plume );
}


/* routine de deplacement de plume de plume.
 */
void Move_Plume( wxPoint pos, int plume )
{
    switch( g_PlotFormat )
    {
    case PLOT_FORMAT_HPGL:
        Move_Plume_HPGL( pos, plume );
        break;

    case PLOT_FORMAT_POST:
        LineTo_PS( pos, plume );
        break;
    }
}


void SetCurrentLineWidth( int width )
{
    switch( g_PlotFormat )
    {
    case PLOT_FORMAT_HPGL:
        break;

    case PLOT_FORMAT_POST:
        SetCurrentLineWidthPS( width );
        break;
    }
}

/*******************************************************************************/
void PlotRect( wxPoint p1, wxPoint p2, int fill, int width )
/*******************************************************************************/
{
    switch( g_PlotFormat )
    {
    case PLOT_FORMAT_HPGL:
        PlotRectHPGL( p1, p2, fill, width );
        break;

    case PLOT_FORMAT_POST:
        PlotRectPS( p1, p2, fill, width );
        break;
    }
}

/*****************************************************************************************/
void PlotArc( wxPoint aCentre, int aStAngle, int aEndAngle, int aRadius, bool aFill, int aWidth )
/*****************************************************************************************/

/** Function PlotArc
  * Plot an arc:
  * @param aCentre = Arc centre
  * @param aStAngle = begining of arc in 0.1 degrees
  * @param aEndAngle = end of arc in 0.1 degrees
  * @param aRadius = Arc radius
  * @param aFill = fill option
  * @param aWidth = Tickness of outlines
 */
{
    switch( g_PlotFormat )
    {
    case PLOT_FORMAT_HPGL:
        PlotArcHPGL( aCentre, aStAngle, aEndAngle, aRadius, aFill, aWidth );
        break;

    case PLOT_FORMAT_POST:
        PlotArcPS( aCentre, aStAngle, aEndAngle, aRadius, aFill, aWidth );
        break;
    }
}


/*****************************************************************/
void PlotCercle( wxPoint pos, int diametre, bool fill, int width )
/*****************************************************************/
{
    switch( g_PlotFormat )
    {
    case PLOT_FORMAT_HPGL:
        PlotCircleHPGL( pos, diametre, fill, width );
        break;

    case PLOT_FORMAT_POST:
        PlotCirclePS( pos, diametre, fill, width );
        break;
    }
}


/******************************************************************/
void PlotPoly( int nb, int* coord, bool fill, int width )
/******************************************************************/

/* Trace un polygone ferme
  * coord = tableau des coord des sommets
  * nb = nombre de coord ( 1 coord = 2 elements: X et Y du tableau )
  * fill : si != 0 polygone rempli
 */
{
    if( nb <= 1 )
        return;

    switch( g_PlotFormat )
    {
    case PLOT_FORMAT_HPGL:
        PlotPolyHPGL( nb, coord, fill, width );
        break;

    case PLOT_FORMAT_POST:
        PlotPolyPS( nb, coord, fill, width );
        break;
    }
}


/**********************************************************/
void PlotNoConnectStruct( DrawNoConnectStruct* Struct )
/**********************************************************/

/* Routine de dessin des symboles de "No Connexion" ..
 */
{
#define DELTA (DRAWNOCONNECT_SIZE / 2)
    int pX, pY;

    pX = Struct->m_Pos.x; pY = Struct->m_Pos.y;

    SetCurrentLineWidth( -1 );
    Move_Plume( wxPoint( pX - DELTA, pY - DELTA ), 'U' );
    Move_Plume( wxPoint( pX + DELTA, pY + DELTA ), 'D' );
    Move_Plume( wxPoint( pX + DELTA, pY - DELTA ), 'U' );
    Move_Plume( wxPoint( pX - DELTA, pY + DELTA ), 'D' );
    Plume( 'U' );
}


/*************************************************/
void PlotLibPart( SCH_COMPONENT* DrawLibItem )
/*************************************************/
/* Polt a component */
{
    int                     ii, t1, t2, * Poly, orient;
    LibEDA_BaseStruct*      DEntry;
    EDA_LibComponentStruct* Entry;
    int                     TransMat[2][2], Multi, convert;
    EDA_Colors              CharColor = UNSPECIFIED_COLOR;
    wxPoint                 pos;
    bool                    draw_bgfill = false;

    Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;;
    memcpy( TransMat, DrawLibItem->m_Transform, sizeof(TransMat) );
    Multi   = DrawLibItem->m_Multi;
    convert = DrawLibItem->m_Convert;

    for( DEntry = Entry->m_Drawings; DEntry != NULL; DEntry = DEntry->Next() )
    {
        /* Elimination des elements non relatifs a l'unite */
        if( Multi && DEntry->m_Unit && (DEntry->m_Unit != Multi) )
            continue;
        if( convert && DEntry->m_Convert && (DEntry->m_Convert != convert) )
            continue;

        Plume( 'U' );
        if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        {
            SetColorMapPS( ReturnLayerColor( LAYER_DEVICE ) );
            draw_bgfill = true;
        }

        switch( DEntry->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
            {
                LibDrawArc* Arc = (LibDrawArc*) DEntry;
                t1    = Arc->t1; t2 = Arc->t2;
                pos = TransformCoordinate( TransMat, Arc->m_Pos ) + DrawLibItem->m_Pos;
                MapAngles( &t1, &t2, TransMat );
                if ( draw_bgfill && Arc->m_Fill == FILLED_WITH_BG_BODYCOLOR )
                {
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                    PlotArc( pos, t1, t2, Arc->m_Rayon, true, 0 );
                }
                if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE ) );
                PlotArc( pos, t1, t2, Arc->m_Rayon, Arc->m_Fill == FILLED_SHAPE ? true : false, Arc->m_Width );
            }
            break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
            {
                LibDrawCircle* Circle = (LibDrawCircle*) DEntry;
                pos = TransformCoordinate( TransMat, Circle->m_Pos ) + DrawLibItem->m_Pos;
                if ( draw_bgfill && Circle->m_Fill == FILLED_WITH_BG_BODYCOLOR )
                {
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                    PlotCercle( pos, Circle->m_Rayon * 2, true, 0 );
                }
                if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE ) );
                PlotCercle( pos, Circle->m_Rayon * 2, Circle->m_Fill == FILLED_SHAPE ? true : false, Circle->m_Width );
            }
            break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
            {
            LibDrawText* Text = (LibDrawText*) DEntry;

            /* The text orientation may need to be flipped if the
              * transformation matrix causes xy axes to be flipped. */
            t1    = (TransMat[0][0] != 0) ^ (Text->m_Orient != 0);
            pos = TransformCoordinate( TransMat, Text->m_Pos ) + DrawLibItem->m_Pos;
            SetCurrentLineWidth( -1 );
            int thickness = Text->m_Width;
			if( thickness == 0 )	//
				thickness = MAX( g_PlotPSMinimunLineWidth, g_DrawMinimunLineWidth );
            PlotGraphicText( g_PlotFormat, pos, CharColor,
                                 Text->m_Text,
                                 t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
                                 Text->m_Size,
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                thickness);
            }
            break;

        case COMPONENT_RECT_DRAW_TYPE:
            {
                LibDrawSquare* Square = (LibDrawSquare*) DEntry;
                pos = TransformCoordinate( TransMat, Square->m_Pos ) + DrawLibItem->m_Pos;
                wxPoint end = TransformCoordinate( TransMat, Square->m_End ) + DrawLibItem->m_Pos;

                if ( draw_bgfill && Square->m_Fill == FILLED_WITH_BG_BODYCOLOR )
                {
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                    PlotRect( pos, end, true, 0 );
                }
                if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE ) );
                PlotRect( pos, end, Square->m_Fill == FILLED_SHAPE ? true : false, Square->m_Width );
            }
            break;

        case COMPONENT_PIN_DRAW_TYPE:     /* Trace des Pins */
            {
                LibDrawPin* Pin = (LibDrawPin*) DEntry;
                if( Pin->m_Attributs & PINNOTDRAW )
                {
                        break;
                }

                /* Calcul de l'orientation reelle de la Pin */
                orient = Pin->ReturnPinDrawOrient( TransMat );
                /* compute Pin Pos */
                pos = TransformCoordinate( TransMat, Pin->m_Pos ) + DrawLibItem->m_Pos;

                /* Dessin de la pin et du symbole special associe */
                SetCurrentLineWidth( -1 );
                PlotPinSymbol( pos, Pin->m_PinLen, orient, Pin->m_PinShape );
                int thickness = 0;   // @todo: calcultae the pen tickness
                Pin->PlotPinTexts( pos, orient,
                                   Entry->m_TextInside,
                                   Entry->m_DrawPinNum, Entry->m_DrawPinName,
                                    thickness, false);
            }
            break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
            {
                LibDrawPolyline* polyline = (LibDrawPolyline*) DEntry;
                Poly = (int*) MyMalloc( sizeof(int) * 2 * polyline->GetCornerCount() );
                for( ii = 0; ii < (int)polyline->GetCornerCount(); ii++ )
                {
                    pos = polyline->m_PolyPoints[ii];
                    pos = TransformCoordinate( TransMat, pos ) + DrawLibItem->m_Pos;
                    Poly[ii * 2] = pos.x;
                    Poly[ii * 2 + 1] = pos.y;
                }

                if ( draw_bgfill && polyline->m_Fill == FILLED_WITH_BG_BODYCOLOR )
                {
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                    PlotPoly( ii, Poly, true, 0 );
                }
                if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
                    SetColorMapPS( ReturnLayerColor( LAYER_DEVICE ) );
                PlotPoly( ii, Poly, polyline->m_Fill == FILLED_SHAPE ? true : false, polyline->m_Width );
                MyFree( Poly );
            }
            break;

        default:
            D(printf("Drawing Type=%d\n", DEntry->Type() )) ;
        }

        /* Fin Switch */
        Plume( 'U' );
    }

    /* Fin Boucle de dessin */

    /* Trace des champs, avec placement et orientation selon orient. du
      * composant
      * Si la reference commence par # elle n'est pas tracee
     */

    if( (Entry->m_Prefix.m_Attributs & TEXT_NO_VISIBLE) == 0 )
    {
        if( Entry->m_UnitCount > 1 )
            PlotTextField( DrawLibItem, REFERENCE, 1, 0 );
        else
            PlotTextField( DrawLibItem, REFERENCE, 0, 0 );
    }

    if( (Entry->m_Name.m_Attributs & TEXT_NO_VISIBLE) == 0 )
        PlotTextField( DrawLibItem, VALUE, 0, 0 );

    for( ii = 2; ii < NUMBER_OF_FIELDS; ii++ )
    {
        PlotTextField( DrawLibItem, ii, 0, 0 );
    }
}


/*************************************************************/
static void PlotTextField( SCH_COMPONENT* DrawLibItem,
                           int FieldNumber, int IsMulti, int DrawMode )
/**************************************************************/

/* Routine de trace des textes type Field du composant.
  * entree:
  *     DrawLibItem: pointeur sur le composant
  *     FieldNumber: Numero du champ
  *     IsMulti: flag Non Null si il y a plusieurs parts par boitier.
  *             n'est utile que pour le champ reference pour ajouter a celui ci
  *             l'identification de la part ( A, B ... )
  *     DrawMode: mode de trace
 */

{
    wxPoint         textpos; /* Position des textes */
    SCH_CMP_FIELD*  field = DrawLibItem->GetField( FieldNumber );
    int             orient;
    EDA_Colors color = UNSPECIFIED_COLOR;

    if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        color = ReturnLayerColor( field->GetLayer() );

    DrawMode = 0;   /* Unused */
    if( field->m_Attributs & TEXT_NO_VISIBLE )
        return;
    if( field->IsVoid() )
        return;

    /* Calcul de la position des textes, selon orientation du composant */
    orient   = field->m_Orient;
    GRTextHorizJustifyType hjustify = field->m_HJustify;
    GRTextVertJustifyType vjustify = field->m_VJustify;
    textpos = field->m_Pos - DrawLibItem->m_Pos;    // textpos is the text position relative to the component anchor

    textpos = TransformCoordinate( DrawLibItem->m_Transform, textpos ) + DrawLibItem->m_Pos;

    /* Y a t-il rotation */
    if( DrawLibItem->m_Transform[0][1] )
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
        /* Y a t-il rotation, miroir (pour les justifications)*/
        GRTextHorizJustifyType tmp = hjustify;
        hjustify = (GRTextHorizJustifyType) vjustify;
        vjustify = (GRTextVertJustifyType) tmp;

        if( DrawLibItem->m_Transform[1][0] < 0 )
            switch ( vjustify )
            {
                case GR_TEXT_VJUSTIFY_BOTTOM:
                    vjustify = GR_TEXT_VJUSTIFY_TOP;
                    break;
                case GR_TEXT_VJUSTIFY_TOP:
                    vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
                    break;
                default:
                    break;
            }
        if( DrawLibItem->m_Transform[1][0] > 0 )
            switch ( hjustify )
            {
                case GR_TEXT_HJUSTIFY_LEFT:
                    hjustify = GR_TEXT_HJUSTIFY_RIGHT;
                    break;
                case GR_TEXT_HJUSTIFY_RIGHT:
                    hjustify = GR_TEXT_HJUSTIFY_LEFT;
                    break;
                default:
                    break;
            }
    }
    else
    {
        /* Texte horizontal: Y a t-il miroir (pour les justifications)*/
        if( DrawLibItem->m_Transform[0][0] < 0 )
            switch ( hjustify )
            {
                case GR_TEXT_HJUSTIFY_LEFT:
                    hjustify = GR_TEXT_HJUSTIFY_RIGHT;
                    break;
                case GR_TEXT_HJUSTIFY_RIGHT:
                    hjustify = GR_TEXT_HJUSTIFY_LEFT;
                    break;
                default:
                    break;
            }
        if( DrawLibItem->m_Transform[1][1] > 0 )
            switch ( vjustify )
            {
                case GR_TEXT_VJUSTIFY_BOTTOM:
                    vjustify = GR_TEXT_VJUSTIFY_TOP;
                    break;
                case GR_TEXT_VJUSTIFY_TOP:
                    vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
                    break;
                default:
                    break;
            }
    }

    int thickness = field->m_Width;
	if( thickness == 0 )
		thickness = MAX( g_PlotPSMinimunLineWidth, g_DrawMinimunLineWidth );
    SetCurrentLineWidth( thickness );

    //@todo not sure what to do here in terms of plotting components that may have multiple REFERENCE entries.
    if( !IsMulti || (FieldNumber != REFERENCE) )
    {
        PlotGraphicText( g_PlotFormat, textpos, color, field->m_Text,
                         orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                         field->m_Size,
                         hjustify, vjustify,
                        thickness, field->m_Italic);
    }
    else    /* We plt the reference, for a multiple parts per package */
    {
            /* Adding A, B ... to the reference */
        wxString Text;
        Text = field->m_Text;
#if defined(KICAD_GOST)
    Text.Append( '.' );
        Text.Append( '1' - 1 + DrawLibItem->m_Multi );
#else
        Text.Append( 'A' - 1 + DrawLibItem->m_Multi );
#endif
        PlotGraphicText( g_PlotFormat, textpos, color, Text,
                         orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                         field->m_Size, hjustify, vjustify,
                        thickness, field->m_Italic );
    }
}


/**************************************************************************/
static void PlotPinSymbol( const wxPoint & pos, int len, int orient, int Shape )
/**************************************************************************/

/* Trace la pin du symbole en cours de trace
 */
{
    int MapX1, MapY1, x1, y1;
    EDA_Colors color = UNSPECIFIED_COLOR;

    color = ReturnLayerColor( LAYER_PIN );

    if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        SetColorMapPS( color );

    SetCurrentLineWidth( -1 );

    MapX1 = MapY1 = 0; x1 = pos.x; y1 = pos.y;

    switch( orient )
    {
    case PIN_UP:
        y1 = pos.y - len; MapY1 = 1;
        break;

    case PIN_DOWN:
        y1 = pos.y + len; MapY1 = -1;
        break;

    case PIN_LEFT:
        x1 = pos.x - len, MapX1 = 1;
        break;

    case PIN_RIGHT:
        x1 = pos.x + len; MapX1 = -1;
        break;
    }

    if( Shape & INVERT )
    {
        PlotCercle( wxPoint( MapX1 * INVERT_PIN_RADIUS + x1,
                             MapY1 * INVERT_PIN_RADIUS + y1),
                    INVERT_PIN_RADIUS * 2,  // diameter
                    false,                  // fill
                    -1 );                   // width

        Move_Plume( wxPoint( MapX1 * INVERT_PIN_RADIUS * 2 + x1,
                             MapY1 * INVERT_PIN_RADIUS * 2 + y1 ), 'U' );
        Move_Plume( pos, 'D' );
    }
    else
    {
        Move_Plume( wxPoint( x1, y1 ), 'U' );
        Move_Plume( pos, 'D' );
    }

    if( Shape & CLOCK )
    {
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            Move_Plume( wxPoint( x1, y1 + CLOCK_PIN_DIM ), 'U' );
            Move_Plume( wxPoint( x1 - MapX1 * CLOCK_PIN_DIM, y1 ), 'D' );
            Move_Plume( wxPoint( x1, y1 - CLOCK_PIN_DIM ), 'D' );
        }
        else    /* MapX1 = 0 */
        {
            Move_Plume( wxPoint( x1 + CLOCK_PIN_DIM, y1 ), 'U' );
            Move_Plume( wxPoint( x1, y1 - MapY1 * CLOCK_PIN_DIM ), 'D' );
            Move_Plume( wxPoint( x1 - CLOCK_PIN_DIM, y1 ), 'D' );
        }
    }

    if( Shape & LOWLEVEL_IN )   /* IEEE symbol "Active Low Input" */
    {
        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            Move_Plume( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2, y1 ), 'U' );
            Move_Plume( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2,
                                 y1 - IEEE_SYMBOL_PIN_DIM ), 'D' );
            Move_Plume( wxPoint( x1, y1 ), 'D' );
        }
        else    /* MapX1 = 0 */
        {
            Move_Plume( wxPoint( x1, y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ), 'U' );
            Move_Plume( wxPoint( x1 - IEEE_SYMBOL_PIN_DIM,
                                 y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ), 'D' );
            Move_Plume( wxPoint( x1, y1 ), 'D' );
        }
    }


    if( Shape & LOWLEVEL_OUT )  /* IEEE symbol "Active Low Output" */
    {
        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            Move_Plume( wxPoint( x1, y1 - IEEE_SYMBOL_PIN_DIM ), 'U' );
            Move_Plume( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2, y1 ), 'D' );
        }
        else    /* MapX1 = 0 */
        {
            Move_Plume( wxPoint( x1 - IEEE_SYMBOL_PIN_DIM, y1 ), 'U' );
            Move_Plume( wxPoint( x1, y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ), 'D' );
        }
    }
    Plume( 'U' );
}


/*******************************************/
void PlotTextStruct( EDA_BaseStruct* Struct )
/*******************************************/

/*
  * Routine de trace des Textes, Labels et Global-Labels.
  * Les textes peuvent avoir 4 directions.
 */
{
    int      Poly[50];
    int      pX, pY, Shape = 0, Orient = 0, offset;
    wxSize   Size;
    wxString Text;
    EDA_Colors color = UNSPECIFIED_COLOR;

    bool italic = false;
    int thickness = 0;

    switch( Struct->Type() )
    {
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_LABEL:
    case TYPE_SCH_TEXT:
        Text   = ( (SCH_TEXT*) Struct )->m_Text;
        Size   = ( (SCH_TEXT*) Struct )->m_Size;
        thickness = ( (SCH_TEXT*) Struct )->m_Width;
        italic = ( (SCH_TEXT*) Struct )->m_Italic;
        Orient = ( (SCH_TEXT*) Struct )->m_Orient;
        Shape  = ( (SCH_TEXT*) Struct )->m_Shape;
        pX     = ( (SCH_TEXT*) Struct )->m_Pos.x;
        pY     = ( (SCH_TEXT*) Struct )->m_Pos.y;
        offset = TXTMARGE;
        if( Struct->Type() == TYPE_SCH_GLOBALLABEL
            || Struct->Type() == TYPE_SCH_HIERLABEL )
            offset += Size.x;       // We must draw the Glabel graphic symbol
        if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
            color = ReturnLayerColor( ( (SCH_TEXT*) Struct )->m_Layer );
        break;

    default:
        return;
    }

    if( Size.x == 0 )
        Size = wxSize( DEFAULT_SIZE_TEXT, DEFAULT_SIZE_TEXT );

    if ( Struct->Type() == TYPE_SCH_GLOBALLABEL )
    {
        offset =  ( (SCH_GLOBALLABEL*) Struct )->m_Width;
        switch( Shape )
        {
        case NET_INPUT:
        case NET_BIDI:
        case NET_TRISTATE:
            offset += Size.x/2;
            break;

        case NET_OUTPUT:
            offset += TXTMARGE;
            break;

        default:
            break;
        }
    }

	if( thickness == 0 )
		thickness = MAX( g_PlotPSMinimunLineWidth, g_DrawMinimunLineWidth );
    SetCurrentLineWidth( thickness );

    switch( Orient )
    {
    case 0:         /* Orientation horiz normale */
        if( Struct->Type() == TYPE_SCH_GLOBALLABEL || Struct->Type() == TYPE_SCH_HIERLABEL )
            PlotGraphicText( g_PlotFormat, wxPoint( pX - offset, pY ),
                             color, Text, TEXT_ORIENT_HORIZ, Size,
                             GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER,
                            thickness, italic );
        else
            PlotGraphicText( g_PlotFormat, wxPoint( pX, pY - offset ),
                             color, Text, TEXT_ORIENT_HORIZ, Size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM,
                            thickness, italic );
        break;

    case 1:         /* Orientation vert UP */
        if( Struct->Type() == TYPE_SCH_GLOBALLABEL || Struct->Type() == TYPE_SCH_HIERLABEL )
            PlotGraphicText( g_PlotFormat, wxPoint( pX, pY + offset ),
                             color, Text, TEXT_ORIENT_VERT, Size,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP,
                            thickness, italic );
        else
            PlotGraphicText( g_PlotFormat, wxPoint( pX - offset, pY ),
                             color, Text, TEXT_ORIENT_VERT, Size,
                             GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_BOTTOM,
                            thickness, italic );
        break;

    case 2:         /* Horiz Orientation - Right justified */
        if( Struct->Type() == TYPE_SCH_GLOBALLABEL || Struct->Type() == TYPE_SCH_HIERLABEL )
            PlotGraphicText( g_PlotFormat, wxPoint( pX + offset, pY ),
                             color, Text, TEXT_ORIENT_HORIZ, Size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                            thickness, italic );
        else
            PlotGraphicText( g_PlotFormat, wxPoint( pX, pY + offset ),
                             color, Text, TEXT_ORIENT_HORIZ, Size,
                             GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_BOTTOM,
                            thickness, italic );
        break;

    case 3:         /* Orientation vert BOTTOM */
        if( Struct->Type() == TYPE_SCH_GLOBALLABEL || Struct->Type() == TYPE_SCH_HIERLABEL )
            PlotGraphicText( g_PlotFormat, wxPoint( pX, pY - offset ),
                             color, Text, TEXT_ORIENT_VERT, Size,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM,
                            thickness, italic );
        else
            PlotGraphicText( g_PlotFormat, wxPoint( pX + offset, pY ),
                             color, Text, TEXT_ORIENT_VERT, Size,
                             GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_TOP,
                            thickness, italic );
        break;
    }

    /* Draw graphic symbol for global or hierachical labels */
    if( Struct->Type() == TYPE_SCH_GLOBALLABEL )
    {
        ( (SCH_GLOBALLABEL*) Struct )->CreateGraphicShape( Poly, wxPoint(pX, pY) );
        PlotPoly( Poly[0], Poly + 1, NOFILL );
    }
    if( Struct->Type() == TYPE_SCH_HIERLABEL )
    {
        ( (SCH_HIERLABEL*) Struct )->CreateGraphicShape( Poly, wxPoint(pX, pY) );
        PlotPoly( Poly[0], Poly + 1, NOFILL );
    }
}


/***********************************************************************/
static void PlotSheetLabelStruct( Hierarchical_PIN_Sheet_Struct* Struct )
/***********************************************************************/
/* Routine de dessin des Sheet Labels type hierarchie */
{
    EDA_Colors txtcolor = UNSPECIFIED_COLOR;
    int posx, tposx, posy, size, size2;
    int coord[16];

    if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        txtcolor = ReturnLayerColor( Struct->GetLayer() );

    posx = Struct->m_Pos.x; posy = Struct->m_Pos.y; size = Struct->m_Size.x;
    GRTextHorizJustifyType side;
    if( Struct->m_Edge )
    {
        tposx = posx - size;
        side  = GR_TEXT_HJUSTIFY_RIGHT;
    }
    else
    {
        tposx = posx + size + (size / 8);
        side  = GR_TEXT_HJUSTIFY_LEFT;
    }
    int thickness = Struct->m_Width;
	if( thickness == 0 )
		thickness = MAX( g_PlotPSMinimunLineWidth, g_DrawMinimunLineWidth );
    SetCurrentLineWidth( thickness );

    bool italic = Struct->m_Italic;
    PlotGraphicText( g_PlotFormat, wxPoint( tposx, posy ), txtcolor,
                     Struct->m_Text, TEXT_ORIENT_HORIZ, wxSize( size, size ),
                     side, GR_TEXT_VJUSTIFY_CENTER,
                    thickness, italic  );
    /* dessin du symbole de connexion */

    if( Struct->m_Edge )
        size = -size;
    coord[0] = posx; coord[1] = posy; size2 = size / 2;

    switch( Struct->m_Shape )
    {
    case 0:         /* input |> */
        coord[2]  = posx; coord[3] = posy - size2;
        coord[4]  = posx + size2; coord[5] = posy - size2;
        coord[6]  = posx + size; coord[7] = posy;
        coord[8]  = posx + size2; coord[9] = posy + size2;
        coord[10] = posx; coord[11] = posy + size2;
        coord[12] = posx; coord[13] = posy;
        PlotPoly( 7, coord, NOFILL );
        break;

    case 1:         /* output <| */
        coord[2]  = posx + size2; coord[3] = posy - size2;
        coord[4]  = posx + size; coord[5] = posy - size2;
        coord[6]  = posx + size; coord[7] = posy + size2;
        coord[8]  = posx + size2; coord[9] = posy + size2;
        coord[10] = posx; coord[11] = posy;
        PlotPoly( 6, coord, NOFILL );
        break;

    case 2:         /* bidi <> */
    case 3:         /* TriSt <> */
        coord[2] = posx + size2; coord[3] = posy - size2;
        coord[4] = posx + size; coord[5] = posy;
        coord[6] = posx + size2; coord[7] = posy + size2;
        coord[8] = posx; coord[9] = posy;
        PlotPoly( 5, coord, NOFILL );
        break;

    default:         /* unsp []*/
        coord[2]  = posx; coord[3] = posy - size2;
        coord[4]  = posx + size; coord[5] = posy - size2;
        coord[6]  = posx + size; coord[7] = posy + size2;
        coord[8]  = posx; coord[9] = posy + size2;
        coord[10] = posx; coord[11] = posy;
        PlotPoly( 6, coord, NOFILL );
        break;
    }
}


/*************************************************/
void PlotSheetStruct( DrawSheetStruct* Struct )
/*************************************************/
/* Routine de dessin du bloc type hierarchie */
{
    Hierarchical_PIN_Sheet_Struct* SheetLabelStruct;
    EDA_Colors txtcolor = UNSPECIFIED_COLOR;
    wxSize   size;
    wxString Text;
    wxPoint  pos;

    if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        SetColorMapPS( ReturnLayerColor( Struct->m_Layer ) );

    int thickness = MAX( g_PlotPSMinimunLineWidth, g_DrawMinimunLineWidth );
    SetCurrentLineWidth( thickness );

    Move_Plume( Struct->m_Pos, 'U' );
    pos = Struct->m_Pos; pos.x += Struct->m_Size.x;

    Move_Plume( pos, 'D' );
    pos.y += Struct->m_Size.y;

    Move_Plume( pos, 'D' );
    pos = Struct->m_Pos; pos.y += Struct->m_Size.y;

    Move_Plume( pos, 'D' );
    Move_Plume( Struct->m_Pos, 'D' );

    Plume( 'U' );

    /* Draw texts: SheetName */
    Text = Struct->m_SheetName;
    size = wxSize( Struct->m_SheetNameSize, Struct->m_SheetNameSize );
    pos  = Struct->m_Pos; pos.y -= 4;

    if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        SetColorMapPS( ReturnLayerColor( LAYER_SHEETNAME ) );

    bool italic = false;
    PlotGraphicText( g_PlotFormat, pos, txtcolor,
                     Text, TEXT_ORIENT_HORIZ, size,
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM,
                    thickness, italic );

    /*Draw texts : FileName */
    Text = Struct->GetFileName();
    size = wxSize( Struct->m_FileNameSize, Struct->m_FileNameSize );

    if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        SetColorMapPS( ReturnLayerColor( LAYER_SHEETFILENAME ) );

    PlotGraphicText( g_PlotFormat,
                     wxPoint( Struct->m_Pos.x, Struct->m_Pos.y + Struct->m_Size.y + 4 ),
                     txtcolor,
                     Text, TEXT_ORIENT_HORIZ, size,
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP,
                    thickness, italic );

    /* Draw texts : SheetLabel */
    SheetLabelStruct = Struct->m_Label;
    if( (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt )
        SetColorMapPS( ReturnLayerColor( Struct->m_Layer ) );

    while( SheetLabelStruct != NULL )
    {
        PlotSheetLabelStruct( SheetLabelStruct );
        SheetLabelStruct = SheetLabelStruct->Next();
    }
}
