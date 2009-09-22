/*****************/
/* class_pin.cpp */
/*****************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "trigo.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"
#include "plot_common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"


const wxChar* MsgPinElectricType[] =
{
    wxT( "input" ),
    wxT( "output" ),
    wxT( "BiDi" ),
    wxT( "3state" ),
    wxT( "passive" ),
    wxT( "unspc" ),
    wxT( "power_in" ),
    wxT( "power_out" ),
    wxT( "openCol" ),
    wxT( "openEm" ),
    wxT( "?????" )
};

LibDrawPin::LibDrawPin(LIB_COMPONENT * aParent) :
    LibEDA_BaseStruct( COMPONENT_PIN_DRAW_TYPE, aParent )
{
    m_PinLen      = 300;              /* default Pin len */
    m_Orient      = PIN_RIGHT;        /* Pin oprient: Up, Down, Left, Right */
    m_PinShape    = NONE;             /* Bit a bit: Pin shape (voir enum prec) */
    m_PinType     = PIN_UNSPECIFIED;  /* electrical type of pin */
    m_Attributs   = 0;                /* bit 0 != 0: pin invisible */
    m_PinNum      = 0;                /* pin number ( i.e. 4 codes ASCII ) */
    m_PinNumSize  = 50;
    m_PinNameSize = 50;               /* Default size for pin name and num */
    m_Width    = 0;
    m_typeName = _( "Pin" );
    m_PinNumShapeOpt     = 0;
    m_PinNameShapeOpt    = 0;
    m_PinNumPositionOpt  = 0;
    m_PinNamePositionOpt = 0;
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPos A wxPoint to test
 * @return bool - true if a hit, else false
 */
bool LibDrawPin::HitTest( const wxPoint& aRefPos )
{
    int mindist = m_Width ? m_Width / 2 : g_DrawDefaultLineThickness / 2;

    // Have a minimal tolerance for hit test
    if( mindist < 3 )
        mindist = 3;        // = 3 mils

    return HitTest( aRefPos, mindist, DefaultTransformMatrix );
}

/** Function HitTest
 * @return true if the point aPosRef is near a pin
 * @param aRefPos = a wxPoint to test
 * @param aThreshold = max distance to a segment
 * @param aTransMat = the transform matrix
 */
bool LibDrawPin::HitTest( wxPoint aRefPos, int aThreshold,
                          const int aTransMat[2][2] )
{
    wxPoint pinPos = TransformCoordinate( aTransMat, m_Pos );
    wxPoint pinEnd = TransformCoordinate( aTransMat, ReturnPinEndPoint() );

    return TestSegmentHit( aRefPos, pinPos, pinEnd, aThreshold );
}


bool LibDrawPin::Save( FILE* ExportFile ) const
{
    wxString StringPinNum;
    int      Etype;

    switch( m_PinType )
    {
    default:
    case PIN_INPUT:
        Etype = 'I';
        break;

    case PIN_OUTPUT:
        Etype = 'O';
        break;

    case PIN_BIDI:
        Etype = 'B';
        break;

    case PIN_TRISTATE:
        Etype = 'T';
        break;

    case PIN_PASSIVE:
        Etype = 'P';
        break;

    case PIN_UNSPECIFIED:
        Etype = 'U';
        break;

    case PIN_POWER_IN:
        Etype = 'W';
        break;

    case PIN_POWER_OUT:
        Etype = 'w';
        break;

    case PIN_OPENCOLLECTOR:
        Etype = 'C';
        break;

    case PIN_OPENEMITTER:
        Etype = 'E';
        break;
    }

    ReturnPinStringNum( StringPinNum );
    if( StringPinNum.IsEmpty() )
        StringPinNum = wxT( "~" );

    if( !m_PinName.IsEmpty() )
    {
        if( fprintf( ExportFile, "X %s", CONV_TO_UTF8( m_PinName ) ) < 0 )
            return false;
    }
    else
    {
        if( fprintf( ExportFile, "X ~" ) < 0 )
            return false;
    }

    if( fprintf( ExportFile, " %s %d %d %d %c %d %d %d %d %c",
                 CONV_TO_UTF8( StringPinNum ), m_Pos.x, m_Pos.y,
                 (int) m_PinLen, (int) m_Orient, m_PinNumSize, m_PinNameSize,
                 m_Unit, m_Convert, Etype ) < 0 )
        return false;

    if( ( m_PinShape ) || ( m_Attributs & PINNOTDRAW ) )
    {
        if( fprintf( ExportFile, " " ) < 0 )
            return false;
    }
    if( m_Attributs & PINNOTDRAW
        && fprintf( ExportFile, "N" ) < 0 )
        return false;
    if( m_PinShape & INVERT
        && fprintf( ExportFile, "I" ) < 0 )
        return false;
    if( m_PinShape & CLOCK
        && fprintf( ExportFile, "C" ) < 0 )
        return false;
    if( m_PinShape & LOWLEVEL_IN
        && fprintf( ExportFile, "L" ) < 0 )
        return false;
    if( m_PinShape & LOWLEVEL_OUT
        && fprintf( ExportFile, "V" ) < 0 )
        return false;

    if( fprintf( ExportFile, "\n" ) < 0 )
        return false;

    return true;
}


bool LibDrawPin::Load( char* line, wxString& errorMsg )
{
    int  i, j;
    char pinAttrs[64];
    char pinName[256];
    char pinNum[64];
    char pinOrient[64];
    char pinType[64];

    *pinAttrs = 0;

    i = sscanf( line + 2, "%s %s %d %d %d %s %d %d %d %d %s %s", pinName,
                pinNum, &m_Pos.x, &m_Pos.y, &m_PinLen, pinOrient, &m_PinNumSize,
                &m_PinNameSize, &m_Unit, &m_Convert, pinType, pinAttrs );

    if( i < 11 )
    {
        errorMsg.Printf( wxT( "pin only had %d parameters of the required 11 or 12" ), i );
        return false;
    }

    m_Orient = pinOrient[0] & 255;
    strncpy( (char*) &m_PinNum, pinNum, 4 );
    m_PinName = CONV_FROM_UTF8( pinName );

    switch( *pinType & 255 )
    {
    case 'I':
        m_PinType = PIN_INPUT;
        break;

    case 'O':
        m_PinType = PIN_OUTPUT;
        break;

    case 'B':
        m_PinType = PIN_BIDI;
        break;

    case 'T':
        m_PinType = PIN_TRISTATE;
        break;

    case 'P':
        m_PinType = PIN_PASSIVE;
        break;

    case 'U':
        m_PinType = PIN_UNSPECIFIED;
        break;

    case 'W':
        m_PinType = PIN_POWER_IN;
        break;

    case 'w':
        m_PinType = PIN_POWER_OUT;
        break;

    case 'C':
        m_PinType = PIN_OPENCOLLECTOR;
        break;

    case 'E':
        m_PinType = PIN_OPENEMITTER;
        break;

    default:
        errorMsg.Printf( wxT( "unknown pin type [%c]" ), *pinType & 255 );
        return false;
    }

    if( i == 12 )       /* Special Symbol defined */
    {
        for( j = strlen( pinAttrs ); j > 0; )
        {
            switch( pinAttrs[--j] )
            {
            case '~':
                break;

            case 'N':
                m_Attributs |= PINNOTDRAW;
                break;

            case 'I':
                m_PinShape |= INVERT;
                break;

            case 'C':
                m_PinShape |= CLOCK;
                break;

            case 'L':
                m_PinShape |= LOWLEVEL_IN;
                break;

            case 'V':
                m_PinShape |= LOWLEVEL_OUT;
                break;

            default:
                errorMsg.Printf( wxT( "unknown pin attribute [%c]" ),
                                 pinAttrs[j] );
                return false;
            }
        }
    }

    return true;
}


/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawPin::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}


void LibDrawPin::Draw( WinEDA_DrawPanel* aPanel,
                       wxDC*             aDC,
                       const wxPoint&    aOffset,
                       int               aColor,
                       int               aDrawMode,
                       void*             aData,
                       const int         aTransformMatrix[2][2] )
{
    // Invisible pins are only drawn on request.  In libedit they are drawn
    // in g_InvisibleItemColor because we must see them.
    WinEDA_SchematicFrame* frame =
        (WinEDA_SchematicFrame*) wxGetApp().GetTopWindow();

    if( ( m_Attributs & PINNOTDRAW ) )
    {
        if( frame->m_LibeditFrame && frame->m_LibeditFrame->IsActive() )
            aColor = g_InvisibleItemColor;
        else if( !frame->m_ShowAllPins )
            return;
    }

    LIB_COMPONENT* Entry = GetParent();
    bool DrawPinText = true;

    if( ( aData != NULL ) && ( (bool*) aData == false ) )
        DrawPinText = false;

    /* Calculate pin orient taking in account the component orientation. */
    int orient = ReturnPinDrawOrient( aTransformMatrix );

    /* Calculate the pin position */
    wxPoint pos1 = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;

    /* Dessin de la pin et du symbole special associe */
    DrawPinSymbol( aPanel, aDC, pos1, orient, aDrawMode, aColor );

    if( DrawPinText )
    {
        DrawPinTexts( aPanel, aDC, pos1, orient, Entry->m_TextInside,
                      Entry->m_DrawPinNum, Entry->m_DrawPinName,
                      aColor, aDrawMode );
    }
}


/** Function DrawPinSymbol
 * Draw the pin symbol (without texts)
 *  if Color != 0 draw with Color, else with the normal pin color
 */
void LibDrawPin::DrawPinSymbol( WinEDA_DrawPanel* aPanel,
                                wxDC*             aDC,
                                const wxPoint&    aPinPos,
                                int               aOrient,
                                int               aDrawMode,
                                int               aColor )
{
    int          MapX1, MapY1, x1, y1;
    int          color;
    int          width  = GetPenSize( );
    int          posX   = aPinPos.x, posY = aPinPos.y, len = m_PinLen;
    BASE_SCREEN* screen = aPanel->GetScreen();


    color = ReturnLayerColor( LAYER_PIN );
    if( aColor < 0 )       // Used normal color or selected color
    {
        if( (m_Selected & IS_SELECTED) )
            color = g_ItemSelectetColor;
    }
    else
        color = aColor;

    GRSetDrawMode( aDC, aDrawMode );

    MapX1 = MapY1 = 0; x1 = posX; y1 = posY;

    switch( aOrient )
    {
    case PIN_UP:
        y1 = posY - len; MapY1 = 1;
        break;

    case PIN_DOWN:
        y1 = posY + len; MapY1 = -1;
        break;

    case PIN_LEFT:
        x1 = posX - len, MapX1 = 1;
        break;

    case PIN_RIGHT:
        x1 = posX + len; MapX1 = -1;
        break;
    }

    if( m_PinShape & INVERT )
    {
        GRCircle( &aPanel->m_ClipBox, aDC, MapX1 * INVERT_PIN_RADIUS + x1,
                  MapY1 * INVERT_PIN_RADIUS + y1,
                  INVERT_PIN_RADIUS, width, color );

        GRMoveTo( MapX1 * INVERT_PIN_RADIUS * 2 + x1,
                  MapY1 * INVERT_PIN_RADIUS * 2 + y1 );
        GRLineTo( &aPanel->m_ClipBox, aDC, posX, posY, width, color );
    }
    else
    {
        GRMoveTo( x1, y1 );
        GRLineTo( &aPanel->m_ClipBox, aDC, posX, posY, width, color );
    }

    if( m_PinShape & CLOCK )
    {
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 + CLOCK_PIN_DIM );
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      x1 - MapX1 * CLOCK_PIN_DIM,
                      y1,
                      width,
                      color );
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      x1,
                      y1 - CLOCK_PIN_DIM,
                      width,
                      color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 + CLOCK_PIN_DIM, y1 );
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      x1,
                      y1 - MapY1 * CLOCK_PIN_DIM,
                      width,
                      color );
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      x1 - CLOCK_PIN_DIM,
                      y1,
                      width,
                      color );
        }
    }

    if( m_PinShape & LOWLEVEL_IN )  /* IEEE symbol "Active Low Input" */
    {
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2, y1 );
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2,
                      y1 - IEEE_SYMBOL_PIN_DIM,
                      width,
                      color );
            GRLineTo( &aPanel->m_ClipBox, aDC, x1, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1, y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2 );
            GRLineTo( &aPanel->m_ClipBox, aDC, x1 - IEEE_SYMBOL_PIN_DIM,
                      y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2, width, color );
            GRLineTo( &aPanel->m_ClipBox, aDC, x1, y1, width, color );
        }
    }


    if( m_PinShape & LOWLEVEL_OUT ) /* IEEE symbol "Active Low Output" */
    {
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 - IEEE_SYMBOL_PIN_DIM );
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      x1 + MapX1 * IEEE_SYMBOL_PIN_DIM * 2,
                      y1,
                      width,
                      color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 - IEEE_SYMBOL_PIN_DIM, y1 );
            GRLineTo( &aPanel->m_ClipBox,
                      aDC,
                      x1,
                      y1 + MapY1 * IEEE_SYMBOL_PIN_DIM * 2,
                      width,
                      color );
        }
    }

    /* Draw the pin end target (active end of the pin)
     * Draw but do not print the pin end target 1 pixel width
     */
    if( !screen->m_IsPrinting )
        GRCircle( &aPanel->m_ClipBox,
                  aDC,
                  posX,
                  posY,
                  TARGET_PIN_DIAM,
                  0,
                  color );
}


/*****************************************************************************
*  Put out pin number and pin text info, given the pin line coordinates.
*  The line must be vertical or horizontal.
*  If PinText == NULL nothing is printed. If PinNum = 0 no number is printed.
*  Current Zoom factor is taken into account.
*  If TextInside then the text is been put inside,otherwise all is drawn outside.
*  Pin Name:    substring beteween '~' is negated
*****************************************************************************/
void LibDrawPin::DrawPinTexts( WinEDA_DrawPanel* panel,
                               wxDC*             DC,
                               wxPoint&          pin_pos,
                               int               orient,
                               int               TextInside,
                               bool              DrawPinNum,
                               bool              DrawPinName,
                               int               Color,
                               int               DrawMode )
/* DrawMode = GR_OR, XOR ... */
{
    int        x, y, x1, y1;
    wxString   StringPinNum;
    EDA_Colors NameColor, NumColor;

    wxSize     PinNameSize( m_PinNameSize, m_PinNameSize );
    wxSize     PinNumSize( m_PinNumSize, m_PinNumSize );

    int        nameLineWidth = GetPenSize( );

    nameLineWidth = Clamp_Text_PenSize( nameLineWidth, m_PinNameSize, false );
    int        numLineWidth = GetPenSize( );
    numLineWidth = Clamp_Text_PenSize( numLineWidth, m_PinNumSize, false );

    GRSetDrawMode( DC, DrawMode );

    /* Get the num and name colors */
    if( (Color < 0) && (m_Selected & IS_SELECTED) )
        Color = g_ItemSelectetColor;
    NameColor = (EDA_Colors) ( Color == -1 ? ReturnLayerColor( LAYER_PINNAM ) : Color );
    NumColor  = (EDA_Colors) ( Color == -1 ? ReturnLayerColor( LAYER_PINNUM ) : Color );

    /* Create the pin num string */
    ReturnPinStringNum( StringPinNum );

    x1 = pin_pos.x; y1 = pin_pos.y;

    switch( orient )
    {
    case PIN_UP:
        y1 -= m_PinLen; break;

    case PIN_DOWN:
        y1 += m_PinLen; break;

    case PIN_LEFT:
        x1 -= m_PinLen; break;

    case PIN_RIGHT:
        x1 += m_PinLen; break;
    }

    if( m_PinName.IsEmpty() )
        DrawPinName = FALSE;

    if( TextInside )  /* Draw the text inside, but the pin numbers outside. */
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
        {
            // It is an horizontal line
            if( DrawPinName )
            {
                if( orient == PIN_RIGHT )
                {
                    x = x1 + TextInside;
                    DrawGraphicText( panel, DC, wxPoint( x, y1 ), NameColor,
                                     m_PinName,
                                     TEXT_ORIENT_HORIZ,
                                     PinNameSize,
                                     GR_TEXT_HJUSTIFY_LEFT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );
                }
                else    // Orient == PIN_LEFT
                {
                    x = x1 - TextInside;
                    DrawGraphicText( panel, DC, wxPoint( x, y1 ), NameColor,
                                     m_PinName,
                                     TEXT_ORIENT_HORIZ,
                                     PinNameSize,
                                     GR_TEXT_HJUSTIFY_RIGHT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );
                }
            }

            if( DrawPinNum )
            {
                DrawGraphicText( panel, DC,
                                 wxPoint( (x1 + pin_pos.x) / 2,
                                          y1 - TXTMARGE ), NumColor,
                                 StringPinNum,
                                 TEXT_ORIENT_HORIZ, PinNumSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_BOTTOM, numLineWidth,
                                 false, false );
            }
        }
        else            /* Its a vertical line. */
        {
            // Text is drawn from bottom to top (i.e. to negative value for Y axis)
            if( orient == PIN_DOWN )
            {
                y = y1 + TextInside;

                if( DrawPinName )
                    DrawGraphicText( panel, DC, wxPoint( x1, y ), NameColor,
                                     m_PinName,
                                     TEXT_ORIENT_VERT, PinNameSize,
                                     GR_TEXT_HJUSTIFY_RIGHT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );
                if( DrawPinNum )
                    DrawGraphicText( panel, DC,
                                     wxPoint( x1 - TXTMARGE,
                                              (y1 + pin_pos.y) / 2 ), NumColor,
                                     StringPinNum,
                                     TEXT_ORIENT_VERT, PinNumSize,
                                     GR_TEXT_HJUSTIFY_CENTER,
                                     GR_TEXT_VJUSTIFY_BOTTOM, numLineWidth,
                                     false, false );
            }
            else        /* PIN_UP */
            {
                y = y1 - TextInside;

                if( DrawPinName )
                    DrawGraphicText( panel, DC, wxPoint( x1, y ), NameColor,
                                     m_PinName,
                                     TEXT_ORIENT_VERT, PinNameSize,
                                     GR_TEXT_HJUSTIFY_LEFT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );
                if( DrawPinNum )
                    DrawGraphicText( panel, DC,
                                     wxPoint( x1 - TXTMARGE,
                                              (y1 + pin_pos.y) / 2 ), NumColor,
                                     StringPinNum,
                                     TEXT_ORIENT_VERT, PinNumSize,
                                     GR_TEXT_HJUSTIFY_CENTER,
                                     GR_TEXT_VJUSTIFY_BOTTOM, numLineWidth,
                                     false, false );
            }
        }
    }
    else     /**** Draw num & text pin outside  ****/
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
        {
            /* Its an horizontal line. */
            if( DrawPinName )
            {
                x = (x1 + pin_pos.x) / 2;
                DrawGraphicText( panel, DC, wxPoint( x, y1 - TXTMARGE ),
                                 NameColor, m_PinName,
                                 TEXT_ORIENT_HORIZ, PinNameSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_BOTTOM, nameLineWidth,
                                 false, false );
            }
            if( DrawPinNum )
            {
                x = (x1 + pin_pos.x) / 2;
                DrawGraphicText( panel, DC, wxPoint( x, y1 + TXTMARGE ),
                                 NumColor, StringPinNum,
                                 TEXT_ORIENT_HORIZ, PinNumSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_TOP, numLineWidth,
                                 false, false );
            }
        }
        else     /* Its a vertical line. */
        {
            if( DrawPinName )
            {
                y = (y1 + pin_pos.y) / 2;
                DrawGraphicText( panel, DC, wxPoint( x1 - TXTMARGE, y ),
                                 NameColor, m_PinName,
                                 TEXT_ORIENT_VERT, PinNameSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_BOTTOM, nameLineWidth,
                                 false, false );
            }

            if( DrawPinNum )
            {
                DrawGraphicText( panel, DC,
                                 wxPoint( x1 + TXTMARGE, (y1 + pin_pos.y) / 2 ),
                                 NumColor, StringPinNum,
                                 TEXT_ORIENT_VERT, PinNumSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_TOP, numLineWidth,
                                 false, false );
            }
        }
    }
}


/*****************************************************************************
* Plot pin number and pin text info, given the pin line coordinates.      *
* Same as DrawPinTexts((), but output is the plotter
* The line must be vertical or horizontal.                        *
* If PinNext == NULL nothing is printed.                                    *
* Current Zoom factor is taken into account.                     *
* If TextInside then the text is been put inside (moving from x1, y1 in      *
* the opposite direction to x2,y2), otherwise all is drawn outside.      *
*****************************************************************************/
void LibDrawPin::PlotPinTexts( PLOTTER *plotter,
                               wxPoint& pin_pos,
                               int      orient,
                               int      TextInside,
                               bool     DrawPinNum,
                               bool     DrawPinName,
                               int      aWidth )
{
    int        x, y, x1, y1;
    wxString   StringPinNum;
    EDA_Colors NameColor, NumColor;
    wxSize     PinNameSize = wxSize( m_PinNameSize, m_PinNameSize );
    wxSize     PinNumSize  = wxSize( m_PinNumSize, m_PinNumSize );

    /* Get the num and name colors */
    NameColor = ReturnLayerColor( LAYER_PINNAM );
    NumColor  = ReturnLayerColor( LAYER_PINNUM );

    /* Create the pin num string */
    ReturnPinStringNum( StringPinNum );
    x1 = pin_pos.x; y1 = pin_pos.y;

    switch( orient )
    {
    case PIN_UP:
        y1 -= m_PinLen; break;

    case PIN_DOWN:
        y1 += m_PinLen; break;

    case PIN_LEFT:
        x1 -= m_PinLen; break;

    case PIN_RIGHT:
        x1 += m_PinLen; break;
    }

    if( m_PinName.IsEmpty() )
        DrawPinName = FALSE;

    if( TextInside )                                        /* Draw the text inside, but the pin numbers outside. */
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) ) /* Its an horizontal line. */
        {
            if( DrawPinName )
            {
                if( orient == PIN_RIGHT )
                {
                    x = x1 + TextInside;
                    plotter->text( wxPoint( x, y1 ), NameColor,
                                   m_PinName,
                                   TEXT_ORIENT_HORIZ,
                                   PinNameSize,
                                   GR_TEXT_HJUSTIFY_LEFT,
                                   GR_TEXT_VJUSTIFY_CENTER,
                                   aWidth, false, false );
                }
                else    // orient == PIN_LEFT
                {
                    x = x1 - TextInside;
                    if( DrawPinName )
                        plotter->text( wxPoint( x, y1 ),
                                       NameColor, m_PinName, TEXT_ORIENT_HORIZ,
                                       PinNameSize,
                                       GR_TEXT_HJUSTIFY_RIGHT,
                                       GR_TEXT_VJUSTIFY_CENTER,
                                       aWidth, false, false );
                }
                if( DrawPinNum )
                {
                    plotter->text( wxPoint( (x1 + pin_pos.x) / 2,
                                            y1 - TXTMARGE ),
                                   NumColor, StringPinNum,
                                   TEXT_ORIENT_HORIZ, PinNumSize,
                                   GR_TEXT_HJUSTIFY_CENTER,
                                   GR_TEXT_VJUSTIFY_BOTTOM,
                                   aWidth, false, false );
                }
            }
        }
        else         /* Its a vertical line. */
        {
            if( orient == PIN_DOWN )
            {
                y = y1 + TextInside;

                if( DrawPinName )
                    plotter->text( wxPoint( x1, y ), NameColor,
                                   m_PinName,
                                   TEXT_ORIENT_VERT, PinNameSize,
                                   GR_TEXT_HJUSTIFY_RIGHT,
                                   GR_TEXT_VJUSTIFY_CENTER,
                                   aWidth, false, false );
                if( DrawPinNum )
                {
                    plotter->text( wxPoint( x1 - TXTMARGE,
                                            (y1 + pin_pos.y) / 2 ),
                                   NumColor, StringPinNum,
                                   TEXT_ORIENT_VERT, PinNumSize,
                                   GR_TEXT_HJUSTIFY_CENTER,
                                   GR_TEXT_VJUSTIFY_BOTTOM,
                                   aWidth, false, false );
                }
            }
            else        /* PIN_UP */
            {
                y = y1 - TextInside;

                if( DrawPinName )
                    plotter->text( wxPoint( x1, y ), NameColor,
                                   m_PinName,
                                   TEXT_ORIENT_VERT, PinNameSize,
                                   GR_TEXT_HJUSTIFY_LEFT,
                                   GR_TEXT_VJUSTIFY_CENTER,
                                   aWidth, false, false );
                if( DrawPinNum )
                {
                    plotter->text( wxPoint( x1 - TXTMARGE,
                                            (y1 + pin_pos.y) / 2 ),
                                   NumColor, StringPinNum,
                                   TEXT_ORIENT_VERT, PinNumSize,
                                   GR_TEXT_HJUSTIFY_CENTER,
                                   GR_TEXT_VJUSTIFY_BOTTOM,
                                   aWidth, false, false );
                }
            }
        }
    }
    else     /* Draw num & text pin outside */
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
        {
            /* Its an horizontal line. */
            if( DrawPinName )
            {
                x = (x1 + pin_pos.x) / 2;
                plotter->text( wxPoint( x, y1 - TXTMARGE ),
                               NameColor, m_PinName,
                               TEXT_ORIENT_HORIZ, PinNameSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_BOTTOM,
                               aWidth, false, false );
            }
            if( DrawPinNum )
            {
                x = (x1 + pin_pos.x) / 2;
                plotter->text( wxPoint( x, y1 + TXTMARGE ),
                               NumColor, StringPinNum,
                               TEXT_ORIENT_HORIZ, PinNumSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_TOP,
                               aWidth, false, false );
            }
        }
        else     /* Its a vertical line. */
        {
            if( DrawPinName )
            {
                y = (y1 + pin_pos.y) / 2;
                plotter->text( wxPoint( x1 - TXTMARGE, y ),
                               NameColor, m_PinName,
                               TEXT_ORIENT_VERT, PinNameSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_BOTTOM,
                               aWidth, false, false );
            }

            if( DrawPinNum )
            {
                plotter->text( wxPoint( x1 + TXTMARGE, (y1 + pin_pos.y) / 2 ),
                               NumColor, StringPinNum,
                               TEXT_ORIENT_VERT, PinNumSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_TOP,
                               aWidth, false, false );
            }
        }
    }
}


/******************************************/
wxPoint LibDrawPin::ReturnPinEndPoint()
/******************************************/

/* return the pin end position, for a component in normal orient
 */
{
    wxPoint pos = m_Pos;

    switch( m_Orient )
    {
    case PIN_UP:
        pos.y += m_PinLen; break;

    case PIN_DOWN:
        pos.y -= m_PinLen; break;

    case PIN_LEFT:
        pos.x -= m_PinLen; break;

    case PIN_RIGHT:
        pos.x += m_PinLen; break;
    }

    return pos;
}


/** Function ReturnPinDrawOrient
 * Return the pin real orientation (PIN_UP, PIN_DOWN, PIN_RIGHT, PIN_LEFT),
 *  according to its orientation and the matrix transform (rot, mirror) TransMat
 * @param  TransMat = transform matrix
 */
int LibDrawPin::ReturnPinDrawOrient( const int TransMat[2][2] )
{
    int     orient;
    wxPoint end;    // position of a end pin starting at 0,0 according to its orientation, lenght = 1

    switch( m_Orient )
    {
    case PIN_UP:
        end.y = 1; break;

    case PIN_DOWN:
        end.y = -1; break;

    case PIN_LEFT:
        end.x = -1; break;

    case PIN_RIGHT:
        end.x = 1; break;
    }

    end    = TransformCoordinate( TransMat, end );    // = pos of end point, according to the component orientation
    orient = PIN_UP;
    if( end.x == 0 )
    {
        if( end.y > 0 )
            orient = PIN_DOWN;
    }
    else
    {
        orient = PIN_RIGHT;
        if( end.x < 0 )
            orient = PIN_LEFT;
    }

    return orient;
}


/** Function ReturnPinStringNum
 * fill a buffer with pin num as a wxString
 *  Pin num is coded as a long or 4 ascii chars
 *  Used to print/draw the pin num
 * @param aStringBuffer = the wxString to store the pin num as an unicode string
 */
void LibDrawPin::ReturnPinStringNum( wxString& aStringBuffer ) const
{
    aStringBuffer = ReturnPinStringNum( m_PinNum );
}

/** Function ReturnPinStringNum (static function)
 *  Pin num is coded as a long or 4 ascii chars
 * @param aPinNum = a long containing a pin num
 * @return aStringBuffer = the wxString to store the pin num as an unicode string
 */
wxString LibDrawPin::ReturnPinStringNum( long aPinNum )
{
    char ascii_buf[5];

    memcpy( ascii_buf, &aPinNum, 4 );
    ascii_buf[4] = 0;

    wxString buffer = CONV_FROM_UTF8( ascii_buf );

    return buffer;
}


/** Function LibDrawPin::SetPinNumFromString()
 * fill the buffer with pin num as a wxString
 *  Pin num is coded as a long
 *  Used to print/draw the pin num
 */
void LibDrawPin::SetPinNumFromString( wxString& buffer )
{
    char     ascii_buf[4];
    unsigned ii, len = buffer.Len();

    ascii_buf[0] = ascii_buf[1] = ascii_buf[2] = ascii_buf[3] = 0;
    if( len > 4 )
        len = 4;
    for( ii = 0; ii < len; ii++ )
    {
        ascii_buf[ii]  = buffer.GetChar( ii );
        ascii_buf[ii] &= 0xFF;
    }

    strncpy( (char*) &m_PinNum, ascii_buf, 4 );
}


/*************************************/
LibEDA_BaseStruct* LibDrawPin::DoGenCopy()
/*************************************/
{
    LibDrawPin* newpin = new LibDrawPin( GetParent() );

    newpin->m_Pos                = m_Pos;
    newpin->m_PinLen             = m_PinLen;
    newpin->m_Orient             = m_Orient;
    newpin->m_PinShape           = m_PinShape;
    newpin->m_PinType            = m_PinType;
    newpin->m_Attributs          = m_Attributs;
    newpin->m_PinNum             = m_PinNum;
    newpin->m_PinNumSize         = m_PinNumSize;
    newpin->m_PinNameSize        = m_PinNameSize;
    newpin->m_PinNumShapeOpt     = m_PinNumShapeOpt;
    newpin->m_PinNameShapeOpt    = m_PinNameShapeOpt;
    newpin->m_PinNumPositionOpt  = m_PinNumPositionOpt;
    newpin->m_PinNamePositionOpt = m_PinNamePositionOpt;
    newpin->m_Unit               = m_Unit;
    newpin->m_Convert            = m_Convert;
    newpin->m_Flags              = m_Flags;
    newpin->m_Width              = m_Width;
    newpin->m_PinName            = m_PinName;

    return (LibEDA_BaseStruct*) newpin;
}


bool LibDrawPin::DoCompare( const LibEDA_BaseStruct& other ) const
{
    wxASSERT( other.Type() == COMPONENT_PIN_DRAW_TYPE );

    const LibDrawPin* tmp = ( LibDrawPin* ) &other;

    return ( m_Pos == tmp->m_Pos );
}


void LibDrawPin::DoOffset( const wxPoint& offset )
{
    m_Pos += offset;
}


bool LibDrawPin::DoTestInside( EDA_Rect& rect )
{
    wxPoint end = ReturnPinEndPoint();

    return rect.Inside( m_Pos.x, -m_Pos.y ) || rect.Inside( end.x, -end.y );
}


/** Function LibDrawPin::DisplayInfo
 * Displays info (pin num and name, orientation ...
 * on the Info window
 */
void LibDrawPin::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString Text;
    int      ii;

    LibEDA_BaseStruct::DisplayInfo( frame );

    /* Affichage du nom */
    frame->MsgPanel->Affiche_1_Parametre( 30, _( "PinName" ), m_PinName,
                                          DARKCYAN );

    /* Affichage du numero */
    if( m_PinNum == 0 )
        Text = wxT( "?" );
    else
        ReturnPinStringNum( Text );

    frame->MsgPanel->Affiche_1_Parametre( 38, _( "PinNum" ), Text, DARKCYAN );

    /* Affichage du type */
    ii = m_PinType;
    frame->MsgPanel->Affiche_1_Parametre( 44, _( "PinType" ),
                                          MsgPinElectricType[ii], RED );

    /* Affichage de la visiblite */
    ii = m_Attributs;
    if( ii & 1 )
        Text = _( "no" );
    else
        Text = _( "yes" );
    frame->MsgPanel->Affiche_1_Parametre( 50, _( "Display" ), Text, DARKGREEN );

    /* Display pin length */
    Text = ReturnStringFromValue( g_UnitMetric, m_PinLen,
                                  EESCHEMA_INTERNAL_UNIT, true );
    frame->MsgPanel->Affiche_1_Parametre( 56, _( "Length" ), Text, MAGENTA );

    /* Affichage de l'orientation */
    switch( m_Orient )
    {
    case PIN_UP:
        Text = _( "Up" );
        break;

    case PIN_DOWN:
        Text = _( "Down" );
        break;

    case PIN_LEFT:
        Text = _( "Left" );
        break;

    case PIN_RIGHT:
        Text = _( "Right" );
        break;

    default:
        Text = wxT( "??" );
        break;
    }

    frame->MsgPanel->Affiche_1_Parametre( 62, _( "Orient" ), Text, MAGENTA );
}


/** Function LibDrawPin::GetBoundingBox
 * @return the boundary box for this, in schematic coordinates
 */
EDA_Rect LibDrawPin::GetBoundingBox()
{
    wxPoint pt = m_Pos;

    pt.y *= -1;     // Reverse the Y axis, according to the schematic orientation

    return EDA_Rect( pt, wxSize( 1, 1 ) );
}
