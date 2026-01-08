/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef _SCH_REFERENCE_LIST_H_
#define _SCH_REFERENCE_LIST_H_

#include <map>

#include <lib_symbol.h>
#include <macros.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <erc/erc_settings.h>

class REFDES_TRACKER;

/** Schematic annotation scope options. */
enum ANNOTATE_SCOPE_T
{
    ANNOTATE_ALL,           ///< Annotate the full schematic
    ANNOTATE_CURRENT_SHEET, ///< Annotate the current sheet
    ANNOTATE_SELECTION      ///< Annotate the selection
};


/** Schematic annotation order options. */
enum ANNOTATE_ORDER_T
{
    SORT_BY_X_POSITION, ///< Annotate by X position from left to right.
    SORT_BY_Y_POSITION, ///< Annotate by Y position from top to bottom.
    UNSORTED,           ///< Annotate by position of symbol in the schematic sheet
                        ///< object list.
};


/** Schematic annotation type options. */
enum ANNOTATE_ALGO_T
{
    INCREMENTAL_BY_REF,  ///< Annotate incrementally using the first free reference number.
    SHEET_NUMBER_X_100,  ///< Annotate using the first free reference number starting at
                         ///< the sheet number * 100.
    SHEET_NUMBER_X_1000, ///< Annotate using the first free reference number starting at
                         ///< the sheet number * 1000.
};


/**
 * A helper to define a symbol's reference designator in a schematic.
 *
 * This helper is required in a complex hierarchy because a symbol can be used more than once
 * and its reference depends on the sheet path.  This class is used to flatten the schematic
 * hierarchy for annotation, net list generation, and bill of material generation.
 */
class SCH_REFERENCE
{
public:
    SCH_REFERENCE() :
            m_sheetPath()
    {
        m_rootSymbol      = nullptr;
        m_unit            = 0;
        m_isNew           = false;
        m_numRef          = 0;
        m_flag            = 0;
        m_sheetNum        = 0;
    }

    SCH_REFERENCE( SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aSheetPath );

    SCH_SYMBOL* GetSymbol() const           { return m_rootSymbol; }

    LIB_SYMBOL* GetLibPart() const          { return m_rootSymbol->GetLibSymbolRef().get(); }

    const SCH_SHEET_PATH& GetSheetPath() const { return m_sheetPath; }

    SCH_SHEET_PATH& GetSheetPath()             { return m_sheetPath; }

    int GetUnit() const                        { return m_unit; }
    void SetUnit( int aUnit )                  { m_unit = aUnit; }
    bool IsMultiUnit() const                   { return GetLibPart()->GetUnitCount() > 1; }

    const wxString GetValue() const            { return m_value; }
    void SetValue( const wxString& aValue )    { m_value = aValue; }

    const wxString GetFootprint() const        { return m_footprint; }
    void SetFootprint( const wxString& aFP )   { m_footprint = aFP; }

    void SetSheetNumber( int aSheetNumber )    { m_sheetNum = aSheetNumber; }

     /**
     * @return the sheet path containing the symbol item
     */
    const wxString GetPath() const
    {
        return m_sheetPath.PathAsString();
    }

    /**
     * @return the full path of the symbol item
     */
    const wxString GetFullPath() const
    {
        return m_sheetPath.PathAsString() + m_symbolUuid.AsString();
    }

    /**
     * Compare by full path to make std::set work.
     */
    bool operator<( const SCH_REFERENCE& aRef ) const { return GetFullPath() < aRef.GetFullPath(); }

    /**
     * Update the annotation of the symbol according the current object state.
     */
    void Annotate();

    /**
     * Verify the reference should always be automatically annotated.
     *
     * @return true if the symbol reference should always be automatically annotated otherwise
     *         false.
     */
    bool AlwaysAnnotate() const;

    /**
     * Attempt to split the reference designator into a name (U) and number (1).
     *
     * If the last character is '?' or not a digit, the reference is tagged as not annotated.
     * For symbols with multiple parts per package that are not already annotated, keeps the unit
     * number the same. E.g. U?A or U?B
     */
    void Split();

    /**
     * Determine if this reference needs to be split or if it likely already has been.
     *
     * @return true if this reference hasn't been split yet.
     */
    bool IsSplitNeeded();

    void SetRef( const wxString& aReference ) { m_ref = aReference; }
    wxString GetRef() const { return m_ref; }

    void SetRefStr( const std::string& aReference ) { m_ref = aReference; }
    const char* GetRefStr() const { return m_ref.c_str(); }

    /// Return reference name with unit altogether.
    wxString GetFullRef( bool aIncludeUnit = true ) const
    {
        wxString refNum = m_numRefStr;

        if( refNum.IsEmpty() )
            refNum << m_numRef;

        if( aIncludeUnit && GetSymbol()->GetUnitCount() > 1 )
            return GetRef() + refNum + GetSymbol()->SubReference( GetUnit() );
        else
            return GetRef() + refNum;
    }

    wxString GetRefNumber() const
    {
        if( m_numRef < 0 )
            return wxT( "?" );
        else
            return m_numRefStr;
    }

    int CompareValue( const SCH_REFERENCE& item ) const
    {
        return m_value.Cmp( item.m_value );
    }

    int CompareRef( const SCH_REFERENCE& item ) const
    {
        return m_ref.CmpNoCase( item.m_ref );
    }

    int CompareLibName( const SCH_REFERENCE& item ) const
    {
        return m_rootSymbol->GetLibId().GetLibItemName().compare(
            item.m_rootSymbol->GetLibId().GetLibItemName() );
    }

    /**
     * Return whether this reference refers to the same symbol instance (symbol and sheet) as
     * another.
     */
    bool IsSameInstance( const SCH_REFERENCE& other ) const
    {
        // Only compare symbol and path.
        // We may have changed the unit number or the designator but
        // can still be referencing the same instance.
        return GetSymbol() == other.GetSymbol()
               && GetSheetPath().Path() == other.GetSheetPath().Path();
    }

    bool IsUnitsLocked()
    {
        if( GetLibPart() )
            return GetLibPart()->UnitsLocked();
        else
            return true; // Assume units locked when we don't have a library
    }

    void SetRefNum( int aNum )
    {
        m_numRef = aNum;
        m_numRefStr = formatRefStr( aNum );
    }

    bool GetSymbolDNP( const wxString& aVariant = wxEmptyString ) const;
    bool GetSymbolExcludedFromBOM( const wxString& aVariant = wxEmptyString ) const;
    bool GetSymbolExcludedFromSim( const wxString& aVariant = wxEmptyString ) const;
    bool GetSymbolExcludedFromBoard() const;

    void SetSymbolDNP( bool aEnable, const wxString& aVariant = wxEmptyString );
    void SetSymbolExcludedFromBOM( bool aEnable, const wxString& aVariant = wxEmptyString );
    void SetSymbolExcludedFromSim( bool aEnable, const wxString& aVariant = wxEmptyString );
    void SetSymbolExcludedFromBoard( bool aEnable );

private:
    wxString formatRefStr( int aNumber ) const;

private:
    friend class SCH_REFERENCE_LIST;

    /// Symbol reference prefix, without number (for IC1, this is IC) )
    wxString        m_ref;               // it's private, use the accessors please
    SCH_SYMBOL*     m_rootSymbol;        ///< The symbol associated the reference object.
    VECTOR2I        m_symbolPos;         ///< The physical position of the symbol in schematic
                                         ///< used to annotate by X or Y position
    int             m_unit;              ///< The unit number for symbol with multiple parts
                                         ///< per package.
    wxString        m_value;             ///< The symbol value.
    wxString        m_footprint;         ///< The footprint assigned.
    SCH_SHEET_PATH  m_sheetPath;         ///< The sheet path for this reference.
    bool            m_isNew;             ///< True if not yet annotated.
    int             m_sheetNum;          ///< The sheet number for the reference.
    KIID            m_symbolUuid;        ///< UUID of the symbol.
    int             m_numRef;            ///< The numeric part of the reference designator.
    wxString        m_numRefStr;         ///< The numeric part in original string form (may have
                                         ///< leading zeroes).
    int             m_flag;
};


/**
 * Define a standard error handler for annotation errors.
 */
typedef std::function<void( ERCE_T aType, const wxString& aMsg, SCH_REFERENCE* aItemA,
                            SCH_REFERENCE* aItemB )> ANNOTATION_ERROR_HANDLER;


/**
 * Container to create a flattened list of symbols because in a complex hierarchy, a symbol
 * can be used more than once and its reference designator is dependent on the sheet path for
 * the same symbol.
 *
 *  This flattened list is used for netlist generation, BOM generation, and schematic annotation.
 */
class SCH_REFERENCE_LIST
{
public:
    SCH_REFERENCE_LIST()
    {}

    SCH_REFERENCE& operator[]( int aIndex )
    {
        return m_flatList[ aIndex ];
    }

    const SCH_REFERENCE& operator[]( int aIndex ) const
    {
        return m_flatList[ aIndex ];
    }

    void Clear()
    {
        m_flatList.clear();
    }

    size_t GetCount() const { return m_flatList.size(); }

    SCH_REFERENCE& GetItem( size_t aIdx ) { return m_flatList[aIdx]; }
    const SCH_REFERENCE& GetItem( size_t aIdx ) const { return m_flatList[aIdx]; }

    SCH_REFERENCE* FindItem( const SCH_REFERENCE& aItem );

    void AddItem( const SCH_REFERENCE& aItem ) { m_flatList.push_back( aItem ); }

    /**
     * Remove an item from the list of references.
     *
     * @param aIndex is the index of the item to be removed.
     */
    void RemoveItem( unsigned int aIndex );

    /**
     * Return true if aItem exists in this list.
     *
     * @param aItem Reference to check.
     * @return true if aItem exists in this list.
     */
    bool Contains( const SCH_REFERENCE& aItem ) const;

    /* Sort functions:
     * Sort functions are used to sort symbols for annotation or BOM generation.  Because
     * sorting depends on what we want to do, there are many sort functions.
     * Note:
     *    When creating BOM, symbols are fully annotated.  References are something like U3,
     *    U5 or R4, R8.  When annotating,  some or all symbols are not annotated, i.e. ref is
     *    only U or R, with no number.
     */

    /**
     * Attempt to split all reference designators into a name (U) and number (1).
     *
     * If the last character is '?' or not a digit, the reference is tagged as not annotated.
     * For symbols with multiple parts, keeps the unit number intact
     * @see SCH_REFERENCE::Split()
     */
    void SplitReferences()
    {
        for( unsigned ii = 0; ii < GetCount(); ii++ )
            m_flatList[ii].Split();
    }

    /**
     * Treat all symbols in this list as non-annotated. Does not update annotation state of the
     * symbols.
     *
     * @see SCH_REFERENCE_LIST::UpdateAnnotation()
     */
    void RemoveAnnotation()
    {
        for( unsigned ii = 0; ii < GetCount(); ii++ )
            m_flatList[ii].m_isNew = true;
    }

    /**
     * Update the symbol references for the schematic project (or the current sheet).
     *
     * @note This function does not calculate the reference numbers stored in m_numRef so it
     *       must be called after calculation of new reference numbers.
     *
     * @see SCH_REFERENCE::Annotate()
     */
    void UpdateAnnotation()
    {
        /* update the reference numbers */
        for( unsigned ii = 0; ii < GetCount(); ii++ )
            m_flatList[ii].Annotate();
    }

    /**
     * Forces reannotation of the provided references. Will also reannotate
     * associated multi-unit symbols.
     *
     * @param aSortOption Define the annotation order.  See #ANNOTATE_ORDER_T.
     * @param aAlgoOption Define the annotation style.  See #ANNOTATE_ALGO_T.
     * @param aStartNumber The start number for non-sheet-based annotation styles.
     * @param aAdditionalReferences Additional references to check for duplicates
     * @param aStartAtCurrent Use m_numRef for each reference as the start number (overrides
     *        aStartNumber)
     * @param aHierarchy Optional sheet path hierarchy for resetting the references'
     *        sheet numbers based on their sheet's place in the hierarchy. Set
     *        nullptr if not desired.
     */
    void ReannotateByOptions( ANNOTATE_ORDER_T             aSortOption,
                              ANNOTATE_ALGO_T              aAlgoOption,
                              int                          aStartNumber,
                              const SCH_REFERENCE_LIST&    aAdditionalRefs,
                              bool                         aStartAtCurrent,
                              SCH_SHEET_LIST*              aHierarchy );

    /**
     * Convenience function for the Paste Unique functionality.
     *
     * @note Do not use as a general reannotation method.
     *
     * Replaces any duplicate reference designators with the next available number after the
     * present number obeying the current annotation style.
     *
     * Multi-unit symbols are reannotated together.
     */
    void ReannotateDuplicates( const SCH_REFERENCE_LIST& aAdditionalReferences, ANNOTATE_ALGO_T aAlgoOption );

    /**
     * Annotate the references by the provided options.
     *
     * @param aSortOption Define the annotation order.  See #ANNOTATE_ORDER_T.
     * @param aAlgoOption Define the annotation style.  See #ANNOTATE_ALGO_T.
     * @param aStartNumber The start number for non-sheet-based annotation styles.
     * @param appendUndo True if the annotation operation should be added to an existing undo,
     *                   false if it should be separately undo-able.
     * @param aLockedUnitMap A SCH_MULTI_UNIT_REFERENCE_MAP of reference designator wxStrings
     *      to SCH_REFERENCE_LISTs. May be an empty map. If not empty, any multi-unit parts
     *      found in this map will be annotated as a group rather than individually.
     * @param aAdditionalReferences Additional references to check for duplicates
     * @param aStartAtCurrent Use m_numRef for each reference as the start number (overrides
     *        aStartNumber)
     */
    void AnnotateByOptions( enum ANNOTATE_ORDER_T                   aSortOption,
                            enum ANNOTATE_ALGO_T                    aAlgoOption,
                            int                                     aStartNumber,
                            const SCH_MULTI_UNIT_REFERENCE_MAP&     aLockedUnitMap,
                            const SCH_REFERENCE_LIST&               aAdditionalRefs,
                            bool                                    aStartAtCurrent );

    /**
     * Set the reference designators in the list that have not been annotated.
     *
     * If a the sheet number is 2 and \a aSheetIntervalId is 100, then the first reference
     * designator would be 201 and the last reference designator would be 299 when no overlap
     * occurs with sheet number 3.  If there are 150 items in sheet number 2, then items are
     * referenced U201 to U351, and items in sheet 3 start from U352
     *
     * @param aUseSheetNum Set to true to start annotation for each sheet at the sheet number
     *                     times \a aSheetIntervalId.  Otherwise annotate incrementally.
     * @param aSheetIntervalId The per sheet reference designator multiplier.
     * @param aStartNumber The number to start with if NOT numbering based on sheet number.
     * @param aLockedUnitMap A SCH_MULTI_UNIT_REFERENCE_MAP of reference designator wxStrings
     *      to SCH_REFERENCE_LISTs. May be an empty map. If not empty, any multi-unit parts
     *      found in this map will be annotated as a group rather than individually.
     * @param aAdditionalRefs Additional references to use for checking that there a reference
     *      designator doesn't already exist. The caller must ensure that none of the references
     *      in aAdditionalRefs exist in this list.
     * @param aStartAtCurrent Use m_numRef for each reference as the start number (overrides
            aStartNumber)
     */
    void Annotate( bool aUseSheetNum, int aSheetIntervalId, int aStartNumber,
                   const SCH_MULTI_UNIT_REFERENCE_MAP& aLockedUnitMap,
                   const SCH_REFERENCE_LIST& aAdditionalRefs,
                   bool aStartAtCurrent = false );

    /**
     * Check for annotations errors.
     *
     * The following annotation error conditions are tested:
     *  - Symbols not annotated.
     *  - Symbols having the same reference designator (duplicates).
     *  - Symbols with multiple parts per package having different reference designators.
     *  - Symbols with multiple parts per package with invalid part count.
     *
     * @param aErrorHandler A handler for errors.
     * @return The number of errors found.
     */
    int CheckAnnotation( ANNOTATION_ERROR_HANDLER aErrorHandler );

    /**
     * Sort the list of references by X position.
     *
     * Symbols are sorted as follows:
     *  - Numeric value of reference designator.
     *  - Sheet number.
     *  - X coordinate position.
     *  - Y coordinate position.
     *  - Time stamp.
     */
    void SortByXCoordinate()
    {
        sort( m_flatList.begin(), m_flatList.end(), sortByXPosition );
    }

    /**
     * Sort the list of references by Y position.
     *
     * Symbols are sorted as follows:
     *  - Numeric value of reference designator.
     *  - Sheet number.
     *  - Y coordinate position.
     *  - X coordinate position.
     *  - Time stamp.
     */
    void SortByYCoordinate()
    {
        sort( m_flatList.begin(), m_flatList.end(), sortByYPosition );
    }

    /**
     * Sort the flat list by Time Stamp (sheet path + timestamp).
     *
     * Useful to detect duplicate Time Stamps
     */
    void SortByTimeStamp()
    {
        sort( m_flatList.begin(), m_flatList.end(), sortByTimeStamp );
    }

    /**
     * Sort the list of references by value.
     *
     * Symbols are sorted in the following order:
     *  - Numeric value of reference designator.
     *  - Value of symbol.
     *  - Unit number when symbol has multiple parts.
     *  - Sheet number.
     *  - X coordinate position.
     *  - Y coordinate position.
     */
    void SortByRefAndValue()
    {
        sort( m_flatList.begin(), m_flatList.end(), sortByRefAndValue );
    }

    /**
     * Sort the list of references by reference.
     *
     * Symbols are sorted in the following order:
     *  - Numeric value of reference designator.
     *  - Unit number when symbol has multiple parts.
     */
    void SortByReferenceOnly()
    {
        sort( m_flatList.begin(), m_flatList.end(), sortByReferenceOnly );
    }

    /**
     * Sort the list by the symbol pointer.
     *
     * Because symbols are shared in complex hierarchies, this sorting can be used to coalesce symbol
     * instance changes into a single commit instead of per instances commits.
     */
    void SortBySymbolPtr()
    {
        sort( m_flatList.begin(), m_flatList.end(), sortBySymbolPtr );
    }

    /**
     * Search the list for a symbol with a given reference.
     */
    int FindRef( const wxString& aPath ) const;

    /**
     * Search the list for a symbol with the given KIID path (as string).
     *
     * @param aFullPath is the path of the symbol item to search.
     * @return an index in m_flatList if found or -1 if not found.
     */
    int FindRefByFullPath( const wxString& aFullPath ) const;

    /**
     * Add all the reference designator numbers greater than \a aMinRefId to \a aIdList
     * skipping the reference at \a aIndex.
     *
     * @param aIndex is the current symbol's index to use for reference prefix filtering.
     * @param aIdList is the buffer to fill.
     * @param aMinRefId is the minimum ID value to store. All values < aMinRefId are ignored.
     */
    void GetRefsInUse( int aIndex, std::vector<int>& aIdList, int aMinRefId ) const;

    /**
     * Return all the unit numbers for a given reference, comparing library reference, value,
     * reference number and reference prefix.
     *
     * @param aRef is the index of a symbol to use for reference prefix and number filtering.
     */
    std::vector<int> GetUnitsMatchingRef( const SCH_REFERENCE& aRef ) const;

    /**
     * Return the first unused reference number from the properties given in aRef, ensuring
     * all of the units in aRequiredUnits are also unused.
     *
     * @param aIndex The index of the reference item used for the search pattern.
     * @param aMinValue The minimum value for the current search.
     * @param aRequiredUnits List of units to ensure are free
     */
    int FindFirstUnusedReference( const SCH_REFERENCE& aRef, int aMinValue,
                                  const std::vector<int>& aRequiredUnits ) const;

    std::vector<SCH_SYMBOL_INSTANCE> GetSymbolInstances() const;

#if defined(DEBUG)
    void Show( const char* aPrefix = "" );
#endif

    /**
     * Return a shorthand string representing all the references in the list.  For instance,
     * "R1, R2, R4 - R7, U1"
     * @param spaced Add spaces between references
     */
    static wxString Shorthand( std::vector<SCH_REFERENCE> aList, const wxString& refDelimiter,
                               const wxString& refRangeDelimiter );

    std::shared_ptr<REFDES_TRACKER> GetRefDesTracker() const
    {
        return m_refDesTracker;
    }

    void SetRefDesTracker( std::shared_ptr<REFDES_TRACKER> aTracker )
    {
        m_refDesTracker = aTracker;
    }

    friend class BACK_ANNOTATION;

    typedef std::vector<SCH_REFERENCE>::iterator       iterator;
    typedef std::vector<SCH_REFERENCE>::const_iterator const_iterator;

    iterator begin() { return m_flatList.begin(); }

    iterator end() { return m_flatList.end(); }

    const_iterator begin() const { return m_flatList.begin(); }

    const_iterator end() const { return m_flatList.end(); }

    iterator erase( iterator position ) { return m_flatList.erase( position ); }

    iterator erase( iterator first, iterator last ) { return m_flatList.erase( first, last ); }

private:
    static bool sortByRefAndValue( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByXPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByYPosition( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByTimeStamp( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortByReferenceOnly( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    static bool sortBySymbolPtr( const SCH_REFERENCE& item1, const SCH_REFERENCE& item2 );

    // Used for sorting static sortByTimeStamp function
    friend class BACK_ANNOTATE;

    std::vector<SCH_REFERENCE> m_flatList;

    std::shared_ptr<REFDES_TRACKER> m_refDesTracker; ///< A list of previously used reference designators.
};

#endif    // _SCH_REFERENCE_LIST_H_
