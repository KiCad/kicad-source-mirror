/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <wx/string.h>

class LIB_SYMBOL;


/**
 * Information about a symbol available for import.
 */
struct SYMBOL_IMPORT_INFO
{
    wxString                    m_name;           ///< Symbol name
    std::unique_ptr<LIB_SYMBOL> m_symbol;         ///< Loaded symbol (may be null)
    wxString                    m_parentName;     ///< Parent symbol name if derived
    bool                        m_isPower;        ///< True if power symbol
    bool                        m_existsInDest;   ///< True if symbol exists in destination library
    bool                        m_checked;        ///< User's manual selection state
    bool                        m_autoSelected;   ///< True if auto-selected as dependency

    SYMBOL_IMPORT_INFO() :
            m_isPower( false ),
            m_existsInDest( false ),
            m_checked( false ),
            m_autoSelected( false )
    {
    }
};


/**
 * Result of conflict resolution for a single symbol.
 */
enum class CONFLICT_RESOLUTION
{
    SKIP,      ///< Don't import this symbol
    OVERWRITE  ///< Overwrite existing symbol
};


/**
 * Manages the logic for selecting symbols to import from a library file.
 *
 * This class handles:
 * - Loading and tracking symbol metadata from source libraries
 * - Building dependency graphs for derived symbols
 * - Auto-selecting parent symbols when derived symbols are selected
 * - Tracking conflicts with existing symbols in destination library
 * - Filtering symbols by search criteria
 *
 * The class is designed to be UI-independent for testability.
 */
class SYMBOL_IMPORT_MANAGER
{
public:
    /**
     * Callback type for checking if a symbol exists in the destination library.
     * @param symbolName The name of the symbol to check
     * @return true if the symbol already exists
     */
    using SYMBOL_EXISTS_FUNC = std::function<bool( const wxString& symbolName )>;

    SYMBOL_IMPORT_MANAGER();
    ~SYMBOL_IMPORT_MANAGER();

    /**
     * Clear all loaded symbols and reset state.
     */
    void Clear();

    /**
     * Add a symbol to the import list.
     *
     * @param aName Symbol name
     * @param aParentName Parent symbol name (empty if not derived)
     * @param aIsPower True if this is a power symbol
     * @param aSymbol Optional loaded symbol (takes ownership)
     */
    void AddSymbol( const wxString& aName, const wxString& aParentName, bool aIsPower,
                    LIB_SYMBOL* aSymbol = nullptr );

    /**
     * Mark symbols that exist in the destination library.
     *
     * @param aExistsFunc Function to check if a symbol exists in destination
     */
    void CheckExistingSymbols( SYMBOL_EXISTS_FUNC aExistsFunc );

    /**
     * Build the dependency maps from parent relationships.
     * Call this after all symbols have been added.
     */
    void BuildDependencyMaps();

    /**
     * Get all symbol names.
     */
    std::vector<wxString> GetSymbolNames() const;

    /**
     * Get symbol info by name.
     * @return Pointer to info or nullptr if not found
     */
    SYMBOL_IMPORT_INFO* GetSymbolInfo( const wxString& aName );
    const SYMBOL_IMPORT_INFO* GetSymbolInfo( const wxString& aName ) const;

    /**
     * Get all ancestors of a symbol (full inheritance chain).
     *
     * @param aSymbolName Name of the symbol
     * @return Set of ancestor symbol names (may be empty)
     */
    std::set<wxString> GetAncestors( const wxString& aSymbolName ) const;

    /**
     * Get all descendants of a symbol (all derived symbols recursively).
     *
     * @param aSymbolName Name of the symbol
     * @return Set of descendant symbol names (may be empty)
     */
    std::set<wxString> GetDescendants( const wxString& aSymbolName ) const;

    /**
     * Get the direct parent of a symbol.
     *
     * @param aSymbolName Name of the symbol
     * @return Parent name or empty string if none
     */
    wxString GetParent( const wxString& aSymbolName ) const;

    /**
     * Get direct children (derivatives) of a symbol.
     *
     * @param aSymbolName Name of the symbol
     * @return Vector of direct derivative names
     */
    std::vector<wxString> GetDirectDerivatives( const wxString& aSymbolName ) const;

    /**
     * Check if a symbol is derived from another.
     */
    bool IsDerived( const wxString& aSymbolName ) const;

    /**
     * Set the selection state of a symbol.
     * This handles auto-selection of ancestors.
     *
     * @param aSymbolName Symbol to select/deselect
     * @param aSelected New selection state
     * @return Vector of symbol names whose auto-selection state changed
     */
    std::vector<wxString> SetSymbolSelected( const wxString& aSymbolName, bool aSelected );

    /**
     * Get selected descendants that would be orphaned if a symbol is deselected.
     *
     * @param aSymbolName Symbol to check
     * @return Vector of selected descendant names that depend on this symbol
     */
    std::vector<wxString> GetSelectedDescendants( const wxString& aSymbolName ) const;

    /**
     * Force deselection of a symbol and all its descendants.
     * Used after user confirms cascade deselection.
     *
     * @param aSymbolName Symbol to deselect along with descendants
     */
    void DeselectWithDescendants( const wxString& aSymbolName );

    /**
     * Select all symbols (optionally filtered).
     *
     * @param aFilter Optional filter function - only select symbols where filter returns true
     */
    void SelectAll( std::function<bool( const wxString& )> aFilter = nullptr );

    /**
     * Deselect all symbols (optionally filtered).
     *
     * @param aFilter Optional filter function - only deselect symbols where filter returns true
     */
    void DeselectAll( std::function<bool( const wxString& )> aFilter = nullptr );

    /**
     * Get list of all symbols that will be imported (checked + auto-selected).
     */
    std::vector<wxString> GetSymbolsToImport() const;

    /**
     * Get count of manually selected symbols.
     */
    int GetManualSelectionCount() const;

    /**
     * Get count of auto-selected symbols (dependencies).
     */
    int GetAutoSelectionCount() const;

    /**
     * Get list of symbols that will conflict (selected and exist in destination).
     */
    std::vector<wxString> GetConflicts() const;

    /**
     * Check if a symbol name matches a filter string (case-insensitive contains).
     */
    static bool MatchesFilter( const wxString& aSymbolName, const wxString& aFilter );

    /**
     * Get total number of symbols.
     */
    size_t GetSymbolCount() const { return m_symbols.size(); }

private:
    /**
     * Recalculate auto-selection state for all symbols based on current manual selections.
     */
    void recalculateAutoSelections();

    /// All symbols available for import, keyed by name
    std::map<wxString, SYMBOL_IMPORT_INFO> m_symbols;

    /// Map from symbol name to its parent name (for derived symbols)
    std::map<wxString, wxString> m_parentMap;

    /// Map from parent name to list of direct derivative names
    std::map<wxString, std::vector<wxString>> m_derivativesMap;
};
