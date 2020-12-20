/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see change_log.txt for contributors.
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

#ifndef CLASS_LIBENTRY_H
#define CLASS_LIBENTRY_H

#include <general.h>
#include <lib_tree_item.h>
#include <lib_item.h>
#include <lib_field.h>
#include <vector>
#include <multivector.h>

class EDA_RECT;
class LINE_READER;
class OUTPUTFORMATTER;
class PART_LIB;
class LIB_PART;
class LIB_FIELD;
class TEST_LIB_PART_FIXTURE;


typedef std::shared_ptr<LIB_PART>       PART_SPTR;      ///< shared pointer to LIB_PART
typedef std::weak_ptr<LIB_PART>         PART_REF;       ///< weak pointer to LIB_PART
typedef MULTIVECTOR<LIB_ITEM, LIB_ARC_T, LIB_FIELD_T> LIB_ITEMS_CONTAINER;
typedef LIB_ITEMS_CONTAINER::ITEM_PTR_VECTOR LIB_ITEMS;


/* values for member .m_options */
enum  LIBRENTRYOPTIONS
{
    ENTRY_NORMAL,   // Libentry is a standard part (real or alias)
    ENTRY_POWER     // Libentry is a power symbol
};


extern bool operator<( const LIB_PART& aItem1, const LIB_PART& aItem2 );


struct PART_DRAW_OPTIONS
{
    TRANSFORM transform;           // Coordinate adjustment settings
    bool draw_visible_fields;      // Whether to draw "visible" fields
    bool draw_hidden_fields;       // Whether to draw "hidden" fields
    bool show_elec_type;           // Whether to show the pin electrical type

    PART_DRAW_OPTIONS()
    {
        transform = DefaultTransform;
        draw_visible_fields = true;
        draw_hidden_fields = true;
        show_elec_type = false;
    }
};


struct PART_UNITS
{
    int m_unit;                       ///< The unit number.
    int m_convert;                    ///< The alternate body style of the unit.
    std::vector<LIB_ITEM*> m_items;   ///< The items unique to this unit and alternate body style.
};


/**
 * Define a library symbol object.
 *
 * A library symbol object is typically saved and loaded in a part library file (.lib).
 * Library symbols are different from schematic symbols.
 */
class LIB_PART : public EDA_ITEM, public LIB_TREE_ITEM
{
public:
    LIB_PART( const wxString& aName, LIB_PART* aParent = nullptr, PART_LIB* aLibrary = nullptr );

    /**
     * Copy constructor.
     */
    LIB_PART( const LIB_PART& aPart, PART_LIB* aLibrary = NULL );

    virtual ~LIB_PART();

    ///< http://www.boost.org/doc/libs/1_55_0/libs/smart_ptr/sp_techniques.html#weak_without_shared
    PART_SPTR SharedPtr() { return m_me; }

    /**
     * Function Duplicate
     * Creates a copy of a LIB_PART and assigns unique KIIDs to the copy and its children
     */
    virtual LIB_PART* Duplicate() const
    {
        LIB_PART* dupe                    = new LIB_PART( *this, m_library );
        const_cast<KIID&>( dupe->m_Uuid ) = KIID();

        for( LIB_ITEM& item : dupe->m_drawings )
            const_cast<KIID&>( item.m_Uuid ) = KIID();

        return dupe;
    }

    void SetParent( LIB_PART* aParent = nullptr );
    PART_REF& GetParent() { return m_parent; }

    virtual wxString GetClass() const override
    {
        return wxT( "LIB_PART" );
    }

    virtual void SetName( const wxString& aName );
    wxString GetName() const override { return m_name; }

    LIB_ID GetLibId() const override { return m_libId; }
    void SetLibId( const LIB_ID& aLibId ) { m_libId = aLibId; }

    wxString GetLibNickname() const override { return GetLibraryName(); }

    void SetDescription( const wxString& aDescription ) { m_description = aDescription; }

    wxString GetDescription() override
    {
        if( m_description.IsEmpty() && IsAlias() )
        {
            if( PART_SPTR parent = m_parent.lock() )
                return parent->GetDescription();
        }

        return m_description;
    }

    void SetKeyWords( const wxString& aKeyWords ) { m_keyWords = aKeyWords; }

    wxString GetKeyWords() const
    {
        if( m_keyWords.IsEmpty() && IsAlias() )
        {
            if( PART_SPTR parent = m_parent.lock() )
                return parent->GetKeyWords();
        }

        return m_keyWords;
    }

    wxString GetSearchText() override;

    /**
     * For symbols derived from other symbols, IsRoot() indicates no derivation.
     */
    bool IsRoot() const override { return m_parent.use_count() == 0; }
    bool IsAlias() const { return !m_parent.expired() && m_parent.use_count() > 0; }

    const wxString GetLibraryName() const;

    PART_LIB* GetLib()                { return m_library; }
    void SetLib( PART_LIB* aLibrary ) { m_library = aLibrary; }

    timestamp_t GetLastModDate() const { return m_lastModDate; }

    void SetFPFilters( const wxArrayString& aFilters ) { m_fpFilters = aFilters; }

    wxArrayString GetFPFilters() const
    {
        if( m_fpFilters.IsEmpty() && IsAlias() )
        {
            if( PART_SPTR parent = m_parent.lock() )
                return parent->GetFPFilters();
        }

        return m_fpFilters;
    }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /**
     * Get the bounding box for the symbol.
     *
     * @return the part bounding box ( in user coordinates )
     * @param aUnit = unit selection = 0, or 1..n
     * @param aConvert = 0, 1 or 2
     *  If aUnit == 0, unit is not used
     *  if aConvert == 0 Convert is non used
     *  Invisible fields are not taken in account
     **/
    const EDA_RECT GetUnitBoundingBox( int aUnit, int aConvert ) const;

    /**
     * Get the symbol bounding box excluding fields.
     *
     * @return the part bounding box ( in user coordinates ) without fields
     * @param aUnit = unit selection = 0, or 1..n
     * @param aConvert = 0, 1 or 2
     *  If aUnit == 0, unit is not used
     *  if aConvert == 0 Convert is non used
     *  Fields are not taken in account
     **/
    const EDA_RECT GetBodyBoundingBox( int aUnit, int aConvert ) const;

    const EDA_RECT GetBoundingBox() const override
    {
        return GetUnitBoundingBox( 0, 0 );
    }

    bool IsPower() const  { return m_options == ENTRY_POWER; }
    bool IsNormal() const { return m_options == ENTRY_NORMAL; }

    void SetPower()     { m_options = ENTRY_POWER; }
    void SetNormal()    { m_options = ENTRY_NORMAL; }

    /**
     * Set interchangeable the property for part units.
     * @param aLockUnits when true then units are set as not interchangeable.
     */
    void LockUnits( bool aLockUnits ) { m_unitsLocked = aLockUnits; }

    /**
     * Check whether part units are interchangeable.
     * @return False when interchangeable, true otherwise.
     */
    bool UnitsLocked() const { return m_unitsLocked; }

    /**
     * Overwrite all the existing fields in this symbol with fields supplied
     * in \a aFieldsList.
     *
     * The only known caller of this function is the library part field editor, and it
     * establishes needed behavior.
     *
     * @param aFieldsList is a set of fields to import, removing all previous fields.
     */
    void SetFields( const std::vector <LIB_FIELD>& aFieldsList );

    /**
     * Return a list of fields within this part.
     *
     * @param aList - List to add fields to
     */
    void GetFields( std::vector<LIB_FIELD*>& aList );
    void GetFields( std::vector<LIB_FIELD>& aList );

    /**
     * Add a field.  Takes ownership of the pointer.
     * @param aField
     */
    void AddField( LIB_FIELD* aField );

    /**
     * Find a field within this part matching \a aFieldName and returns it
     * or NULL if not found.
     */
    LIB_FIELD* FindField( const wxString& aFieldName );

    const LIB_FIELD* FindField( const wxString& aFieldName ) const;

    /**
     * Return pointer to the requested field.
     *
     * @param aId - Id of field to return.
     * @return The field if found, otherwise NULL.
     */
    LIB_FIELD* GetField( int aId ) const;

    /** Return reference to the value field. */
    LIB_FIELD& GetValueField();

    /** Return reference to the reference designator field. */
    LIB_FIELD& GetReferenceField();

    /** Return reference to the footprint field */
    LIB_FIELD& GetFootprintField();

    /** Return reference to the datasheet field. */
    LIB_FIELD& GetDatasheetField();

    /**
     * Print part.
     *
     * @param aOffset - Position of part.
     * @param aMulti - unit if multiple units per part.
     * @param aConvert - Component conversion (DeMorgan) if available.
     * @param aOpts - Drawing options
     */
    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                int aMulti, int aConvert, const PART_DRAW_OPTIONS& aOpts );

    /**
     * Plot lib part to plotter.
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
     * Plot Lib Fields only of the part to plotter.
     * is used to plot the full lib part, outside the schematic
     *
     * @param aPlotter - Plotter object to plot to.
     * @param aUnit - Component part to plot.
     * @param aConvert - Component alternate body style to plot.
     * @param aOffset - Distance to shift the plot coordinates.
     * @param aTransform - Component plot transform matrix.
     */
    void PlotLibFields( PLOTTER* aPlotter, int aUnit, int aConvert, const wxPoint& aOffset,
                        const TRANSFORM& aTransform );

    /**
     * Add a new draw \a aItem to the draw object list.
     *
     * @param aItem - New draw object to add to part.
     */
    void AddDrawItem( LIB_ITEM* aItem );

    /**
     * Remove draw \a aItem from list.
     *
     * @param aItem - Draw item to remove from list.
     */
    void RemoveDrawItem( LIB_ITEM* aItem );

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

    size_t GetPinCount() const { return m_drawings.size( LIB_PIN_T ); }

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
     * Note pin objects are owned by the draw list of the part.
     * Deleting any of the objects will leave list in a unstable state
     * and will likely segfault when the list is destroyed.
     *
     * @param aList - Pin list to place pin object pointers into.
     * @param aUnit - Unit number of pin to add to list.  Set to 0 to
     *                get pins from any part unit.
     * @param aConvert - Convert number of pin to add to list.  Set to 0 to
     *                   get pins from any convert of part.
     */
    void GetPins( LIB_PINS& aList, int aUnit = 0, int aConvert = 0 );

    /**
     * Return pin object with the requested pin \a aNumber.
     *
     * @param aNumber - Number of the pin to find.
     * @param aUnit - Unit of the part to find.  Set to 0 if a specific
     *                unit number is not required.
     * @param aConvert - Alternate body style filter (DeMorgan).  Set to 0 if
     *                   no alternate body style is required.
     * @return The pin object if found.  Otherwise NULL.
     */
    LIB_PIN* GetPin( const wxString& aNumber, int aUnit = 0, int aConvert = 0 );

    /**
     * Return true if this part's pins do not match another part's pins. This
     * is used to detect whether the project cache is out of sync with the
     * system libs.
     *
     * @param aOtherPart - The other library part to test
     * @param aTestNums - Whether two pins at the same point must have the same number.
     * @param aTestNames - Whether two pins at the same point must have the same name.
     * @param aTestType - Whether two pins at the same point must have the same electrical type.
     * @param aTestOrientation - Whether two pins at the same point must have the same orientation.
     * @param aTestLength - Whether two pins at the same point must have the same length.
     */
    bool PinsConflictWith( LIB_PART& aOtherPart, bool aTestNums, bool aTestNames,
                           bool aTestType, bool aTestOrientation, bool aTestLength );

    /**
     * Move the part \a aOffset.
     *
     * @param aOffset - Offset displacement.
     */
    void SetOffset( const wxPoint& aOffset );

    /**
     * Remove duplicate draw items from list.
     */
    void RemoveDuplicateDrawItems();

    /**
     * Test if part has more than one body conversion type (DeMorgan).
     *
     * @return True if part has more than one conversion.
     */
    bool HasConversion() const;

    /**
     * Clears the status flag all draw objects in this part.
     */
    void ClearTempFlags();
    void ClearEditFlags();

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
    LIB_ITEM* LocateDrawItem( int aUnit, int aConvert, KICAD_T aType, const wxPoint& aPoint,
                              const TRANSFORM& aTransform );

    /**
     * Return a reference to the draw item list.
     *
     * @return LIB_ITEMS_CONTAINER& - Reference to the draw item object container.
     */
    LIB_ITEMS_CONTAINER& GetDrawItems() { return m_drawings; }

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    /**
     * Set the units per part count.
     *
     * If the count is greater than the current count, then the all of the
     * current draw items are duplicated for each additional part.  If the
     * count is less than the current count, all draw objects for units
     * greater that count are removed from the part.
     *
     * @param aCount - Number of units per package.
     * @param aDuplicateDrawItems Create duplicate draw items of unit 1 for each additionl unit.
     */
    void SetUnitCount( int aCount, bool aDuplicateDrawItems = true );
    int GetUnitCount() const override;

    /**
     * Return an identifier for \a aUnit for symbols with units.
     */
    wxString GetUnitReference( int aUnit ) override;

    /**
     * @return true if the part has multiple units per part.
     * When true, the reference has a sub reference to identify part.
     */
    bool IsMulti() const { return m_unitCount > 1; }

    /**
     * @return the sub reference for part having multiple units per part.
     * The sub reference identify the part (or unit)
     * @param aUnit = the part identifier ( 1 to max count)
     * @param aAddSeparator = true (default) to prepend the sub ref
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

    /**
     * Set the separator char between the subpart id and the reference
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
     * Set or clear the alternate body style (DeMorgan) for the part.
     *
     * If the part already has an alternate body style set and a
     * asConvert if false, all of the existing draw items for the alternate
     * body style are remove.  If the alternate body style is not set and
     * asConvert is true, than the base draw items are duplicated and
     * added to the part.
     *
     * @param aSetConvert - Set or clear the part alternate body style.
     * @param aDuplicatePins - Duplicate all pins from original body style if true.
     */
    void SetConversion( bool aSetConvert, bool aDuplicatePins = true );

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
     * @param aShow - True to make the part pin names visible.
     */
    void SetShowPinNames( bool aShow ) { m_showPinNames = aShow; }
    bool ShowPinNames() { return m_showPinNames; }

    /**
     * Set or clear the pin number visibility flag.
     *
     * @param aShow - True to make the part pin numbers visible.
     */
    void SetShowPinNumbers( bool aShow ) { m_showPinNumbers = aShow; }
    bool ShowPinNumbers() { return m_showPinNumbers; }

    /**
     * Set or clear the include in schematic bill of materials flag.
     *
     * @param aIncludeInBom true to include symbol in schematic bill of material
     */
    void SetIncludeInBom( bool aIncludeInBom ) { m_includeInBom = aIncludeInBom; }
    bool GetIncludeInBom() const { return m_includeInBom; }

    /**
     * Set or clear include in board netlist flag.
     *
     * @param aIncludeOnBoard true to include symbol in the board netlist
     */
    void SetIncludeOnBoard( bool aIncludeOnBoard ) { m_includeOnBoard = aIncludeOnBoard; }
    bool GetIncludeOnBoard() const { return m_includeOnBoard; }

    /**
     * Comparison test that can be used for operators.
     *
     * @param aRhs is the right hand side symbol used for comparison.
     *
     * @return -1 if this symbol is less than \a aRhs
     *         1 if this symbol is greater than \a aRhs
     *         0 if this symbol is the same as \a aRhs
     */
    int Compare( const LIB_PART& aRhs ) const;

    bool operator==( const LIB_PART* aPart ) const { return this == aPart; }
    bool operator==( const LIB_PART& aPart ) const { return Compare( aPart ) == 0; }
    bool operator!=( const LIB_PART& aPart ) const { return Compare( aPart ) != 0; }

    const LIB_PART& operator=( const LIB_PART& aPart );

    /**
     * Return a flattened symbol inheritance to the caller.
     *
     * If the symbol does not inherit from another symbol, a copy of the symbol is returned.
     *
     * @return a flattened symbol on the heap
     */
    std::unique_ptr< LIB_PART > Flatten() const;

    /**
     * Return a list of LIB_ITEM objects separated by unit and convert number.
     *
     * @note This does not include LIB_FIELD objects since they are not associated with
     *       unit and/or convert numbers.
     */
    std::vector<struct PART_UNITS> GetUnitDrawItems();

    /**
     * Return a list of unit numbers that are unique to this symbol.
     *
     * If the symbol is inherited (alias), the unique units of the parent symbol are returned.
     * When comparing pins, the pin number is ignored.
     *
     * @return a list of unique unit numbers and their associated draw items.
     */
    std::vector<struct PART_UNITS> GetUniqueUnits();

    /**
     * Return a list of item pointers for \a aUnit and \a aConvert for this symbol.
     *
     * @note #LIB_FIELD objects are not included.
     *
     * @param aUnit is the unit number of the item, -1 includes all units.
     * @param aConvert is the alternate body styple of the item, -1 includes all body styles.
     *
     * @return a list of unit items.
     */
    std::vector<LIB_ITEM*> GetUnitItems( int aUnit, int aConvert );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    // We create a different set parent function for this class, so we hide the inherited one.
    using EDA_ITEM::SetParent;

    void deleteAllFields();

private:
    PART_SPTR           m_me;
    PART_REF            m_parent;           ///< Use for inherited symbols.
    LIB_ID              m_libId;
    timestamp_t         m_lastModDate;

    int                 m_unitCount;        ///< Number of units (parts) per package.
    bool                m_unitsLocked;      ///< True if part has multiple units and changing one
                                            ///< unit does not automatically change another unit.

    int                 m_pinNameOffset;    ///< The offset in mils to draw the pin name.  Set to
                                            ///< 0 to draw the pin name above the pin.
    bool                m_showPinNames;
    bool                m_showPinNumbers;

    bool                m_includeInBom;
    bool                m_includeOnBoard;
    LIBRENTRYOPTIONS    m_options;          ///< Special part features such as POWER or NORMAL.)

    LIB_ITEMS_CONTAINER m_drawings;

    PART_LIB*           m_library;
    wxString            m_name;
    wxString            m_description;
    wxString            m_keyWords;         ///< Search keywords
    wxArrayString       m_fpFilters;        ///< List of suitable footprint names for the
                                            ///<  part (wild card names accepted).

    static int  m_subpartIdSeparator;       ///< the separator char between
                                            ///< the subpart id and the reference like U1A
                                            ///< ( m_subpartIdSeparator = 0 ) or U1.A or U1-A
    static int  m_subpartFirstId;           ///< the ASCII char value to calculate the subpart
                                            ///< symbol id from the part number: only 'A', 'a'
                                            ///< or '1' can be used, other values have no sense.
};

#endif  //  CLASS_LIBENTRY_H
