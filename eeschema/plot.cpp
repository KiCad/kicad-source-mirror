/***************************************************/
/* Plot functions common to different plot formats */
/***************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "plot_common.h"
#include "worksheet.h"
#include "base_struct.h"
#include "drawtxt.h"
#include "trigo.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"

/* Local Variables : */
static void Plot_Hierarchical_PIN_Sheet( Plotter*                       plotter,
                                         Hierarchical_PIN_Sheet_Struct* Struct );
static void PlotTextField( Plotter* plotter, SCH_COMPONENT* DrawLibItem,
                           int FieldNumber, int IsMulti, int DrawMode );
static void PlotPinSymbol( Plotter* plotter, const wxPoint& pos,
                           int len, int orient, int Shape );

/***/

/**********************************************************/
static void PlotNoConnectStruct( Plotter* plotter, DrawNoConnectStruct* Struct )
/**********************************************************/

/* Routine de dessin des symboles de "No Connexion" ..
 */
{
#define DELTA (DRAWNOCONNECT_SIZE / 2)
    int pX, pY;

    pX = Struct->m_Pos.x; pY = Struct->m_Pos.y;

    plotter->set_current_line_width( -1 );
    plotter->move_to( wxPoint( pX - DELTA, pY - DELTA ) );
    plotter->finish_to( wxPoint( pX + DELTA, pY + DELTA ) );
    plotter->move_to( wxPoint( pX + DELTA, pY - DELTA ) );
    plotter->finish_to( wxPoint( pX - DELTA, pY + DELTA ) );
}


/*************************************************/
static void PlotLibPart( Plotter* plotter, SCH_COMPONENT* DrawLibItem )
/*************************************************/
/* Polt a component */
{
    int        ii, t1, t2, * Poly, orient;
    EDA_LibComponentStruct* Entry;
    int        TransMat[2][2], Multi, convert;
    EDA_Colors CharColor = UNSPECIFIED_COLOR;
    wxPoint    pos;
    bool       draw_bgfill = false;

    Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;;
    memcpy( TransMat, DrawLibItem->m_Transform, sizeof(TransMat) );
    Multi   = DrawLibItem->m_Multi;
    convert = DrawLibItem->m_Convert;

    plotter->set_current_line_width( g_DrawDefaultLineThickness );
    for( LibEDA_BaseStruct* DEntry = Entry->m_Drawings;
        DEntry != NULL; DEntry = DEntry->Next() )
    {
        /* Elimination des elements non relatifs a l'unite */
        if( Multi && DEntry->m_Unit && (DEntry->m_Unit != Multi) )
            continue;
        if( convert && DEntry->m_Convert && (DEntry->m_Convert != convert) )
            continue;

        plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
        draw_bgfill = plotter->get_color_mode();

        switch( DEntry->Type() )
        {
        case COMPONENT_ARC_DRAW_TYPE:
        {
            LibDrawArc* Arc = (LibDrawArc*) DEntry;
            t1  = Arc->t1; t2 = Arc->t2;
            pos = TransformCoordinate( TransMat, Arc->m_Pos ) + DrawLibItem->m_Pos;
            MapAngles( &t1, &t2, TransMat );
            if( draw_bgfill && Arc->m_Fill == FILLED_WITH_BG_BODYCOLOR )
            {
                plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                plotter->arc( pos, -t1, -t2, Arc->m_Rayon, FILLED_SHAPE, 0 );
            }
            plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
            plotter->arc( pos,
                          -t1,
                          -t2,
                          Arc->m_Rayon,
                          Arc->m_Fill,
                          Arc->m_Width );
        }
        break;

        case COMPONENT_CIRCLE_DRAW_TYPE:
        {
            LibDrawCircle* Circle = (LibDrawCircle*) DEntry;
            pos = TransformCoordinate( TransMat, Circle->m_Pos ) + DrawLibItem->m_Pos;
            if( draw_bgfill && Circle->m_Fill == FILLED_WITH_BG_BODYCOLOR )
            {
                plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                plotter->circle( pos, Circle->m_Rayon * 2, FILLED_SHAPE, 0 );
            }
            plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
            plotter->circle( pos,
                             Circle->m_Rayon * 2,
                             Circle->m_Fill,
                             Circle->m_Width );
        }
        break;

        case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        {
            LibDrawText* Text = (LibDrawText*) DEntry;

            /* The text orientation may need to be flipped if the
             * transformation matrix causes xy axes to be flipped. */
            t1  = (TransMat[0][0] != 0) ^ (Text->m_Orient != 0);
            pos = TransformCoordinate( TransMat, Text->m_Pos ) + DrawLibItem->m_Pos;
            plotter->set_current_line_width( -1 );
            int thickness = (Text->m_Width == 0) ? g_DrawDefaultLineThickness : Text->m_Width;
            thickness = Clamp_Text_PenSize( thickness, Text->m_Size, Text->m_Bold );

            plotter->text( pos, CharColor,
                           Text->m_Text,
                           t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT,
                           Text->m_Size,
                           GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                           thickness, Text->m_Italic, Text->m_Bold );
        }
        break;

        case COMPONENT_RECT_DRAW_TYPE:
        {
            LibDrawSquare* Square = (LibDrawSquare*) DEntry;
            pos = TransformCoordinate( TransMat, Square->m_Pos ) + DrawLibItem->m_Pos;
            wxPoint        end =
                TransformCoordinate( TransMat, Square->m_End ) + DrawLibItem->m_Pos;

            if( draw_bgfill && Square->m_Fill == FILLED_WITH_BG_BODYCOLOR )
            {
                plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                plotter->rect( pos, end, FILLED_WITH_BG_BODYCOLOR, 0 );
            }
            plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
            plotter->rect( pos, end, Square->m_Fill, Square->m_Width );
        }
        break;

        case COMPONENT_PIN_DRAW_TYPE:     /* Trace des Pins */
        {
            LibDrawPin* Pin = (LibDrawPin*) DEntry;
            if( Pin->m_Attributs & PINNOTDRAW )
                break;

            /* Calcul de l'orientation reelle de la Pin */
            orient = Pin->ReturnPinDrawOrient( TransMat );
            /* compute Pin Pos */
            pos = TransformCoordinate( TransMat, Pin->m_Pos ) + DrawLibItem->m_Pos;

            /* Dessin de la pin et du symbole special associe */
            PlotPinSymbol( plotter, pos, Pin->m_PinLen, orient, Pin->m_PinShape );
            int thickness = g_DrawDefaultLineThickness;
            Pin->PlotPinTexts( plotter, pos, orient,
                               Entry->m_TextInside,
                               Entry->m_DrawPinNum, Entry->m_DrawPinName,
                               thickness );
        }
        break;

        case COMPONENT_POLYLINE_DRAW_TYPE:
        {
            LibDrawPolyline* polyline = (LibDrawPolyline*) DEntry;
            Poly = (int*) MyMalloc( sizeof(int) * 2 * polyline->GetCornerCount() );
            for( ii = 0; ii < (int) polyline->GetCornerCount(); ii++ )
            {
                pos = polyline->m_PolyPoints[ii];
                pos = TransformCoordinate( TransMat, pos ) + DrawLibItem->m_Pos;
                Poly[ii * 2]     = pos.x;
                Poly[ii * 2 + 1] = pos.y;
            }

            if( draw_bgfill && polyline->m_Fill == FILLED_WITH_BG_BODYCOLOR )
            {
                plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                plotter->poly( ii, Poly, FILLED_WITH_BG_BODYCOLOR, 0 );
            }
            plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
            plotter->poly( ii, Poly, polyline->m_Fill, polyline->m_Width );
            MyFree( Poly );
        }
        break;

        case COMPONENT_BEZIER_DRAW_TYPE:
        {
            LibDrawBezier* polyline = (LibDrawBezier*) DEntry;
            Poly = (int*) MyMalloc( sizeof(int) * 2 * polyline->GetCornerCount() );
            for( ii = 0; ii < (int) polyline->GetCornerCount(); ii++ )
            {
                pos = polyline->m_PolyPoints[ii];
                pos = TransformCoordinate( TransMat, pos ) + DrawLibItem->m_Pos;
                Poly[ii * 2]     = pos.x;
                Poly[ii * 2 + 1] = pos.y;
            }

            if( draw_bgfill && polyline->m_Fill == FILLED_WITH_BG_BODYCOLOR )
            {
                plotter->set_color( ReturnLayerColor( LAYER_DEVICE_BACKGROUND ) );
                plotter->poly( ii, Poly, FILLED_WITH_BG_BODYCOLOR, 0 );
            }
            plotter->set_color( ReturnLayerColor( LAYER_DEVICE ) );
            plotter->poly( ii, Poly, polyline->m_Fill, polyline->m_Width );
            MyFree( Poly );
        }

        default:
            D( printf( "Drawing Type=%d\n", DEntry->Type() ) );
        }

        /* Fin Switch */
    }

    /* Fin Boucle de dessin */

    /* Trace des champs, avec placement et orientation selon orient. du
     * composant
     * Si la reference commence par # elle n'est pas tracee
     */

    if( (Entry->m_Prefix.m_Attributs & TEXT_NO_VISIBLE) == 0 )
    {
        if( Entry->m_UnitCount > 1 )
            PlotTextField( plotter, DrawLibItem, REFERENCE, 1, 0 );
        else
            PlotTextField( plotter, DrawLibItem, REFERENCE, 0, 0 );
    }

    if( (Entry->m_Name.m_Attributs & TEXT_NO_VISIBLE) == 0 )
        PlotTextField( plotter, DrawLibItem, VALUE, 0, 0 );

    for( ii = 2; ii < NUMBER_OF_FIELDS; ii++ )
    {
        PlotTextField( plotter, DrawLibItem, ii, 0, 0 );
    }
}


/*************************************************************/
static void PlotTextField( Plotter* plotter, SCH_COMPONENT* DrawLibItem,
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
    wxPoint        textpos;  /* Position des textes */
    SCH_CMP_FIELD* field = DrawLibItem->GetField( FieldNumber );
    int            orient;
    EDA_Colors     color = UNSPECIFIED_COLOR;

    color = ReturnLayerColor( field->GetLayer() );

    DrawMode = 0;   /* Unused */
    if( field->m_Attributs & TEXT_NO_VISIBLE )
        return;
    if( field->IsVoid() )
        return;

    /* Calcul de la position des textes, selon orientation du composant */
    orient = field->m_Orient;
    GRTextHorizJustifyType hjustify = field->m_HJustify;
    GRTextVertJustifyType  vjustify = field->m_VJustify;
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
            switch( vjustify )
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
            switch( hjustify )
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
            switch( hjustify )
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
            switch( vjustify )
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

    int thickness = (field->m_Width == 0) ? g_DrawDefaultLineThickness : field->m_Width;
    thickness = Clamp_Text_PenSize( thickness, field->m_Size, field->m_Bold );
    plotter->set_current_line_width( thickness );

    if( !IsMulti || (FieldNumber != REFERENCE) )
    {
        plotter->text( textpos, color, field->m_Text,
                       orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                       field->m_Size,
                       hjustify, vjustify,
                       thickness, field->m_Italic, field->m_Bold );
    }
    else    /* We plt the reference, for a multiple parts per package */
    {
        /* Adding A, B ... to the reference */
        wxString Text;
        Text = field->m_Text;
        char     unit_id;
#if defined(KICAD_GOST)
        Text.Append( '.' );
        unit_id = '1' - 1 + DrawLibItem->m_Multi;
#else
        unit_id = 'A' - 1 + DrawLibItem->m_Multi;
#endif
        Text.Append( unit_id );
        plotter->text( textpos, color, Text,
                       orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                       field->m_Size, hjustify, vjustify,
                       thickness, field->m_Italic, field->m_Bold );
    }
}


/**************************************************************************/
static void PlotPinSymbol( Plotter* plotter, const wxPoint& pos,
                           int len, int orient, int Shape )
/**************************************************************************/

/* Trace la pin du symbole en cours de trace
 */
{
    int        MapX1, MapY1, x1, y1;
    EDA_Colors color = UNSPECIFIED_COLOR;

    color = ReturnLayerColor( LAYER_PIN );

    plotter->set_color( color );

    plotter->set_current_line_width( -1 );

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
        plotter->circle( wxPoint( MapX1 * INVERT_PIN_RADIUS + x1,
                                  MapY1 * INVERT_PIN_RADIUS + y1 ),
                         INVERT_PIN_RADIUS * 2, // diameter
                         NO_FILL,               // fill
                         -1 );                  // width

        plotter->move_to( wxPoint( MapX1 * INVERT_PIN_RADIUS * 2 + x1,
                                   MapY1 * INVERT_PIN_RADIUS * 2 + y1 ) );
        plotter->finish_to( pos );
    }
    else
    {
        plotter->move_to( wxPoint( x1, y1 ) );
        plotter->finish_to( pos );
    }

    if( Shape & CLOCK )
    {
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            plotter->move_to( wxPoint( x1, y1 + CLOCK_PIN_DIM ) );
            plotter->line_to( wxPoint( x1 - MapX1 * CLOCK_PIN_DIM, y1 ) );
            plotter->finish_to( wxPoint( x1, y1 - CLOCK_PIN_DIM ) );
        }
        else    /* MapX1 = 0 */
        {
            plotter->move_to( wxPoint( x1 + CLOCK_PIN_DIM, y1 ) );
            plotter->line_to( wxPoint( x1, y1 - MapY1 * CLOCK_PIN_DIM ) );
            plotter->finish_to( wxPoint( x1 - CLOCK_PIN_DIM, y1 ) );
        }
    }

    if( Shape & LOWLEVEL_IN )   /* IEEE symbol "Active Low Input" */
    {
        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            plotter->move_to( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2, y1 ) );
            plotter->line_to( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2,
                                       y1 - IEEE_SYMBOL_PIN_DIM ) );
            plotter->finish_to( wxPoint( x1, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            plotter->move_to( wxPoint( x1, y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ) );
            plotter->line_to( wxPoint( x1 - IEEE_SYMBOL_PIN_DIM,
                                       y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ) );
            plotter->finish_to( wxPoint( x1, y1 ) );
        }
    }


    if( Shape & LOWLEVEL_OUT )  /* IEEE symbol "Active Low Output" */
    {
        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            plotter->move_to( wxPoint( x1, y1 - IEEE_SYMBOL_PIN_DIM ) );
            plotter->finish_to( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            plotter->move_to( wxPoint( x1 - IEEE_SYMBOL_PIN_DIM, y1 ) );
            plotter->finish_to( wxPoint( x1, y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ) );
        }
    }
}


/*******************************************/
static void PlotTextStruct( Plotter* plotter, EDA_BaseStruct* Struct )
/*******************************************/

/*
 * Routine de trace des Textes, Labels et Global-Labels.
 * Les textes peuvent avoir 4 directions.
 */
{
    static std::vector <wxPoint> Poly;

    switch( Struct->Type() )
    {
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_LABEL:
    case TYPE_SCH_TEXT:
        break;

    default:
        return;
    }

    SCH_TEXT*  schText = (SCH_TEXT*) Struct;
    EDA_Colors color   = UNSPECIFIED_COLOR;
    color = ReturnLayerColor( schText->m_Layer );
    wxPoint    textpos   = schText->m_Pos + schText->GetSchematicTextOffset();
    int        thickness = (schText->m_Width == 0) ? g_DrawDefaultLineThickness : schText->m_Width;
    thickness = Clamp_Text_PenSize( thickness, schText->m_Size, schText->m_Bold );

    plotter->set_current_line_width( thickness );

    if( schText->m_MultilineAllowed )
    {
        wxPoint        pos  = textpos;
        wxArrayString* list = wxStringSplit( schText->m_Text, '\n' );
        wxPoint        offset;

        offset.y = schText->GetInterline();

        RotatePoint( &offset, schText->m_Orient );
        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            plotter->text( pos,
                           color, txt, schText->m_Orient, schText->m_Size,
                           schText->m_HJustify, schText->m_VJustify,
                           thickness, schText->m_Italic, schText->m_Bold );
            pos += offset;
        }

        delete (list);
    }
    else
        plotter->text( textpos,
                       color, schText->m_Text, schText->m_Orient, schText->m_Size,
                       schText->m_HJustify, schText->m_VJustify,
                       thickness, schText->m_Italic, schText->m_Bold );

    /* Draw graphic symbol for global or hierachical labels */
    if( Struct->Type() == TYPE_SCH_GLOBALLABEL )
    {
        ( (SCH_GLOBALLABEL*) Struct )->CreateGraphicShape( Poly, schText->m_Pos );
        plotter->poly( Poly.size(), &Poly[0].x, NO_FILL );
    }
    if( Struct->Type() == TYPE_SCH_HIERLABEL )
    {
        ( (SCH_HIERLABEL*) Struct )->CreateGraphicShape( Poly, schText->m_Pos );
        plotter->poly( Poly.size(), &Poly[0].x, NO_FILL );
    }
}


/*****************************************************************************************/
static void Plot_Hierarchical_PIN_Sheet( Plotter*                       plotter,
                                         Hierarchical_PIN_Sheet_Struct* aHierarchical_PIN )
/****************************************************************************************/

/* Plot a Hierarchical_PIN_Sheet
 */
{
    EDA_Colors txtcolor = UNSPECIFIED_COLOR;
    int        posx, tposx, posy, size;

    static std::vector <wxPoint> Poly;

    txtcolor = ReturnLayerColor( aHierarchical_PIN->GetLayer() );

    posx = aHierarchical_PIN->m_Pos.x;
    posy = aHierarchical_PIN->m_Pos.y;
    size = aHierarchical_PIN->m_Size.x;
    GRTextHorizJustifyType side;
    if( aHierarchical_PIN->m_Edge )
    {
        tposx = posx - size;
        side  = GR_TEXT_HJUSTIFY_RIGHT;
    }
    else
    {
        tposx = posx + size + (size / 8);
        side  = GR_TEXT_HJUSTIFY_LEFT;
    }

    int thickness =
        (aHierarchical_PIN->m_Width ==
         0) ? g_DrawDefaultLineThickness : aHierarchical_PIN->m_Width;
    thickness = Clamp_Text_PenSize( thickness,
                                    aHierarchical_PIN->m_Size,
                                    aHierarchical_PIN->m_Bold );
    plotter->set_current_line_width( thickness );

    plotter->text( wxPoint( tposx, posy ), txtcolor,
                   aHierarchical_PIN->m_Text, TEXT_ORIENT_HORIZ, wxSize( size, size ),
                   side, GR_TEXT_VJUSTIFY_CENTER,
                   thickness, aHierarchical_PIN->m_Italic, aHierarchical_PIN->m_Bold );

    /* Draw the associated graphic symbol */
    aHierarchical_PIN->CreateGraphicShape( Poly, aHierarchical_PIN->m_Pos );

    plotter->poly( Poly.size(), &Poly[0].x, NO_FILL );
}


/*************************************************/
static void PlotSheetStruct( Plotter* plotter, DrawSheetStruct* Struct )
/*************************************************/
/* Routine de dessin du bloc type hierarchie */
{
    Hierarchical_PIN_Sheet_Struct* SheetLabelStruct;
    EDA_Colors txtcolor = UNSPECIFIED_COLOR;
    wxSize     size;
    wxString   Text;
    wxPoint    pos;

    plotter->set_color( ReturnLayerColor( Struct->m_Layer ) );

    int thickness = g_DrawDefaultLineThickness;
    plotter->set_current_line_width( thickness );

    plotter->move_to( Struct->m_Pos );
    pos = Struct->m_Pos; pos.x += Struct->m_Size.x;

    plotter->line_to( pos );
    pos.y += Struct->m_Size.y;

    plotter->line_to( pos );
    pos = Struct->m_Pos; pos.y += Struct->m_Size.y;

    plotter->line_to( pos );
    plotter->finish_to( Struct->m_Pos );

    /* Draw texts: SheetName */
    Text = Struct->m_SheetName;
    size = wxSize( Struct->m_SheetNameSize, Struct->m_SheetNameSize );
    pos  = Struct->m_Pos; pos.y -= 4;
    thickness = g_DrawDefaultLineThickness;
    thickness = Clamp_Text_PenSize( thickness, size, false );

    plotter->set_color( ReturnLayerColor( LAYER_SHEETNAME ) );

    bool italic = false;
    plotter->text( pos, txtcolor,
                   Text, TEXT_ORIENT_HORIZ, size,
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM,
                   thickness, italic, false );

    /*Draw texts : FileName */
    Text = Struct->GetFileName();
    size = wxSize( Struct->m_FileNameSize, Struct->m_FileNameSize );
    thickness = g_DrawDefaultLineThickness;
    thickness = Clamp_Text_PenSize( thickness, size, false );

    plotter->set_color( ReturnLayerColor( LAYER_SHEETFILENAME ) );

    plotter->text( wxPoint( Struct->m_Pos.x, Struct->m_Pos.y + Struct->m_Size.y + 4 ),
                   txtcolor,
                   Text, TEXT_ORIENT_HORIZ, size,
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP,
                   thickness, italic, false );

    /* Draw texts : SheetLabel */
    SheetLabelStruct = Struct->m_Label;
    plotter->set_color( ReturnLayerColor( Struct->m_Layer ) );

    while( SheetLabelStruct != NULL )
    {
        Plot_Hierarchical_PIN_Sheet( plotter, SheetLabelStruct );
        SheetLabelStruct = SheetLabelStruct->Next();
    }
}


/*************************************************/
void PlotDrawlist( Plotter* plotter, SCH_ITEM* drawlist )
/*************************************************/
{
    while( drawlist )  /* tracage */
    {
        SCH_COMPONENT* DrawLibItem;
        int            layer;
        wxPoint        StartPos, EndPos;

        switch( drawlist->Type() )
        {
        case DRAW_BUSENTRY_STRUCT_TYPE:             /* Struct Raccord et Segment sont identiques */
        case DRAW_SEGMENT_STRUCT_TYPE:
            if( drawlist->Type() == DRAW_BUSENTRY_STRUCT_TYPE )
            {
            #undef STRUCT
            #define STRUCT ( (DrawBusEntryStruct*) drawlist )
                StartPos = STRUCT->m_Pos;
                EndPos   = STRUCT->m_End();
                layer    = STRUCT->GetLayer();
                plotter->set_color( ReturnLayerColor( layer ) );
            }
            else
            {
            #undef STRUCT
            #define STRUCT ( (EDA_DrawLineStruct*) drawlist )
                StartPos = STRUCT->m_Start;
                EndPos   = STRUCT->m_End;
                layer    = STRUCT->GetLayer();
                plotter->set_color( ReturnLayerColor( layer ) );
            }

            switch( layer )
            {
            case LAYER_NOTES:         /* Trace en pointilles */
                plotter->set_current_line_width( g_DrawDefaultLineThickness );
                plotter->set_dash( true );
                plotter->move_to( StartPos );
                plotter->finish_to( EndPos );
                plotter->set_dash( false );
                break;

            case LAYER_BUS:         /* Trait large */
            {
                int thickness = wxRound( g_DrawDefaultLineThickness * 2 );
                if( thickness < 3 )
                    thickness = 3;
                /* We NEED it to be thick, even on HPGL */
                plotter->thick_segment( StartPos, EndPos, thickness, FILLED );
                plotter->set_current_line_width( g_DrawDefaultLineThickness );
            }
            break;

            default:
                plotter->set_current_line_width( g_DrawDefaultLineThickness );
                plotter->move_to( StartPos );
                plotter->finish_to( EndPos );
                break;
            }

            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawJunctionStruct*) drawlist )
            plotter->set_color( ReturnLayerColor( STRUCT->GetLayer() ) );
            plotter->circle( STRUCT->m_Pos, DRAWJUNCTION_SIZE, FILLED_SHAPE );
            break;

        case TYPE_SCH_TEXT:
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
            PlotTextStruct( plotter, drawlist );
            break;

        case TYPE_SCH_COMPONENT:
            DrawLibItem = (SCH_COMPONENT*) drawlist;
            PlotLibPart( plotter, DrawLibItem );
            break;

        case DRAW_PICK_ITEM_STRUCT_TYPE:
            break;

        case DRAW_POLYLINE_STRUCT_TYPE:
            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            break;

        case DRAW_MARKER_STRUCT_TYPE:
            break;

        case DRAW_SHEET_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawSheetStruct*) drawlist )
            PlotSheetStruct( plotter, STRUCT );
            break;

        case DRAW_NOCONNECT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (DrawNoConnectStruct*) drawlist )
            plotter->set_color( ReturnLayerColor( LAYER_NOCONNECT ) );
            PlotNoConnectStruct( plotter, STRUCT );
            break;

        default:
            break;
        }
        drawlist = drawlist->Next();
    }
}
