/******************************************/
/*  Library component object definitions. */
/******************************************/

#ifndef CLASS_LIBENTRY_H
#define CLASS_LIBENTRY_H

#include "lib_draw_item.h"
#include "lib_field.h"

#include <map>


class CMP_LIBRARY;
class LIB_ALIAS;


/**
 * LIB_ALIAS map sorting.
 */
struct AliasMapSort
{
    bool operator() ( const wxString& aItem1, const wxString& aItem2 ) const
        { return aItem1.CmpNoCase( aItem2 ) < 0; }
};

/**
 * Alias map used by component library object.
 */
typedef std::map< wxString, LIB_ALIAS*, AliasMapSort > LIB_ALIAS_MAP;

typedef std::vector< LIB_ALIAS* > LIB_ALIAS_LIST;

/* Types for components in libraries
 * components can be a true component or an alias of a true component.
 */
enum LibrEntryType
{
    ROOT,       /* This is a true component standard LIB_COMPONENT */
    ALIAS       /* This is an alias of a true component */
};

/* values for member .m_options */
enum  LibrEntryOptions
{
    ENTRY_NORMAL,   // Libentry is a standard component (real or alias)
    ENTRY_POWER     // Libentry is a power symbol
};


/**
 * Class CMP_LIB_ENTRY
 * is a base class to describe library components and aliases.
 *
 * This class is not to be used directly.
 */
class CMP_LIB_ENTRY : public EDA_BaseStruct
{
protected:
    wxString         name;

    /// Library object that entry is attached to.
    CMP_LIBRARY*     library;

    /// Entry type, either ROOT or ALIAS.
    LibrEntryType    type;

    wxString         description;  ///< documentation for info
    wxString         keyWords;     ///< keyword list (used for search for components by keyword)
    wxString         docFileName;  ///< Associate doc file name

public:
    CMP_LIB_ENTRY( LibrEntryType aType, const wxString& aName, CMP_LIBRARY* aLibrary = NULL );
    CMP_LIB_ENTRY( CMP_LIB_ENTRY& aEntry, CMP_LIBRARY* aLibrary = NULL );

    virtual ~CMP_LIB_ENTRY();

    virtual wxString GetClass() const
    {
        return wxT( "CMP_LIB_ENTRY" );
    }

    virtual wxString GetLibraryName();

    CMP_LIBRARY* GetLibrary() { return library; }

    virtual const wxString& GetName() const { return name; }

    virtual void SetName( const wxString& aName ) { name = aName; }

    bool isComponent() const { return type == ROOT; }

    bool isAlias() const { return type == ALIAS; }

    int GetType() const { return type; }

    void SetDescription( const wxString& aDescription )
    {
        description = aDescription;
    }

    wxString GetDescription() const { return description; }

    void SetKeyWords( const wxString& aKeyWords )
    {
        keyWords = aKeyWords;
    }

    wxString GetKeyWords() const { return keyWords; }

    void SetDocFileName( const wxString& aDocFileName )
    {
        docFileName = aDocFileName;
    }

    wxString GetDocFileName() const { return docFileName; }

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

    bool operator==( const wxString& aName ) const { return *this == ( const wxChar* ) aName; }
};

extern bool operator<( const CMP_LIB_ENTRY& aItem1, const CMP_LIB_ENTRY& aItem2 );

extern int LibraryEntryCompare( const CMP_LIB_ENTRY* aItem1, const CMP_LIB_ENTRY* aItem2 );


/**
 * Library component object definition.
 *
 * A library component object is typically saved and loaded in a component library file (.lib).
 * Library components are different from schematic components.
 */
class LIB_COMPONENT : public CMP_LIB_ENTRY
{
    int                m_pinNameOffset;  ///< The offset in mils to draw the pin name.  Set to 0
                                         ///< to draw the pin name above the pin.
    bool               m_unitsLocked;    ///< True if component has multiple parts and changing
                                         ///< one part does not automatically change another part.
    bool               m_showPinNames;   ///< Determines if component pin names are visible.
    bool               m_showPinNumbers; ///< Determines if component pin numbers are visible.
    long               m_dateModified;   ///< Date the component was last modified.
    LibrEntryOptions   m_options;        ///< Special component features such as POWER or NORMAL.)
    int                unitCount;        ///< Number of units (parts) per package.
    LIB_DRAW_ITEM_LIST drawings;         ///< How to draw this part.
    wxArrayString      m_FootprintList;  /**< List of suitable footprint names for the
                                              component (wild card names accepted). */
    LIB_ALIAS_LIST     m_aliases;        ///< List of alias object pointers associated with the
                                         ///< component.

    void deleteAllFields();

    friend class CMP_LIBRARY;
    friend class LIB_ALIAS;

public:
    LIB_COMPONENT( const wxString& aName, CMP_LIBRARY* aLibrary = NULL );
    LIB_COMPONENT( LIB_COMPONENT& aComponent, CMP_LIBRARY* aLibrary = NULL );

    virtual ~LIB_COMPONENT();

    virtual wxString GetClass() const
    {
        return wxT( "LIB_COMPONENT" );
    }


    virtual void SetName( const wxString& aName )
    {
        CMP_LIB_ENTRY::SetName( aName );
        GetValueField().m_Text = aName;
    }

    virtual wxString GetLibraryName();

    wxArrayString GetAliasNames( bool aIncludeRoot = true ) const;

    size_t GetAliasCount() const { return m_aliases.size(); }

    LIB_ALIAS* GetAlias( size_t aIndex );

    LIB_ALIAS* GetAlias( const wxString& aName );

    /**
     * Test if alias \a aName is in component alias list.
     *
     * Alias name comparisons are case insensitive.
     *
     * @param aName - Name of alias.
     * @return True if alias name in alias list.
     */
    bool HasAlias( const wxString& aName ) const;

    void SetAliases( const wxArrayString& aAliasList );

    void RemoveAlias( const wxString& aName );

    LIB_ALIAS* RemoveAlias( LIB_ALIAS* aAlias );

    wxArrayString& GetFootPrints() { return m_FootprintList; }

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
     * Load component definition from \a aFile.
     *
     * @param aFile - File descriptor of file to load form.
     * @param aLine - The first line of the component definition.
     * @param aLineNum - The current line number in the file.
     * @param aErrorMsg - Description of error on load failure.
     * @return True if the load was successful, false if there was an error.
     */
    bool Load( FILE* aFile, char* aLine, int* aLineNum, wxString& aErrorMsg );
    bool LoadField( char* aLine, wxString& aErrorMsg );
    bool LoadDrawEntries( FILE* aFile, char* aLine, int* aLineNum, wxString& aErrorMsg );
    bool LoadAliases( char* aLine, wxString& aErrorMsg );
    bool LoadFootprints( FILE* aFile, char* aLine, int* aLineNum, wxString& aErrorMsg );

    bool IsPower() { return m_options == ENTRY_POWER; }
    bool IsNormal() { return m_options == ENTRY_NORMAL; }

    void SetPower() { m_options = ENTRY_POWER; }
    void SetNormal() { m_options = ENTRY_NORMAL; }

    void LockUnits( bool aLockUnits ) { m_unitsLocked = aLockUnits; }
    bool UnitsLocked() { return m_unitsLocked; }

    /**
     * Function SetFields
     * overwrites all the existing in this component with fields supplied
     * in \a aFieldsList.  The only known caller of this function is the
     * library component field editor, and it establishes needed behavior.
     *
`     * @param aFieldsList is a set of fields to import, removing all previous fields.
     */
    void SetFields( const std::vector <LIB_FIELD>& aFieldsList );

    /**
     * Function GetFields
     * returns a list of fields withing this component. The only known caller of
     * this function is the library component field editor, and it establishes
     * needed behavior.
     *
     * @param aList - List to add fields to
     */
    void GetFields( LIB_FIELD_LIST& aList );

    /**
     * Function FindField
     * finds a field within this component matching \a aFieldName and returns
     * it or NULL if not found.
     */
    LIB_FIELD* FindField( const wxString& aFieldName );

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
               const TRANSFORM& aTransform = DefaultTransform,
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
               const TRANSFORM& aTransform );

    /**
     * Add a new draw \a aItem to the draw object list.
     *
     * @param item - New draw object to add to component.
     */
    void AddDrawItem( LIB_DRAW_ITEM* aItem );

    /**
     * Remove draw \a aItem from list.
     *
     * @param aItem - Draw item to remove from list.
     * @param aPanel - Panel to remove part from.
     * @param aDc - Device context to remove part from.
     */
    void RemoveDrawItem( LIB_DRAW_ITEM* aItem, WinEDA_DrawPanel* aPanel = NULL, wxDC* aDc = NULL );

    /**
     * Return the next draw object pointer.
     *
     * @param aItem - Pointer to the current draw item.  Setting item NULL
     *                with return the first item of type in the list.
     * @param aType - type of searched item (filter).
     *                if TYPE_NOT_INIT search for all items types
     * @return - The next drawing object in the list if found, otherwise NULL.
     */
    LIB_DRAW_ITEM* GetNextDrawItem( LIB_DRAW_ITEM* aItem = NULL, KICAD_T aType = TYPE_NOT_INIT );

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
        return (LIB_PIN*) GetNextDrawItem( (LIB_DRAW_ITEM*) aItem, COMPONENT_PIN_DRAW_TYPE );
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
     * Return pin object with the requested pin \a aNumber.
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
     * Move the component \a aOffset.
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
    int SelectItems( EDA_Rect& aRect, int aUnit, int aConvert, bool aEditPinByPin );

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
    LIB_DRAW_ITEM* LocateDrawItem( int aUnit, int aConvert, KICAD_T aType, const wxPoint& aPoint );

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
                                   const wxPoint& aPoint, const TRANSFORM& aTransfrom );

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

    /** function IsMulti
     * @return true if the component has multiple parts per package.
     * When happens, the reference has a sub reference ti identify part
     */
    bool IsMulti() { return unitCount > 1; }

    /** function IsMulti
     * @return the sub reference for component having multiple parts per package.
     * The sub reference identify the part (or unit)
     * @param aUnit = the part identifier ( 1 to max count)
     * Note: this is a static function.
     */
    static wxString ReturnSubReference( int aUnit );

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

    /**
     * Set the offset in mils of the pin name text from the pin symbol.
     *
     * Set the offset to 0 to draw the pin name above the pin symbol.
     *
     * @param aOffset - The offset in mils.
     */
    void SetPinNameOffset( int aOffset ) { m_pinNameOffset = aOffset; }

    int GetPinNameOffset() { return m_pinNameOffset; }

    /**
     * Set or clear the pin name visibility flag.
     *
     * @param aShow - True to make the component pin names visible.
     */
    void SetShowPinNames( bool aShow ) { m_showPinNames = aShow; }

    bool ShowPinNames() { return m_showPinNames; }

    /**
     * Set or clear the pin number visibility flag.
     *
     * @param aShow - True to make the component pin numbers visible.
     */
    void SetShowPinNumbers( bool aShow ) { m_showPinNumbers = aShow; }

    bool ShowPinNumbers() { return m_showPinNumbers; }

    bool operator==( const LIB_COMPONENT* aComponent ) const { return this == aComponent; }
};


/**
 * Component library alias object definition.
 *
 * Component aliases are not really components.  They are references
 * to an actual component object.
 */
class LIB_ALIAS : public CMP_LIB_ENTRY
{
    friend class LIB_COMPONENT;

protected:
    /**
     * The actual component of the alias.
     *
     * @note - Do not delete the root component.  The root component is actually shared by
     *         all of the aliases associated with it.  The component pointer will be delete
     *         in the destructor of the last alias that shares this component is deleted.
     *         Deleting the root component will likely cause EESchema to crash.
     */
    LIB_COMPONENT* root;

public:
    LIB_ALIAS( const wxString& aName, LIB_COMPONENT* aRootComponent );
    LIB_ALIAS( LIB_ALIAS& aAlias, LIB_COMPONENT* aRootComponent = NULL );

    virtual ~LIB_ALIAS();

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

    virtual wxString GetLibraryName();

    bool IsRoot() const { return name.CmpNoCase( root->GetName() ) == 0; }

    bool operator==( const LIB_ALIAS* aAlias ) const { return this == aAlias; }
};


#endif  //  CLASS_LIBENTRY_H
