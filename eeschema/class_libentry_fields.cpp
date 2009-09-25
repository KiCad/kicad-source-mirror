/**********************************************************/
/*  libclass.cpp                                          */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "base_struct.h"
#include "drawtxt.h"
#include "kicad_string.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_libentry.h"

#include <wx/tokenzr.h>
#include <wx/stream.h>
#include <wx/txtstrm.h>


/***************************/
/* class LibraryFieldEntry */
/***************************/

/**
 * a Field is a string linked to a component.
 *  Unlike a pure graphic text, fields can be used in netlist generation
 *  and other tools (BOM).
 *
 *  The first 4 fields have a special meaning:
 *
 *  0 = REFERENCE
 *  1 = VALUE
 *  3 = FOOTPRINT (default Footprint)
 *  4 = DOCUMENTATION (user doc link)
 *
 *  others = free fields
 */
LibDrawField::LibDrawField(LIB_COMPONENT * aParent, int idfield ) :
    LIB_DRAW_ITEM( COMPONENT_FIELD_DRAW_TYPE, aParent )
{
    m_FieldId = idfield;
    m_Size.x = m_Size.y = DEFAULT_SIZE_TEXT;
}

LibDrawField::LibDrawField( int idfield ) :
    LIB_DRAW_ITEM( COMPONENT_FIELD_DRAW_TYPE, NULL )
{
    m_FieldId = idfield;
    m_Size.x = m_Size.y = DEFAULT_SIZE_TEXT;
}

LibDrawField::LibDrawField( const LibDrawField& field ) :
    LIB_DRAW_ITEM( field )
{
    m_Pos       = field.m_Pos;
    m_Size      = field.m_Size;
    m_Width     = field.m_Width;
    m_Orient    = field.m_Orient;
    m_Attributs = field.m_Attributs;
    m_Text      = field.m_Text;
    m_Name      = field.m_Name;
    m_HJustify  = field.m_HJustify;
    m_VJustify  = field.m_VJustify;
    m_Italic    = field.m_Italic;
    m_Bold      = field.m_Bold;
}


LibDrawField::~LibDrawField()
{
}


bool LibDrawField::Save( FILE* ExportFile ) const
{
    int      hjustify, vjustify;
    wxString text = m_Text;

    hjustify = 'C';
    if( m_HJustify == GR_TEXT_HJUSTIFY_LEFT )
        hjustify = 'L';
    else if( m_HJustify == GR_TEXT_HJUSTIFY_RIGHT )
        hjustify = 'R';
    vjustify = 'C';
    if( m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM )
        vjustify = 'B';
    else if( m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        vjustify = 'T';
    if( text.IsEmpty() )
        text = wxT( "~" );
    if( fprintf( ExportFile, "F%d \"%s\" %d %d %d %c %c %c %c%c%c",
                 m_FieldId, CONV_TO_UTF8( text ), m_Pos.x, m_Pos.y, m_Size.x,
                 m_Orient == 0 ? 'H' : 'V',
                 (m_Attributs & TEXT_NO_VISIBLE ) ? 'I' : 'V',
                 hjustify, vjustify,
                 m_Italic ? 'I' : 'N',
                 m_Bold ? 'B' : 'N' ) < 0 )
        return false;

    /* Save field name, if necessary
     * Field name is saved only if it is not the default name.
     * Just because default name depends on the language and can change from
     * a country to an other
     */
    if( m_FieldId >= FIELD1 && !m_Name.IsEmpty()
        && m_Name != ReturnDefaultFieldName( m_FieldId )
        && fprintf( ExportFile, " \"%s\"", CONV_TO_UTF8( m_Name ) ) < 0 )
        return false;

    if( fprintf( ExportFile, "\n" ) < 0 )
        return false;

    return true;
}


bool LibDrawField::Load( char* line, wxString& errorMsg )
{
    int   cnt;
    char  textOrient;
    char  textVisible;
    char  textHJustify;
    char  textVJustify[256];
    char  fieldUserName[1024];
    char* text;

    if( sscanf( line + 1, "%d", &m_FieldId ) != 1
        || m_FieldId < REFERENCE || m_FieldId >= NUMBER_OF_FIELDS )
    {
        errorMsg = _( "invalid field number defined" );
        return false;
    }

    /* Recherche du debut des donnees (debut du texte suivant) */
    while( *line != 0 )
        line++;

    while( *line == 0 )
        line++;

    /* recherche du texte */
    while( *line && (*line != '"') )
        line++;

    if( *line == 0 )
        return false;
    line++;

    text = line;

    /* recherche fin de texte */
    while( *line && (*line != '"') )
        line++;

    if( *line == 0 )
        return false;

    *line = 0;
    line++;

    fieldUserName[0] = 0;
    memset( textVJustify, 0, sizeof( textVJustify ) );

    cnt = sscanf( line, " %d %d %d %c %c %c %s", &m_Pos.x, &m_Pos.y, &m_Size.y,
                  &textOrient, &textVisible, &textHJustify, textVJustify );

    if( cnt < 5 )
    {
        errorMsg.Printf( _( "field %d does not have the correct number of \
parameters" ),
                         m_FieldId );
        return false;
    }

    m_Text = CONV_FROM_UTF8( text );
    m_Size.x = m_Size.y;

    if( textOrient == 'H' )
        m_Orient = TEXT_ORIENT_HORIZ;
    else if( textOrient == 'V' )
        m_Orient = TEXT_ORIENT_VERT;
    else
    {
        errorMsg.Printf( _( "field %d text orientation parameter <%c> is \
not valid" ),
                         textOrient );
        return false;
    }

    if( textVisible == 'V' )
        m_Attributs &= ~TEXT_NO_VISIBLE;
    else if ( textVisible == 'I' )
        m_Attributs |= TEXT_NO_VISIBLE;
    else
    {
        errorMsg.Printf( _( "field %d text visible parameter <%c> is not \
valid" ),
                         textVisible );
        return false;
    }

    m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
    m_VJustify = GR_TEXT_VJUSTIFY_CENTER;

    if( cnt >= 6 )
    {
        if( textHJustify == 'C' )
            m_HJustify = GR_TEXT_HJUSTIFY_CENTER;
        else if( textHJustify == 'L' )
            m_HJustify = GR_TEXT_HJUSTIFY_LEFT;
        else if( textHJustify == 'R' )
            m_HJustify = GR_TEXT_HJUSTIFY_RIGHT;
        else
        {
            errorMsg.Printf( _( "field %d text horizontal justification \
parameter <%c> is not valid" ),
                             textHJustify );
            return false;
        }

        if( textVJustify[0] == 'C' )
            m_VJustify = GR_TEXT_VJUSTIFY_CENTER;
        else if( textVJustify[0] == 'B' )
            m_VJustify = GR_TEXT_VJUSTIFY_BOTTOM;
        else if( textVJustify[0] == 'T' )
            m_VJustify = GR_TEXT_VJUSTIFY_TOP;
        else
        {
            errorMsg.Printf( _( "field %d text vertical justification \
parameter <%c> is not valid" ),
                             textVJustify[0] );
            return false;
        }

        if ( textVJustify[1] == 'I' )  // Italic
            m_Italic = true;
        if ( textVJustify[2] == 'B' )  // Bold
            m_Bold = true;
    }

    if( m_FieldId >= FIELD1 )
    {
        ReadDelimitedText( fieldUserName, line, sizeof( fieldUserName ) );
        m_Name = CONV_FROM_UTF8( fieldUserName );
    }

    return true;
}



/** Function GetPenSize
 * @return the size of the "pen" that be used to draw or plot this item
 */
int LibDrawField::GetPenSize()
{
    return ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;
}


/*
 * if aData not NULL, aData must point a wxString which is used instead of
 * the m_Text
 */
void LibDrawField::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC,
                         const wxPoint& aOffset, int aColor, int aDrawMode,
                         void* aData, const int aTransformMatrix[2][2] )
{
    wxPoint  text_pos;
    int      color;
    int      linewidth = GetPenSize();

    linewidth = Clamp_Text_PenSize( linewidth, m_Size, m_Bold );

    if( ( m_Attributs & TEXT_NO_VISIBLE ) && ( aColor < 0 ) )
    {
        color = g_InvisibleItemColor;
    }
    else if( ( m_Selected & IS_SELECTED ) && ( aColor < 0 ) )
    {
        color = g_ItemSelectetColor;
    }
    else
    {
        color = aColor;
    }

    if( color < 0 )
    {
        switch( m_FieldId )
        {
        case REFERENCE:
            color = ReturnLayerColor( LAYER_REFERENCEPART );
            break;

        case VALUE:
            color = ReturnLayerColor( LAYER_VALUEPART );
            break;

        default:
            color = ReturnLayerColor( LAYER_FIELDS );
            break;
        }
    }

    text_pos = TransformCoordinate( aTransformMatrix, m_Pos ) + aOffset;
    wxString* text = aData ? (wxString*)aData : &m_Text;
    GRSetDrawMode( aDC, aDrawMode );
    DrawGraphicText( aPanel, aDC, text_pos, (EDA_Colors) color, *text,
                     m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                     m_Size, m_HJustify, m_VJustify, linewidth, m_Italic,
                     m_Bold );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param refPos A wxPoint to test, in Field coordinate system
 * @return bool - true if a hit, else false
 */
bool LibDrawField::HitTest( const wxPoint& refPos )
{
    return HitTest( refPos, 0, DefaultTransformMatrix );
}

 /** Function HitTest
 * @return true if the point aPosRef is near this object
 * @param aPosRef = a wxPoint to test
 * @param aThreshold =  unused here (TextHitTest calculates its threshold )
 * @param aTransMat = the transform matrix
 */
bool LibDrawField::HitTest( wxPoint aPosRef, int aThreshold,
                            const int aTransMat[2][2] )
{
    int extraCharCount = 0;
    // Reference designator text has one or 2 additional character (displays
    // U? or U?A)
    if( m_FieldId == REFERENCE )
    {
        extraCharCount++;
        m_Text.Append('?');
        LIB_COMPONENT* parent = (LIB_COMPONENT*)m_Parent;
        if ( parent && ( parent->m_UnitCount > 1 ) )
        {
            m_Text.Append('A');
            extraCharCount++;
        }
    }

    wxPoint physicalpos = TransformCoordinate( aTransMat, m_Pos );
    wxPoint tmp = m_Pos;
    m_Pos = physicalpos;
    /* The text orientation may need to be flipped if the
     *  transformation matrix causes xy axes to be flipped.
     * this simple algo works only for schematic matrix (rot 90 or/and mirror)
    */
    int t1 = ( aTransMat[0][0] != 0 ) ^ ( m_Orient != 0 );
    int orient = t1 ? TEXT_ORIENT_HORIZ : TEXT_ORIENT_VERT;
    EXCHG( m_Orient, orient );

    bool hit = TextHitTest(aPosRef);

    EXCHG( m_Orient, orient );
    m_Pos = tmp;

    while( extraCharCount-- )
        m_Text.RemoveLast( );

    return hit;
}

// Creation et Duplication d'un field
LIB_DRAW_ITEM* LibDrawField::DoGenCopy()
{
    LibDrawField* newfield = new LibDrawField( m_FieldId );

    Copy( newfield );

    return (LIB_DRAW_ITEM*) newfield;
}


/** Function Copy
 * copy parameters of this to Target. Pointers are not copied
 * @param Target = the LibDrawField to set with "this" values
 */
void LibDrawField::Copy( LibDrawField* Target ) const
{
    Target->SetParent( m_Parent );
    Target->m_Pos       = m_Pos;
    Target->m_Size      = m_Size;
    Target->m_Width     = m_Width;
    Target->m_Orient    = m_Orient;
    Target->m_Attributs = m_Attributs;
    Target->m_Text      = m_Text;
    Target->m_Name      = m_Name;
    Target->m_HJustify  = m_HJustify;
    Target->m_VJustify  = m_VJustify;
    Target->m_Italic    = m_Italic;
    Target->m_Bold      = m_Bold;
}


bool LibDrawField::DoCompare( const LIB_DRAW_ITEM& other ) const
{
    wxASSERT( other.Type() == COMPONENT_FIELD_DRAW_TYPE );

    const LibDrawField* tmp = ( LibDrawField* ) &other;

    return ( ( m_FieldId == tmp->m_FieldId ) && ( m_Text == tmp->m_Text )
             && ( m_Pos == tmp->m_Pos ) && ( m_Size == tmp->m_Size ) );
}


void LibDrawField::DoOffset( const wxPoint& offset )
{
    m_Pos += offset;
}


bool LibDrawField::DoTestInside( EDA_Rect& rect )
{
    /*
     * FIXME: This fails to take into acount the size and/or orientation of
     *        the text.
     */
    return rect.Inside( m_Pos.x, -m_Pos.y );
}


void LibDrawField::DoMove( const wxPoint& newPosition )
{
    m_Pos = newPosition;
}


/*
 * If the field is the reference, return reference like schematic,
 * i.e U -> U? or U?A or the field text for others
 *
 * @fixme This should be handled by the field object.
 */
wxString LibDrawField::GetFullText( int unit )
{
    if( m_FieldId != REFERENCE )
        return m_Text;

    wxString text = m_Text;

    if( GetParent()->m_UnitCount > 1 )
    {
#if defined(KICAD_GOST)
        text.Printf( wxT( "%s?.%c" ),
                     m_Text.GetData(), unit + '1' - 1 );
#else

        text.Printf( wxT( "%s?%c" ),
                     m_Text.GetData(), unit + 'A' - 1 );
#endif
    }
    else
        text << wxT( "?" );

    return text;
}


/**
 * Function ReturnDefaultFieldName
 * Return the default field name from its index (REFERENCE, VALUE ..)
 * FieldDefaultNameList is not static, because we want the text translation
 * for I18n
 * @param aFieldNdx = Filed number (>= 0)
 */
wxString ReturnDefaultFieldName( int aFieldNdx )
{
    // avoid unnecessarily copying wxStrings at runtime.
    static const wxString defaults[] = {
        _( "Reference" ),   // Reference of part, i.e. "IC21"
        _( "Value" ),       // Value of part, i.e. "3.3K" and name in lib
                            // for lib entries
        _( "Footprint" ),   // Footprint, used by cvpcb or pcbnew, i.e.
                            // "16DIP300"
        _( "Datasheet" ),   // A link to an user document, if wanted
    };

    if( (unsigned) aFieldNdx <= DATASHEET )
        return defaults[ aFieldNdx ];

    else
    {
        wxString ret = _( "Field" );
        ret << ( aFieldNdx - FIELD1 + 1);
        return ret;
    }
}
