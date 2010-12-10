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

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "lib_pin.h"
#include "sch_items.h"
#include "sch_component.h"
#include "sch_sheet.h"
#include "sch_text.h"
#include "template_fieldnames.h"


/* Local functions : */
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
    TRANSFORM temp = TRANSFORM();

    Entry = CMP_LIBRARY::FindLibraryComponent( DrawLibItem->m_ChipName );

    if( Entry == NULL )
        return;;

    temp = DrawLibItem->m_Transform;

    Entry->Plot( plotter, DrawLibItem->m_Multi, DrawLibItem->m_Convert, DrawLibItem->m_Pos, temp );
    bool isMulti = Entry->GetPartCount() > 1;

    for( int fieldId = 0; fieldId < DrawLibItem->GetFieldCount(); fieldId++ )
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
    if( DrawLibItem->m_Transform.y1 )  // Rotate component 90 deg.
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
    EDA_Rect BoundaryBox = field->GetBoundingBox();
    GRTextHorizJustifyType hjustify = GR_TEXT_HJUSTIFY_CENTER;
    GRTextVertJustifyType vjustify  = GR_TEXT_VJUSTIFY_CENTER;
    wxPoint  textpos = BoundaryBox.Centre();

    int      thickness = field->GetPenSize();

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
        Text = field->m_Text + LIB_COMPONENT::ReturnSubReference( DrawLibItem->m_Multi );
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
    case SCH_SHEET_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_LABEL_T:
    case SCH_TEXT_T:
        break;

    default:
        return;
    }

    EDA_Colors color = UNSPECIFIED_COLOR;
    color = ReturnLayerColor( aSchText->GetLayer() );
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
    aSchText->CreateGraphicShape( Poly, aSchText->m_Pos );
    if( Poly.size() )
        plotter->poly( Poly.size(), &Poly[0].x, NO_FILL );
}


static void PlotSheetStruct( PLOTTER* plotter, SCH_SHEET* Struct )
{
    EDA_Colors txtcolor = UNSPECIFIED_COLOR;
    wxSize     size;
    wxString   Text;
    int        name_orientation;
    wxPoint    pos_sheetname, pos_filename;
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

    if( Struct->IsVerticalOrientation() )
    {
        pos_sheetname    = wxPoint( Struct->m_Pos.x - 8, Struct->m_Pos.y + Struct->m_Size.y );
        pos_filename     = wxPoint( Struct->m_Pos.x + Struct->m_Size.x + 4,
                                    Struct->m_Pos.y + Struct->m_Size.y );
        name_orientation = TEXT_ORIENT_VERT;
    }
    else
    {
        pos_sheetname    = wxPoint( Struct->m_Pos.x, Struct->m_Pos.y - 4 );
        pos_filename     = wxPoint( Struct->m_Pos.x, Struct->m_Pos.y + Struct->m_Size.y + 4 );
        name_orientation = TEXT_ORIENT_HORIZ;
    }
    /* Draw texts: SheetName */
    Text = Struct->m_SheetName;
    size = wxSize( Struct->m_SheetNameSize, Struct->m_SheetNameSize );

    //pos  = Struct->m_Pos; pos.y -= 4;
    thickness = g_DrawDefaultLineThickness;
    thickness = Clamp_Text_PenSize( thickness, size, false );

    plotter->set_color( ReturnLayerColor( LAYER_SHEETNAME ) );

    bool italic = false;
    plotter->text( pos_sheetname, txtcolor,
                   Text, name_orientation, size,
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_BOTTOM,
                   thickness, italic, false );

    /*Draw texts : FileName */
    Text = Struct->GetFileName();
    size = wxSize( Struct->m_FileNameSize, Struct->m_FileNameSize );
    thickness = g_DrawDefaultLineThickness;
    thickness = Clamp_Text_PenSize( thickness, size, false );

    plotter->set_color( ReturnLayerColor( LAYER_SHEETFILENAME ) );

    plotter->text( pos_filename, txtcolor,
                   Text, name_orientation, size,
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_TOP,
                   thickness, italic, false );

    plotter->set_color( ReturnLayerColor( Struct->m_Layer ) );

    /* Draw texts : SheetLabel */
    BOOST_FOREACH( SCH_SHEET_PIN & pin_sheet, Struct->GetSheetPins() ) {
        //pin_sheet.Plot( plotter );
        PlotTextStruct( plotter, &pin_sheet );
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
        case SCH_BUS_ENTRY_T:
        case SCH_LINE_T:
            if( aDrawlist->Type() == SCH_BUS_ENTRY_T )
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

        case SCH_JUNCTION_T:
            #undef STRUCT
            #define STRUCT ( (SCH_JUNCTION*) aDrawlist )
            plotter->set_color( ReturnLayerColor( STRUCT->GetLayer() ) );
            plotter->circle( STRUCT->m_Pos, STRUCT->m_Size.x, FILLED_SHAPE );
            break;

        case SCH_TEXT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            PlotTextStruct( plotter, (SCH_TEXT*) aDrawlist );
            break;

        case SCH_COMPONENT_T:
            DrawLibItem = (SCH_COMPONENT*) aDrawlist;
            PlotLibPart( plotter, DrawLibItem );
            break;

        case SCH_POLYLINE_T:
            break;

        case SCH_SHEET_LABEL_T:
            break;

        case SCH_MARKER_T:
            break;

        case SCH_SHEET_T:
            PlotSheetStruct( plotter, (SCH_SHEET*) aDrawlist );
            break;

        case SCH_NO_CONNECT_T:
            plotter->set_color( ReturnLayerColor( LAYER_NOCONNECT ) );
            PlotNoConnectStruct( plotter, (SCH_NO_CONNECT*) aDrawlist );
            break;

        default:
            break;
        }
        aDrawlist = aDrawlist->Next();
    }
}
