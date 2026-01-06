/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2022 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <base_units.h>
#include <embedded_files.h>
#include <symbol.h>
#include <sch_field.h>
#include <sch_pin.h>
#include <lib_tree_item.h>
#include <vector>
#include <core/multivector.h>
#include <default_values.h>

class LINE_READER;
class OUTPUTFORMATTER;
class REPORTER;
class LEGACY_SYMBOL_LIB;
class LIB_SYMBOL;
class TEST_LIB_SYMBOL_FIXTURE;

namespace KIFONT
{
    class OUTLINE_FONT;
}


typedef MULTIVECTOR<SCH_ITEM, SCH_SHAPE_T, SCH_PIN_T> LIB_ITEMS_CONTAINER;
typedef LIB_ITEMS_CONTAINER::ITEM_PTR_VECTOR LIB_ITEMS;


/* values for member .m_options */
enum LIBRENTRYOPTIONS
{
    ENTRY_NORMAL,     // Libentry is a standard symbol (real or alias)
    ENTRY_GLOBAL_POWER,      // Libentry is a power symbol
    ENTRY_LOCAL_POWER // Libentry is a local power symbol
};


extern bool operator<( const LIB_SYMBOL& aItem1, const LIB_SYMBOL& aItem2 );


struct LIB_SYMBOL_UNIT
{
    int m_unit;                       ///< The unit number.
    int m_bodyStyle;                  ///< The alternate body style of the unit.
    std::vector<SCH_ITEM*> m_items;   ///< The items unique to this unit and alternate body style.
};


/**
 * Define a library symbol object.
 *
 * A library symbol object is typically saved and loaded in a symbol library file (.lib).
 * Library symbols are different from schematic symbols.
 */
class LIB_SYMBOL : public SYMBOL, public LIB_TREE_ITEM, public EMBEDDED_FILES
{
public:
    LIB_SYMBOL( const wxString& aName, LIB_SYMBOL* aParent = nullptr,
                LEGACY_SYMBOL_LIB* aLibrary = nullptr );

    LIB_SYMBOL( const LIB_SYMBOL& aSymbol, LEGACY_SYMBOL_LIB* aLibrary = nullptr, bool aCopyEmbeddedFiles = true );

    virtual ~LIB_SYMBOL() = default;

    /// http://www.boost.org/doc/libs/1_55_0/libs/smart_ptr/sp_techniques.html#weak_without_shared.
    std::shared_ptr<LIB_SYMBOL> SharedPtr() const { return m_me; }

    /**
     * Create a copy of a LIB_SYMBOL and assigns unique KIIDs to the copy and its children.
     */
    virtual LIB_SYMBOL* Duplicate() const
    {
        LIB_SYMBOL* dupe = new LIB_SYMBOL( *this, m_library );
        const_cast<KIID&>( dupe->m_Uuid ) = KIID();

        for( SCH_ITEM& item : dupe->m_drawings )
            const_cast<KIID&>( item.m_Uuid ) = KIID();

        return dupe;
    }

    /**
     * Returns a dummy LIB_SYMBOL, used when one is missing in the schematic
     */
    static LIB_SYMBOL* GetDummy();

    void SetParent( LIB_SYMBOL* aParent = nullptr );
    std::weak_ptr<LIB_SYMBOL>& GetParent() { return m_parent; }
    const std::weak_ptr<LIB_SYMBOL>& GetParent() const { return m_parent; }

    /**
     * Get the number of parents for this symbol.
     *
     * @return the inheritance depth for this symbol.
     */
    unsigned GetInheritanceDepth() const;

    /**
     * Get the parent symbol that does not have another parent.
     *
     * Now that derived symbols can be derived from other derived symbols, this method provides
     * way to get to the base symbol in the derivation change.
     *
     * @return the weak_ptr to the root symbol of this symbol.
     */
    std::shared_ptr<LIB_SYMBOL> GetRootSymbol() const;

    virtual wxString GetClass() const override
    {
        return wxT( "LIB_SYMBOL" );
    }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == LIB_SYMBOL_T;
    }

    virtual void SetName( const wxString& aName );
    wxString GetName() const override { return m_name; }

    LIB_ID GetLIB_ID() const override { return m_libId; }
    wxString GetDesc() override { return GetShownDescription(); }
    wxString GetFootprint() override { return GetFootprintField().GetShownText( false ); }
    int GetSubUnitCount() const override { return GetUnitCount(); }

    const LIB_ID& GetLibId() const override { return m_libId; }
    void SetLibId( const LIB_ID& aLibId ) { m_libId = aLibId; }

    LIB_ID GetSourceLibId() const { return m_sourceLibId; }
    void SetSourceLibId( const LIB_ID& aLibId ) { m_sourceLibId = aLibId; }

    wxString GetLibNickname() const override { return GetLibraryName(); }

    ///< Sets the Description field text value
    void SetDescription( const wxString& aDescription )
    {
        GetDescriptionField().SetText( aDescription );
    }

    ///< Gets the Description field text value */
    wxString GetDescription() const override
    {
        if( GetDescriptionField().GetText().IsEmpty() && IsDerived() )
        {
            if( std::shared_ptr<LIB_SYMBOL> parent = m_parent.lock() )
                return parent->GetDescription();
        }

        return GetDescriptionField().GetText();
    }

    wxString GetShownDescription( int aDepth = 0 ) const override;

    void SetKeyWords( const wxString& aKeyWords ) { m_keyWords = aKeyWords; }

    wxString GetKeyWords() const override
    {
        if( m_keyWords.IsEmpty() && IsDerived() )
        {
            if( std::shared_ptr<LIB_SYMBOL> parent = m_parent.lock() )
                return parent->GetKeyWords();
        }

        return m_keyWords;
    }

    wxString GetShownKeyWords( int aDepth = 0 ) const override;

    std::vector<SEARCH_TERM> GetSearchTerms() override;

    void GetChooserFields( std::map<wxString , wxString>& aColumnMap ) override;

    /**
     * For symbols derived from other symbols, IsRoot() indicates no derivation.
     */
    bool IsRoot() const override { return m_parent.use_count() == 0; }
    bool IsDerived() const { return !m_parent.expired() && m_parent.use_count() > 0; }

    const wxString GetLibraryName() const;

    LEGACY_SYMBOL_LIB* GetLib() const          { return m_library; }
    void SetLib( LEGACY_SYMBOL_LIB* aLibrary ) { m_library = aLibrary; }

    timestamp_t GetLastModDate() const { return m_lastModDate; }

    void SetFPFilters( const wxArrayString& aFilters ) { m_fpFilters = aFilters; }

    wxArrayString GetFPFilters() const
    {
        if( m_fpFilters.IsEmpty() && IsDerived() )
        {
            if( std::shared_ptr<LIB_SYMBOL> parent = m_parent.lock() )
                return parent->GetFPFilters();
        }

        return m_fpFilters;
    }

    /**
     * Get the bounding box for the symbol.
     *
     * @return the symbol bounding box ( in user coordinates )
     * @param aUnit = unit selection = 0, or 1..n
     * @param aBodyStyle = body style selection = 0, or 1..n
     *  If aUnit == 0, unit is not used
     *  if aBodyStyle == 0, body style is not used
     * @param aIgnoreHiddenFields default true, ignores any hidden fields
     * @param aIgnoreLabelsOnInvisiblePins default true, ignores pin number and pin name
     * of invisible pins
     **/
    const BOX2I GetUnitBoundingBox( int aUnit, int aBodyStyle, bool aIgnoreHiddenFields = true,
                                    bool aIgnoreLabelsOnInvisiblePins = true ) const;

    const BOX2I GetBoundingBox() const override
    {
        return GetUnitBoundingBox( 0, 0 );
    }

    /**
     * Get the symbol bounding box excluding fields.
     *
     * @return the symbol bounding box ( in user coordinates ) without fields
     * @param aUnit = unit selection = 0, or 1..n
     * @param aBodyStyle = body style selection = 0, or 1..n
     *  If aUnit == 0, unit is not used
     *  if aBodyStyle == 0, body style is not used
     *  Fields are not taken in account
     */
    const BOX2I GetBodyBoundingBox( int aUnit, int aBodyStyle, bool aIncludePins,
                                    bool aIncludePrivateItems ) const;

    BOX2I GetBodyBoundingBox() const override
    {
        return GetBodyBoundingBox( m_previewUnit, m_previewBodyStyle, false, false );
    }

    BOX2I GetBodyAndPinsBoundingBox() const override
    {
        return GetBodyBoundingBox( m_previewUnit, m_previewBodyStyle, true, false );
    }

    bool IsGlobalPower() const override;
    bool IsLocalPower() const override;
    bool IsPower() const override;
    bool IsNormal() const override;

    void SetGlobalPower();
    void SetLocalPower();
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
     * Overwrite all the existing fields in this symbol with fields supplied in \a aFieldsList.
     *
     * @param aFieldsList is a set of fields to import, removing all previous fields.
     */
    void SetFields( const std::vector<SCH_FIELD>& aFieldsList );

    /**
     * Populate a std::vector with SCH_FIELDs, sorted in ordinal order.
     *
     * @param aList is the vector to populate.
     */
    void GetFields( std::vector<SCH_FIELD*>& aList, bool aVisibleOnly = false ) const override;

    /**
     * Create a copy of the SCH_FIELDs, sorted in ordinal order.
     */
    void CopyFields( std::vector<SCH_FIELD>& aList );

    /**
     * Add a field.  Takes ownership of the pointer.
     */
    void AddField( SCH_FIELD* aField );

    void AddField( SCH_FIELD& aField ) { AddField( new SCH_FIELD( aField ) ); }

    /**
     * Return the next ordinal for a user field for this symbol
     */
    int GetNextFieldOrdinal() const;

    /**
     * Find a field within this symbol matching \a aFieldName; return nullptr if not found.
     */
    SCH_FIELD* GetField( const wxString& aFieldName );
    const SCH_FIELD* GetField( const wxString& aFieldName ) const;

    SCH_FIELD* FindFieldCaseInsensitive( const wxString& aFieldName );

    const SCH_FIELD* GetField( FIELD_T aFieldType ) const;
    SCH_FIELD* GetField( FIELD_T aFieldType );

    /** Return reference to the value field. */
    SCH_FIELD& GetValueField() { return *GetField( FIELD_T::VALUE ); }
    const SCH_FIELD& GetValueField() const;

    /** Return reference to the reference designator field. */
    SCH_FIELD& GetReferenceField() { return *GetField( FIELD_T::REFERENCE ); }
    const SCH_FIELD& GetReferenceField() const;

    /** Return reference to the footprint field */
    SCH_FIELD& GetFootprintField() { return *GetField( FIELD_T::FOOTPRINT ); }
    const SCH_FIELD& GetFootprintField() const;

    /** Return reference to the datasheet field. */
    SCH_FIELD& GetDatasheetField() { return *GetField( FIELD_T::DATASHEET ); }
    const SCH_FIELD& GetDatasheetField() const;

    /** Return reference to the description field. */
    SCH_FIELD& GetDescriptionField() {return *GetField( FIELD_T::DESCRIPTION ); }
    const SCH_FIELD& GetDescriptionField() const;

    wxString GetPrefix();

    const wxString GetRef( const SCH_SHEET_PATH* aSheet, bool aIncludeUnit = false ) const override
    {
        return GetReferenceField().GetText();
    }

    const wxString GetValue( bool aResolve, const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                             const wxString& aVariantName = wxEmptyString ) const override
    {
        return GetValueField().GetText();
    }

    /*
     * Field access for property manager
     */
    wxString GetRefProp() const
    {
        return GetReferenceField().GetText();
    }

    void SetRefProp( const wxString& aRef )
    {
        GetReferenceField().SetText( aRef );
    }

    wxString GetValueProp() const
    {
        return GetValueField().GetText();
    }

    void SetValueProp( const wxString& aValue )
    {
        GetValueField().SetText( aValue );
    }

    wxString GetFootprintProp() const
    {
        return GetFootprintField().GetText();
    }

    void SetFootprintProp( const wxString& aFootprint )
    {
        GetFootprintField().SetText( aFootprint );
    }

    wxString GetDatasheetProp() const
    {
        return GetDatasheetField().GetText();
    }

    void SetDatasheetProp( const wxString& aDatasheet )
    {
        GetDatasheetField().SetText( aDatasheet );
    }

    wxString GetKeywordsProp() const
    {
        return GetKeyWords();
    }

    void SetKeywordsProp( const wxString& aKeywords )
    {
        SetKeyWords( aKeywords );
    }

    bool GetPowerSymbolProp() const
    {
        return IsPower();
    }

    void SetPowerSymbolProp( bool aIsPower )
    {
        if( aIsPower )
            SetGlobalPower();
        else
            SetNormal();
    }

    bool GetLocalPowerSymbolProp() const
    {
        return IsLocalPower();
    }

    void SetLocalPowerSymbolProp( bool aIsLocalPower )
    {
        if( aIsLocalPower )
            SetLocalPower();
        else if( IsPower() )
            SetGlobalPower();
        else
            SetNormal();
    }

    bool GetPinNamesInsideProp() const
    {
        return GetPinNameOffset() != 0;
    }

    void SetPinNamesInsideProp( bool aInside )
    {
        if( aInside && GetPinNameOffset() == 0 )
            SetPinNameOffset( schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET ) );
        else if( !aInside )
            SetPinNameOffset( 0 );
    }

    int GetUnitProp() const
    {
        return GetUnitCount();
    }

    void SetUnitProp( int aUnits )
    {
        SetUnitCount( aUnits, true );
    }

    bool GetUnitsInterchangeableProp() const
    {
        return !UnitsLocked();
    }

    void SetUnitsInterchangeableProp( bool aInterchangeable )
    {
        LockUnits( !aInterchangeable );
    }

    wxString GetBodyStyleProp() const override
    {
        return GetBodyStyleDescription( 1, false );
    }

    void SetBodyStyleProp( const wxString& aBodyStyle ) override
    {
        // Body style setting is more complex for LIB_SYMBOL
        // For now, this is primarily for display purposes
    }

    bool GetExcludedFromSimProp() const
    {
        return GetExcludedFromSim();
    }

    void SetExcludedFromSimProp( bool aExclude )
    {
        SetExcludedFromSim( aExclude );
    }

    bool GetExcludedFromBOMProp() const
    {
        return GetExcludedFromBOM();
    }

    void SetExcludedFromBOMProp( bool aExclude )
    {
        SetExcludedFromBOM( aExclude );
    }

    bool GetExcludedFromBoardProp() const
    {
        return GetExcludedFromBoard();
    }

    void SetExcludedFromBoardProp( bool aExclude )
    {
        SetExcludedFromBoard( aExclude );
    }

    bool GetExcludedFromPosFilesProp() const { return GetExcludedFromPosFiles(); }
    void SetExcludedFromPosFilesProp( bool aExclude ) { SetExcludedFromPosFiles( aExclude ); }

    std::set<KIFONT::OUTLINE_FONT*> GetFonts() const override;

    EMBEDDED_FILES* GetEmbeddedFiles() override;
    const EMBEDDED_FILES* GetEmbeddedFiles() const;
    void AppendParentEmbeddedFiles( std::vector<EMBEDDED_FILES*>& aStack ) const;

    void EmbedFonts() override;

    /**
     * Automatically orient all the fields in the symbol.
     *
     * @param aScreen is the SCH_SCREEN associated with the current instance of the symbol.
     *                Required when \a aAlgo is AUTOPLACE_MANUAL; optional otherwise.
     */
    void AutoplaceFields( SCH_SCREEN* aScreen, AUTOPLACE_ALGO aAlgo ) override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction, RECURSE_MODE aMode ) override;

    /**
     * Resolve any references to system tokens supported by the symbol.
     *
     * @param aDepth a counter to limit recursion and circular references.
     */
    bool ResolveTextVar( wxString* token, int aDepth = 0 ) const;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    /**
     * Plot symbol fields.
     */
    void PlotFields( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                     int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed );

    /**
     * Add a new draw \a aItem to the draw object list and sort according to \a aSort.
     *
     * @param aItem is the new draw object to add to the symbol.
     * @param aSort is the flag to determine if the newly added item should be sorted.
     */
    void AddDrawItem( SCH_ITEM* aItem, bool aSort = true );

    /**
     * Remove draw \a aItem from list.
     *
     * @param aItem - Draw item to remove from list.
     */
    void RemoveDrawItem( SCH_ITEM* aItem );

    void RemoveField( SCH_FIELD* aField ) { RemoveDrawItem( aField ); }

    /**
     * Graphical pins: Return schematic pin objects as drawn (unexpanded), filtered by unit/body.
     *
     * Note: pin objects are owned by the symbol's draw list; do not delete them.
     *
     * @param aUnit Unit number to collect; 0 = all units
     * @param aBodyStyle Alternate body style to collect; 0 = all body styles
     */
    std::vector<SCH_PIN*> GetGraphicalPins( int aUnit = 0, int aBodyStyle = 0 ) const;

    /**
     * Logical pins: Return expanded logical pins based on stacked-pin notation.
     * Each returned item pairs a base graphical pin with a single expanded logical number.
     */
    struct LOGICAL_PIN
    {
        SCH_PIN*   pin;        ///< pointer to the base graphical pin
        wxString   number;     ///< expanded logical pin number
    };

    /**
     * Return all logical pins (expanded) filtered by unit/body.
     * For non-stacked pins, the single logical pin's number equals the base pin number.
     */
    std::vector<LOGICAL_PIN> GetLogicalPins( int aUnit, int aBodyStyle ) const;

    struct UNIT_PIN_INFO
    {
        wxString              m_unitName;
        std::vector<wxString> m_pinNumbers;
    };

    /**
     * Return pin-number lists for each unit, ordered consistently for gate swapping.
     */
    std::vector<UNIT_PIN_INFO> GetUnitPinInfo() const;

    // Deprecated: use GetGraphicalPins(). This override remains to satisfy SYMBOL's pure virtual.
    std::vector<SCH_PIN*> GetPins() const override;

    /**
     * @return a count of pins for all units / converts.
     */
    int GetPinCount() override;

    /**
     * Return pin object with the requested pin \a aNumber.
     *
     * @param aNumber - Number of the pin to find.
     * @param aUnit - Unit filter.  Set to 0 if a specific unit number is not required.
     * @param aBodyStyle - Body style filter.  Set to 0 if no specific body style is not required.
     * @return The pin object if found.  Otherwise NULL.
     */
    SCH_PIN* GetPin( const wxString& aNumber, int aUnit = 0, int aBodyStyle = 0 ) const;

    /**
     * Return true if this symbol's pins do not match another symbol's pins. This is used to
     * detect whether the project cache is out of sync with the system libs.
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
    void Move( const VECTOR2I& aOffset ) override;

    /**
     * Before V10 we didn't store the number of body styles in a symbol -- we just looked through all
     * its drawings each time we wanted to know.  This is now only used to set the count when a legacy
     * symbol is first read.  (Legacy symbols also didn't support arbitrary body styles, so the count
     * is always 1 or 2, and when 2 it is always a De Morgan pair.)
     */
    bool HasLegacyAlternateBodyStyle() const;

    /**
     * @return the highest pin number of the symbol's pins.
     * If none of the pins has pin number assigned it returns 0.
     */
    int GetMaxPinNumber() const;

    /**
     * Clears the status flag all draw objects in this symbol.
     */
    void ClearTempFlags() override;
    void ClearEditFlags() override;

    /**
     * Locate a draw object.
     *
     * @param aUnit - Unit number of draw item.
     * @param aBodyStyle - Body style of draw item.
     * @param aType - Draw object type, set to 0 to search for any type.
     * @param aPoint - Coordinate for hit testing.
     * @return The draw object if found.  Otherwise NULL.
     */
    SCH_ITEM* LocateDrawItem( int aUnit, int aBodyStyle, KICAD_T aType, const VECTOR2I& aPoint );

    /**
     * Locate a draw object (overlaid)
     *
     * @param aUnit - Unit number of draw item.
     * @param aBodyStyle - Body style of draw item.
     * @param aType - Draw object type, set to 0 to search for any type.
     * @param aPoint - Coordinate for hit testing.
     * @param aTransform = the transform matrix
     * @return The draw object if found.  Otherwise NULL.
     */
    SCH_ITEM* LocateDrawItem( int aUnit, int aBodyStyle, KICAD_T aType, const VECTOR2I& aPoint,
                              const TRANSFORM& aTransform );

    /**
     * Return a reference to the draw item list.
     *
     * @return LIB_ITEMS_CONTAINER& - Reference to the draw item object container.
     */
    LIB_ITEMS_CONTAINER& GetDrawItems() { return m_drawings; }
    const LIB_ITEMS_CONTAINER& GetDrawItems() const { return m_drawings; }

    /**
     * This function finds the filled draw items that are covering up smaller draw items
     * and replaces their body fill color with the background fill color.
    */
    void FixupDrawItems();

    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    /**
     * Set the units per symbol count.
     *
     * If the count is greater than the current count, then the all of the
     * current draw items are duplicated for each additional symbol.  If the
     * count is less than the current count, all draw objects for units
     * greater that count are removed from the symbol.
     *
     * @param aCount - Number of units per package.
     * @param aDuplicateDrawItems Create duplicate draw items of unit 1 for each additional unit.
     */
    void SetUnitCount( int aCount, bool aDuplicateDrawItems );
    int GetUnitCount() const override;

    wxString GetUnitName( int aUnit ) const override
    {
        return GetUnitDisplayName( aUnit, true );
    }

    /**
     * Return the user-defined display name for \a aUnit for symbols with units.
     */
    wxString GetUnitDisplayName( int aUnit, bool aLabel ) const override;

    wxString GetBodyStyleDescription( int aBodyStyle, bool aLabel ) const override;

    std::map<int, wxString>& GetUnitDisplayNames() { return m_unitDisplayNames; }
    const std::map<int, wxString>& GetUnitDisplayNames() const { return m_unitDisplayNames; }

    bool GetDuplicatePinNumbersAreJumpers() const { return m_duplicatePinNumbersAreJumpers; }
    void SetDuplicatePinNumbersAreJumpers( bool aEnabled ) { m_duplicatePinNumbersAreJumpers = aEnabled; }

    /**
     * Each jumper pin group is a set of pin numbers that should be treated as internally connected.
     * @return The list of jumper pin groups in this symbols
     */
    std::vector<std::set<wxString>>& JumperPinGroups() { return m_jumperPinGroups; }
    const std::vector<std::set<wxString>>& JumperPinGroups() const { return m_jumperPinGroups; }

    /// Retrieves the jumper group containing the specified pin number, if one exists
    std::optional<const std::set<wxString>> GetJumperPinGroup( const wxString& aPinNumber ) const;

    /**
     * @return true if the symbol has multiple units per symbol.
     * When true, the reference has a sub reference to identify symbol.
     */
    bool IsMultiUnit() const override { return m_unitCount > 1; }

    static wxString LetterSubReference( int aUnit, wxChar aInitialLetter );

    bool IsMultiBodyStyle() const override { return GetBodyStyleCount() > 1; }

    int GetBodyStyleCount() const override
    {
        if( m_demorgan )
            return 2;
        else
            return std::max( 1, (int) m_bodyStyleNames.size() );
    }

    bool HasDeMorganBodyStyles() const override { return m_demorgan; }
    void SetHasDeMorganBodyStyles( bool aFlag ) { m_demorgan = aFlag; }

    const std::vector<wxString>& GetBodyStyleNames() const { return m_bodyStyleNames; }
    void SetBodyStyleNames( const std::vector<wxString>& aBodyStyleNames ) { m_bodyStyleNames = aBodyStyleNames; }

    /**
     * Set or clear the alternate body style (DeMorgan) for the symbol.
     *
     * If the symbol already has an alternate body style set and aHasAlternate is false, all
     * of the existing draw items for the alternate body style are remove.  If the alternate
     * body style is not set and aHasAlternate is true, than the base draw items are duplicated
     * and added to the symbol.
     *
     * @param aHasAlternate - Set or clear the symbol alternate body style.
     * @param aDuplicatePins - Duplicate all pins from original body style if true.
     */
    void SetBodyStyleCount( int aCount, bool aDuplicateDrawItems, bool aDuplicatePins );

    /**
     * Comparison test that can be used for operators.
     *
     * @param aRhs is the right hand side symbol used for comparison.
     *
     * @return -1 if this symbol is less than \a aRhs
     *         1 if this symbol is greater than \a aRhs
     *         0 if this symbol is the same as \a aRhs
     */
    int Compare( const LIB_SYMBOL& aRhs, int aCompareFlags = 0, REPORTER* aReporter = nullptr ) const;

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
     * Return a list of SCH_ITEM objects separated by unit and convert number.
     *
     * @note This does not include SCH_FIELD objects since they are not associated with
     *       unit and/or convert numbers.
     */
    std::vector<struct LIB_SYMBOL_UNIT> GetUnitDrawItems();

    /**
     * Return a list of item pointers for \a aUnit and \a aBodyStyle for this symbol.
     *
     * @note #SCH_FIELD objects are not included.
     *
     * @param aUnit is the unit number of the item, -1 includes all units.
     * @param aBodyStyle is the alternate body style of the item, -1 includes all body styles.
     *
     * @return a list of unit items.
     */
    std::vector<SCH_ITEM*> GetUnitDrawItems( int aUnit, int aBodyStyle );

    /**
     * Return a measure of similarity between this symbol and \a aSymbol.
     * @param aSymbol is the symbol to compare to.
     *
     * @return a measure of similarity from 1.0 (identical) to 0.0 (no similarity).
    */
    double Similarity( const SCH_ITEM& aSymbol ) const override;

    void SetParentName( const wxString& aParentName ) { m_parentName = aParentName; }
    const wxString& GetParentName() const { return m_parentName; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif

private:
    // We create a different set parent function for this class, so we hide the inherited one.
    using EDA_ITEM::SetParent;

    /**
     * The library symbol specific sort order is as follows:
     *
     *   - The result of #SCH_ITEM::compare()
     */
    int compare( const SCH_ITEM& aOther, int aCompareFlags = SCH_ITEM::COMPARE_FLAGS::EQUALITY ) const override;

    void deleteAllFields();

private:
    std::shared_ptr<LIB_SYMBOL> m_me;
    std::weak_ptr<LIB_SYMBOL>   m_parent;   ///< Use for inherited symbols.

    wxString            m_parentName;       ///< The name of the parent symbol or empty if root symbol.

    LIB_ID              m_libId;
    LIB_ID              m_sourceLibId;      ///< For database library symbols; the original symbol
    timestamp_t         m_lastModDate;

    int                 m_unitCount;        ///< Number of units (parts) per package.
    bool                m_unitsLocked;      ///< True if symbol has multiple units and changing one unit
                                            ///< does not automatically change another unit.

    bool                m_demorgan;         ///< True if there are two body styles: normal and De Morgan
                                            ///< If false, the body style count is taken from m_bodyStyleNames
                                            ///< size

    LIBRENTRYOPTIONS    m_options;          ///< Special symbol features such as POWER or NORMAL.

    LIB_ITEMS_CONTAINER m_drawings;

    LEGACY_SYMBOL_LIB*  m_library;
    wxString            m_name;
    wxString            m_keyWords;         ///< Search keywords
    wxArrayString       m_fpFilters;        ///< List of suitable footprint names for the symbol (wild card
                                            ///< names accepted).

    /// A list of jumper pin groups, each of which is a set of pin numbers that should be jumpered
    /// together (treated as internally connected for the purposes of connectivity)
    std::vector<std::set<wxString> > m_jumperPinGroups;

    /// Flag that this symbol should automatically treat sets of two or more pins with the same
    /// number as jumpered pin groups
    bool m_duplicatePinNumbersAreJumpers;

    std::map<int, wxString> m_unitDisplayNames;
    std::vector<wxString>   m_bodyStyleNames;
};
