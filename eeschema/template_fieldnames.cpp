

#include "template_fieldnames.h"
#include "dsnlexer.h"
#include "macros.h"

using namespace DSN;    // enum TFIELD_T is in this namespace


wxString TEMPLATE_FIELDNAME::GetDefaultFieldName( int aFieldNdx )
{
    // Fixed values for the first few default fields used by EESCHEMA
    static const wxString fixedNames[] = {
        _( "Reference" ),   // The component reference, R1, C1, etc.
        _( "Value" ),       // The component value + name
        _( "Footprint" ),   // The footprint for use with PCBNEW
        _( "Datasheet" ),   // Link to a datasheet for component
    };

    if ( (unsigned) aFieldNdx <  DIM(fixedNames) )
        return fixedNames[aFieldNdx];

    else
    {
        wxString fieldName = _("Field");

        fieldName << aFieldNdx;

        return fieldName;
    }
}

void TEMPLATE_FIELDNAME::Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR )
{
    // user may want spaces in his field name, ug, so quote them for the parser.
    out->Print( nestLevel, "(field (name \"%s\")", CONV_TO_UTF8(m_Name) );

    if( !m_Value.IsEmpty() )
        out->Print( 0, "(value \"%s\")", CONV_TO_UTF8(m_Value) );

    if( m_Visible )
        out->Print( 0, " visible" );

    out->Print( 0, ")\n" );
}


void TEMPLATE_FIELDNAME::Parse( TEMPLATE_FIELDNAMES_LEXER* in ) throw( IO_ERROR )
{
    TFIELD_T    tok;

    in->NeedLEFT();     // begin (name ...)

    if( (tok = in->NextTok()) != T_name )
        in->Expecting( T_name );

    in->NeedSYMBOLorNUMBER();

    m_Name = CONV_FROM_UTF8( in->CurText() );

    in->NeedRIGHT();    // end (name ...)

    while( (tok = in->NextTok() ) != T_RIGHT && tok != T_EOF )
    {
        // "visible" has no '(' prefix, "value" does, so T_LEFT is optional.
        if( tok == T_LEFT )
            tok = in->NextTok();

        switch( tok )
        {
        case T_value:
            in->NeedSYMBOLorNUMBER();
            m_Value = CONV_FROM_UTF8( in->CurText() );
            in->NeedRIGHT();
            break;

        case T_visible:
            m_Visible = true;
            break;

        default:
            in->Expecting( wxT( "value|visible" ) );
            break;
        }
    }
}


void TEMPLATES::Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR )
{
    // We'll keep this general, and include the \n, even though the only known
    // use at this time will not want the newlines or the indentation.
    out->Print( nestLevel, "(templatefields" );
    for( unsigned i=0;  i<m_Fields.size();  ++i )
        m_Fields[i].Format( out, nestLevel+1 );
    out->Print( 0, ")\n" );
}

void TEMPLATES::Parse( TEMPLATE_FIELDNAMES_LEXER* in ) throw( IO_ERROR )
{
    TFIELD_T        tok;

    while( (tok = in->NextTok() ) != T_RIGHT && tok != T_EOF )
    {
        if( tok == T_LEFT )
            tok = in->NextTok();

        switch( tok )
        {
        case T_templatefields:  // a token indicating class TEMPLATES.

            // Be flexible regarding the starting point of the TEMPLATE_FIELDNAMES_LEXER
            // stream.  Caller may not have read the first two tokens out of the
            // stream: T_LEFT and T_templatefields, so ignore them if seen here.
            break;

        case T_field:
            {
                // instantiate on stack, so if exception is thrown,
                // destructor runs
                TEMPLATE_FIELDNAME  field;

                field.Parse( in );

                // add the field
                AddTemplateFieldName( field );
            }
            break;

        default:
            in->Unexpected( CONV_FROM_UTF8( in->CurText() ) );
            break;
        }
    }

    D(printf("tok:%d\n", tok);)
}


int TEMPLATES::AddTemplateFieldName( const TEMPLATE_FIELDNAME& aFieldName )
{
    // Ensure that the template fieldname does not match a fixed fieldname.
    for( int i=0;  i<MANDATORY_FIELDS;  ++i )
    {
        if( TEMPLATE_FIELDNAME::GetDefaultFieldName(i) == aFieldName.m_Name )
        {
            return -1;
        }
    }

    // ensure uniqueness, overwrite any template fieldname by the same name.
    for( unsigned i=0; i<m_Fields.size();  ++i )
    {
        if( m_Fields[i].m_Name == aFieldName.m_Name )
        {
            D(printf("inserting template fieldname:'%s' at %d\n",
                CONV_TO_UTF8(aFieldName.m_Name), i );)

            m_Fields[i] = aFieldName;
            return i;   // return the container index
        }
    }

    // D(printf("appending template fieldname:'%s'\n", CONV_TO_UTF8(aFieldName.m_Name) );)

    // the name is legal and not previously added to the config container, append
    // it and return its index within the container.
    m_Fields.push_back( aFieldName );

    return m_Fields.size() - 1; // return the index of insertion.
}

