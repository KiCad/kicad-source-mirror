/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file class_libentry.h
 */

#ifndef CLASS_LIBENTRY_H
#define CLASS_LIBENTRY_H

#include <general.h>
#include <lib_draw_item.h>
#include <lib_field.h>

#include <map>


class LINE_READER;
class OUTPUTFORMATTER;
class CMP_LIBRARY;
class LIB_ALIAS;
class LIB_COMPONENT;
class LIB_FIELD;


/// Compiler controlled string compare function, either case independent or not:
inline int Cmp_KEEPCASE( const wxString& aString1, const wxString& aString2 )
{
#ifdef KICAD_KEEPCASE
    // case specificity:
    return aString1.Cmp( aString2 );
#else
    // case independence:
    return aString1.CmpNoCase( aString2 );
#endif
}


/**
 * LIB_ALIAS map sorting.
 */
struct AliasMapSort
{
    bool operator() ( const wxString& aItem1, const wxString& aItem2 ) const
    {
        return Cmp_KEEPCASE( aItem1, aItem2 ) < 0;
    }
};


/**
 * Alias map used by component library object.
 */
typedef std::map< wxString, LIB_ALIAS*, AliasMapSort > LIB_ALIAS_MAP;

typedef std::vector< LIB_ALIAS* > LIB_ALIASES;

/* values for member .m_options */
enum  LibrEntryOptions
{
    ENTRY_NORMAL,   // Libentry is a standard component (real or alias)
    ENTRY_POWER     // Libentry is a power symbol
};


/**
 * Component library alias object definition.
 *
 * Component aliases are not really components.  An alias uses the component definition
 * (graphic, pins...)  but has its own name, keywords and documentation.  Therefore, when
 * the component is modified, alias of this component are modified.  This is a simple
 * method to create components that have the same physical layout with different names
 * such as 74LS00, 74HC00 ... and many op amps.
 */
class LIB_ALIAS : public EDA_ITEM
{
    /**
     * The actual component of the alias.
     *
     * @note - Do not delete the root component.  The root component is actually shared by
     *         all of the aliases associated with it.  The component pointer will be delete
     *         in the destructor of the last alias that shares this component is deleted.
     *         Deleting the root component will likely cause Eeschema to crash.
     */
    LIB_COMPONENT*   root;

    friend class LIB_COMPONENT;

protected:
    wxString         name;
    wxString         description;  ///< documentation for info
    wxString         keyWords;     ///< keyword list (used for search for components by keyword)
    wxString         docFileName;  ///< Associate doc file name

public:
    LIB_ALIAS( const wxString& aName, LIB_COMPONENT* aRootComponent );
    LIB_ALIAS( const LIB_ALIAS& aAlias, LIB_COMPONENT* aRootComponent = NULL );

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

    bool IsRoot() const;

    CMP_LIBRARY* GetLibrary();

    virtual const wxString& GetName() const { return name; }

    virtual void SetName( const wxString& aName ) { name = aName; }

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
     * Function SaveDocs
     * rrite the entry document information to \a aFormatter in "*.dcm" format.
     *
     * @param aFormatter The #OUTPUTFORMATTER to write the alias documents to.
     * @return True if success writing else false.
     */
    bool SaveDoc( OUTPUTFORMATTER& aFormatter );

    /**
     * Case insensitive comparison of the component entry name.
     */
    bool operator==( const wxChar* aName ) const;
    bool operator!=( const wxChar* aName ) const
    {
        return !( *this == aName );
    }

    bool operator==( const LIB_ALIAS* aAlias ) const { return this == aAlias; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif
};

extern bool operator<( const LIB_ALIAS& aItem1, const LIB_ALIAS& aItem2 );

extern int LibraryEntryCompare( const LIB_ALIAS* aItem1, const LIB_ALIAS* aItem2 );


/**
 * Class LIB_COMPONENT
 * defines a library component object.
 *
 * A library component object is typically saved and loaded in a component library file (.lib).
 * Library components are different from schematic components.
 */
class LIB_COMPONENT : public EDA_ITEM
{
    friend class CMP_LIBRARY;
    friend class LIB_ALIAS;

    wxString           m_name;
    int                m_pinNameOffset;  ///< The offset in mils to draw the pin name.  Set to 0
                                         ///< to draw the pin name above the pin.
    bool               m_unitsLocked;    ///< True if component has multiple parts and changing
                                         ///< one part does not automatically change another part.
    bool               m_showPinNames;   ///< Determines if component pin names are visible.
    bool               m_showPinNumbers; ///< Determines if component pin numbers are visible.
    long               m_dateModified;   ///< Date the component was last modified.
    LibrEntryOptions   m_options;        ///< Special component features such as POWER or NORMAL.)
    int                m_unitCount;      ///< Number of units (parts) per package.
    LIB_ITEMS          drawings;         ///< How to draw this part.
    wxArrayString      m_FootprintList;  /**< List of suitable footprint names for the
                                              component (wild card names accepted). */
    LIB_ALIASES        m_aliases;        ///< List of alias object pointers associated with the
                                         ///< component.
    CMP_LIBRARY*       m_library;        ///< Library the component belongs to if any.

    static int  m_subpartIdSeparator;    ///< the separator char between
                                         ///< the subpart id and the reference
                                         ///< like U1A ( m_subpartIdSeparator = 0 ) or U1.A or U1-A
    static int  m_subpartFirstId;        ///< the ascii char value to calculate the subpart symbol id
                                         ///< from the part number: only 'A', 'a' or '1' can be used,
                                         ///< other values have no sense.
private:
    void deleteAllFields();

public:
    LIB_COMPONENT( const wxString& aName, CMP_LIBRARY* aLibrary = NULL );
    LIB_COMPONENT( LIB_COMPONENT& aComponent, CMP_LIBRARY* aLibrary = NULL );

    virtual ~LIB_COMPONENT();

    virtual wxString GetClass() const
    {
        return wxT( "LIB_COMPONENT" );
    }


    virtual void SetName( const wxString& aName );

    wxString GetName() { return m_name; }

    wxString GetLibraryName();

    CMP_LIBRARY* GetLibrary() { return m_library; }

    wxArrayString GetAliasNames( bool aIncludeRoot = true ) const;

    size_t GetAliasCount() const { return m_aliases.size(); }

    LIB_ALIAS* GetAlias( size_t aIndex );

    LIB_ALIAS* GetAlias( const wxString& aName );

    /**
     * Function AddAlias
     *
     * Add an alias \a aName to the component.
     *
     * Duplicate alias names are not added to the alias list.  Debug builds will raise an
     * assertion.  Release builds will fail silently.
     *
     * @param aName - Name of alias to add.
     */
    void AddAlias( const wxString& aName );

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

    void RemoveAllAliases();

    wxArrayString& GetFootPrints() { return m_FootprintList; }

    /**
     * Function GetBoundingBox
     * @return the component boundary box ( in user coordinates )
     * @param aUnit = unit selection = 0, or 1..n
     * @param aConvert = 0, 1 or 2
     *  If aUnit == 0, unit is not used
     *  if aConvert == 0 Convert is non used
     *  Invisible fields are not taken in account
     **/
    EDA_RECT GetBoundingBox( int aUnit, int aConvert ) const;

    /**
     * Function GetBodyBoundingBox
     * @return the component boundary box ( in user coordinates ) without fields
     * @param aUnit = unit selection = 0, or 1..n
     * @param aConvert = 0, 1 or 2
     *  If aUnit == 0, unit is not used
     *  if aConvert == 0 Convert is non used
     *  Fields are not taken in account
     **/
    EDA_RECT GetBodyBoundingBox( int aUnit, int aConvert ) const;

    /**
     * Function SaveDateAndTime
     * write the date and time of component to \a aFile in the format:
     * "Ti yy/mm/jj hh:mm:ss"
     *
     * @param aFormatter A reference to an #OUTPUTFORMATTER object containing the
     *                   output format to write to.
     * @return True if the date and time were successfully written to \a aFormatter.
     */
    bool SaveDateAndTime( OUTPUTFORMATTER& aFormatter );

    bool LoadDateAndTime( char* aLine );

    /**
     * Function Save
     * writes the data structures out to \a aFormatter in the component library "*.lib"
     * format.
     *
     * @param aFormatter A reference to an OUTPUTFORMATTER to write to.
     * @return True if success writing else false.
     */
    bool Save( OUTPUTFORMATTER& aFormatter );

    /**
     * Load component definition from \a aReader.
     *
     * @param aReader A LINE_READER object to load file from.
     * @param aErrorMsg - Description of error on load failure.
     * @return True if the load was successful, false if there was an error.
     */
    bool Load( LINE_READER& aReader, wxString& aErrorMsg );
    bool LoadField( LINE_READER& aReader, wxString& aErrorMsg );
    bool LoadDrawEntries( LINE_READER& aReader, wxString& aErrorMsg );
    bool LoadAliases( char* aLine, wxString& aErrorMsg );
    bool LoadFootprints( LINE_READER& aReader, wxString& aErrorMsg );

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
    void GetFields( LIB_FIELDS& aList );

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
     * @param aPanel - Window to draw on. Can be NULL if not available.
     * @param aDc - Device context to draw on.
     * @param aOffset - Position to component.
     * @param aMulti - Component unit if multiple parts per component.
     * @param aConvert - Component conversion (DeMorgan) if available.
     * @param aDrawMode - Device context drawing mode, see wxDC.
     * @param aColor - Color to draw component.
     * @param aTransform - Coordinate adjustment settings.
     * @param aShowPinText - Show pin text if true.
     * @param aDrawFields - Draw field text if true otherwise just draw
     *                      body items (useful to draw a body in schematic,
     *                      because fields of schematic components replace
     *                      the lib component fields).
     * @param aOnlySelected - Draws only the body items that are selected.
     *                        Used for block move redraws.
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDc, const wxPoint& aOffset,
               int aMulti, int aConvert, GR_DRAWMODE aDrawMode,
               EDA_COLOR_T aColor = UNSPECIFIED_COLOR,
               const TRANSFORM& aTransform = DefaultTransform,
               bool aShowPinText = true, bool aDrawFields = true,
               bool aOnlySelected = false );

    /**
     * Plot lib component to plotter.
     * Lib Fields not are plotted here, because this plot function
     * is used to plot schematic items, which have they own fields
     *
     * @param aPlotter - Plotter object to plot to.
     * @param aUnit - Component part to plot.
     * @param aConvert - Component alternate body style to plot.
     * @param aOffset - Distance to shift the plot coordinates.
     * @param aTransform - Component plot transform matrix.
     */
    void Plot( PLOTTER* aPlotter, int aUnit, int aConvert, const wxPoint& aOffset,
                const TRANSFORM& aTransform );

    /**
     * Plot Lib Fields only of the component to plotter.
     * is used to plot the full lib component, outside the schematic
     *
     * @param aPlotter - Plotter object to plot to.
     * @param aUnit - Component part to plot.
     * @param aConvert - Component alternate body style to plot.
     * @param aOffset - Distance to shift the plot coordinates.
     * @param aTransform - Component plot transform matrix.
     */
    void PlotLibFields( PLOTTER* aPlotter, int aUnit, int aConvert,
                        const wxPoint& aOffset, const TRANSFORM& aTransform );

    /**
     * Add a new draw \a aItem to the draw object list.
     *
     * @param aItem - New draw object to add to component.
     */
    void AddDrawItem( LIB_ITEM* aItem );

    /**
     * Remove draw \a aItem from list.
     *
     * @param aItem - Draw item to remove from list.
     * @param aPanel - Panel to remove part from.
     * @param aDc - Device context to remove part from.
     */
    void RemoveDrawItem( LIB_ITEM* aItem, EDA_DRAW_PANEL* aPanel = NULL, wxDC* aDc = NULL );

    /**
     * Return the next draw object pointer.
     *
     * @param aItem - Pointer to the current draw item.  Setting item NULL
     *                with return the first item of type in the list.
     * @param aType - type of searched item (filter).
     *                if TYPE_NOT_INIT search for all items types
     * @return - The next drawing object in the list if found, otherwise NULL.
     */
    LIB_ITEM* GetNextDrawItem( LIB_ITEM* aItem = NULL, KICAD_T aType = TYPE_NOT_INIT );

    /**
     * Return the next pin object from the draw list.
     *
     * This is just a pin object specific version of GetNextDrawItem().
     *
     * @param aItem - Pointer to the previous pin item, or NULL to get the
     *                first pin in the draw object list.
     * @return - The next pin object in the list if found, otherwise NULL.
     */
    LIB_PIN* GetNextPin( LIB_PIN* aItem = NULL )
    {
        return (LIB_PIN*) GetNextDrawItem( (LIB_ITEM*) aItem, LIB_PIN_T );
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
    void GetPins( LIB_PINS& aList, int aUnit = 0, int aConvert = 0 );

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
    int SelectItems( EDA_RECT& aRect, int aUnit, int aConvert, bool aEditPinByPin );

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
     * Vertically (Y axis) mirror selected draw items about a point.
     *
     * @param aCenter - Center point to mirror around.
     */
    void MirrorSelectedItemsV( const wxPoint& aCenter );

    /**
     * Rotate CCW selected draw items about a point.
     *
     * @param aCenter - Center point to mirror around.
     */
    void RotateSelectedItems( const wxPoint& aCenter );

    /**
     * Locate a draw object.
     *
     * @param aUnit - Unit number of draw item.
     * @param aConvert - Body style of draw item.
     * @param aType - Draw object type, set to 0 to search for any type.
     * @param aPoint - Coordinate for hit testing.
     * @return The draw object if found.  Otherwise NULL.
     */
    LIB_ITEM* LocateDrawItem( int aUnit, int aConvert, KICAD_T aType, const wxPoint& aPoint );

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
    LIB_ITEM* LocateDrawItem( int aUnit, int aConvert, KICAD_T aType,
                              const wxPoint& aPoint, const TRANSFORM& aTransform );

    /**
     * Return a reference to the draw item list.
     *
     * @return LIB_ITEMS& - Reference to the draw item object list.
     */
    LIB_ITEMS& GetDrawItemList() { return drawings; }

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

    int GetPartCount() const { return m_unitCount; }

    /**
     * Function IsMulti
     * @return true if the component has multiple parts per package.
     * When happens, the reference has a sub reference ti identify part
     */
    bool IsMulti() const { return m_unitCount > 1; }

    /**
     * Function SubReference
     * @return the sub reference for component having multiple parts per package.
     * The sub reference identify the part (or unit)
     * @param aUnit = the part identifier ( 1 to max count)
     * @param aAddSeparator = true (default) to prpebd the sub ref
     *    by the separator symbol (if any)
     * Note: this is a static function.
     */
    static wxString SubReference( int aUnit, bool aAddSeparator = true );

    // Accessors to sub ref parameters
    static int GetSubpartIdSeparator() { return m_subpartIdSeparator; }

    /** return a reference to m_subpartIdSeparator,
     * only for read/save setting functions
     */
    static int* SubpartIdSeparatorPtr() { return &m_subpartIdSeparator; }

    static int GetSubpartFirstId() { return m_subpartFirstId; }

    /** return a reference to m_subpartFirstId, only for read/save setting functions
     */
    static int* SubpartFirstIdPtr() { return &m_subpartFirstId; }

    /** Set the separator char between the subpart id and the reference
     * 0 (no separator) or '.' , '-' and '_'
     * and the ascii char value to calculate the subpart symbol id from the part number:
     * 'A' or '1' only are allowed. (to print U1.A or U1.1)
     * if this is a digit, a number is used as id symbol
     * Note also if the subpart symbol is a digit, the separator cannot be null.
     * @param aSep = the separator symbol (0 (no separator) or '.' , '-' and '_')
     * @param aFirstId = the Id of the first part ('A' or '1')
     */
    static void SetSubpartIdNotation( int aSep, int aFirstId );

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

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif
};

#endif  //  CLASS_LIBENTRY_H
