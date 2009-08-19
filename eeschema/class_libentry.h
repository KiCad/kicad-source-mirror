/******************************************/
/*  Library component object definitions. */
/******************************************/

#ifndef CLASS_LIBENTRY_H
#define CLASS_LIBENTRY_H

#include "dlist.h"

#include "classes_body_items.h"
#include "class_libentry_fields.h"

/* Types for components in libraries
 * components can be a true component or an alias of a true component.
 */
enum LibrEntryType {
    ROOT,       /* This is a true component standard EDA_LibComponentStruct */
    ALIAS       /* This is an alias of a true component */
};

/* values for member .m_Options */
enum  LibrEntryOptions {
    ENTRY_NORMAL,   // Libentry is a standard component (real or alias)
    ENTRY_POWER     // Libentry is a power symbol
};


/**
 * Base class to describe library components and aliases.
 *
 * This class is not to be  used directly.
 */
class LibCmpEntry : public EDA_BaseStruct
{
public:
    LibrEntryType    Type;      /* Type = ROOT;
                                 *      = ALIAS pour struct LibraryAliasType */
    LibDrawField     m_Name;    // name (74LS00 ..) in lib ( = VALUE )
    wxString         m_Doc;     /* documentation for info */
    wxString         m_KeyWord; /* keyword list (used to select a group of
                                 * components by keyword) */
    wxString         m_DocFile; /* Associate doc file name */
    LibrEntryOptions m_Options; // special features (i.e. Entry is a POWER)

public:
    LibCmpEntry( LibrEntryType CmpType, const wxChar* CmpName );
    virtual ~LibCmpEntry();
    virtual wxString GetClass() const
    {
        return wxT( "LibCmpEntry" );
    }


    /**
     * Writes the doc info out to a FILE in "*.dcm" format.
     *
     * @param aFile The FILE to write to.
     *
     * @return bool - true if success writing else false.
     */
    bool SaveDoc( FILE* aFile );
};


/**
 * Library component object definition.
 *
 * A library component object is typically save and loaded in a component
 * library file (.lib).  Library components are different from schematic
 * components.
 */
class EDA_LibComponentStruct : public LibCmpEntry
{
public:
    LibDrawField       m_Prefix;         /* Prefix ( U, IC ... ) = REFERENCE */
    wxArrayString      m_AliasList;      /* ALIAS list for the component */
    wxArrayString      m_FootprintList;  /* list of suitable footprint names
                                          * for the component (wildcard names
                                          * accepted) */
    int                m_UnitCount;      /* Units (or sections) per package */
    bool               m_UnitSelectionLocked;  /* True if units are different
                                                * and their selection is
                                                * locked (i.e. if part A cannot
                                                * be automatically changed in
                                                * part B */
    int                m_TextInside;     /* if 0: pin name drawn on the pin
                                          * itself if > 0 pin name drawn inside
                                          * the component, with a distance of
                                          * m_TextInside in mils */
    bool               m_DrawPinNum;
    bool               m_DrawPinName;
    DLIST<LibDrawField> m_Fields;         /* Auxiliary Field list (id >= 2 ) */
    LibEDA_BaseStruct* m_Drawings;        /* How to draw this part */
    long               m_LastDate;        // Last change Date

public:
    virtual wxString GetClass() const
    {
        return wxT( "EDA_LibComponentStruct" );
    }


    EDA_LibComponentStruct( const wxChar* CmpName );
    ~EDA_LibComponentStruct();

    EDA_Rect GetBoundaryBox( int Unit, int Convert );

    void     SortDrawItems();

    bool SaveDateAndTime( FILE* ExportFile );
    bool LoadDateAndTime( char* Line );

    /**
     * Write the data structures out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     *
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile );

    /**
     * Load component definition from file.
     *
     * @param file - File descriptor of file to load form.
     * @param line - The first line of the component definition.
     * @param lineNum - The current line number in the file.
     * @param errorMsg - Description of error on load failure.
     *
     * @return bool - Result of the load, false if there was an error.
     */
    bool Load( FILE* file, char* line, int* lineNum, wxString& errorMsg );
    bool LoadField( char* line, wxString& errorMsg );
    bool LoadDrawEntries( FILE* f, char* line,
                          int* lineNum, wxString& errorMsg );
    bool LoadAliases( char* line, wxString& Error );
    bool LoadFootprints( FILE* file, char* line,
                         int* lineNum, wxString& errorMsg );

    /**
     * Initialize fields from a vector of fields.
     *
     * @param aFields - a std::vector <LibDrawField> to import.
     */
    void SetFields( const std::vector <LibDrawField> aFields );
};


/**
 * Component library alias object definition.
 *
 * Component aliases are not really components.  They are references
 * to an actual component object.
 *
 * @todo Alias objects should really be defined as children of a component
 *       object not as children of a library object.  This would greatly
 *       simply searching for components in libraries.
 */
class EDA_LibCmpAliasStruct : public LibCmpEntry
{
public:
    wxString m_RootName;        /* Root component Part name */

public:
    EDA_LibCmpAliasStruct( const wxChar* CmpName, const wxChar* CmpRootName );
    ~EDA_LibCmpAliasStruct();

    virtual wxString GetClass() const
    {
        return wxT( "EDA_LibCmpAliasStruct" );
    }
};


#endif  //  CLASS_LIBENTRY_H
