/***************************************************/
/* Plot functions common to different plot formats */
/***************************************************/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "worksheet.h"
#include "base_struct.h"
#include "drawtxt.h"
#include "trigo.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "class_pin.h"


/* Local Variables : */
static void Plot_Hierarchical_PIN_Sheet( PLOTTER* plotter,
                                         SCH_SHEET_PIN* Struct );
static void PlotTextField( PLOTTER* plotter, SCH_COMPONENT* DrawLibItem,
                           int FieldNumber, bool IsMulti, int DrawMode );


static void PlotNoConnectStruct( PLOTTER* plotter, SCH_NO_CONNECT* Struct )
{
    int delta = Struct->m_Size.x / 2;
    int pX, pY;

    pX = Struct->m_Pos.x; pY = Struct->m_Pos.y;

    plotter->set_current_line_width( Struct->GetPenSize() );
    plotter->move_to( wxPoint( pX - delta, pY - delta ) );
    plotter->finish_to( wxPoint( pX + delta, pY + delta ) );
    plotter->move_to( wxPoint( pX + delta, pY - delta ) );
    plotter->finish_to( wxPoint( pX - delta, pY + delta ) );
}


static void PlotLibPart( PLOTTER* plotter, SCH_COMPONENT* DrawLibItem )
{
    LIB_COMPONENT* Entry;
    int            TransMat[2][2];

    Entry = CMP_LIBRARY::FindLibraryComponent( DrawLibItem->m_ChipName );

    if( Entry == NULL )
        return;;

    memcpy( TransMat, DrawLibItem->m_Transform, sizeof(TransMat) );

    Entry->Plot( plotter, DrawLibItem->m_Multi, DrawLibItem->m_Convert,
                 DrawLibItem->m_Pos, TransMat );
    bool isMulti = Entry->GetPartCount() > 1;
    for( int fieldId = 0; fieldId < NUMBER_OF_FIELDS; fieldId++ )
    {
        PlotTextField( plotter, DrawLibItem, fieldId, isMulti, 0 );
    }
}


/* Plot field text.
 * Input:
 * DrawLibItem: pointer to the component
 * FieldNumber: Number Field
 * IsMulti: true flag if there are several parts per package.
 * Only useful for the field to add a reference to this one
 * The identification from (A, B ...)
 * DrawMode: trace mode
 */
static void PlotTextField( PLOTTER* plotter, SCH_COMPONENT* DrawLibItem,
                           int FieldNumber, bool IsMulti, int DrawMode )
{
    SCH_FIELD* field = DrawLibItem->GetField( FieldNumber );
    EDA_Colors color = UNSPECIFIED_COLOR;

    color = ReturnLayerColor( field->GetLayer() );

    DrawMode = 0;   /* Unused */
    if( field->m_Attributs & TEXT_NO_VISIBLE )
        return;
    if( field->IsVoid() )
        return;

    /* Calculate the text orientation, according to the component
     * orientation/mirror */
    int orient = field->m_Orient;
    if( DrawLibItem->m_Transform[0][1] )  // Rotate component 90 deg.
    {
        if( orient == TEXT_ORIENT_HORIZ )
            orient = TEXT_ORIENT_VERT;
        else
            orient = TEXT_ORIENT_HORIZ;
    }

    /* Calculate the text justification, according to the component
     * orientation/mirror
     * this is a bit complicated due to cumulative calculations:
     * - numerous cases (mirrored or not, rotation)
     * - the DrawGraphicText function recalculate also H and H justifications
     *      according to the text orientation.
     * - When a component is mirrored, the text is not mirrored and
     *   justifications are complicated to calculate
     * so the more easily way is to use no justifications ( Centered text )
     * and use GetBoundaryBox to know the text coordinate considered as centered
     */
    EDA_Rect BoundaryBox = field->GetBoundaryBox();
    GRTextHorizJustifyType hjustify = GR_TEXT_HJUSTIFY_CENTER;
    GRTextVertJustifyType vjustify = GR_TEXT_VJUSTIFY_CENTER;
    wxPoint textpos = BoundaryBox.Centre();

    int thickness = field->GetPenSize();

    if( !IsMulti || (FieldNumber != REFERENCE) )
    {
        plotter->text( textpos, color, field->m_Text,
                       orient,
                       field->m_Size,
                       hjustify, vjustify,
                       thickness, field->m_Italic, field->m_Bold );
    }
    else    /* We plot the reference, for a multiple parts per package */
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
                       orient,
                       field->m_Size, hjustify, vjustify,
                       thickness, field->m_Italic, field->m_Bold );
    }
}


void PlotPinSymbol( PLOTTER* plotter, const wxPoint& pos,
                    int len, int orient, int Shape )
{
    int        MapX1, MapY1, x1, y1;
    EDA_Colors color = UNSPECIFIED_COLOR;

    color = ReturnLayerColor( LAYER_PIN );

    plotter->set_color( color );

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
            plotter->move_to( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2,
                                       y1 ) );
            plotter->line_to( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2,
                                       y1 - IEEE_SYMBOL_PIN_DIM ) );
            plotter->finish_to( wxPoint( x1, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            plotter->move_to( wxPoint( x1,
                                       y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ) );
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
            plotter->finish_to( wxPoint( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2,
                                         y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            plotter->move_to( wxPoint( x1 - IEEE_SYMBOL_PIN_DIM, y1 ) );
            plotter->finish_to( wxPoint( x1,
                                         y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 ) );
        }
    }
}


static void PlotTextStruct( PLOTTER* plotter, SCH_TEXT* aSchText )
{
    static std::vector <wxPoint> Poly;

    switch( aSchText->Type() )
    {
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_LABEL:
    case TYPE_SCH_TEXT:
        break;

    default:
        return;
    }

    EDA_Colors color = UNSPECIFIED_COLOR;
    color = ReturnLayerColor( aSchText->m_Layer );
    wxPoint    textpos   = aSchText->m_Pos + aSchText->GetSchematicTextOffset();
    int        thickness = aSchText->GetPenSize();

    plotter->set_current_line_width( thickness );

    if( aSchText->m_MultilineAllowed )
    {
        wxPoint        pos  = textpos;
        wxArrayString* list = wxStringSplit( aSchText->m_Text, '\n' );
        wxPoint        offset;

        offset.y = aSchText->GetInterline();

        RotatePoint( &offset, aSchText->m_Orient );
        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            plotter->text( pos, color, txt, aSchText->m_Orient,
                           aSchText->m_Size, aSchText->m_HJustify,
                           aSchText->m_VJustify, thickness,
                           aSchText->m_Italic, aSchText->m_Bold );
            pos += offset;
        }

        delete (list);
    }
    else
        plotter->text( textpos, color, aSchText->m_Text, aSchText->m_Orient,
                       aSchText->m_Size, aSchText->m_HJustify,
                       aSchText->m_VJustify, thickness, aSchText->m_Italic,
                       aSchText->m_Bold );

    /* Draw graphic symbol for global or hierarchical labels */
    if( aSchText->Type() == TYPE_SCH_GLOBALLABEL )
    {
        ( (SCH_GLOBALLABEL*) aSchText )->CreateGraphicShape( Poly,
                                                             aSchText->m_Pos );
        plotter->poly( Poly.size(), &Poly[0].x, NO_FILL );
    }
    if( aSchText->Type() == TYPE_SCH_HIERLABEL )
    {
        ( (SCH_HIERLABEL*) aSchText )->CreateGraphicShape( Poly,
                                                           aSchText->m_Pos );
        plotter->poly( Poly.size(), &Poly[0].x, NO_FILL );
    }
}


static void Plot_Hierarchical_PIN_Sheet( PLOTTER*       plotter,
                                         SCH_SHEET_PIN* aHierarchical_PIN )
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

    int thickness = aHierarchical_PIN->GetPenSize();
    plotter->set_current_line_width( thickness );

    plotter->text( wxPoint( tposx, posy ), txtcolor, aHierarchical_PIN->m_Text,
                   TEXT_ORIENT_HORIZ, wxSize( size, size ),
                   side, GR_TEXT_VJUSTIFY_CENTER, thickness,
                   aHierarchical_PIN->m_Italic, aHierarchical_PIN->m_Bold );

    /* Draw the associated graphic symbol */
    aHierarchical_PIN->CreateGraphicShape( Poly, aHierarchical_PIN->m_Pos );

    plotter->poly( Poly.size(), &Poly[0].x, NO_FILL );
}


static void PlotSheetStruct( PLOTTER* plotter, SCH_SHEET* Struct )
{
    SCH_SHEET_PIN* SheetLabelStruct;
    EDA_Colors txtcolor = UNSPECIFIED_COLOR;
    wxSize     size;
    wxString   Text;
    wxPoint    pos;

    plotter->set_color( ReturnLayerColor( Struct->m_Layer ) );

    int thickness = Struct->GetPenSize();
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

    plotter->text( wxPoint( Struct->m_Pos.x,
                            Struct->m_Pos.y + Struct->m_Size.y + 4 ),
                   txtcolor, Text, TEXT_ORIENT_HORIZ, size,
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


void PlotDrawlist( PLOTTER* plotter, SCH_ITEM* aDrawlist )
{
    while( aDrawlist )  /* Plot each item in draw list */
    {
        SCH_COMPONENT* DrawLibItem;
        int            layer;
        wxPoint        StartPos, EndPos;

        plotter->set_current_line_width( aDrawlist->GetPenSize() );
        switch( aDrawlist->Type() )
        {
        case DRAW_BUSENTRY_STRUCT_TYPE:
        case DRAW_SEGMENT_STRUCT_TYPE:
            if( aDrawlist->Type() == DRAW_BUSENTRY_STRUCT_TYPE )
            {
            #undef STRUCT
            #define STRUCT ( (SCH_BUS_ENTRY*) aDrawlist )
                StartPos = STRUCT->m_Pos;
                EndPos   = STRUCT->m_End();
                layer    = STRUCT->GetLayer();
                plotter->set_color( ReturnLayerColor( layer ) );
            }
            else
            {
            #undef STRUCT
            #define STRUCT ( (SCH_LINE*) aDrawlist )
                StartPos = STRUCT->m_Start;
                EndPos   = STRUCT->m_End;
                layer    = STRUCT->GetLayer();
                plotter->set_color( ReturnLayerColor( layer ) );
            }

            if( layer == LAYER_NOTES )
                plotter->set_dash( true );
            plotter->move_to( StartPos );
            plotter->finish_to( EndPos );
            if( layer == LAYER_NOTES )
                plotter->set_dash( false );

            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (SCH_JUNCTION*) aDrawlist )
            plotter->set_color( ReturnLayerColor( STRUCT->GetLayer() ) );
            plotter->circle( STRUCT->m_Pos, STRUCT->m_Size.x, FILLED_SHAPE );
            break;

        case TYPE_SCH_TEXT:
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
            PlotTextStruct( plotter, (SCH_TEXT*) aDrawlist );
            break;

        case TYPE_SCH_COMPONENT:
            DrawLibItem = (SCH_COMPONENT*) aDrawlist;
            PlotLibPart( plotter, DrawLibItem );
            break;

        case DRAW_POLYLINE_STRUCT_TYPE:
            break;

        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
            break;

        case TYPE_SCH_MARKER:
            break;

        case DRAW_SHEET_STRUCT_TYPE:
            PlotSheetStruct( plotter, (SCH_SHEET*) aDrawlist );
            break;

        case DRAW_NOCONNECT_STRUCT_TYPE:
            plotter->set_color( ReturnLayerColor( LAYER_NOCONNECT ) );
            PlotNoConnectStruct( plotter, (SCH_NO_CONNECT*) aDrawlist );
            break;

        default:
            break;
        }
        aDrawlist = aDrawlist->Next();
    }
}
