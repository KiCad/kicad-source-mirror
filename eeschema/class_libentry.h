/******************************************/
/*  Library component object definitions. */
/******************************************/

#ifndef CLASS_LIBENTRY_H
#define CLASS_LIBENTRY_H

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

protected:
    wxString         name;

    /** Library object that entry is attached to. */
    CMP_LIBRARY*     library;

    /** Entry type, either ROOT or ALIAS. */
    LibrEntryType    type;

    wxString         description;  /* documentation for info */
    wxString         keyWords;     /* keyword list (used for search for
                                    * components by keyword) */
    wxString         docFileName;  /* Associate doc file name */
    LibrEntryOptions options;      // special features (i.e. Entry is a POWER)

public:
    CMP_LIB_ENTRY( LibrEntryType aType, const wxString& aName,
                   CMP_LIBRARY* aLibrary = NULL );
    CMP_LIB_ENTRY( CMP_LIB_ENTRY& aEntry, CMP_LIBRARY* aLibrary = NULL );

    virtual ~CMP_LIB_ENTRY();

    virtual wxString GetClass() const
    {
        return wxT( "CMP_LIB_ENTRY" );
    }

    wxString GetLibraryName();

    virtual const wxString& GetName() const { return name; }

    virtual void SetName( const wxString& aName ) { name = aName; }

    bool isComponent() { return type == ROOT; }

    bool isAlias() { return type == ALIAS; }

    int GetType() { return type; }

    bool isPower() { return options == ENTRY_POWER; }
    bool isNormal() { return options == ENTRY_NORMAL; }

    void SetPower() { options = ENTRY_POWER; }
    void SetNormal() { options = ENTRY_NORMAL; }

    void SetDescription( const wxString& aDescription )
    {
        description = aDescription;
    }

    wxString GetDescription() { return description; }

    void SetKeyWords( const wxString& aKeyWords )
    {
        keyWords = aKeyWords;
    }

    wxString GetKeyWords() { return keyWords; }

    void SetDocFileName( const wxString& aDocFileName )
    {
        docFileName = aDocFileName;
    }

    wxString GetDocFileName() { return docFileName; }

    /**
     * Write the entry document information to a FILE in "*.dcm" format.
     *
     * @param aFile The FILE to write to.
     * @return True if success writing else false.
     */
    bool SaveDoc( FILE* aFile );

    /**
     * Case insensitive comparison of the component entry name.
     */
    bool operator==( const wxChar* aName ) const;
    bool operator!=( const wxChar* aName ) const
    {
        return !( *this == aName );
    }
};


typedef boost::ptr_vector< CMP_LIB_ENTRY > LIB_ENTRY_LIST;

extern bool operator<( const CMP_LIB_ENTRY& aItem1, const CMP_LIB_ENTRY& aItem2 );

extern int LibraryEntryCompare( const CMP_LIB_ENTRY* aItem1, const CMP_LIB_ENTRY* aItem2 );


/**
 * Library component object definition.
 *
 * Library component object definition.
 *
 * A library component object is typically saved and loaded
 * in a component library file (.lib).
 * Library components are different from schematic components.
 */
class LIB_COMPONENT : public CMP_LIB_ENTRY
{
public:
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
    long               m_LastDate;       // Last change Date

protected:
    int                unitCount;      /* Units (parts) per package */
    LIB_DRAW_ITEM_LIST drawings;       /* How to draw this part */

public:
    virtual wxString GetClass() const
    {
        return wxT( "LIB_COMPONENT" );
    }


    virtual void SetName( const wxString& aName )
    {
        CMP_LIB_ENTRY::SetName( aName );
        GetValueField().m_Text = aName;
    }

    LIB_COMPONENT( const wxString& aName, CMP_LIBRARY* aLibrary = NULL );
    LIB_COMPONENT( LIB_COMPONENT& aComponent, CMP_LIBRARY* aLibrary = NULL );

    ~LIB_COMPONENT();

    EDA_Rect GetBoundaryBox( int aUnit, int aConvert );

    bool SaveDateAndTime( FILE* aFile );
    bool LoadDateAndTime( char* aLine );

    /**
     * Write the data structures out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return True if success writing else false.
     */
    bool Save( FILE* aFile );

    /**
     * Load component definition from /a aFile.
     *
     * @param aFile - File descriptor of file to load form.
     * @param aLine - The first line of the component definition.
     * @param aLineNum - The current line number in the file.
     * @param aErrorMsg - Description of error on load failure.
     * @return True if the load was successful, false if there was an error.
     */
    bool Load( FILE* aFile, char* aLine, int* aLineNum, wxString& aErrorMsg );
    bool LoadField( char* aLine, wxString& aErrorMsg );
    bool LoadDrawEntries( FILE* aFile, char* aLine,
                          int* aLineNum, wxString& aErrorMsg );
    bool LoadAliases( char* aLine, wxString& aErrorMsg );
    bool LoadFootprints( FILE* aFile, char* aLine,
                         int* aLineNum, wxString& aErrorMsg );

    /**
     * Initialize fields from a vector of fields.
     *
     * @param aFields - a std::vector <LIB_FIELD> to import.
     */
    void SetFields( const std::vector <LIB_FIELD> aFields );

    /**
     * Return list of field references of component.
     *
     * @param aList - List to add field references to.
     */
    void GetFields( LIB_FIELD_LIST& aList );

    /**
     * Return pointer to the requested field.
     *
     * @param aId - Id of field to return.
     * @return The field if found, otherwise NULL.
     */
    LIB_FIELD* GetField( int aId );

    /** Return reference to the value field. */
    LIB_FIELD& GetValueField();

    /** Return reference to the reference designator field. */
    LIB_FIELD& GetReferenceField();

    /**
     * Draw component.
     *
     * @param aPanel - Window to draw on.
     * @param aDc - Device context to draw on.
     * @param aOffset - Position to component.
     * @param aMulti - Component unit if multiple parts per component.
     * @param aConvert - Component conversion (DeMorgan) if available.
     * @param aDrawMode - Device context drawing mode, see wxDC.
     * @param aColor - Color to draw component.
     * @param aTransformMatrix - Coordinate adjustment settings.
     * @param aShowPinText - Show pin text if true.
     * @param aDrawFields - Draw field text if true otherwise just draw
     *                      body items (useful to draw a body in schematic,
     *                      because fields of schematic components replace
     *                      the lib component fields).
     * @param aOnlySelected - Draws only the body items that are selected.
     *                        Used for block move redraws.
     */
    void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDc, const wxPoint& aOffset,
               int aMulti, int aConvert, int aDrawMode, int aColor = -1,
               const int aTransform[2][2] = DefaultTransformMatrix,
               bool aShowPinText = true, bool aDrawFields = true,
               bool aOnlySelected = false );

    /**
     * Plot component to plotter.
     *
     * @param aPlotter - Plotter object to plot to.
     * @param aUnit - Component part to plot.
     * @param aConvert - Component alternate body style to plot.
     * @param aTransform - Component plot transform matrix.
     */
    void Plot( PLOTTER* aPlotter, int aUnit, int aConvert, const wxPoint& aOffset,
               const int aTransform[2][2] );

    /**
     * Add a new draw /a aItem to the draw object list.
     *
     * @param item - New draw object to add to component.
     */
    void AddDrawItem( LIB_DRAW_ITEM* aItem );

    /**
     * Remove draw /a aItem from list.
     *
     * @param aItem - Draw item to remove from list.
     * @param aPanel - Panel to remove part from.
     * @param aDc - Device context to remove part from.
     */
    void RemoveDrawItem( LIB_DRAW_ITEM* aItem,
                         WinEDA_DrawPanel* aPanel = NULL,
                         wxDC* aDc = NULL );

    /**
     * Return the next draw object pointer.
     *
     * @param aItem - Pointer to the current draw item.  Setting item NULL
     *                with return the first item of type in the list.
     * @param aType - type of searched item (filter).
     *                if TYPE_NOT_INIT search for all items types
     * @return - The next drawing object in the list if found, otherwise NULL.
     */

    LIB_DRAW_ITEM* GetNextDrawItem( LIB_DRAW_ITEM* aItem = NULL,
                                    KICAD_T aType = TYPE_NOT_INIT );

    /**
     * Return the next pin object from the draw list.
     *
     * This is just a pin object specific version of GetNextDrawItem().
     *
     * @param item - Pointer to the previous pin item, or NULL to get the
     *               first pin in the draw object list.
     * @return - The next pin object in the list if found, otherwise NULL.
     */
    LIB_PIN* GetNextPin( LIB_PIN* aItem = NULL )
    {
        return (LIB_PIN*) GetNextDrawItem( (LIB_DRAW_ITEM*) aItem,
                                           COMPONENT_PIN_DRAW_TYPE );
    }


    /**
     * Return a list of pin object pointers from the draw item list.
     *
     * Note pin objects are owned by the draw list of the component.
     * Deleting any of the objects will leave list in a unstable state
     * and will likely segfault when the list is destroyed.
     *
     * @param aList - Pin list to place pin object pointers into.
     * @param aUnit - Unit number of pin to add to list.  Set to 0 to
     *                get pins from any component part.
     * @param aConvert - Convert number of pin to add to list.  Set to 0 to
     *                   get pins from any convert of component.
     */
    void GetPins( LIB_PIN_LIST& aList, int aUnit = 0, int aConvert = 0 );

    /**
     * Return pin object with the requested pin /a aNumber.
     *
     * @param aNumber - Number of the pin to find.
     * @param aUnit - Unit of the component to find.  Set to 0 if a specific
     *                unit number is not required.
     * @param aConvert - Alternate body style filter (DeMorgan).  Set to 0 if
     *                   no alternate body style is required.
     * @return The pin object if found.  Otherwise NULL.
     */
    LIB_PIN* GetPin( const wxString& aNumber, int aUnit = 0, int aConvert = 0 );

    /**
     * Move the component /a aOffset.
     *
     * @param aOffset - Offset displacement.
     */
    void SetOffset( const wxPoint& aOffset );

    /**
     * Remove duplicate draw items from list.
     */
    void RemoveDuplicateDrawItems();

    /**
     * Test if component has more than one body conversion type (DeMorgan).
     *
     * @return True if component has more than one conversion.
     */
    bool HasConversion() const;

    /**
     * Test if alias /a aName is in component alias list.
     *
     * Alias name comparisons are case insensitive.
     *
     * @param aName - Name of alias.
     * @return True if alias name in alias list.
     */
    bool HasAlias( const wxChar* aName )
    {
        wxASSERT( aName != NULL );
        return m_AliasList.Index( aName ) != wxNOT_FOUND;
    }

    /**
     * Clears the status flag all draw objects in this component.
     */
    void ClearStatus();

    /**
     * Checks all draw objects of component to see if they are with block.
     *
     * Use this method to mark draw objects as selected during block
     * functions.
     *
     * @param aRect - The bounding rectangle to test in draw items are inside.
     * @param aUnit - The current unit number to test against.
     * @param aConvert - Are the draw items being selected a conversion.
     * @param aEditPinByPin - Used to ignore pin selections when in edit pin
     *                        by pin mode is enabled.
     * @return The number of draw objects found inside the block select
     *         rectangle.
     */
    int SelectItems( EDA_Rect& aRect, int aUnit, int aConvert,
                     bool aEditPinByPin );

    /**
     * Clears all the draw items marked by a block select.
     */
    void ClearSelectedItems();

    /**
     * Deletes the select draw items marked by a block select.
     *
     * The name and reference field will not be deleted.  They are the
     * minimum drawing items required for any component.  Their properties
     * can be changed but the cannot be removed.
     */
    void DeleteSelectedItems();

    /**
     * Move the selected draw items marked by a block select.
     */
    void MoveSelectedItems( const wxPoint& aOffset );

    /**
     * Make a copy of the selected draw items marked by a block select.
     *
     * Fields are not copied.  Only component body items are copied.
     * Copying fields would result in duplicate fields which does not
     * make sense in this context.
     */
    void CopySelectedItems( const wxPoint& aOffset );

    /**
     * Horizontally (X axis) mirror selected draw items about a point.
     *
     * @param aCenter - Center point to mirror around.
     */
    void MirrorSelectedItemsH( const wxPoint& aCenter );

    /**
     * Locate a draw object.
     *
     * @param aUnit - Unit number of draw item.
     * @param aConvert - Body style of draw item.
     * @param aType - Draw object type, set to 0 to search for any type.
     * @param aPoint - Coordinate for hit testing.
     * @return The draw object if found.  Otherwise NULL.
     */
    LIB_DRAW_ITEM* LocateDrawItem( int aUnit, int aConvert, KICAD_T aType,
                                   const wxPoint& aPoint );

    /**
     * Locate a draw object (overlaid)
     *
     * @param aUnit - Unit number of draw item.
     * @param aConvert - Body style of draw item.
     * @param aType - Draw object type, set to 0 to search for any type.
     * @param aPoint - Coordinate for hit testing.
     * @param aTransform = the transform matrix
     * @return The draw object if found.  Otherwise NULL.
     */
    LIB_DRAW_ITEM* LocateDrawItem( int aUnit, int aConvert, KICAD_T aType,
                                   const wxPoint& aPoint,
                                   const int aTransfrom[2][2] );

    /**
     * Return a reference to the draw item list.
     *
     * @return LIB_DRAW_ITEM_LIST& - Reference to the draw item object list.
     */
    LIB_DRAW_ITEM_LIST& GetDrawItemList() { return drawings; }

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

    int GetPartCount() { return unitCount; }

    /**
     * Set or clear the alternate body style (DeMorgan) for the component.
     *
     * If the component already has an alternate body style set and a
     * asConvert if false, all of the existing draw items for the alternate
     * body style are remove.  If the alternate body style is not set and
     * asConvert is true, than the base draw items are duplicated and
     * added to the component.
     *
     * @param aSetConvert - Set or clear the component alternate body style.
     */
    void SetConversion( bool aSetConvert );
};


/**
 * Component library alias object definition.
 *
 * Component aliases are not really components.  They are references
 * to an actual component object.
 */
class LIB_ALIAS : public CMP_LIB_ENTRY
{
protected:
    /**
     * The actual component of the alias.
     *
     * @note - Do not delete the root component.  The root component is owned
     *         by library the component is part of.  Deleting the root component
     *         will likely cause EESchema to crash.
     *         Or, if the root component is deleted, aliases must be deleted or their .root member
     *         must be changed to point a new root component
     */
    LIB_COMPONENT* root;

public:
    LIB_ALIAS( const wxString& aName, LIB_COMPONENT* aRootComponent,
               CMP_LIBRARY* aLibrary = NULL );
    LIB_ALIAS( LIB_ALIAS& aAlias, CMP_LIBRARY* aLibrary = NULL );
    ~LIB_ALIAS();

    virtual wxString GetClass() const
    {
        return wxT( "LIB_ALIAS" );
    }

    /**
     * Get the alias root component.
     */
    LIB_COMPONENT* GetComponent() const
    {
        return root;
    }

    /**
     * Set the alias root component.
     */
    void SetComponent( LIB_COMPONENT* aComponent );
};


#endif  //  CLASS_LIBENTRY_H
