
#ifndef _TEMPLATE_FIELDNAME_H_
#define _TEMPLATE_FIELDNAME_H_

#include <richio.h>
#include <wxstruct.h>
#include <macros.h>
#include <template_fieldnames_lexer.h>

class TEMPLATE_FIELDNAMES_LEXER;


/**
 * Enum NumFieldType
 * is the set of all field indices assuming an array like sequence that a
 * SCH_COMPONENT or LIB_COMPONENT can hold.
 * The first fields are called fixed fields and the quantity of them is
 * given by MANDATORY_FIELDS.  After that come an unlimited number of
 * user defined fields, only some of which have indices defined here.
 */
enum  NumFieldType {
    REFERENCE = 0,          ///< Field Reference of part, i.e. "IC21"
    VALUE,                  ///< Field Value of part, i.e. "3.3K"
    FOOTPRINT,              ///< Field Name Module PCB, i.e. "16DIP300"
    DATASHEET,              ///< name of datasheet

    /// The first 4 are mandatory, and must be instantiated in SCH_COMPONENT
    /// and LIB_COMPONENT constructors
    MANDATORY_FIELDS,

    FIELD1 = MANDATORY_FIELDS,
    FIELD2,
    FIELD3,
    FIELD4,
    FIELD5,
    FIELD6,
    FIELD7,
    FIELD8
};


/**
 * Struct TEMPLATE_FIELDNAME
 * holds a name of a component's field, field value, and default visibility.
 * Template fieldnames are wanted fieldnames for use in the symbol/component
 * property editors.
 */
struct TEMPLATE_FIELDNAME
{
    wxString    m_Name;         ///<  The field name
    wxString    m_Value;        ///<  The default value or empty
    bool        m_Visible;      ///<  If first appearance of the field's editor has as visible.

    TEMPLATE_FIELDNAME() :
        m_Visible( false )
    {
    }

    TEMPLATE_FIELDNAME( const wxString& aName ) :
        m_Name( aName ),
        m_Visible( false )
    {
    }

    TEMPLATE_FIELDNAME( const TEMPLATE_FIELDNAME& ref )
    {
        m_Name = ref.m_Name;
        m_Value = ref.m_Value;
        m_Visible = ref.m_Visible;
    }

    /**
     * Function Format
     * serializes this object out as text into the given OUTPUTFORMATTER.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR );

    /**
     * Function Parse
     * fills this object from information in the input stream \a aSpec, which
     * is a TEMPLATE_FIELDNAMES_LEXER. The entire textual element spec is <br>
     * (field (name _yourfieldname_)(value _yourvalue_) visible)) <br>
     * The presence of value is optional, the presence of visible is optional.
     * When this function is called, the input token stream given by \a aSpec
     * is assumed to be positioned at the '^' in the following example, i.e. just after the
     * identifying keyword and before the content specifying stuff.<br>
     * (field ^ (....) )
     *
     * @param aSpec is the input token stream of keywords and symbols.
     */
    void Parse( TEMPLATE_FIELDNAMES_LEXER* aSpec ) throw( IO_ERROR );

    /**
     * Function GetDefaultFieldName
     * returns a default symbol field name for field \a aFieldNdx for all components.
     * These fieldnames are not modifiable, but template fieldnames are.
     * @param aFieldNdx The field number index, > 0
     */
    static wxString GetDefaultFieldName( int aFieldNdx );
};

typedef std::vector< TEMPLATE_FIELDNAME > TEMPLATE_FIELDNAMES;


class TEMPLATES
{
private:
    TEMPLATE_FIELDNAMES     m_Fields;

public:

    /**
     * Function Format
     * serializes this object out as text into the given OUTPUTFORMATTER.
     */
    void Format( OUTPUTFORMATTER* out, int nestLevel ) const throw( IO_ERROR );

    /**
     * Function Parse
     * fills this object from information in the input stream handled by TEMPLATE_FIELDNAMES_LEXER
     */
    void Parse( TEMPLATE_FIELDNAMES_LEXER* in ) throw( IO_ERROR );


    /**
     * Function AddTemplateFieldName
     * inserts or appends a wanted symbol field name into the fieldnames
     * template.  Should be used for any symbol property editor.  If the name
     * already exists, it overwrites the same name.
     *
     * @param aFieldName is a full description of the wanted field, and it must not match
     *          any of the default fieldnames.
     * @return int - the index within the config container at which aFieldName was
     *          added, or -1 if the name is illegal because it matches a default fieldname.
     */
    int AddTemplateFieldName( const TEMPLATE_FIELDNAME& aFieldName );

    /**
     * Function DeleteAllTemplateFieldNames
     * deletes the entire contents.
     */
    void DeleteAllTemplateFieldNames()
    {
        m_Fields.clear();
    }

    /**
     * Function GetTemplateFieldName
     * returns a template fieldnames list for read only access.
     */
    const TEMPLATE_FIELDNAMES& GetTemplateFieldNames()
    {
        return m_Fields;
    }

    /**
     * Function HasFieldName
     * checks for \a aName in the the template field name list.
     *
     * @param aName A wxString object containing the field name to search for.
     * @return True if \a aName is found in the list.
     */
    bool HasFieldName( const wxString& aName ) const;
};

#endif // _TEMPLATE_FIELDNAME_H_
