/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file lib_pin.cpp
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <macros.h>
#include <trigo.h>
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <plot_common.h>
#include <schframe.h>
#include <richio.h>
#include <base_units.h>
#include <msgpanel.h>

#include <general.h>
#include <libeditframe.h>
#include <class_libentry.h>
#include <lib_pin.h>
#include <transform.h>
#include <sch_component.h>


static const int pin_orientation_codes[] =
{
    PIN_RIGHT,
    PIN_LEFT,
    PIN_UP,
    PIN_DOWN
};
#define PIN_ORIENTATION_CNT DIM( pin_orientation_codes )

// small margin in internal units between the pin text and the pin line
#define PIN_TEXT_MARGIN 4

// bitmaps to show pins orientations in dialog editor
// must have same order than pin_orientation_names
static const BITMAP_DEF iconsPinsOrientations[] =
{
    pinorient_right_xpm,
    pinorient_left_xpm,
    pinorient_up_xpm,
    pinorient_down_xpm,
};


const wxString LIB_PIN::GetCanonicalElectricalTypeName( ELECTRICAL_PINTYPE aType )
{
    if( aType < 0 || aType >= (int) PINTYPE_COUNT )
        return wxT( "???" );

    // These strings are the canonical name of the electrictal type
    // Not translated, no space in name, only ASCII chars.
    // to use when the string name must be known and well defined
    // must have same order than enum ELECTRICAL_PINTYPE (see lib_pin.h)
    static const wxChar* msgPinElectricType[] =
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
        wxT( "NotConnected" )
    };

    return msgPinElectricType[ aType ];
}


// Helper functions to get the pin orientation name from pin_orientation_codes
// Note: the strings are *not* static because they are translated and must be built
// on the fly, to be properly translated

static const wxString getPinOrientationName( unsigned aPinOrientationCode )
{
    /* Note: The following name lists are sentence capitalized per the GNOME UI
     *       standards for list controls.  Please do not change the capitalization
     *       of these strings unless the GNOME UI standards are changed.
     */
    const wxString pin_orientation_names[] =
    {
        _( "Right" ),
        _( "Left" ),
        _( "Up" ),
        _( "Down" ),
        wxT( "???" )
    };

    if( aPinOrientationCode > PIN_ORIENTATION_CNT )
        aPinOrientationCode = PIN_ORIENTATION_CNT;

    return pin_orientation_names[ aPinOrientationCode ];
}

/// Utility for getting the size of the 'internal' pin decorators (as a radius)
// i.e. the clock symbols (falling clock is actually external but is of
// the same kind)

static int InternalPinDecoSize( const LIB_PIN &aPin )
{
    return aPin.GetNameTextSize() / 2;
}

/// Utility for getting the size of the 'external' pin decorators (as a radius)
// i.e. the negation circle, the polarity 'slopes' and the nonlogic
// marker
static int ExternalPinDecoSize( const LIB_PIN &aPin )
{
    return aPin.GetNumberTextSize() / 2;
}

LIB_PIN::LIB_PIN( LIB_PART*      aParent ) :
    LIB_ITEM( LIB_PIN_T, aParent ),
    m_shape( PINSHAPE_LINE )
{
    m_length = LIB_EDIT_FRAME::GetDefaultPinLength();
    m_orientation = PIN_RIGHT;                  // Pin orient: Up, Down, Left, Right
    m_type = PIN_UNSPECIFIED;                   // electrical type of pin
    m_attributes = 0;                           // bit 0 != 0: pin invisible
    m_number = 0;                               // pin number (i.e. 4 ASCII chars)
    m_numTextSize = LIB_EDIT_FRAME::GetPinNumDefaultSize();
    m_nameTextSize = LIB_EDIT_FRAME::GetPinNameDefaultSize();
    m_width = 0;
    m_typeName = _( "Pin" );
}


void LIB_PIN::SetName( const wxString& aName )
{
    wxString tmp = ( aName.IsEmpty() ) ? wxT( "~" ) : aName;

    tmp.Replace( wxT( " " ), wxT( "_" ) );

    if( m_name != tmp )
    {
        m_name = tmp;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0 || pinList[i]->m_name == m_name )
            continue;

        pinList[i]->m_name = m_name;
        SetModified();
    }
}


void LIB_PIN::SetNameTextSize( int size )
{
    if( size != m_nameTextSize )
    {
        m_nameTextSize = size;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0 || pinList[i]->m_nameTextSize == size )
            continue;

        pinList[i]->m_nameTextSize = size;
        SetModified();
    }
}


void LIB_PIN::SetNumber( const wxString& number )
{
    wxString tmp = ( number.IsEmpty() ) ? wxT( "~" ) : number;

    tmp.Replace( wxT( " " ), wxT( "_" ) );
    long     oldNumber = m_number;
    SetPinNumFromString( tmp );

    if( m_number != oldNumber )
        SetFlags( IS_CHANGED );

    /* Others pin numbers marked by EnableEditMode() are not modified
     * because each pin has its own number
     */
}


void LIB_PIN::SetNumberTextSize( int size )
{
    if( size != m_numTextSize )
    {
        m_numTextSize = size;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0 || pinList[i]->m_numTextSize == size )
            continue;

        pinList[i]->m_numTextSize = size;
        SetModified();
    }
}


void LIB_PIN::SetOrientation( int orientation )
{
    if( m_orientation != orientation )
    {
        m_orientation = orientation;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0 ||
              pinList[i]->m_orientation == orientation )
            continue;

        pinList[i]->m_orientation = orientation;
        SetModified();
    }
}


void LIB_PIN::SetShape( GRAPHIC_PINSHAPE aShape )
{
    assert( aShape >= 0 && aShape < int( PINSHAPE_COUNT ) );

    if( m_shape != aShape )
    {
        m_shape = aShape;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0
           || pinList[i]->m_Convert != m_Convert
           || pinList[i]->m_shape == aShape )
            continue;

        pinList[i]->m_shape = aShape;
        SetModified();
    }
}


void LIB_PIN::SetType( ELECTRICAL_PINTYPE aType )
{
    assert( aType >= 0 && aType < (int)PINTYPE_COUNT );

    if( aType < PIN_INPUT )
        aType = PIN_INPUT;

    if( aType >= (int)PINTYPE_COUNT )
        aType = PIN_NC;

    if( m_type != aType )
    {
        m_type = aType;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0 || pinList[i]->m_type == aType )
            continue;

        pinList[i]->m_type = aType;
        SetModified();
    }
}


void LIB_PIN::SetLength( int length )
{
    if( m_length != length )
    {
        m_length = length;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0
           || pinList[i]->m_Convert != m_Convert
           || pinList[i]->m_length == length )
            continue;

        pinList[i]->m_length = length;
        SetModified();
    }
}


void LIB_PIN::SetPinPosition( wxPoint aPosition )
{
    if( m_position != aPosition )
    {
        m_position = aPosition;
        SetModified();
    }

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0
           || pinList[i]->m_Convert != m_Convert
           || pinList[i]->m_position == aPosition )
            continue;

        pinList[i]->m_position = aPosition;
        SetModified();
    }
}


void LIB_PIN::SetPartNumber( int part )
{
    if( m_Unit == part )
        return;

    m_Unit = part;
    SetModified();

    if( m_Unit == 0 )
    {
        LIB_PIN* pin;
        LIB_PIN* tmp = GetParent()->GetNextPin();

        while( tmp != NULL )
        {
            pin = tmp;
            tmp = GetParent()->GetNextPin( pin );

            if( pin->m_Flags == 0 || pin == this
               || ( m_Convert && ( m_Convert != pin->m_Convert ) )
               || ( m_position != pin->m_position )
               || ( pin->m_orientation != m_orientation ) )
                continue;

            GetParent()->RemoveDrawItem( (LIB_ITEM*) pin );
        }
    }
}


void LIB_PIN::SetConversion( int style )
{
    if( m_Convert == style )
        return;

    m_Convert = style;
    SetFlags( IS_CHANGED );

    if( style == 0 )
    {
        LIB_PIN* pin;
        LIB_PIN* tmp = GetParent()->GetNextPin();

        while( tmp != NULL )
        {
            pin = tmp;
            tmp = GetParent()->GetNextPin( pin );

            if( ( pin->m_Flags & IS_LINKED ) == 0
               || ( pin == this )
               || ( m_Unit && ( m_Unit != pin->m_Unit ) )
               || ( m_position != pin->m_position )
               || ( pin->m_orientation != m_orientation ) )
                continue;

            GetParent()->RemoveDrawItem( (LIB_ITEM*) pin );
        }
    }
}


void LIB_PIN::SetVisible( bool visible )
{
    if( visible == IsVisible() )
        return;

    if( visible )
        m_attributes &= ~PIN_INVISIBLE;
    else
        m_attributes |= PIN_INVISIBLE;

    SetModified();

    if( GetParent() == NULL )
        return;

    LIB_PINS pinList;
    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( ( pinList[i]->m_Flags & IS_LINKED ) == 0 || pinList[i]->IsVisible() == visible )
            continue;

        if( visible )
            pinList[i]->m_attributes &= ~PIN_INVISIBLE;
        else
            pinList[i]->m_attributes |= PIN_INVISIBLE;

        SetModified();
    }
}


void LIB_PIN::EnableEditMode( bool enable, bool editPinByPin )
{
    LIB_PINS pinList;

    if( GetParent() == NULL )
        return;

    GetParent()->GetPins( pinList );

    for( size_t i = 0; i < pinList.size(); i++ )
    {
        if( pinList[i] == this )
            continue;

        if( ( pinList[i]->m_position == m_position )
           && ( pinList[i]->m_orientation == m_orientation )
           && !IsNew()
           && editPinByPin == false
           && enable )
            pinList[i]->SetFlags( IS_LINKED | IN_EDIT );
        else
            pinList[i]->ClearFlags( IS_LINKED | IN_EDIT );
    }
}


bool LIB_PIN::HitTest( const wxPoint& aPosition ) const
{
    return HitTest( aPosition, 0, DefaultTransform );
}


bool LIB_PIN::HitTest( const wxPoint &aPosition, int aThreshold, const TRANSFORM& aTransform ) const
{
    if( aThreshold < 0 )
        aThreshold = 0;

    TRANSFORM transform = DefaultTransform;
    DefaultTransform = aTransform;

    EDA_RECT rect = GetBoundingBox();
    rect.Inflate( aThreshold );

    //Restore matrix
    DefaultTransform = transform;

    return rect.Contains( aPosition );
}


bool LIB_PIN::Save( OUTPUTFORMATTER& aFormatter )
{
    wxString StringPinNum;
    int      Etype;

    switch( m_type )
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

    case PIN_NC:
        Etype = 'N';
        break;
    }

    PinStringNum( StringPinNum );

    if( StringPinNum.IsEmpty() )
        StringPinNum = wxT( "~" );

    if( !m_name.IsEmpty() )
    {
        if( aFormatter.Print( 0, "X %s", TO_UTF8( m_name ) ) < 0 )
            return false;
    }
    else
    {
        if( aFormatter.Print( 0, "X ~" ) < 0 )
            return false;
    }

    if( aFormatter.Print( 0, " %s %d %d %d %c %d %d %d %d %c",
                          TO_UTF8( StringPinNum ), m_position.x, m_position.y,
                          (int) m_length, (int) m_orientation, m_numTextSize, m_nameTextSize,
                          m_Unit, m_Convert, Etype ) < 0 )
        return false;

    if( m_shape || !IsVisible() )
    {
        if( aFormatter.Print( 0, " " ) < 0 )
            return false;
    }

    if( !IsVisible() && aFormatter.Print( 0, "N" ) < 0 )
        return false;

    switch( m_shape )
    {
    case PINSHAPE_LINE:
        break;

    case PINSHAPE_INVERTED:
        if( aFormatter.Print( 0, "I" ) < 0 )
            return false;
        break;

    case PINSHAPE_CLOCK:
        if( aFormatter.Print( 0, "C" ) < 0 )
            return false;
        break;

    case PINSHAPE_INVERTED_CLOCK:
        if( aFormatter.Print( 0, "IC" ) < 0 )
            return false;
        break;

    case PINSHAPE_INPUT_LOW:
        if( aFormatter.Print( 0, "L" ) < 0 )
            return false;
        break;

    case PINSHAPE_CLOCK_LOW:
        if( aFormatter.Print( 0, "CL" ) < 0 )
            return false;
        break;

    case PINSHAPE_OUTPUT_LOW:
        if( aFormatter.Print( 0, "V" ) < 0 )
            return false;
        break;

    case PINSHAPE_FALLING_EDGE_CLOCK:
        if( aFormatter.Print( 0, "F" ) < 0 )
            return false;
        break;

    case PINSHAPE_NONLOGIC:
        if( aFormatter.Print( 0, "X" ) < 0 )
            return false;
        break;

    default:
        assert( !"Invalid pin shape" );
        return false;
    }

    if( aFormatter.Print( 0, "\n" ) < 0 )
        return false;

    ClearFlags( IS_CHANGED );

    return true;
}

#include <wx/tokenzr.h>

bool LIB_PIN::Load( LINE_READER& aLineReader, wxString& aErrorMsg )
{
    char pinAttrs[64];
    char pinOrient[64];
    char pinType[64];

    *pinAttrs = 0;

    // We cannot use sscanf, at least on Windows, to parse the pin description.
    // The reason is the pin name is free, and use UTF8 encoding.
    // We encourtered issues (Windows specific) to read this name for some UTF8
    // cyrillic codes
    // So, read the pin name (and num) after conversion from UTF8, and read the others
    // parameters (in ASCII) using sscanf

    // the full line starts by "X ". The pin data starts at line + 2.
    wxString utf8line = FROM_UTF8( aLineReader.Line() + 2 );
	wxStringTokenizer tokenizer( utf8line, wxT(" \n\r" ) );
    int prms_count = tokenizer.CountTokens();

    if( prms_count < 11 )
    {
        aErrorMsg.Printf( wxT( "pin had %d parameters of the required 11 or 12" ), prms_count );
        return false;
    }

    // Extract the pinName (UTF8 encoded)
    m_name = tokenizer.GetNextToken();

    wxString tmp;

    // Extract the pinName (UTF8 encoded accepted, but should be only ASCII8.)
    tmp = tokenizer.GetNextToken();
    SetPinNumFromString( tmp );

    // Read other parameters, in pure ASCII
    char line[1024];
    tmp = tokenizer.GetString();

    unsigned len = tmp.Length();

    if( len >= sizeof( line ) )     // Should not occur.
        len = sizeof( line) - 1;

    strncpy( line, TO_UTF8( tmp ), len );
    line[len] = 0;

    int cnt = sscanf( line, "%d %d %d %63s %d %d %d %d %63s %63s",
                &m_position.x, &m_position.y, &m_length, pinOrient, &m_numTextSize,
                &m_nameTextSize, &m_Unit, &m_Convert, pinType, pinAttrs );

    if( cnt != (prms_count - 2) )
    {
        aErrorMsg.Printf( wxT( "pin parameters read issue" ) );
        return false;
    }

    m_orientation = pinOrient[0] & 255;

    switch( *pinType & 255 )
    {
    case 'I':
        m_type = PIN_INPUT;
        break;

    case 'O':
        m_type = PIN_OUTPUT;
        break;

    case 'B':
        m_type = PIN_BIDI;
        break;

    case 'T':
        m_type = PIN_TRISTATE;
        break;

    case 'P':
        m_type = PIN_PASSIVE;
        break;

    case 'U':
        m_type = PIN_UNSPECIFIED;
        break;

    case 'W':
        m_type = PIN_POWER_IN;
        break;

    case 'w':
        m_type = PIN_POWER_OUT;
        break;

    case 'C':
        m_type = PIN_OPENCOLLECTOR;
        break;

    case 'E':
        m_type = PIN_OPENEMITTER;
        break;

    case 'N':
        m_type = PIN_NC;
        break;

    default:
        aErrorMsg.Printf( wxT( "unknown pin type [%c]" ), *pinType & 255 );
        return false;
    }

    if( prms_count >= 12 )       /* Special Symbol defined */
    {
        enum
        {
            INVERTED        = 1 << 0,
            CLOCK           = 1 << 1,
            LOWLEVEL_IN     = 1 << 2,
            LOWLEVEL_OUT    = 1 << 3,
            FALLING_EDGE    = 1 << 4,
            NONLOGIC        = 1 << 5
        };

        int flags = 0;

        for( int j = strlen( pinAttrs ); j > 0; )
        {
            switch( pinAttrs[--j] )
            {
            case '~':
                break;

            case 'N':
                m_attributes |= PIN_INVISIBLE;
                break;

            case 'I':
                flags |= INVERTED;
                break;

            case 'C':
                flags |= CLOCK;
                break;

            case 'L':
                flags |= LOWLEVEL_IN;
                break;

            case 'V':
                flags |= LOWLEVEL_OUT;
                break;

            case 'F':
                flags |= FALLING_EDGE;
                break;

            case 'X':
                flags |= NONLOGIC;
                break;

            default:
                aErrorMsg.Printf( wxT( "unknown pin attribute [%c]" ), pinAttrs[j] );
                return false;
            }
        }

        switch( flags )
        {
        case 0:
            m_shape = PINSHAPE_LINE;
            break;

        case INVERTED:
            m_shape = PINSHAPE_INVERTED;
            break;

        case CLOCK:
            m_shape = PINSHAPE_CLOCK;
            break;

        case INVERTED | CLOCK:
            m_shape = PINSHAPE_INVERTED_CLOCK;
            break;

        case LOWLEVEL_IN:
            m_shape = PINSHAPE_INPUT_LOW;
            break;

        case LOWLEVEL_IN | CLOCK:
            m_shape = PINSHAPE_CLOCK_LOW;
            break;

        case LOWLEVEL_OUT:
            m_shape = PINSHAPE_OUTPUT_LOW;
            break;

        case FALLING_EDGE:
            m_shape = PINSHAPE_FALLING_EDGE_CLOCK;
            break;

        case NONLOGIC:
            m_shape = PINSHAPE_NONLOGIC;
            break;

        default:
            aErrorMsg.Printf( wxT( "pin attributes do not give a valid pin shape [%s]" ), pinAttrs );
            return false;
        }
    }

    return true;
}


int LIB_PIN::GetPenSize() const
{
    return ( m_width == 0 ) ? GetDefaultLineThickness() : m_width;
}


void LIB_PIN::drawGraphic( EDA_DRAW_PANEL*  aPanel,
                           wxDC*            aDC,
                           const wxPoint&   aOffset,
                           EDA_COLOR_T      aColor,
                           GR_DRAWMODE      aDrawMode,
                           void*            aData,
                           const TRANSFORM& aTransform )
{
    // aData is used here as a bitfield of flags.
    uintptr_t flags = (uintptr_t) aData;
    bool drawPinText = flags & PIN_DRAW_TEXTS;
    bool drawPinDangling = flags & PIN_DRAW_DANGLING;
    bool drawDanglingHidden = flags & PIN_DANGLING_HIDDEN;
    bool drawElectricalTypeName = flags & PIN_DRAW_ELECTRICAL_TYPE_NAME;

    LIB_PART* Entry = GetParent();

    /* Calculate pin orient taking in account the component orientation. */
    int     orient = PinDrawOrient( aTransform );

    /* Calculate the pin position */
    wxPoint pos1 = aTransform.TransformCoordinate( m_position ) + aOffset;

    // Invisible pins are only drawn on request.
    // They are drawn in GetInvisibleItemColor().
    // in schematic, they are drawn only if m_showAllPins is true.
    // In other windows, they are always drawn because we must see them.
    if( ! IsVisible() )
    {
        EDA_DRAW_FRAME* frame = NULL;

        if( aPanel && aPanel->GetParent() )
            frame = (EDA_DRAW_FRAME*)aPanel->GetParent();

        if( frame && frame->IsType( FRAME_SCH ) &&
            ! ((SCH_EDIT_FRAME*)frame)->GetShowAllPins() )
        {
            if( drawPinDangling && drawDanglingHidden )
            {
                // Draw the target
                DrawPinSymbol( aPanel, aDC, pos1, orient, aDrawMode, aColor, drawPinDangling,
                        /* aOnlyTarget */ true );
            }
            return;
        }

        aColor = GetInvisibleItemColor();
    }

    /* Drawing from the pin and the special symbol combination */
    DrawPinSymbol( aPanel, aDC, pos1, orient, aDrawMode, aColor, drawPinDangling );

    if( drawPinText )
    {
        DrawPinTexts( aPanel, aDC, pos1, orient, Entry->GetPinNameOffset(),
                      Entry->ShowPinNumbers(), Entry->ShowPinNames(),
                      aColor, aDrawMode );
    }

    if( drawElectricalTypeName )
        DrawPinElectricalTypeName( aPanel, aDC, pos1, orient, aColor, aDrawMode );


    /* Set to one (1) to draw bounding box around pin to validate bounding
     * box calculation. */
#if 0
    EDA_RECT  bBox    = GetBoundingBox();
    bBox.RevertYAxis();
    bBox = aTransform.TransformCoordinate( bBox );
    bBox.Move( aOffset );
    GRRect( aPanel ? aPanel->GetClipBox() : NULL, aDC, bBox, 0, LIGHTMAGENTA );
#endif
}


void LIB_PIN::DrawPinSymbol( EDA_DRAW_PANEL* aPanel,
                             wxDC*           aDC,
                             const wxPoint&  aPinPos,
                             int             aOrient,
                             GR_DRAWMODE     aDrawMode,
                             EDA_COLOR_T     aColor,
                             bool            aDrawDangling,
                             bool            aOnlyTarget )
{
    int       MapX1, MapY1, x1, y1;
    int       width   = GetPenSize();
    int       posX    = aPinPos.x, posY = aPinPos.y, len = m_length;
    EDA_RECT* clipbox = aPanel ? aPanel->GetClipBox() : NULL;

    EDA_COLOR_T color = GetLayerColor( LAYER_PIN );

    if( aColor < 0 )       // Used normal color or selected color
    {
        if( IsSelected() )
            color = GetItemSelectedColor();
    }
    else
        color = aColor;

    GRSetDrawMode( aDC, aDrawMode );

    MapX1 = MapY1 = 0;
    x1    = posX;
    y1    = posY;

    switch( aOrient )
    {
    case PIN_UP:
        y1    = posY - len;
        MapY1 = 1;
        break;

    case PIN_DOWN:
        y1    = posY + len;
        MapY1 = -1;
        break;

    case PIN_LEFT:
        x1    = posX - len;
        MapX1 = 1;
        break;

    case PIN_RIGHT:
        x1    = posX + len;
        MapX1 = -1;
        break;
    }

    // Draw the pin end target (active end of the pin)
    BASE_SCREEN* screen = aPanel ? aPanel->GetScreen() : NULL;
    #define NCSYMB_PIN_DIM TARGET_PIN_RADIUS

    // Draw but do not print the pin end target 1 pixel width
    if( m_type != PIN_NC && ( screen == NULL || !screen->m_IsPrinting ) )
    {
        if( aDrawDangling )
            GRCircle( clipbox, aDC, posX, posY, TARGET_PIN_RADIUS, 0, color );
    }

    if( aOnlyTarget )
        return;


    if( m_shape == PINSHAPE_INVERTED || m_shape == PINSHAPE_INVERTED_CLOCK )
    {
        const int radius = ExternalPinDecoSize( *this );
        GRCircle( clipbox, aDC, MapX1 * radius + x1,
                  MapY1 * radius + y1,
                  radius, width, color );

        GRMoveTo( MapX1 * radius * 2 + x1,
                  MapY1 * radius * 2 + y1 );
        GRLineTo( clipbox, aDC, posX, posY, width, color );
    }
    else if( m_shape == PINSHAPE_FALLING_EDGE_CLOCK ) /* an alternative for Inverted Clock */
    {
        const int clock_size = InternalPinDecoSize( *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 + clock_size );
            GRLineTo( clipbox, aDC, x1 + MapX1 * clock_size * 2, y1,
                      width, color );
            GRLineTo( clipbox, aDC, x1, y1 - clock_size, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 + clock_size, y1 );
            GRLineTo( clipbox, aDC, x1, y1 + MapY1 * clock_size * 2,
                      width, color );
            GRLineTo( clipbox, aDC, x1 - clock_size, y1,
                      width, color );
        }
        GRMoveTo( MapX1 * clock_size * 2 + x1, MapY1 * clock_size * 2 + y1 );
        GRLineTo( clipbox, aDC, posX, posY, width, color );
    }
    else
    {
        GRMoveTo( x1, y1 );
        GRLineTo( clipbox, aDC, posX, posY, width, color );
    }

    if( m_shape == PINSHAPE_CLOCK || m_shape == PINSHAPE_INVERTED_CLOCK || m_shape == PINSHAPE_CLOCK_LOW )
    {
        const int clock_size = InternalPinDecoSize( *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 + clock_size );
            GRLineTo( clipbox, aDC, x1 - MapX1 * clock_size * 2, y1,
                      width, color );
            GRLineTo( clipbox, aDC, x1, y1 - clock_size,
                      width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 + clock_size, y1 );
            GRLineTo( clipbox, aDC, x1, y1 - MapY1 * clock_size * 2,
                      width, color );
            GRLineTo( clipbox, aDC, x1 - clock_size, y1,
                      width, color );
        }
    }

    if( m_shape == PINSHAPE_INPUT_LOW || m_shape == PINSHAPE_CLOCK_LOW )
    {
        const int symbol_size = ExternalPinDecoSize( *this );
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1 + MapX1 * symbol_size * 2, y1 );
            GRLineTo( clipbox, aDC,
                      x1 + MapX1 * symbol_size * 2, y1 - symbol_size * 2,
                      width, color );
            GRLineTo( clipbox, aDC, x1, y1, width, color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1, y1 + MapY1 * symbol_size * 2 );
            GRLineTo( clipbox, aDC, x1 - symbol_size * 2,
                      y1 + MapY1 * symbol_size * 2, width, color );
            GRLineTo( clipbox, aDC, x1, y1, width, color );
        }
    }


    if( m_shape == PINSHAPE_OUTPUT_LOW )    /* IEEE symbol "Active Low Output" */
    {
        const int symbol_size = ExternalPinDecoSize( *this );
        if( MapY1 == 0 )            /* MapX1 = +- 1 */
        {
            GRMoveTo( x1, y1 - symbol_size * 2 );
            GRLineTo( clipbox,
                      aDC,
                      x1 + MapX1 * symbol_size * 2,
                      y1,
                      width,
                      color );
        }
        else    /* MapX1 = 0 */
        {
            GRMoveTo( x1 - symbol_size * 2, y1 );
            GRLineTo( clipbox, aDC, x1, y1 + MapY1 * symbol_size * 2,
                      width, color );
        }
    }
    else if( m_shape == PINSHAPE_NONLOGIC ) /* NonLogic pin symbol */
    {
        const int symbol_size = ExternalPinDecoSize( *this );
        GRMoveTo( x1 - (MapX1 + MapY1) * symbol_size,
                  y1 - (MapY1 - MapX1) * symbol_size );
        GRLineTo( clipbox, aDC,
                  x1 + (MapX1 + MapY1) * symbol_size,
                  y1 + (MapY1 - MapX1) * symbol_size,
                  width, color );
        GRMoveTo( x1 - (MapX1 - MapY1) * symbol_size,
                  y1 - (MapY1 + MapX1) * symbol_size );
        GRLineTo( clipbox, aDC,
                  x1 + (MapX1 - MapY1) * symbol_size,
                  y1 + (MapY1 + MapX1) * symbol_size,
                  width, color );
    }

    if( m_type == PIN_NC )   // Draw a N.C. symbol
    {
        GRLine( clipbox, aDC,
                posX - NCSYMB_PIN_DIM, posY - NCSYMB_PIN_DIM,
                posX + NCSYMB_PIN_DIM, posY + NCSYMB_PIN_DIM,
                width, color );
        GRLine( clipbox, aDC,
                posX + NCSYMB_PIN_DIM, posY - NCSYMB_PIN_DIM,
                posX - NCSYMB_PIN_DIM, posY + NCSYMB_PIN_DIM,
                width, color );
    }
}


void LIB_PIN::DrawPinTexts( EDA_DRAW_PANEL* panel,
                            wxDC*           DC,
                            wxPoint&        pin_pos,
                            int             orient,
                            int             TextInside,
                            bool            DrawPinNum,
                            bool            DrawPinName,
                            EDA_COLOR_T     Color,
                            GR_DRAWMODE     DrawMode )
{
    if( !DrawPinName && !DrawPinNum )
        return;

    int         x, y;
    wxString    StringPinNum;

    wxSize      PinNameSize( m_nameTextSize, m_nameTextSize );
    wxSize      PinNumSize( m_numTextSize, m_numTextSize );

    int         nameLineWidth = GetPenSize();

    nameLineWidth = Clamp_Text_PenSize( nameLineWidth, m_nameTextSize, false );
    int         numLineWidth = GetPenSize();
    numLineWidth = Clamp_Text_PenSize( numLineWidth, m_numTextSize, false );

    int         name_offset = PIN_TEXT_MARGIN +
                              ( nameLineWidth + GetDefaultLineThickness() ) / 2;
    int         num_offset = PIN_TEXT_MARGIN +
                             ( numLineWidth + GetDefaultLineThickness() ) / 2;

    GRSetDrawMode( DC, DrawMode );
    EDA_RECT* clipbox = panel? panel->GetClipBox() : NULL;

    /* Get the num and name colors */
    if( (Color < 0) && IsSelected() )
        Color = GetItemSelectedColor();

    EDA_COLOR_T NameColor = Color == UNSPECIFIED_COLOR ?
                                GetLayerColor( LAYER_PINNAM ) : Color;
    EDA_COLOR_T NumColor  = Color == UNSPECIFIED_COLOR ?
                                GetLayerColor( LAYER_PINNUM ) : Color;

    /* Create the pin num string */
    PinStringNum( StringPinNum );

    int x1 = pin_pos.x;
    int y1 = pin_pos.y;

    switch( orient )
    {
    case PIN_UP:
        y1 -= m_length;
        break;

    case PIN_DOWN:
        y1 += m_length;
        break;

    case PIN_LEFT:
        x1 -= m_length;
        break;

    case PIN_RIGHT:
        x1 += m_length;
        break;
    }

    if( m_name.IsEmpty() )
        DrawPinName = false;

    if( TextInside )  // Draw the text inside, but the pin numbers outside.
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
        {
            // It is an horizontal line
            if( DrawPinName )
            {
                if( orient == PIN_RIGHT )
                {
                    x = x1 + TextInside;
                    DrawGraphicText( clipbox, DC, wxPoint( x, y1 ), NameColor,
                                     m_name,
                                     TEXT_ORIENT_HORIZ,
                                     PinNameSize,
                                     GR_TEXT_HJUSTIFY_LEFT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );
                }
                else    // Orient == PIN_LEFT
                {
                    x = x1 - TextInside;
                    DrawGraphicText( clipbox, DC, wxPoint( x, y1 ), NameColor,
                                     m_name,
                                     TEXT_ORIENT_HORIZ,
                                     PinNameSize,
                                     GR_TEXT_HJUSTIFY_RIGHT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );
                }
            }

            if( DrawPinNum )
            {
                DrawGraphicText( clipbox, DC,
                                 wxPoint( (x1 + pin_pos.x) / 2,
                                         y1 - num_offset ), NumColor,
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
                    DrawGraphicText( clipbox, DC, wxPoint( x1, y ), NameColor,
                                     m_name,
                                     TEXT_ORIENT_VERT, PinNameSize,
                                     GR_TEXT_HJUSTIFY_RIGHT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );

                if( DrawPinNum )
                    DrawGraphicText( clipbox, DC,
                                     wxPoint( x1 - num_offset,
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
                    DrawGraphicText( clipbox, DC, wxPoint( x1, y ), NameColor,
                                     m_name,
                                     TEXT_ORIENT_VERT, PinNameSize,
                                     GR_TEXT_HJUSTIFY_LEFT,
                                     GR_TEXT_VJUSTIFY_CENTER, nameLineWidth,
                                     false, false );

                if( DrawPinNum )
                    DrawGraphicText( clipbox, DC,
                                     wxPoint( x1 - num_offset,
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
                DrawGraphicText( clipbox, DC, wxPoint( x, y1 - name_offset ),
                                 NameColor, m_name,
                                 TEXT_ORIENT_HORIZ, PinNameSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_BOTTOM, nameLineWidth,
                                 false, false );
            }
            if( DrawPinNum )
            {
                x = (x1 + pin_pos.x) / 2;
                DrawGraphicText( clipbox, DC, wxPoint( x, y1 + num_offset ),
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
                DrawGraphicText( clipbox, DC, wxPoint( x1 - name_offset, y ),
                                 NameColor, m_name,
                                 TEXT_ORIENT_VERT, PinNameSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_BOTTOM, nameLineWidth,
                                 false, false );
            }

            if( DrawPinNum )
            {
                DrawGraphicText( clipbox, DC,
                                 wxPoint( x1 + num_offset, (y1 + pin_pos.y)
                                          / 2 ),
                                 NumColor, StringPinNum,
                                 TEXT_ORIENT_VERT, PinNumSize,
                                 GR_TEXT_HJUSTIFY_CENTER,
                                 GR_TEXT_VJUSTIFY_TOP, numLineWidth,
                                 false, false );
            }
        }
    }
}



void LIB_PIN::DrawPinElectricalTypeName( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                         wxPoint& aPosition, int aOrientation,
                                         EDA_COLOR_T aColor, GR_DRAWMODE aDrawMode )
{
    wxString    etypeName = GetElectricalTypeName();

    // Use a reasonable (small) size to draw the text
    int         etextSize = (m_nameTextSize*3)/4;

    #define ETXT_MAX_SIZE Millimeter2iu(0.7 )
    if( etextSize > ETXT_MAX_SIZE )
        etextSize = ETXT_MAX_SIZE;

    // Use a reasonable pen size to draw the text
    int pensize = etextSize/6;

    // Get a suitable color
    if( (aColor < 0) && IsSelected() )
        aColor = GetItemSelectedColor();
    else if( !IsVisible() )
        aColor = GetInvisibleItemColor();
    else
        aColor = GetLayerColor( LAYER_NOTES );

    wxPoint txtpos = aPosition;
    int offset = Millimeter2iu( 0.4 );
    EDA_TEXT_HJUSTIFY_T hjustify = GR_TEXT_HJUSTIFY_LEFT;
    int orient = TEXT_ORIENT_HORIZ;

    switch( aOrientation )
    {
    case PIN_UP:
        txtpos.y += offset;
        orient = TEXT_ORIENT_VERT;
        hjustify = GR_TEXT_HJUSTIFY_RIGHT;
        break;

    case PIN_DOWN:
        txtpos.y -= offset;
        orient = TEXT_ORIENT_VERT;
        break;

    case PIN_LEFT:
        txtpos.x += offset;
        break;

    case PIN_RIGHT:
        txtpos.x -= offset;
        hjustify = GR_TEXT_HJUSTIFY_RIGHT;
        break;
    }

    GRSetDrawMode( aDC, aDrawMode );
    EDA_RECT* clipbox = aPanel? aPanel->GetClipBox() : NULL;

    DrawGraphicText( clipbox, aDC, txtpos, aColor, etypeName,
                     orient, wxSize( etextSize, etextSize ),
                     hjustify, GR_TEXT_VJUSTIFY_CENTER, pensize,
                     false, false );
}


void LIB_PIN::PlotSymbol( PLOTTER* aPlotter, const wxPoint& aPosition, int aOrientation )
{
    int         MapX1, MapY1, x1, y1;
    EDA_COLOR_T color = GetLayerColor( LAYER_PIN );

    aPlotter->SetColor( color );
    aPlotter->SetCurrentLineWidth( GetPenSize() );

    MapX1 = MapY1 = 0;
    x1 = aPosition.x; y1 = aPosition.y;

    switch( aOrientation )
    {
    case PIN_UP:
        y1 = aPosition.y - m_length;
        MapY1 = 1;
        break;

    case PIN_DOWN:
        y1 = aPosition.y + m_length;
        MapY1 = -1;
        break;

    case PIN_LEFT:
        x1 = aPosition.x - m_length;
        MapX1 = 1;
        break;

    case PIN_RIGHT:
        x1 = aPosition.x + m_length;
        MapX1 = -1;
        break;
    }

    if( m_shape == PINSHAPE_INVERTED || m_shape == PINSHAPE_INVERTED_CLOCK )
    {
        const int radius = ExternalPinDecoSize( *this );
        aPlotter->Circle( wxPoint( MapX1 * radius + x1,
                                   MapY1 * radius + y1 ),
                          radius * 2,       // diameter
                          NO_FILL,          // fill option
                          GetPenSize() );   // width

        aPlotter->MoveTo( wxPoint( MapX1 * radius * 2 + x1,
                                    MapY1 * radius * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else if( m_shape == PINSHAPE_FALLING_EDGE_CLOCK )
    {
        const int clock_size = InternalPinDecoSize( *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 + clock_size ) );
            aPlotter->LineTo( wxPoint( x1 + MapX1 * clock_size * 2, y1 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 - clock_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1 + clock_size, y1 ) );
            aPlotter->LineTo( wxPoint( x1, y1 + MapY1 * clock_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1 - clock_size, y1 ) );
        }

        aPlotter->MoveTo( wxPoint( MapX1 * clock_size * 2 + x1,
                                    MapY1 * clock_size * 2 + y1 ) );
        aPlotter->FinishTo( aPosition );
    }
    else
    {
        aPlotter->MoveTo( wxPoint( x1, y1 ) );
        aPlotter->FinishTo( aPosition );
    }

    if( m_shape == PINSHAPE_CLOCK || m_shape == PINSHAPE_INVERTED_CLOCK ||
        m_shape == PINSHAPE_CLOCK_LOW )
    {
        const int clock_size = InternalPinDecoSize( *this );
        if( MapY1 == 0 ) /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 + clock_size ) );
            aPlotter->LineTo( wxPoint( x1 - MapX1 * clock_size * 2, y1 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 - clock_size ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1 + clock_size, y1 ) );
            aPlotter->LineTo( wxPoint( x1, y1 - MapY1 * clock_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1 - clock_size, y1 ) );
        }
    }

    if( m_shape == PINSHAPE_INPUT_LOW || m_shape == PINSHAPE_CLOCK_LOW )    /* IEEE symbol "Active Low Input" */
    {
        const int symbol_size = ExternalPinDecoSize( *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1 + MapX1 * symbol_size * 2, y1 ) );
            aPlotter->LineTo( wxPoint( x1 + MapX1 * symbol_size * 2,
                                        y1 - symbol_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 + MapY1 * symbol_size * 2 ) );
            aPlotter->LineTo( wxPoint( x1 - symbol_size * 2,
                                       y1 + MapY1 * symbol_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 ) );
        }
    }


    if( m_shape == PINSHAPE_OUTPUT_LOW )    /* IEEE symbol "Active Low Output" */
    {
        const int symbol_size = ExternalPinDecoSize( *this );

        if( MapY1 == 0 )        /* MapX1 = +- 1 */
        {
            aPlotter->MoveTo( wxPoint( x1, y1 - symbol_size * 2 ) );
            aPlotter->FinishTo( wxPoint( x1 + MapX1 * symbol_size * 2, y1 ) );
        }
        else    /* MapX1 = 0 */
        {
            aPlotter->MoveTo( wxPoint( x1 - symbol_size * 2, y1 ) );
            aPlotter->FinishTo( wxPoint( x1, y1 + MapY1 * symbol_size * 2 ) );
        }
    }
    else if( m_shape == PINSHAPE_NONLOGIC ) /* NonLogic pin symbol */
    {
        const int symbol_size = ExternalPinDecoSize( *this );
        aPlotter->MoveTo( wxPoint( x1 - (MapX1 + MapY1) * symbol_size,
                    y1 - (MapY1 - MapX1) * symbol_size ) );
        aPlotter->FinishTo( wxPoint( x1 + (MapX1 + MapY1) * symbol_size,
                    y1 + (MapY1 - MapX1) * symbol_size ) );
        aPlotter->MoveTo( wxPoint( x1 - (MapX1 - MapY1) * symbol_size,
                    y1 - (MapY1 + MapX1) * symbol_size ) );
        aPlotter->FinishTo( wxPoint( x1 + (MapX1 - MapY1) * symbol_size,
                  y1 + (MapY1 + MapX1) * symbol_size ) );
    }
    if( m_type == PIN_NC )   // Draw a N.C. symbol
    {
        const int ex1 = aPosition.x;
        const int ey1 = aPosition.y;
        aPlotter->MoveTo( wxPoint( ex1 - NCSYMB_PIN_DIM, ey1 - NCSYMB_PIN_DIM ) );
        aPlotter->FinishTo( wxPoint( ex1 + NCSYMB_PIN_DIM, ey1 + NCSYMB_PIN_DIM ) );
        aPlotter->MoveTo( wxPoint( ex1 + NCSYMB_PIN_DIM, ey1 - NCSYMB_PIN_DIM ) );
        aPlotter->FinishTo( wxPoint( ex1 - NCSYMB_PIN_DIM, ey1 + NCSYMB_PIN_DIM ) );
    }
}


void LIB_PIN::PlotPinTexts( PLOTTER* plotter, wxPoint& pin_pos, int  orient,
                            int      TextInside, bool  DrawPinNum,
                            bool     DrawPinName, int  aWidth )
{
    if( m_name.IsEmpty() || m_name == wxT( "~" ) )
        DrawPinName = false;

    /* Create the pin num string */
    wxString    StringPinNum;
    PinStringNum( StringPinNum );

    if( StringPinNum.IsEmpty() )
        DrawPinNum = false;

    if( !DrawPinNum && !DrawPinName )
        return;

    int     x, y;
    wxSize  PinNameSize = wxSize( m_nameTextSize, m_nameTextSize );
    wxSize  PinNumSize  = wxSize( m_numTextSize, m_numTextSize );

    int     nameLineWidth = GetPenSize();
    nameLineWidth = Clamp_Text_PenSize( nameLineWidth, m_nameTextSize, false );
    int     numLineWidth = GetPenSize();
    numLineWidth = Clamp_Text_PenSize( numLineWidth, m_numTextSize, false );

    int     name_offset = PIN_TEXT_MARGIN +
                          ( nameLineWidth + GetDefaultLineThickness() ) / 2;
    int     num_offset = PIN_TEXT_MARGIN +
                         ( numLineWidth + GetDefaultLineThickness() ) / 2;

    /* Get the num and name colors */
    EDA_COLOR_T NameColor = GetLayerColor( LAYER_PINNAM );
    EDA_COLOR_T NumColor  = GetLayerColor( LAYER_PINNUM );

    int x1 = pin_pos.x;
    int y1 = pin_pos.y;

    switch( orient )
    {
    case PIN_UP:
        y1 -= m_length;
        break;

    case PIN_DOWN:
        y1 += m_length;
        break;

    case PIN_LEFT:
        x1 -= m_length;
        break;

    case PIN_RIGHT:
        x1 += m_length;
        break;
    }

    /* Draw the text inside, but the pin numbers outside. */
    if( TextInside )
    {
        if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) ) /* Its an horizontal line. */
        {
            if( DrawPinName )
            {
                if( orient == PIN_RIGHT )
                {
                    x = x1 + TextInside;
                    plotter->Text( wxPoint( x, y1 ), NameColor,
                                   m_name,
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
                        plotter->Text( wxPoint( x, y1 ),
                                       NameColor, m_name, TEXT_ORIENT_HORIZ,
                                       PinNameSize,
                                       GR_TEXT_HJUSTIFY_RIGHT,
                                       GR_TEXT_VJUSTIFY_CENTER,
                                       aWidth, false, false );
                }
            }
            if( DrawPinNum )
            {
                plotter->Text( wxPoint( (x1 + pin_pos.x) / 2,
                                        y1 - num_offset ),
                               NumColor, StringPinNum,
                               TEXT_ORIENT_HORIZ, PinNumSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_BOTTOM,
                               aWidth, false, false );
            }
        }
        else         /* Its a vertical line. */
        {
            if( orient == PIN_DOWN )
            {
                y = y1 + TextInside;

                if( DrawPinName )
                    plotter->Text( wxPoint( x1, y ), NameColor,
                                   m_name,
                                   TEXT_ORIENT_VERT, PinNameSize,
                                   GR_TEXT_HJUSTIFY_RIGHT,
                                   GR_TEXT_VJUSTIFY_CENTER,
                                   aWidth, false, false );

                if( DrawPinNum )
                {
                    plotter->Text( wxPoint( x1 - num_offset,
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
                    plotter->Text( wxPoint( x1, y ), NameColor,
                                   m_name,
                                   TEXT_ORIENT_VERT, PinNameSize,
                                   GR_TEXT_HJUSTIFY_LEFT,
                                   GR_TEXT_VJUSTIFY_CENTER,
                                   aWidth, false, false );

                if( DrawPinNum )
                {
                    plotter->Text( wxPoint( x1 - num_offset,
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
                plotter->Text( wxPoint( x, y1 - name_offset ),
                               NameColor, m_name,
                               TEXT_ORIENT_HORIZ, PinNameSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_BOTTOM,
                               aWidth, false, false );
            }

            if( DrawPinNum )
            {
                x = ( x1 + pin_pos.x ) / 2;
                plotter->Text( wxPoint( x, y1 + num_offset ),
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
                y = ( y1 + pin_pos.y ) / 2;
                plotter->Text( wxPoint( x1 - name_offset, y ),
                               NameColor, m_name,
                               TEXT_ORIENT_VERT, PinNameSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_BOTTOM,
                               aWidth, false, false );
            }

            if( DrawPinNum )
            {
                plotter->Text( wxPoint( x1 + num_offset,
                                        ( y1 + pin_pos.y ) / 2 ),
                               NumColor, StringPinNum,
                               TEXT_ORIENT_VERT, PinNumSize,
                               GR_TEXT_HJUSTIFY_CENTER,
                               GR_TEXT_VJUSTIFY_TOP,
                               aWidth, false, false );
            }
        }
    }
}


wxPoint LIB_PIN::PinEndPoint() const
{
    wxPoint pos = m_position;

    switch( m_orientation )
    {
    case PIN_UP:
        pos.y += m_length;
        break;

    case PIN_DOWN:
        pos.y -= m_length;
        break;

    case PIN_LEFT:
        pos.x -= m_length;
        break;

    case PIN_RIGHT:
        pos.x += m_length;
        break;
    }

    return pos;
}


int LIB_PIN::PinDrawOrient( const TRANSFORM& aTransform ) const
{
    int     orient;
    wxPoint end;   // position of pin end starting at 0,0 according to its orientation, length = 1

    switch( m_orientation )
    {
    case PIN_UP:
        end.y = 1;
        break;

    case PIN_DOWN:
        end.y = -1;
        break;

    case PIN_LEFT:
        end.x = -1;
        break;

    case PIN_RIGHT:
        end.x = 1;
        break;
    }

    // = pos of end point, according to the component orientation
    end    = aTransform.TransformCoordinate( end );
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


void LIB_PIN::PinStringNum( wxString& aStringBuffer ) const
{
    aStringBuffer = PinStringNum( m_number );
}


wxString LIB_PIN::PinStringNum( long aPinNum )
{
    char ascii_buf[5];

    memcpy( ascii_buf, &aPinNum, 4 );
    ascii_buf[4] = 0;

    wxString buffer = FROM_UTF8( ascii_buf );

    return buffer;
}


void LIB_PIN::SetPinNumFromString( wxString& buffer )
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

    strncpy( (char*) &m_number, ascii_buf, 4 );
}


EDA_ITEM* LIB_PIN::Clone() const
{
    return new LIB_PIN( *this );
}


int LIB_PIN::compare( const LIB_ITEM& other ) const
{
    wxASSERT( other.Type() == LIB_PIN_T );

    const LIB_PIN* tmp = (LIB_PIN*) &other;

    if( m_number != tmp->m_number )
        return m_number - tmp->m_number;

    int result = m_name.CmpNoCase( tmp->m_name );

    if( result != 0 )
        return result;

    if( m_position.x != tmp->m_position.x )
        return m_position.x - tmp->m_position.x;

    if( m_position.y != tmp->m_position.y )
        return m_position.y - tmp->m_position.y;

    return 0;
}


void LIB_PIN::SetOffset( const wxPoint& aOffset )
{
    m_position += aOffset;
}


bool LIB_PIN::Inside( EDA_RECT& rect ) const
{
    wxPoint end = PinEndPoint();

    return rect.Contains( m_position.x, -m_position.y ) || rect.Contains( end.x, -end.y );
}


void LIB_PIN::Move( const wxPoint& newPosition )
{
    if( m_position != newPosition )
    {
        m_position = newPosition;
        SetModified();
    }
}


void LIB_PIN::MirrorHorizontal( const wxPoint& center )
{
    m_position.x -= center.x;
    m_position.x *= -1;
    m_position.x += center.x;

    if( m_orientation == PIN_RIGHT )
        m_orientation = PIN_LEFT;
    else if( m_orientation == PIN_LEFT )
        m_orientation = PIN_RIGHT;
}

void LIB_PIN::MirrorVertical( const wxPoint& center )
{
    m_position.y -= center.y;
    m_position.y *= -1;
    m_position.y += center.y;

    if( m_orientation == PIN_UP )
        m_orientation = PIN_DOWN;
    else if( m_orientation == PIN_DOWN )
        m_orientation = PIN_UP;
}

void LIB_PIN::Rotate( const wxPoint& center, bool aRotateCCW )
{
    int rot_angle = aRotateCCW ? -900 : 900;

    RotatePoint( &m_position, center, rot_angle );

    if( aRotateCCW )
    {
        switch( m_orientation )
        {
            case PIN_RIGHT:
                m_orientation = PIN_UP;
                break;

            case PIN_UP:
                m_orientation = PIN_LEFT;
                break;
            case PIN_LEFT:
                m_orientation = PIN_DOWN;
                break;

            case PIN_DOWN:
                m_orientation = PIN_RIGHT;
                break;
        }
    }
    else
    {
        switch( m_orientation )
        {
            case PIN_RIGHT:
                m_orientation = PIN_DOWN;
                break;

            case PIN_UP:
                m_orientation = PIN_RIGHT;
                break;
            case PIN_LEFT:
                m_orientation = PIN_UP;
                break;

            case PIN_DOWN:
                m_orientation = PIN_LEFT;
                break;
        }
    }
}


void LIB_PIN::Plot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                    const TRANSFORM& aTransform )
{
    if( ! IsVisible() )
        return;

    int     orient = PinDrawOrient( aTransform );

    wxPoint pos = aTransform.TransformCoordinate( m_position ) + offset;

    PlotSymbol( plotter, pos, orient );
    PlotPinTexts( plotter, pos, orient, GetParent()->GetPinNameOffset(),
                  GetParent()->ShowPinNumbers(), GetParent()->ShowPinNames(),
                  GetPenSize() );
}


void LIB_PIN::SetWidth( int aWidth )
{
    if( m_width != aWidth )
    {
        m_width = aWidth;
        SetModified();
    }
}


void LIB_PIN::getMsgPanelInfoBase( MSG_PANEL_ITEMS& aList )
{
    wxString text;

    LIB_ITEM::GetMsgPanelInfo( aList );

    aList.push_back( MSG_PANEL_ITEM( _( "Name" ), m_name, DARKCYAN ) );

    if( m_number == 0 )
        text = wxT( "?" );
    else
        PinStringNum( text );

    aList.push_back( MSG_PANEL_ITEM( _( "Number" ), text, DARKCYAN ) );

    aList.push_back( MSG_PANEL_ITEM( _( "Type" ),
                                     GetText( m_type ),
                                     RED ) );

    text = GetText( m_shape );

    aList.push_back( MSG_PANEL_ITEM( _( "Style" ), text, BLUE ) );

    if( IsVisible() )
        text = _( "Yes" );
    else
        text = _( "No" );

    aList.push_back( MSG_PANEL_ITEM( _( "Visible" ), text, DARKGREEN ) );

    // Display pin length
    text = StringFromValue( g_UserUnit, m_length, true );
    aList.push_back( MSG_PANEL_ITEM( _( "Length" ), text, MAGENTA ) );

    text = getPinOrientationName( (unsigned) GetOrientationCodeIndex( m_orientation ) );
    aList.push_back( MSG_PANEL_ITEM( _( "Orientation" ), text, DARKMAGENTA ) );
}

void LIB_PIN::GetMsgPanelInfo( MSG_PANEL_ITEMS& aList )
{
    getMsgPanelInfoBase( aList );

    wxString text;
    wxPoint pinpos = GetPosition();
    pinpos.y = -pinpos.y;   // Display coord are top to bottom
                            // lib items coord are bottom to top

    text = StringFromValue( g_UserUnit, pinpos.x, true );
    aList.push_back( MSG_PANEL_ITEM( _( "Pos X" ), text, DARKMAGENTA ) );

    text = StringFromValue( g_UserUnit, pinpos.y, true );
    aList.push_back( MSG_PANEL_ITEM( _( "Pos Y" ), text, DARKMAGENTA ) );
}

void LIB_PIN::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList,
                               SCH_COMPONENT* aComponent )
{
    getMsgPanelInfoBase( aList );

    if( !aComponent )
        return;

    wxString text;
    wxPoint pinpos = aComponent->GetTransform().TransformCoordinate( GetPosition() )
                     + aComponent->GetPosition();

    text = StringFromValue( g_UserUnit, pinpos.x, true );
    aList.push_back( MSG_PANEL_ITEM( _( "Pos X" ), text, DARKMAGENTA ) );

    text = StringFromValue( g_UserUnit, pinpos.y, true );
    aList.push_back( MSG_PANEL_ITEM( _( "Pos Y" ), text, DARKMAGENTA ) );

    aList.push_back( MSG_PANEL_ITEM( aComponent->GetField( REFERENCE )->GetShownText(),
                                     aComponent->GetField( VALUE )->GetShownText(),
                                     DARKCYAN ) );
}

const EDA_RECT LIB_PIN::GetBoundingBox( bool aIncludeInvisibles ) const
{
    LIB_PART*      entry = (LIB_PART*     ) m_Parent;
    EDA_RECT       bbox;
    wxPoint        begin;
    wxPoint        end;
    int            nameTextOffset = 0;
    bool           showName = !m_name.IsEmpty() && (m_name != wxT( "~" ));
    bool           showNum = m_number != 0;
    int            minsizeV = TARGET_PIN_RADIUS;

    if( !aIncludeInvisibles && !IsVisible() )
        showName = false;

    if( entry )
    {
        if( entry->ShowPinNames() )
            nameTextOffset = entry->GetPinNameOffset();
        else
            showName = false;

        showNum = entry->ShowPinNumbers();
    }

    // First, calculate boundary box corners position
    int numberTextLength = showNum ? m_numTextSize * GetNumberString().Len() : 0;

    // Actual text height is bigger than text size
    int numberTextHeight  = showNum ? KiROUND( m_numTextSize * 1.1 ) : 0;

    if( m_shape == PINSHAPE_INVERTED || m_shape == PINSHAPE_INVERTED_CLOCK )
        minsizeV = std::max( TARGET_PIN_RADIUS, ExternalPinDecoSize( *this ) );

    // calculate top left corner position
    // for the default pin orientation (PIN_RIGHT)
    begin.y = std::max( minsizeV, numberTextHeight + PIN_TEXT_MARGIN );
    begin.x = std::min( -TARGET_PIN_RADIUS, m_length - (numberTextLength / 2) );

    // calculate bottom right corner position and adjust top left corner position
    int nameTextLength = 0;
    int nameTextHeight = 0;

    if( showName )
    {
        int length = m_name.Len();

        // Don't count the line over text symbol.
        if( m_name.Left( 1 ) == wxT( "~" ) )
            length -= 1;

        nameTextLength = ( m_nameTextSize * length ) + nameTextOffset;

        // Actual text height are bigger than text size
        nameTextHeight = KiROUND( m_nameTextSize * 1.1 ) + PIN_TEXT_MARGIN;
    }

    if( nameTextOffset )        // for values > 0, pin name is inside the body
    {
        end.x = m_length + nameTextLength;
        end.y = std::min( -minsizeV, -nameTextHeight / 2 );
    }
    else        // if value == 0:
                // pin name is outside the body, and above the pin line
                // pin num is below the pin line
    {
        end.x = std::max(m_length, nameTextLength);
        end.y = -begin.y;
        begin.y = std::max( minsizeV, nameTextHeight );
    }

    // Now, calculate boundary box corners position for the actual pin orientation
    int orient = PinDrawOrient( DefaultTransform );

    /* Calculate the pin position */
    switch( orient )
    {
    case PIN_UP:
        // Pin is rotated and texts positions are mirrored
        RotatePoint( &begin, wxPoint( 0, 0 ), -900 );
        RotatePoint( &end, wxPoint( 0, 0 ), -900 );
        break;

    case PIN_DOWN:
        RotatePoint( &begin, wxPoint( 0, 0 ), 900 );
        RotatePoint( &end, wxPoint( 0, 0 ), 900 );
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_LEFT:
        begin.x = -begin.x;
        end.x = -end.x;
        break;

    case PIN_RIGHT:
        break;
    }

    begin += m_position;
    end += m_position;

    bbox.SetOrigin( begin );
    bbox.SetEnd( end );
    bbox.Normalize();
    bbox.Inflate( ( GetPenSize() / 2 ) + 1 );

    // Draw Y axis is reversed in schematic:
    bbox.RevertYAxis();

    return bbox;
}


wxArrayString LIB_PIN::GetOrientationNames( void )
{
    wxArrayString tmp;

    for( unsigned ii = 0; ii < PIN_ORIENTATION_CNT; ii++ )
        tmp.Add( getPinOrientationName( ii ) );

    return tmp;
}


int LIB_PIN::GetOrientationCode( int index )
{
    if( index >= 0 && index < (int) PIN_ORIENTATION_CNT )
        return pin_orientation_codes[ index ];

    return PIN_RIGHT;
}


int LIB_PIN::GetOrientationCodeIndex( int code )
{
    size_t i;

    for( i = 0; i < PIN_ORIENTATION_CNT; i++ )
    {
        if( pin_orientation_codes[i] == code )
            return (int) i;
    }

    return wxNOT_FOUND;
}


void LIB_PIN::Rotate()
{
    int orient = PIN_RIGHT;

    switch( GetOrientation() )
    {
        case PIN_UP:
            orient = PIN_LEFT;
            break;

        case PIN_DOWN:
            orient = PIN_RIGHT;
            break;

        case PIN_LEFT:
            orient = PIN_DOWN;
           break;

        case PIN_RIGHT:
            orient = PIN_UP;
            break;
    }

    // Set the new orientation
    SetOrientation( orient );
}


const BITMAP_DEF* LIB_PIN::GetOrientationSymbols()
{
    return iconsPinsOrientations;
}


BITMAP_DEF LIB_PIN::GetMenuImage() const
{
    return GetBitmap( m_type );
}


wxString LIB_PIN::GetSelectMenuText() const
{
    wxString tmp;
    wxString style;

    style = GetText( m_shape );

    tmp.Printf( _( "Pin %s, %s, %s" ),
                GetChars( GetNumberString() ),
                GetChars( GetElectricalTypeName() ),
                GetChars( style )
                );
    return tmp;
}


bool LIB_PIN::Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation )
{
    wxLogTrace( traceFindItem, wxT( "  item " ) + GetSelectMenuText() );

    // Note: this will have to be modified if we add find and replace capability to the
    // compoment library editor.  Otherwise, you wont be able to replace pin text.
    if( !( aSearchData.GetFlags() & FR_SEARCH_ALL_PINS )
        || ( aSearchData.GetFlags() & FR_SEARCH_REPLACE ) )
        return false;

    wxLogTrace( traceFindItem, wxT( "    child item " ) + GetSelectMenuText() );

    if( EDA_ITEM::Matches( GetName(), aSearchData )
        || EDA_ITEM::Matches( GetNumberString(), aSearchData ) )
    {
        if( aFindLocation )
            *aFindLocation =  GetBoundingBox().Centre();

        return true;
    }

    return false;
}


#if defined(DEBUG)

void LIB_PIN::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " num=\"" << GetNumberString().mb_str()
                                 << '"' << "/>\n";

//    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
