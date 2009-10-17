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
    LIB_FIELD        m_Name;    // name (74LS00 ..) in lib ( = VALUE )
    wxString         m_Doc;     /* documentation for info */
    wxString         m_KeyWord; /* keyword list (used to select a group of
                                 * components by keyword) */
    wxString         m_DocFile; /* Associate doc file name */
    LibrEntryOptions m_Options; // special features (i.e. Entry is a POWER)

public:
    CMP_LIB_ENTRY( LibrEntryType CmpType, const wxString& name,
                   CMP_LIBRARY* lib = NULL );
    CMP_LIB_ENTRY( CMP_LIB_ENTRY& entry, CMP_LIBRARY* lib = NULL );

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
    LIB_FIELD          m_Prefix;         /* Prefix ( U, IC ... ) = REFERENCE */
    wxArrayString      m_AliasList;      /* ALIAS list for the component */
    wxArrayString      m_FootprintList;  /* list of suitable footprint names
                                          * for the component (wildcard names
                                          * accepted) */
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
    DLIST<LIB_FIELD>   m_Fields;         /* Auxiliary Field list (id >= 2 ) */
    long               m_LastDate;       // Last change Date

protected:
    int                m_UnitCount;      /* Units (or sections) per package */
    LIB_DRAW_ITEM_LIST m_Drawings;       /* How to draw this part */

public:
    virtual wxString GetClass() const
    {
        return wxT( "LIB_COMPONENT" );
    }


    LIB_COMPONENT( const wxString& name, CMP_LIBRARY* lib = NULL );
    LIB_COMPONENT( LIB_COMPONENT& component, CMP_LIBRARY* lib = NULL );

    ~LIB_COMPONENT();

    EDA_Rect GetBoundaryBox( int Unit, int Convert );

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
     * @param aFields - a std::vector <LIB_FIELD> to import.
     */
    void SetFields( const std::vector <LIB_FIELD> aFields );

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
     *        body items (useful to draw a body in schematic,
     *        because fields of schematic components replace the lib component fields).
     * @param onlySelected - Draws only the body items that are selected.
     *                       Used for block move redraws.
     */
    void Draw( WinEDA_DrawPanel* panel, wxDC* dc, const wxPoint& offset,
               int multi, int convert, int drawMode, int color = -1,
               const int transformMatrix[2][2] = DefaultTransformMatrix,
               bool showPinText = true, bool drawFields = true,
               bool onlySelected = false );

    /**
     * Plot component to plotter.
     *
     * @param plotter - Plotter object to plot to.
     * @param unit - Component part to plot.
     * @param convert - Component alternate body style to plot.
     * @param transform - Component plot transform matrix.
     */
    void Plot( PLOTTER* plotter, int unit, int convert, const wxPoint& offset,
               const int transform[2][2] );

    /**
     * Add a new draw item to the draw object list.
     *
     * @param item - New draw object to add to component.
     */
    void AddDrawItem( LIB_DRAW_ITEM* item );

    /**
     * Remove draw item from list.
     *
     * @param item - Draw item to remove from list.
     * @param panel - Panel to remove part from.
     * @param dc - Device context to remove part from.
     */
    void RemoveDrawItem( LIB_DRAW_ITEM* item,
                         WinEDA_DrawPanel* panel = NULL,
                         wxDC* dc = NULL );

    /** GetNextDrawItem()
     * Return the next draw object pointer.
     *
     * @param item - Pointer to the current draw item.  Setting item NULL
     *               with return the first item of type in the list.
     * @param type - type of searched item (filter).
     *               if TYPE_NOT_INIT search for all items types
     *
     * @return - Pointer to the next drawing object in the list if found,
     *           otherwise NULL.
     */

    LIB_DRAW_ITEM* GetNextDrawItem( LIB_DRAW_ITEM* item = NULL,
                                    KICAD_T type = TYPE_NOT_INIT );

    /**
     * Return the next pin object from the draw list.
     *
     * This is just a pin object specific version of GetNextDrawItem().
     *
     * @param item - Pointer to the previous pin item, or NULL to get the
     *               first pin in the draw object list.
     *
     * @return - Pointer to the next pin object in the list if found,
     *           otherwise NULL.
     */
    LIB_PIN* GetNextPin( LIB_PIN* item = NULL )
    {
        return (LIB_PIN*) GetNextDrawItem( (LIB_DRAW_ITEM*) item,
                                           COMPONENT_PIN_DRAW_TYPE );
    }


    /**
     * Return a list of pin object pointers from the draw item list.
     *
     * Note pin objects are owned by the draw list of the component.
     * Deleting any of the objects will leave list in a unstable state
     * and will likely segfault when the list is destroyed.
     *
     * @param list - Pin list to place pin object pointers into.
     * @param unit - Unit number of pin to add to list.  Set to 0 to
     *               get pins from any component part.
     * @param convert - Convert number of pin to add to list.  Set to 0 to
     *                  get pins from any convert of component.
     */
    void GetPins( LIB_PIN_LIST& pins, int unit = 0, int convert = 0 );

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

    /**
     * Clears the status flag all draw objects in this component.
     */
    void ClearStatus( void );

    /**
     * Checks all draw objects of component to see if they are with block.
     *
     * Use this method to mark draw objects as selected during block
     * functions.
     *
     * @param rect - The bounding rectangle to test in draw items are inside.
     * @param unit - The current unit number to test against.
     * @param convert - Are the draw items being selected a conversion.
     * @param editPinByPin - Used to ignore pin selections when in edit pin
     *                       by pin mode is enabled.
     *
     * @return int - The number of draw object found inside the block select
     *               rectangle.
     */
    int SelectItems( EDA_Rect& rect, int unit, int convert,
                     bool editPinByPin );

    /**
     * Clears all the draw items marked by a block select.
     */
    void ClearSelectedItems( void );

    /**
     * Deletes the select draw items marked by a block select.
     *
     * The name and reference field will not be deleted.  They are the
     * minimum drawing items required for any component.  Thier properties
     * can be changed but the cannot be removed.
     */
    void DeleteSelectedItems( void );

    /**
     * Move the selected draw items marked by a block select.
     */
    void MoveSelectedItems( const wxPoint& offset );

    /**
     * Make a copy of the selected draw items marked by a block select.
     *
     * Fields are not copied.  Only component body items are copied.
     * Copying fields would result in duplicate fields which does not
     * make sense in this context.
     */
    void CopySelectedItems( const wxPoint& offset );

    /**
     * Horizontally (X axis) mirror selected draw items about a point.
     *
     * @param center - Center point to mirror around.
     */
    void MirrorSelectedItemsH( const wxPoint& center );

    /**
     * Locate a draw object.
     *
     * @param unit - Unit number of draw item.
     * @param convert - Body style of draw item.
     * @param type - Draw object type, set to 0 to search for any type.
     * @param pt - Coordinate for hit testing.
     *
     * @return LIB_DRAW_ITEM - Pointer the the draw object if found.
     *                         Otherwise NULL.
     */
    LIB_DRAW_ITEM* LocateDrawItem( int unit, int convert, KICAD_T type,
                                   const wxPoint& pt );

    /**
     * Locate a draw object (overlaid)
     *
     * @param unit - Unit number of draw item.
     * @param convert - Body style of draw item.
     * @param type - Draw object type, set to 0 to search for any type.
     * @param pt - Coordinate for hit testing.
     * @param aTransMat = the transform matrix
     *
     * @return LIB_DRAW_ITEM - Pointer the the draw object if found.
     *                         Otherwise NULL.
     */
    LIB_DRAW_ITEM* LocateDrawItem( int unit, int convert, KICAD_T type,
                                   const wxPoint& pt, const int aTransMat[2][2] );

    /**
     * Return a reference to the draw item list.
     *
     * @return LIB_DRAW_ITEM_LIST& - Reference to the draw item object list.
     */
    LIB_DRAW_ITEM_LIST& GetDrawItemList( void ) { return m_Drawings; }

    /**
     * Set the part per package count.
     *
     * If the count is greater than the current count, then the all of the
     * current draw items are duplicated for each additional part.  If the
     * count is less than the current count, all draw objects for parts
     * greater that count are removed from the component.
     *
     * @param count - Number of parts per package.
     */
    void SetPartCount( int count );

    int GetPartCount( void ) { return m_UnitCount; }

    /**
     * Set or clear the alternate body style (DeMorgan) for the component.
     *
     * If the component already has an alternate body style set and a
     * asConvert if false, all of the existing draw items for the alternate
     * body style are remove.  If the alternate body style is not set and
     * asConvert is true, than the base draw items are duplicated and
     * added to the component.
     *
     * @param asConvert - Set or clear the component alternate body style.
     */
    void SetConversion( bool asConvert );
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
    LIB_ALIAS( LIB_ALIAS& alias, CMP_LIBRARY* lib = NULL );
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
