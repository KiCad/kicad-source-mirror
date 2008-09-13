/****************************************************************/
/*	Headers fo library definition and lib component definitions */
/****************************************************************/

#ifndef CLASS_LIBRARY_H
#define CLASS_LIBRARY_H


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


/******************************/
/* Classe to handle a library */
/******************************/

class LibraryStruct
{
public:
    int            m_Type;                  /* type indicator */
    wxString       m_Name;                  /* Short Name of the loaded library (without path). */
    wxString       m_FullFileName;          /* Full File Name (with path) of library. */
    wxString       m_Header;                /* first line of loaded library. */
    int            m_NumOfParts;            /* Number of parts this library has. */
    PriorQue*      m_Entries;               /* Parts themselves are saved here. */
    LibraryStruct* m_Pnext;                 /* Point on next lib in chain. */
    int            m_Modified;              /* flag indicateur d'edition */
    int            m_Size;                  // Size in bytes (for statistics)
    unsigned long  m_TimeStamp;             // Signature temporelle
    int            m_Flags;                 // variable used in some functions
    bool           m_IsLibCache;            /* False for the "standard" libraries,
                                              * True for the library cache */

public:
    LibraryStruct( int type, const wxString& name, const wxString& fullname );
    ~LibraryStruct();
    bool WriteHeader( FILE* file );
    bool ReadHeader( FILE* file, int* LineNum );
};


#include "classes_body_items.h"


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


    bool WriteDescr( FILE* File );
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
    LibDrawField*      Fields;                  /* Auxiliairy Field list (id = 2 a 11) */
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


#endif  //  CLASS_LIBRARY_H
