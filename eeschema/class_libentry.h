/****************************************************************/
/*	Headers fo lib component (or libentry) definitions */
/****************************************************************/

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


/* basic class to describe components in libraries (true component or alias), non used directly */
class LibCmpEntry            : public EDA_BaseStruct
{
public:
    LibrEntryType    Type;      /* Type = ROOT;
                                 *      = ALIAS pour struct LibraryAliasType */
    LibDrawField     m_Name;    // name	(74LS00 ..) in lib ( = VALUE )
    wxString         m_Doc;     /* documentation for info */
    wxString         m_KeyWord; /* keyword list (used to select a group of components by keyword) */
    wxString         m_DocFile; /* Associed doc filename */
    LibrEntryOptions m_Options; // special features (i.e. Entry is a POWER)

public:
    LibCmpEntry( LibrEntryType CmpType, const wxChar* CmpName );
    virtual ~LibCmpEntry();
    virtual wxString GetClass() const
    {
        return wxT( "LibCmpEntry" );
    }


    /**
     * Function SaveDoc
     * writes the doc info out to a FILE in "*.dcm" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool SaveDoc( FILE* aFile );
};


/*********************************************/
/* class to handle an usual component in lib */
/*********************************************/
class EDA_LibComponentStruct : public LibCmpEntry
{
public:
    LibDrawField       m_Prefix;                /* Prefix ( U, IC ... ) = REFERENCE */
    wxArrayString      m_AliasList;             /* ALIAS list for the component */
    wxArrayString      m_FootprintList;         /* list of suitable footprint names for the component (wildcard names accepted)*/
    int                m_UnitCount;             /* Units (or sections) per package */
    bool               m_UnitSelectionLocked;   // True if units are differents and their selection is locked
                                                // (i.e. if part A cannot be automatically changed in part B
    int                m_TextInside;            /* if 0: pin name drawn on the pin itself
                                                 *  if > 0 pin name drawn inside the component,
                                                 *  with a distance of m_TextInside in mils */
    bool               m_DrawPinNum;
    bool               m_DrawPinName;
    DLIST<LibDrawField> m_Fields;                  /* Auxiliairy Field list (id >= 2 ) */
    LibEDA_BaseStruct* m_Drawings;              /* How to draw this part */
    long               m_LastDate;              // Last change Date

public:
    virtual wxString GetClass() const
    {
        return wxT( "EDA_LibComponentStruct" );
    }


    EDA_LibComponentStruct( const wxChar* CmpName );
    EDA_Rect GetBoundaryBox( int Unit, int Convert );    /* return Box around the part. */

    ~EDA_LibComponentStruct();
    void     SortDrawItems();

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.lib" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool     Save( FILE* aFile );

    /** Function SetFields
     * initialize fields from a vector of fields
     * @param aFields a std::vector <LibDrawField> to import.
     */
    void     SetFields( const std::vector <LibDrawField> aFields );
};


/**************************************************************************/
/* class to handle an alias of an usual component in lib (root component) */
/**************************************************************************/
class EDA_LibCmpAliasStruct  : public LibCmpEntry
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
