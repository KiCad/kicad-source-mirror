/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file sch_sheet_path.h
 * Definition of the SCH_SHEET_PATH and SCH_SHEET_LIST classes for Eeschema.
 */

#ifndef CLASS_DRAWSHEET_PATH_H
#define CLASS_DRAWSHEET_PATH_H

#include <map>
#include <memory>
#include <optional>

#include <kiid.h>
#include <wx/string.h>

class SCH_SYMBOL;
class SCH_SHEET;

/**
 * Object to store and handle common variant information.
 */
class VARIANT
{
public:
    VARIANT( const wxString& aName = wxEmptyString ) :
        m_Name( aName ),
        m_ExcludedFromSim( false ),
        m_ExcludedFromBOM( false ),
        m_ExcludedFromBoard( false ),
        m_ExcludedFromPosFiles( false ),
        m_DNP( false )
    {
    }

    virtual ~VARIANT() = default;

    wxString                     m_Name;
    wxString                     m_Description;
    bool                         m_ExcludedFromSim;
    bool                         m_ExcludedFromBOM;
    bool                         m_ExcludedFromBoard;
    bool                         m_ExcludedFromPosFiles;
    bool                         m_DNP;
    std::map<wxString, wxString> m_Fields;
};


/**
 * Variant information for a schematic symbol.
 *
 * Schematic symbol variants are a set of field and/or properties differentials against the default symbol
 * values.  Each symbol instance may contain 0 or more variants.
 *
 * @note The #REFERENCE field is immutable across variants. Changing it would effectively be a new board.
 */
class SCH_SYMBOL_VARIANT : public VARIANT
{
public:
    SCH_SYMBOL_VARIANT( const wxString& aName = wxEmptyString ) :
        VARIANT( aName )
    {}

    void InitializeAttributes( const SCH_SYMBOL& aSymbol );

    virtual ~SCH_SYMBOL_VARIANT() = default;
};


/**
 * A simple container for schematic symbol instance information.
 */
struct SCH_SYMBOL_INSTANCE
{
    KIID_PATH m_Path;

    // Things that can be annotated:
    wxString  m_Reference;
    int       m_Unit = 1;

    // Do not use.  This is left over from the dubious decision to instantiate symbol value
    // and footprint fields.  This is now handle by variants.
    wxString  m_Value;
    wxString  m_Footprint;

    // The project name associated with this instance.
    wxString  m_ProjectName;

    bool m_DNP = false;
    bool m_ExcludedFromBOM = false;
    bool m_ExcludedFromSim = false;
    bool m_ExcludedFromBoard = false;
    bool m_ExcludedFromPosFiles = false;

    /// A list of symbol variants.
    std::map<wxString, SCH_SYMBOL_VARIANT> m_Variants;
};


/**
 * Variant information for a schematic sheet.
 *
 * Schematic sheet variants are a set of field and/or properties differentials against the default sheet
 * values.  Each sheet instance may contain 0 or more variants.
 *
 * @note The exceptions to this are the #SHEET_NAME and #SHEET_FILENAME fields and the #SCH_SHEET::m_excludeFromBoard
 *       property.  Changing any of these would effectively be a new board.  They are immutable and will always be
 *       the sheet default value.
 */
class SCH_SHEET_VARIANT : public VARIANT
{
public:
    SCH_SHEET_VARIANT( const wxString& aName = wxEmptyString ) :
        VARIANT( aName )
    {}

    virtual ~SCH_SHEET_VARIANT() = default;

    void InitializeAttributes( const SCH_SHEET& aSheet );
};


/**
 * A simple container for sheet instance information.
 */
struct SCH_SHEET_INSTANCE
{
    KIID_PATH m_Path;

    wxString  m_PageNumber;

    // The project name associated with this instance.
    wxString  m_ProjectName;

    bool m_DNP = false;
    bool m_ExcludedFromBOM = false;
    bool m_ExcludedFromSim = false;
    bool m_ExcludedFromBoard = false;
    bool m_ExcludedFromPosFiles = false;

    /// A list of sheet variants.
    std::map<wxString, SCH_SHEET_VARIANT> m_Variants;
};


/**
 * @defgroup hierarchical_schematics Hierarchical Schematics
 *
 * KiCad supports nesting schematics hierarchically to simplify the creation of complex
 * schematics designs.  A hierarchical schematic uses hierarchical sheets (#SCH_SHEET objects)
 * to reference a given schematic file (#SCH_SCREEN objects).  Each #SCH_SHEET corresponds to
 * a schematic file handled by a #SCH_SCREEN object.  A #SCH_SCREEN object contains schematic
 * drawings and has a filename to read/write its data.
 *
 * In simple hierarchies one #SCH_SHEET object is linked to one #SCH_SCREEN object.
 *
 * In complex hierarchies the a #SCH_SCREEN object shared by more than one #SCH_SHEET object.
 * Therefore all sub-sheets can also be shared. So the same #SCH_SCREEN must handle different
 * symbol references and unit selections depending on which sheet is currently selected, and
 * how a given subsheet is selected. #SCH_SHEET objects share the same #SCH_SCREEN object if
 * they have the same schematic file.
 *
 * In KiCad each #SCH_SYMBOL and #SCH_SHEET receives a UUID when created.  These UUIDs are
 * chained together to form #SCH_SHEET_PATH objects that allow access of instance data in the
 * hierarchy.  The sheet paths have the form /ROOT_SHEET_UUID/SHEET_UUID/SUB_SHEET_UUID/...
 *
 * For a given #SCH_SCREEN #SCH_SHEET_PATH objects must:
 *   1) Handle all #SCH_SYMBOL references and unit instance data.
 *   2) Handle all #SCH_SHEET page number instance data.
 *   2) Update the currently displayed sheet #SCH_SYMBOL references and #SCH_SHEET page numbers.
 *
 * The class #SCH_SHEET_PATH handles paths used to access a sheet.  The class #SCH_SHEET_LIST
 * allows one to handle the full (or partial) list of sheets and their paths in a complex
 * hierarchy.  The class #SCH_SCREENS allows one to handle a list of #SCH_SCREEN objects. It is
 * useful to clear or save data, but is not suitable to handle the full complex hierarchy
 * possibilities (usable in flat and simple hierarchies).
 */


class EDA_ITEM;
class SCH_SHEET;
class SCH_SCREEN;
class SCH_MARKER;
class SCH_ITEM;
class SCH_SYMBOL;
class SCH_REFERENCE_LIST;


/**
 * Container to map reference designators for multi-unit parts.
 */
typedef std::map<wxString, SCH_REFERENCE_LIST> SCH_MULTI_UNIT_REFERENCE_MAP;

/**
 * Handle access to a stack of flattened #SCH_SHEET objects by way of a path for
 * creating a flattened schematic hierarchy.
 *
 * The #SCH_SHEET objects are stored in a list from first (usually the root sheet) to a
 * given sheet in last position.  The _last_ sheet is usually the sheet we want to select
 * or reach (which is what the function Last() returns).   Others sheets constitute the
 * "path" from the first to the last sheet.
 */
class SCH_SHEET_PATH
{
public:
    SCH_SHEET_PATH();

    SCH_SHEET_PATH( const SCH_SHEET_PATH& aOther );

    SCH_SHEET_PATH& operator=( const SCH_SHEET_PATH& aOther );

    // Move assignment operator
    SCH_SHEET_PATH& operator=( SCH_SHEET_PATH&& aOther );

    SCH_SHEET_PATH operator+( const SCH_SHEET_PATH& aOther );

    ~SCH_SHEET_PATH() = default;

    /// Forwarded method from std::vector
    SCH_SHEET* at( size_t aIndex ) const { return m_sheets.at( aIndex ); }

    /// Forwarded method from std::vector
    void clear()
    {
        m_sheets.clear();
        Rehash();
    }

    /// Forwarded method from std::vector
    bool empty() const { return m_sheets.empty(); }

    /// Forwarded method from std::vector
    void pop_back()
    {
        m_sheets.pop_back();
        Rehash();
    }

    /// Forwarded method from std::vector
    void push_back( SCH_SHEET* aSheet )
    {
        m_sheets.push_back( aSheet );
        Rehash();
    }

    /// Forwarded method from std::vector
    size_t size() const { return m_sheets.size(); }

    std::vector<SCH_SHEET*>::iterator erase( std::vector<SCH_SHEET*>::const_iterator aPosition )
    {
        return m_sheets.erase( aPosition );
    }

    void Rehash();

    size_t GetCurrentHash() const { return m_current_hash; }

    /**
     * Set the sheet instance virtual page number.
     *
     * Virtual page numbers are incremental integers set automatically when the sheet path
     * hierarchy is created (@see #SCH_SHEET_LIST::BuildSheetList).  The virtual page
     * numbering is ordered by the X and Y position of the sheet in a schematic which
     * mimics the page numbering code prior to the addition of actual user definable page
     * numbers.  Virtual page numbers should only be use when annotating schematics.
     */
    void SetVirtualPageNumber( int aPageNumber ) { m_virtualPageNumber = aPageNumber; }

    int GetVirtualPageNumber() const { return m_virtualPageNumber; }

    /**
     * Set the sheet instance user definable page number.
     *
     * @note User definable page numbers can be any string devoid of white space characters.
     */
    void SetPageNumber( const wxString& aPageNumber );

    wxString GetPageNumber() const;

    int GetPageNumberAsInt() const;

    const SCH_SHEET* GetSheet( unsigned aIndex ) const
    {
        SCH_SHEET* retv = nullptr;

        if( aIndex < size() )
            retv = at( aIndex );

        return retv;
    }

    /**
     * Compare if this is the same sheet path as \a aSheetPathToTest.
     *
     * @param aSheetPathToTest is the sheet path to compare.
     * @return 1 if this sheet path has more sheets than aSheetPathToTest,
     *   -1 if this sheet path has fewer sheets than aSheetPathToTest,
     *   or 0 if same
     */
    int Cmp( const SCH_SHEET_PATH& aSheetPathToTest ) const;

    void CachePageNumber() const { m_cached_page_number = GetPageNumber(); }
    wxString GetCachedPageNumber() const { return m_cached_page_number; }

    /**
     * Compare sheets by their page number. If the actual page number is equal, use virtual page
     * numbers to compare.
     *
     * @return -1 if aSheetPathToTest is greater than this (should appear later in the sort order)
     *          0 if aSheetPathToTest is equal to this
     *          1 if aSheetPathToTest is less than this (should appear earlier in the sort order)
     */
    int ComparePageNum( const SCH_SHEET_PATH& aSheetPathToTest ) const;

    /**
     * Check if this path is contained inside aSheetPathToTest.
     *
     * @param aSheetPathToTest is the sheet path to compare against.
     * @return true if this path is contained inside or equal to aSheetPathToTest.
     */
    bool IsContainedWithin( const SCH_SHEET_PATH& aSheetPathToTest ) const;

    /**
     * Return a pointer to the last #SCH_SHEET of the list.
     *
     * One can see the others sheet as the "path" to reach this last sheet.
     */
    SCH_SHEET* Last() const;

    /**
     * @return the #SCH_SCREEN relative to the last sheet in list.
     */
    SCH_SCREEN* LastScreen();


    ///< @copydoc SCH_SHEET_PATH::LastScreen()
    SCH_SCREEN* LastScreen() const;

    bool GetExcludedFromSim() const;
    bool GetExcludedFromSim( const wxString& aVariantName ) const;
    bool GetExcludedFromBOM() const;
    bool GetExcludedFromBOM( const wxString& aVariantName ) const;
    bool GetExcludedFromBoard() const;
    bool GetDNP() const;
    bool GetDNP( const wxString& aVariantName ) const;

    /**
     * Fetch a SCH_ITEM by ID.
     */
    SCH_ITEM* ResolveItem( const KIID& aID ) const;

    /**
     * Return the path of time stamps which do not changes even when editing sheet parameters.
     *
     * A path is something like / (root) or /34005677 or /34005677/00AE4523.
     */
    wxString PathAsString() const;

    /**
     * Get the sheet path as an #KIID_PATH.
     *
     * @note This #KIID_PATH includes the root sheet UUID prefixed to the path.
     */
    KIID_PATH Path() const;

    /**
     * Return the sheet path in a human readable form made from the sheet names.
     *
     * The "normal" path instead uses the #KIID objects in the path that do not change
     * even when editing sheet parameters.
     */
    wxString PathHumanReadable( bool aUseShortRootName = true,
                                bool aStripTrailingSeparator = false ) const;

    /**
     * Update all the symbol references for this sheet path.
     *
     * Mandatory in complex hierarchies because sheets may use the same screen (basic schematic)
     * more than once but with different references and units according to the displayed sheet.
     */
    void UpdateAllScreenReferences() const;

    /**
     * Append a #SCH_REFERENCE object to \a aReferences based on \a aSymbol
     *
     * @param aReferences List of references to populate.
     * @param aSymbol A symbol to add to aReferences
     * @param aIncludePowerSymbols set to false to only get normal symbols.
     * @param aForceIncludeOrphanSymbols set to true to include symbols having no symbol found
     *                                   in lib.   The normal option is false, and set to true
     *                                   only to build the full list of symbols.
     */
    void AppendSymbol( SCH_REFERENCE_LIST& aReferences, SCH_SYMBOL* aSymbol,
                       bool aIncludePowerSymbols = true,
                       bool aForceIncludeOrphanSymbols = false ) const;

    /**
     * Adds #SCH_REFERENCE object to \a aReferences for each symbol in the sheet.
     *
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols set to false to only get normal symbols.
     * @param aForceIncludeOrphanSymbols set to true to include symbols having no symbol found
     *                                   in lib.   The normal option is false, and set to true
     *                                   only to build the full list of symbols.
     */
    void GetSymbols( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols = true,
                     bool aForceIncludeOrphanSymbols = false ) const;

    /**
     * Append a #SCH_REFERENCE_LIST object to \a aRefList based on \a aSymbol,
     * storing same-reference set of multi-unit parts together.
     *
     * The map key for each element will be the reference designator.
     *
     * @param aRefList Map of reference designators to reference lists
     * @param aSymbol A symbol to add to aRefList
     * @param aIncludePowerSymbols Set to false to only get normal symbols.
     */
    void AppendMultiUnitSymbol( SCH_MULTI_UNIT_REFERENCE_MAP& aRefList, SCH_SYMBOL* aSymbol,
                                bool aIncludePowerSymbols = true ) const;

    /**
     * Add a #SCH_REFERENCE_LIST object to \a aRefList for each same-reference set of
     * multi-unit parts in the sheet.
     *
     * The map key for each element will be the reference designator.
     *
     * @param aRefList Map of reference designators to reference lists
     * @param aIncludePowerSymbols Set to false to only get normal symbols.
     */
    void GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                              bool aIncludePowerSymbols = true ) const;

    /**
     * Test the SCH_SHEET_PATH file names to check adding the sheet stored in the file
     * \a aSrcFileName to the sheet stored in file \a aDestFileName  will cause a sheet
     * path recursion.
     *
     * @param aSrcFileName is the source file name of the sheet add to \a aDestFileName.
     * @param aDestFileName is the file name of the destination sheet for \a aSrcFileName.
     * @return true if \a aFileName will cause recursion in the sheet path.  Otherwise false.
     */
    bool TestForRecursion( const wxString& aSrcFileName, const wxString& aDestFileName );

    /**
     * Make the sheet file name relative to its parent sheet.
     *
     * This should only be called when changing the parent sheet path such performing a save
     * as or a new schematic without a project in stand alone mode.  The sheet file name is
     * only made relative if the current file name is relative.  Absolute sheet file name paths
     * are a user choice so do not change them.
     *
     * Sheet file name paths are set according to the following criteria:
     *  - If the sheet file name path is in the same as the parent sheet file name path, set
     *    the sheet file name to just the file name and extension with no path.
     *  - If the sheet file name path can be made relative to the parent sheet file name path,
     *    set the sheet file name using the relative path.
     *  - If the sheet file name path cannot be converted to a relative path, then fall back to
     *    the absolute file name path.
     */
    void MakeFilePathRelativeToParentSheet();

    /**
     * Attempt to add new symbol instances for all symbols in this sheet path prefixed
     * with \a aPrefixSheetPath.
     *
     * The new symbol instance data will be assigned by the following criteria:
     *  - If the instance data can be found for this sheet path, use the instance data.
     *  - If the instance data cannot be found for this sheet path and the instance data cache
     *    for the symbol is not empty, use the first instance data in the cache.
     *  - If the cache is empty and the library symbol link is valid, set the instance data
     *    from the library symbol.
     *  - If all else fails, set the reference to "U?", the unit to 1, and everything else to
     *    an empty string.
     *
     * @param aPrefixSheetPath is the sheet path to prefix to this sheet path for the new symbol
     *        instance.
     * @param aProjectName is the name of the project for the new symbol instance data.
     */
    void AddNewSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                                const wxString& aProjectName );

    void RemoveSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath );

    void CheckForMissingSymbolInstances( const wxString& aProjectName );

    bool operator==( const SCH_SHEET_PATH& d1 ) const;

    bool operator!=( const SCH_SHEET_PATH& d1 ) const { return !( *this == d1 ) ; }

    bool operator<( const SCH_SHEET_PATH& d1 ) const { return m_sheets < d1.m_sheets; }

private:
    void initFromOther( const SCH_SHEET_PATH& aOther );

protected:
    std::vector<SCH_SHEET*> m_sheets;

    size_t                  m_current_hash;
    mutable wxString        m_cached_page_number;

    int m_virtualPageNumber;           ///< Page numbers are maintained by the sheet load order.

    std::map<std::pair<wxString, wxString>, bool> m_recursion_test_cache;
};


namespace std
{
    template<> struct hash<SCH_SHEET_PATH>
    {
        size_t operator()( const SCH_SHEET_PATH& path ) const;
    };
}

struct SHEET_PATH_HASH
{
    size_t operator()( const SCH_SHEET_PATH& path ) const
    {
        return path.GetCurrentHash();
    }
};

struct SHEET_PATH_CMP
{
    bool operator()( const SCH_SHEET_PATH& lhs, const SCH_SHEET_PATH& rhs ) const
    {
        return lhs.GetCurrentHash() < rhs.GetCurrentHash();
    }
};


/**
 * A container for handling #SCH_SHEET_PATH objects in a flattened hierarchy.
 *
 * #SCH_SHEET objects are not unique, there can be many sheets with the same filename and
 * that share the same #SCH_SCREEN reference.   Each The schematic file (#SCH_SCREEN) may
 * be shared between these sheets and symbol references are specific to a sheet path.
 * When a sheet is entered, symbol references and sheet page number are updated.
 */
class SCH_SHEET_LIST : public std::vector<SCH_SHEET_PATH>
{
public:
    /**
     * Construct a flattened list of SCH_SHEET_PATH objects from \a aSheet.
     *
     * If aSheet == NULL, then this is an empty hierarchy which the user can populate.
     */
    SCH_SHEET_LIST( SCH_SHEET* aSheet = nullptr );

    ~SCH_SHEET_LIST() {}

    /**
     * Check the entire hierarchy for any modifications.
     *
     * @return True if the hierarchy is modified otherwise false.
     */
    bool IsModified() const;

    void ClearModifyStatus();

    /**
     * Fetch a SCH_ITEM by ID.
     *
     * Also returns the sheet the item was found on in \a aPathOut.
     */
    SCH_ITEM* ResolveItem( const KIID& aID, SCH_SHEET_PATH* aPathOut = nullptr,
                           bool aAllowNullptrReturn = false ) const;

    /**
     * Fill an item cache for temporary use when many items need to be fetched.
     */
    void FillItemMap( std::map<KIID, EDA_ITEM*>& aMap );

    /**
     * Silently annotate the not yet annotated power symbols of the entire hierarchy of the
     * sheet path list.
     *
     * It is called before creating a netlist, to annotate power symbols, without prompting
     * the user about not annotated or duplicate for these symbols, if only these symbols
     * need annotation ( a very frequent case ).
     */
    void AnnotatePowerSymbols();

    /**
     * Add a #SCH_REFERENCE object to \a aReferences for each symbol in the list of sheets.
     *
     * @param aReferences List of references to populate.
     * @param aIncludePowerSymbols Set to false to only get normal symbols.
     * @param aForceIncludeOrphanSymbols Set to true to include symbols having no symbol found
     *                                   in lib.   The normal option is false, and set to true
     *                                   only to build the full list of symbols.
     */
    void GetSymbols( SCH_REFERENCE_LIST& aReferences, bool aIncludePowerSymbols = true,
                     bool aForceIncludeOrphanSymbols = false ) const;

    /**
     * Add a #SCH_REFERENCE object to \a aReferences for each symbol in the list of sheets that are
     * contained within \a aSheetPath as well as recursively downwards inside aSheetPath.
     *
     * @param aReferences List of references to populate.
     * @param aSheetPath Path to return symbols from
     * @param aIncludePowerSymbols Set to false to only get normal symbols.
     * @param aForceIncludeOrphanSymbols Set to true to include symbols having no symbol found
     *                                   in lib.   The normal option is false, and set to true
     *                                   only to build the full list of symbols.
     */
    void GetSymbolsWithinPath( SCH_REFERENCE_LIST& aReferences, const SCH_SHEET_PATH& aSheetPath,
                               bool aIncludePowerSymbols = true,
                               bool aForceIncludeOrphanSymbols = false ) const;

    /**
     * Add a #SCH_SHEET_PATH object to \a aSheets for each sheet in the list that are
     * contained within \a aSheetPath as well as recursively downwards inside aSheetPath.
     *
     * @param aReferences List of sheets to populate.
     * @param aSheetPath Path to return sheets from
     */
    void GetSheetsWithinPath( std::vector<SCH_SHEET_PATH>& aSheets,
                              const SCH_SHEET_PATH& aSheetPath ) const;


    /**
     * Finds a SCH_SHEET_PATH that matches the provided KIID_PATH.
     *
     * @param aPath The KIID_PATH to search for.
     */
    std::optional<SCH_SHEET_PATH> GetSheetPathByKIIDPath( const KIID_PATH& aPath,
                                                          bool aIncludeLastSheet = true ) const;


    /**
     * Add a #SCH_REFERENCE_LIST object to \a aRefList for each same-reference set of
     * multi-unit parts in the list of sheets. The map key for each element will be the
     * reference designator.
     *
     * @param aRefList Map of reference designators to reference lists
     * @param aIncludePowerSymbols Set to false to only get normal symbols.
     */
    void GetMultiUnitSymbols( SCH_MULTI_UNIT_REFERENCE_MAP &aRefList,
                              bool aIncludePowerSymbols = true ) const;

    /**
     * Test every #SCH_SHEET_PATH in this #SCH_SHEET_LIST to verify if adding the sheets stored
     * in \a aSrcSheetHierarchy to the sheet stored in \a aDestFileName  will cause recursion.
     *
     * @param aSrcSheetHierarchy is the SCH_SHEET_LIST of the source sheet add to \a aDestFileName.
     * @param aDestFileName is the file name of the destination sheet for \a aSrcFileName.
     * @return true if \a aFileName will cause recursion in the sheet path.  Otherwise false.
     */
    bool TestForRecursion( const SCH_SHEET_LIST& aSrcSheetHierarchy,
                           const wxString& aDestFileName );

    /**
     * Return a pointer to the first #SCH_SHEET_PATH object (not necessarily the only one)
     * matching the provided path. Returns nullptr if not found.
     */
    SCH_SHEET_PATH* FindSheetForPath( const SCH_SHEET_PATH* aPath );

    /**
     * Return the first #SCH_SHEET_PATH object (not necessarily the only one) using a particular
     * screen.
     */
    SCH_SHEET_PATH FindSheetForScreen( const SCH_SCREEN* aScreen );

    /**
     * Return a #SCH_SHEET_LIST with a copy of all the #SCH_SHEET_PATH using a particular screen.
     */
    SCH_SHEET_LIST FindAllSheetsForScreen( const SCH_SCREEN* aScreen ) const;

    /**
     * Build the list of sheets and their sheet path from \a aSheet.
     *
     * If \a aSheet is the root sheet, the full sheet path and sheet list are built.
     *
     * The list will be ordered as per #SCH_SCREEN::BuildSheetList which results in sheets
     * being ordered in the legacy way of using the X and Y positions of the sheets.
     *
     * @see #SortByPageNumbers to sort by page numbers
     *
     * @param aSheet is the starting sheet from which the list is built, or NULL
     *               indicating that g_RootSheet should be used.
     * @throw std::bad_alloc if the memory for the sheet path list could not be allocated.
     */
    void BuildSheetList( SCH_SHEET* aSheet, bool aCheckIntegrity );

    /**
     * Sort the list of sheets by page number. This should be called after #BuildSheetList
     *
     * If page numbers happen to be equal, then it compares the sheet names to ensure deterministic
     * ordering.
     *
     * @param aUpdateVirtualPageNums If true, updates the virtual page numbers to match the new
     * ordering
     */
    void SortByPageNumbers( bool aUpdateVirtualPageNums = true );

    /**
     * This works like #SortByPageNumbers, but it sorts the sheets first by their hierarchical
     * depth and then by their page numbers.  This ensures that printouts follow the
     * hierarchical structure of the schematic.
     */
    void SortByHierarchicalPageNumbers( bool aUpdateVirtualPageNums = true );

    bool NameExists( const wxString& aSheetName ) const;

    bool PageNumberExists( const wxString& aPageNumber ) const;

    /**
     * Truncates the list by removing sheet's with page numbers not in the given list
     *
     * @param aPageInclusions List of Page Numbers (non-virtual) to keep
     */
    void TrimToPageNumbers( const std::vector<wxString>& aPageInclusions );

    /**
     * Update all of the symbol instance information using \a aSymbolInstances.
     *
     * @warning Do not call this on anything other than the full hierarchy.
     *
     * @param aSymbolInstances is the symbol path information loaded from the root schematic.
     */
    void UpdateSymbolInstanceData( const std::vector<SCH_SYMBOL_INSTANCE>& aSymbolInstances );

    /**
     * Update all of the sheet instance information using \a aSheetInstances.
     *
     * @warning Do not call this on anything other than the full hierarchy.
     *
     * @param aSymbolInstances is the symbol path information loaded from the root schematic.
     */
    void UpdateSheetInstanceData( const std::vector<SCH_SHEET_INSTANCE>& aSheetInstances );

    std::vector<KIID_PATH> GetPaths() const;

    /**
     * Fetch the instance information for all of the sheets in the hierarchy.
     *
     * @return all of the sheet instance data for the hierarchy.
     */
    std::vector<SCH_SHEET_INSTANCE> GetSheetInstances() const;

    /**
     * Check all of the sheet instance for empty page numbers.
     *
     * @note This should only return true when loading a legacy schematic or an s-expression
     *       schematic before version 20201005.
     *
     * @return true if all sheet instance page numbers are not defined.  Otherwise false.
     */
    bool AllSheetPageNumbersEmpty() const;

    /**
     * Set initial sheet page numbers.
     *
     * The number scheme is base on the old pseudo sheet numbering algorithm prior to
     * the implementation of user definable sheet page numbers.
     */
    void SetInitialPageNumbers();

    /**
     * Attempt to add new symbol instances for all symbols in this list of sheet paths prefixed
     * with \a aPrefixSheetPath.
     *
     * @param aPrefixSheetPath is the sheet path to append the new symbol instances to.
     * @param aProjectName is the name of the project for the new symbol instance data.
     */
    void AddNewSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                                const wxString& aProjectName );

    void AddNewSheetInstances( const SCH_SHEET_PATH& aPrefixSheetPath,
                               int aLastVirtualPageNumber );

    int GetLastVirtualPageNumber() const;

    void RemoveSymbolInstances( const SCH_SHEET_PATH& aPrefixSheetPath );

    void CheckForMissingSymbolInstances( const wxString& aProjectName );

    bool HasPath( const KIID_PATH& aPath ) const;

    bool ContainsSheet( const SCH_SHEET* aSheet ) const;

    /**
     * Return the ordinal sheet path of \a aScreen.
     *
     * @warning In order for this method to work correctly, the sheet list must be sorted by page
     *          number.
     *
     * @return an optional #SCH_SHEET_PATH for the ordinal path of \a aScreen or an empty value
     *         if \a aScreen has no ordinal sheet path in the list.
     */
    std::optional<SCH_SHEET_PATH> GetOrdinalPath( const SCH_SCREEN* aScreen ) const;

private:
    SCH_SHEET_PATH  m_currentSheetPath;
};

#endif // CLASS_DRAWSHEET_PATH_H
