/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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
class SYMBOL_LIB;
class LIB_SYMBOL;
class LIB_FIELD;
class TEST_LIB_SYMBOL_FIXTURE;


typedef std::shared_ptr<LIB_SYMBOL>       LIB_SYMBOL_SPTR;      ///< shared pointer to LIB_SYMBOL
typedef std::weak_ptr<LIB_SYMBOL>         LIB_SYMBOL_REF;       ///< weak pointer to LIB_SYMBOL
typedef MULTIVECTOR<LIB_ITEM, LIB_SHAPE_T, LIB_FIELD_T> LIB_ITEMS_CONTAINER;
typedef LIB_ITEMS_CONTAINER::ITEM_PTR_VECTOR LIB_ITEMS;


/* values for member .m_options */
enum LIBRENTRYOPTIONS
{
    ENTRY_NORMAL,   // Libentry is a standard symbol (real or alias)
    ENTRY_POWER     // Libentry is a power symbol
};


extern bool operator<( const LIB_SYMBOL& aItem1, const LIB_SYMBOL& aItem2 );


struct LIB_SYMBOL_OPTIONS
{
    TRANSFORM transform;            // Coordinate adjustment settings
    bool draw_visible_fields;       // Whether to draw "visible" fields
    bool draw_hidden_fields;        // Whether to draw "hidden" fields
    bool show_elec_type;            // Whether to show the pin electrical type
    bool show_connect_point;        // Whether to show the pin connect point marker (small circle)
                                    // useful in dialog pin properties

    LIB_SYMBOL_OPTIONS()
    {
        transform = DefaultTransform;
        draw_visible_fields = true;
        draw_hidden_fields = true;
        show_elec_type = false;
        show_connect_point = false;
    }
};


struct LIB_SYMBOL_UNITS
{
    int m_unit;                       ///< The unit number.
    int m_convert;                    ///< The alternate body style of the unit.
    std::vector<LIB_ITEM*> m_items;   ///< The items unique to this unit and alternate body style.
};


/**
 * Define a library symbol object.
 *
 * A library symbol object is typically saved and loaded in a symbol library file (.lib).
 * Library symbols are different from schematic symbols.
 */
class LIB_SYMBOL : public EDA_ITEM, public LIB_TREE_ITEM
{
public:
    LIB_SYMBOL( const wxString& aName, LIB_SYMBOL* aParent = nullptr,
                SYMBOL_LIB* aLibrary = nullptr );

    LIB_SYMBOL( const LIB_SYMBOL& aSymbol, SYMBOL_LIB* aLibrary = nullptr );

    virtual ~LIB_SYMBOL();

    ///< http://www.boost.org/doc/libs/1_55_0/libs/smart_ptr/sp_techniques.html#weak_without_shared
    LIB_SYMBOL_SPTR SharedPtr() const { return m_me; }

    /**
     * Create a copy of a LIB_SYMBOL and assigns unique KIIDs to the copy and its children.
     */
    virtual LIB_SYMBOL* Duplicate() const
    {
        LIB_SYMBOL* dupe = new LIB_SYMBOL( *this, m_library );
        const_cast<KIID&>( dupe->m_Uuid ) = KIID();

        for( LIB_ITEM& item : dupe->m_drawings )
            const_cast<KIID&>( item.m_Uuid ) = KIID();

        return dupe;
    }

    void SetParent( LIB_SYMBOL* aParent = nullptr );
    LIB_SYMBOL_REF& GetParent() { return m_parent; }
    const LIB_SYMBOL_REF& GetParent() const { return m_parent; }

    virtual wxString GetClass() const override
    {
        return wxT( "LIB_SYMBOL" );
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
            if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
                return parent->GetDescription();
        }

        return m_description;
    }

    void SetKeyWords( const wxString& aKeyWords ) { m_keyWords = aKeyWords; }

    wxString GetKeyWords() const
    {
        if( m_keyWords.IsEmpty() && IsAlias() )
        {
            if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
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

    SYMBOL_LIB* GetLib() const          { return m_library; }
    void SetLib( SYMBOL_LIB* aLibrary ) { m_library = aLibrary; }

    timestamp_t GetLastModDate() const { return m_lastModDate; }

    void SetFPFilters( const wxArrayString& aFilters ) { m_fpFilters = aFilters; }

    wxArrayString GetFPFilters() const
    {
        if( m_fpFilters.IsEmpty() && IsAlias() )
        {
            if( LIB_SYMBOL_SPTR parent = m_parent.lock() )
                return parent->GetFPFilters();
        }

        return m_fpFilters;
    }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /**
     * Get the bounding box for the symbol.
     *
     * @return the symbol bounding box ( in user coordinates )
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
     * @return the symbol bounding box ( in user coordinates ) without fields
     * @param aUnit = unit selection = 0, or 1..n
     * @param aConvert = 0, 1 or 2
     *  If aUnit == 0, unit is not used
     *  if aConvert == 0 Convert is non used
     *  Fields are not taken in account
     **/
    const EDA_RECT GetBodyBoundingBox( int aUnit, int aConvert, bool aIncludePins ) const;

    const EDA_RECT GetBoundingBox() const override
    {
        return GetUnitBoundingBox( 0, 0 );
    }

    bool IsPower() const;
    bool IsNormal() const;

    void SetPower();
    void SetNormal();

    /**
     * Set interchangeable the property for symbol units.
     * @param aLockUnits when true then units are set as not interchangeable.
     */
    void LockUnits( bool aLockUnits ) { m_unitsLocked = aLockUnits; }

    /**
     * Check whether symbol units are interchangeable.
     * @return False when interchangeable, true otherwise.
     */
    bool UnitsLocked() const { return m_unitsLocked; }

    /**
     * Overwrite all the existing fields in this symbol with fields supplied
     * in \a aFieldsList.
     *
     * The only known caller of this function is the library symbol field editor, and it
     * establishes needed behavior.
     *
     * @param aFieldsList is a set of fields to import, removing all previous fields.
     */
    void SetFields( const std::vector <LIB_FIELD>& aFieldsList );

    /**
     * Return a list of fields within this symbol.
     *
     * @param aList - List to add fields to
     */
    void GetFields( std::vector<LIB_FIELD*>& aList );
    void GetFields( std::vector<LIB_FIELD>& aList );

    /**
     * Add a field.  Takes ownership of the pointer.
     */
    void AddField( LIB_FIELD* aField );

    /**
     * Find a field within this symbol matching \a aFieldName and returns it
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
    LIB_FIELD* GetFieldById( int aId ) const;

    /** Return reference to the value field. */
    LIB_FIELD& GetValueField();

    /** Return reference to the reference designator field. */
    LIB_FIELD& GetReferenceField();

    /** Return reference to the footprint field */
    LIB_FIELD& GetFootprintField();

    /** Return reference to the datasheet field. */
    LIB_FIELD& GetDatasheetField();

    /**
     * Print symbol.
     *
     * @param aOffset - Position of symbol.
     * @param aMulti - unit if multiple units per symbol.
     * @param aConvert - Symbol conversion (DeMorgan) if available.
     * @param aOpts - Drawing options
     */
    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                int aMulti, int aConvert, const LIB_SYMBOL_OPTIONS& aOpts );

    /**
     * Plot lib symbol to plotter.
     * Lib Fields not are plotted here, because this plot function
     * is used to plot schematic items, which have they own fields
     *
     * @param aPlotter - Plotter object to plot to.
     * @param aUnit - Symbol symbol to plot.
     * @param aConvert - Symbol alternate body style to plot.
     * @param aOffset - Distance to shift the plot coordinates.
     * @param aTransform - Symbol plot transform matrix.
     */
    void Plot( PLOTTER* aPlotter, int aUnit, int aConvert, const wxPoint& aOffset,
               const TRANSFORM& aTransform ) const;

    /**
     * Plot Lib Fields only of the symbol to plotter.
     * is used to plot the full lib symbol, outside the schematic
     *
     * @param aPlotter - Plotter object to plot to.
     * @param aUnit - Symbol to plot.
     * @param aConvert - Symbol alternate body style to plot.
     * @param aOffset - Distance to shift the plot coordinates.
     * @param aTransform - Symbol plot transform matrix.
     */
    void PlotLibFields( PLOTTER* aPlotter, int aUnit, int aConvert, const wxPoint& aOffset,
                        const TRANSFORM& aTransform );

    /**
     * Add a new draw \a aItem to the draw object list and sort according to \a aSort.
     *
     * @param aItem is the new draw object to add to the symbol.
     * @param aSort is the flag to determine if the newly added item should be sorted.
     */
    void AddDrawItem( LIB_ITEM* aItem, bool aSort = true );

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
    LIB_ITEM* GetNextDrawItem( const LIB_ITEM* aItem = nullptr, KICAD_T aType = TYPE_NOT_INIT );

    size_t GetPinCount() const { return m_drawings.size( LIB_PIN_T ); }

    size_t GetFieldCount() const { return m_drawings.size( LIB_FIELD_T ); }

    /**
     * Return the next pin object from the draw list.
     *
     * This is just a pin object specific version of GetNextDrawItem().
     *
     * @param aItem - Pointer to the previous pin item, or NULL to get the
     *                first pin in the draw object list.
     * @return - The next pin object in the list if found, otherwise NULL.
     */
    LIB_PIN* GetNextPin( LIB_PIN* aItem = nullptr )
    {
        return (LIB_PIN*) GetNextDrawItem( (LIB_ITEM*) aItem, LIB_PIN_T );
    }

    /**
     * Return a list of pin object pointers from the draw item list.
     *
     * Note pin objects are owned by the draw list of the symbol.
     * Deleting any of the objects will leave list in a unstable state
     * and will likely segfault when the list is destroyed.
     *
     * @param aList - Pin list to place pin object pointers into.
     * @param aUnit - Unit number of pin to add to list.  Set to 0 to
     *                get pins from any symbol unit.
     * @param aConvert - Convert number of pin to add to list.  Set to 0 to
     *                   get pins from any convert of symbol.
     */
    void GetPins( LIB_PINS& aList, int aUnit = 0, int aConvert = 0 ) const;

    /**
     * Return pin object with the requested pin \a aNumber.
     *
     * @param aNumber - Number of the pin to find.
     * @param aUnit - Unit of the symbol to find.  Set to 0 if a specific
     *                unit number is not required.
     * @param aConvert - Alternate body style filter (DeMorgan).  Set to 0 if
     *                   no alternate body style is required.
     * @return The pin object if found.  Otherwise NULL.
     */
    LIB_PIN* GetPin( const wxString& aNumber, int aUnit = 0, int aConvert = 0 ) const;

    /**
     * Return true if this symbol's pins do not match another symbol's pins. This
     * is used to detect whether the project cache is out of sync with the
     * system libs.
     *
     * @param aOtherSymbol - The other library symbol to test
     * @param aTestNums - Whether two pins at the same point must have the same number.
     * @param aTestNames - Whether two pins at the same point must have the same name.
     * @param aTestType - Whether two pins at the same point must have the same electrical type.
     * @param aTestOrientation - Whether two pins at the same point must have the same orientation.
     * @param aTestLength - Whether two pins at the same point must have the same length.
     */
    bool PinsConflictWith( const LIB_SYMBOL& aOtherSymbol, bool aTestNums, bool aTestNames,
                           bool aTestType, bool aTestOrientation, bool aTestLength ) const;

    /**
     * Move the symbol \a aOffset.
     *
     * @param aOffset - Offset displacement.
     */
    void SetOffset( const wxPoint& aOffset );

    /**
     * Remove duplicate draw items from list.
     */
    void RemoveDuplicateDrawItems();

    /**
     * Test if symbol has more than one body conversion type (DeMorgan).
     *
     * @return True if symbol has more than one conversion.
     */
    bool HasConversion() const;

    /**
     * Clears the status flag all draw objects in this symbol.
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
    const LIB_ITEMS_CONTAINER& GetDrawItems() const { return m_drawings; }

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    /**
     * Set the units per symbol count.
     *
     * If the count is greater than the current count, then the all of the
     * current draw items are duplicated for each additional symbol.  If the
     * count is less than the current count, all draw objects for units
     * greater that count are removed from the symbol.
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
     * @return true if the symbol has multiple units per symbol.
     * When true, the reference has a sub reference to identify symbol.
     */
    bool IsMulti() const { return m_unitCount > 1; }

    /**
     * @return the sub reference for symbol having multiple units per symbol.
     * The sub reference identify the symbol (or unit)
     * @param aUnit = the symbol identifier ( 1 to max count)
     * @param aAddSeparator = true (default) to prepend the sub ref
     *    by the separator symbol (if any)
     * Note: this is a static function.
     */
    static wxString SubReference( int aUnit, bool aAddSeparator = true );

    // Accessors to sub ref parameters
    static int GetSubpartIdSeparator() { return m_subpartIdSeparator; }

    /**
     * Return a reference to m_subpartIdSeparator, only for read/save setting functions.
     */
    static int* SubpartIdSeparatorPtr() { return &m_subpartIdSeparator; }
    static int GetSubpartFirstId() { return m_subpartFirstId; }

    /**
     * Return a reference to m_subpartFirstId, only for read/save setting functions.
     */
    static int* SubpartFirstIdPtr() { return &m_subpartFirstId; }

    /**
     * Set the separator char between the subpart id and the reference
     * 0 (no separator) or '.' , '-' and '_'
     * and the ascii char value to calculate the subpart symbol id from the symbol number:
     * 'A' or '1' only are allowed. (to print U1.A or U1.1)
     * if this is a digit, a number is used as id symbol
     * Note also if the subpart symbol is a digit, the separator cannot be null.
     * @param aSep = the separator symbol (0 (no separator) or '.' , '-' and '_')
     * @param aFirstId = the Id of the first symbol ('A' or '1')
     */
    static void SetSubpartIdNotation( int aSep, int aFirstId );

    /**
     * Set or clear the alternate body style (DeMorgan) for the symbol.
     *
     * If the symbol already has an alternate body style set and a
     * asConvert if false, all of the existing draw items for the alternate
     * body style are remove.  If the alternate body style is not set and
     * asConvert is true, than the base draw items are duplicated and
     * added to the symbol.
     *
     * @param aSetConvert - Set or clear the symbol alternate body style.
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
    int GetPinNameOffset() const { return m_pinNameOffset; }

    /**
     * Set or clear the pin name visibility flag.
     *
     * @param aShow - True to make the symbol pin names visible.
     */
    void SetShowPinNames( bool aShow ) { m_showPinNames = aShow; }
    bool ShowPinNames() const { return m_showPinNames; }

    /**
     * Set or clear the pin number visibility flag.
     *
     * @param aShow - True to make the symbol pin numbers visible.
     */
    void SetShowPinNumbers( bool aShow ) { m_showPinNumbers = aShow; }
    bool ShowPinNumbers() const { return m_showPinNumbers; }

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
    int Compare( const LIB_SYMBOL& aRhs ) const;

    bool operator==( const LIB_SYMBOL* aSymbol ) const { return this == aSymbol; }
    bool operator==( const LIB_SYMBOL& aSymbol ) const { return Compare( aSymbol ) == 0; }
    bool operator!=( const LIB_SYMBOL& aSymbol ) const { return Compare( aSymbol ) != 0; }

    const LIB_SYMBOL& operator=( const LIB_SYMBOL& aSymbol );

    /**
     * Return a flattened symbol inheritance to the caller.
     *
     * If the symbol does not inherit from another symbol, a copy of the symbol is returned.
     *
     * @return a flattened symbol on the heap
     */
    std::unique_ptr< LIB_SYMBOL > Flatten() const;

    /**
     * Return a list of LIB_ITEM objects separated by unit and convert number.
     *
     * @note This does not include LIB_FIELD objects since they are not associated with
     *       unit and/or convert numbers.
     */
    std::vector<struct LIB_SYMBOL_UNITS> GetUnitDrawItems();

    /**
     * Return a list of unit numbers that are unique to this symbol.
     *
     * If the symbol is inherited (alias), the unique units of the parent symbol are returned.
     * When comparing pins, the pin number is ignored.
     *
     * @return a list of unique unit numbers and their associated draw items.
     */
    std::vector<struct LIB_SYMBOL_UNITS> GetUniqueUnits();

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
    LIB_SYMBOL_SPTR     m_me;
    LIB_SYMBOL_REF      m_parent;           ///< Use for inherited symbols.
    LIB_ID              m_libId;
    timestamp_t         m_lastModDate;

    int                 m_unitCount;        ///< Number of units (parts) per package.
    bool                m_unitsLocked;      ///< True if symbol has multiple units and changing one
                                            ///< unit does not automatically change another unit.

    int                 m_pinNameOffset;    ///< The offset in mils to draw the pin name.  Set to
                                            ///< 0 to draw the pin name above the pin.
    bool                m_showPinNames;
    bool                m_showPinNumbers;

    bool                m_includeInBom;
    bool                m_includeOnBoard;
    LIBRENTRYOPTIONS    m_options;          ///< Special symbol features such as POWER or NORMAL.)

    LIB_ITEMS_CONTAINER m_drawings;

    SYMBOL_LIB*         m_library;
    wxString            m_name;
    wxString            m_description;
    wxString            m_keyWords;         ///< Search keywords
    wxArrayString       m_fpFilters;        ///< List of suitable footprint names for the
                                            ///<  symbol (wild card names accepted).

    static int  m_subpartIdSeparator;       ///< the separator char between
                                            ///< the subpart id and the reference like U1A
                                            ///< ( m_subpartIdSeparator = 0 ) or U1.A or U1-A
    static int  m_subpartFirstId;           ///< the ASCII char value to calculate the subpart
                                            ///< symbol id from the symbol number: only 'A', 'a'
                                            ///< or '1' can be used, other values have no sense.
};

#endif  //  CLASS_LIBENTRY_H
