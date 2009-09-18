/******************************************/
/*  Library component object definitions. */
/******************************************/

#ifndef CLASS_LIBENTRY_H
#define CLASS_LIBENTRY_H

#include "dlist.h"

#include "classes_body_items.h"
#include "class_libentry_fields.h"

#include <boost/ptr_container/ptr_vector.hpp>


class CMP_LIBRARY;


/* Types for components in libraries
 * components can be a true component or an alias of a true component.
 */
enum LibrEntryType
{
    ROOT,       /* This is a true component standard LIB_COMPONENT */
    ALIAS       /* This is an alias of a true component */
};

/* values for member .m_Options */
enum  LibrEntryOptions
{
    ENTRY_NORMAL,   // Libentry is a standard component (real or alias)
    ENTRY_POWER     // Libentry is a power symbol
};


/**
 * Base class to describe library components and aliases.
 *
 * This class is not to be used directly.
 */
class CMP_LIB_ENTRY : public EDA_BaseStruct
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
    CMP_LIB_ENTRY( LibrEntryType CmpType, const wxString& name,
                   CMP_LIBRARY* lib = NULL );
    virtual ~CMP_LIB_ENTRY();

    virtual wxString GetClass() const
    {
        return wxT( "CMP_LIB_ENTRY" );
    }

    wxString GetLibraryName();

    const wxString& GetName() { return m_Name.m_Text; }

    /**
     * Write the entry document information to a FILE in "*.dcm" format.
     *
     * @param aFile The FILE to write to.
     *
     * @return bool - true if success writing else false.
     */
    bool SaveDoc( FILE* aFile );

    /**
     * Case insensitive comparison of the component entry name.
     */
    bool operator==( const wxChar* name ) const;
    bool operator!=( const wxChar* name ) const
    {
        return !( *this == name );
    }

protected:
    CMP_LIBRARY* m_lib;
};


typedef boost::ptr_vector< CMP_LIB_ENTRY > LIB_ENTRY_LIST;

extern bool operator<( const CMP_LIB_ENTRY& item1, const CMP_LIB_ENTRY& item2 );

extern int LibraryEntryCompare( const CMP_LIB_ENTRY* LE1,
                                const CMP_LIB_ENTRY* LE2 );


/**
 * Library component object definition.
 *
 * A library component object is typically save and loaded in a component
 * library file (.lib).  Library components are different from schematic
 * components.
 */
class LIB_COMPONENT : public CMP_LIB_ENTRY
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
        return wxT( "LIB_COMPONENT" );
    }


    LIB_COMPONENT( const wxString& name, CMP_LIBRARY* lib = NULL );
    ~LIB_COMPONENT();

    EDA_Rect GetBoundaryBox( int Unit, int Convert );

    void SortDrawItems();

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

    /**
     * Draw component.
     *
     * @param panel - Window to draw on.
     * @param dc - Device context to draw on.
     * @param offset - Position to component.
     * @param multi - Component unit if multiple parts per component.
     * @param convert - Component conversion (DeMorgan) if available.
     * @param drawMode - Device context drawing mode, see wxDC.
     * @param color - Color to draw component.
     * @param transformMatrix - Cooridinate adjustment settings.
     * @param showPinText - Show pin text if true.
     * @param drawFields - Draw field text if true otherwise just draw
     *                     body items.
     * @param onlySelected - Draws only the body items that are selected.
     *                       Used for block move redraws.
     */
    void Draw( WinEDA_DrawPanel* panel, wxDC* dc, const wxPoint& offset,
               int multi, int convert, int drawMode, int color = -1,
               const int transformMatrix[2][2] = DefaultTransformMatrix,
               bool showPinText = true, bool drawFields = true,
               bool onlySelected = false );

    /**
     * Remove draw item from list.
     *
     * @param item - Draw item to remove from list.
     * @param panel - Panel to remove part from.
     * @param dc - Device context to remove part from.
     */
    void RemoveDrawItem( LibEDA_BaseStruct* item,
                         WinEDA_DrawPanel* panel = NULL,
                         wxDC* dc = NULL );

    /**
     * Move the component offset.
     *
     * @param offset - Offset displacement.
     */
    void SetOffset( const wxPoint& offset );

    /**
     * Remove duplicate draw items from list.
     */
    void RemoveDuplicateDrawItems();

    /**
     * Test if component has more than one body conversion type (DeMorgan).
     *
     * @return bool - True if component has more than one conversion.
     */
    bool HasConversion() const;

    /**
     * Test if alias name is in component alias list.
     *
     * Alias name comparisons are case insensitive.
     *
     * @param name - Name of alias.
     *
     * @return bool - True if alias name in alias list.
     */
    bool HasAlias( const wxChar* name )
    {
        wxASSERT( name != NULL );
        return m_AliasList.Index( name ) != wxNOT_FOUND;
    }
};


/**
 * Component library alias object definition.
 *
 * Component aliases are not really components.  They are references
 * to an actual component object.
 *
 * @todo Alias objects should really be defined as children of a component
 *       object not as children of a library object.  This would greatly
 *       simplify searching for components in libraries.
 */
class LIB_ALIAS : public CMP_LIB_ENTRY
{
protected:
    LIB_COMPONENT* m_root;    /* Root component of the alias. */

public:
    LIB_ALIAS( const wxString& name, LIB_COMPONENT* root,
                           CMP_LIBRARY* lib = NULL );
    ~LIB_ALIAS();

    virtual wxString GetClass() const
    {
        return wxT( "LIB_ALIAS" );
    }

    /**
     * Get the alias root component.
     */
    LIB_COMPONENT* GetComponent( void ) const
    {
        return m_root;
    }

    /**
     * Set the alias root component.
     */
    void SetComponent( LIB_COMPONENT* component );
};


#endif  //  CLASS_LIBENTRY_H
