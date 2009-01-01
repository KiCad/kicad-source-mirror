/**********************************************************/
/*	libclass.cpp										  */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/***************************/
/* class LibraryFieldEntry */
/***************************/

/* a Field is a string linked to a component.
 *  Unlike a pure graphic text, fields can be used in netlist generation
 *  and other told (BOM).
 *
 *  4 fields have a special meaning:
 *      REFERENCE
 *      VALUE
 *      FOOTPRINT NAME
 *      DOCUMENTATION LINK (reserved but not used in kicad)
 */
LibDrawField::LibDrawField( int idfield ) : LibEDA_BaseStruct( COMPONENT_FIELD_DRAW_TYPE )
{
    m_FieldId = idfield;                /*  0 = REFERENCE
                                         *  1 = VALUE
                                         *  3 = FOOTPRINT (default Footprint)
                                         *  4 = DOCUMENTATION (user doc link)
                                         *  others = free fields
                                         */
    m_Size.x  = m_Size.y = DEFAULT_SIZE_TEXT;
}


LibDrawField::~LibDrawField()
{
}


// Creation et Duplication d'un field
LibDrawField* LibDrawField::GenCopy()
{
    LibDrawField* newfield = new LibDrawField( m_FieldId );

    Copy( newfield );

    return newfield;
}


/** Function Copy
 * copy parameters of this to Target. Pointers are not copied
 * @param Target = the LibDrawField to set with "this" values
 */
void LibDrawField::Copy( LibDrawField* Target ) const
{
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
}


/**************************************************/
bool LibDrawField::Save( FILE* ExportFile ) const
/**************************************************/
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
    fprintf( ExportFile, "F%d \"%s\" %d %d %d %c %c %c %c%c%c",
             m_FieldId, CONV_TO_UTF8( text ),
             m_Pos.x, m_Pos.y,
             m_Size.x,
             m_Orient == 0 ? 'H' : 'V',
             (m_Attributs & TEXT_NO_VISIBLE ) ? 'I' : 'V',
             hjustify, vjustify,
             m_Italic ? 'I' : 'N',
             m_Width > 1 ? 'B' : 'N' );

    /* Save field name, if necessary
     * Field name is saved only if it is not the default name.
     * Just because default name depends on the language and can change from a country to an other
     */
    if( m_FieldId >= FIELD1 && !m_Name.IsEmpty() && m_Name != ReturnDefaultFieldName( m_FieldId ))
        fprintf( ExportFile, " \"%s\"", CONV_TO_UTF8( m_Name ) );

    fprintf( ExportFile, "\n" );
    return true;
}


/****************************************************************/
wxString ReturnDefaultFieldName( int aFieldNdx )
/****************************************************************/

/** Function ReturnDefaultFieldName
 * Return the default field name from its index (REFERENCE, VALUE ..)
 * FieldDefaultNameList is not static, because we want the text translation for I18n
 * @param aFieldNdx = Filed number (>= 0)
 */
{
    // avoid unnecessarily copying wxStrings at runtime.
    static const wxString defaults[] = {
        _( "Reference" ),   // Reference of part, i.e. "IC21"
        _( "Value" ),       // Value of part, i.e. "3.3K" and name in lib for lib entries
        _( "Footprint" ),   // Footprint, used by cvpcb or pcbnew, i.e. "16DIP300"
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

