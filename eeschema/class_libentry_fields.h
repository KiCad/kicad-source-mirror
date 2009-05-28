/**************************************************************/
/*	Lib component definitions (libentry) definition of fields */
/**************************************************************/

#ifndef CLASS_LIBENTRY_FIELDS_H
#define CLASS_LIBENTRY_FIELDS_H


/* Fields , same as component fields.
 * can be defined in libraries (mandatory for ref and value, ca be useful for
 * footprints)
 * 2 Fields are always defined :
 *     Prefix (U, IC..) with gives the reference in schematic)
 *     Name (74LS00..) used to find the component in libraries, and give the
 * default value in schematic
 */

class LibDrawField : public LibEDA_BaseStruct,
    public EDA_TextStruct
{
public:
    int m_FieldId;     /*  0 = REFERENCE
                        *  1 = VALUE
                        *  3 = FOOTPRINT (default Footprint)
                        *  4 = DOCUMENTATION (user doc link)
                        *  others = free fields
                        */
    wxString m_Name;   /* Field Name (not the field text itself, that is
                        * .m_Text) */

public:

    LibDrawField* Next() const { return (LibDrawField*) Pnext; }
    LibDrawField* Back() const { return (LibDrawField*) Pback; }


    LibDrawField( int idfield = 2 );
    ~LibDrawField();
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawField" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

    LibDrawField* GenCopy();

    /** Function Copy
     * copy parameters of this to Target. Pointers are not copied
     * @param aTarget = the LibDrawField to set with "this" values
     */
    void          Copy( LibDrawField* aTarget ) const;

    void          SetFields( const std::vector <LibDrawField> aFields );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test, in Field coordinate system
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& refPos );

    void operator=( const LibDrawField& field )
    {
        m_FieldId = field.m_FieldId;
        m_Text = field.m_Text;
        m_Pos = field.m_Pos;
        m_Size = field.m_Size;
        m_Width = field.m_Width;
        m_Orient = field.m_Orient;
        m_Mirror = field.m_Mirror;
        m_Attributs = field.m_Attributs;
        m_Italic = field.m_Italic;
        m_Bold = field.m_Bold;
        m_HJustify = field.m_HJustify;
        m_VJustify = field.m_VJustify;
    }
};

#endif  //  CLASS_LIBENTRY_FIELDS_H
