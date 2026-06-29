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

#ifndef PANEL_SYMBOL_PIN_MAP_H
#define PANEL_SYMBOL_PIN_MAP_H

#include <map>
#include <memory>
#include <set>
#include <vector>
#include <wx/string.h>

#include <pin_map.h>
#include <panel_symbol_pin_map_base.h>

class LIB_SYMBOL;
class PIN_MAP_GRID_TRICKS;


/**
 * Composed widget that edits a symbol's named pin maps and the footprint each is bound to.
 *
 * Each grid column is a named pin map.  Row 0 binds a footprint to the map; the remaining rows
 * are the symbol's logical pins, whose cells carry the pad each pin maps to.  A cell left at its
 * pin number is identity and is not stored.  The leading columns (unit, number, name) are
 * read-only.
 */
class PANEL_SYMBOL_PIN_MAP : public PANEL_SYMBOL_PIN_MAP_BASE
{
public:
    PANEL_SYMBOL_PIN_MAP( wxWindow* aParent );
    ~PANEL_SYMBOL_PIN_MAP() override;

    /**
     * Load the editable state from \a aSymbol.
     *
     * The symbol is not retained; only its pin list, pin maps and associated
     * footprints are copied into the working model.
     */
    void SetSymbol( LIB_SYMBOL* aSymbol );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /**
     * Write the edited pin maps and associations into \a aSymbol.
     *
     * Provided so the host dialog can target a symbol other than the one used to
     * populate the panel.  Equivalent to TransferDataFromWindow() when given the
     * same symbol.
     */
    void ApplyToSymbol( LIB_SYMBOL* aSymbol );

    /// Commit any in-progress grid cell edit; @return false if the edit is invalid.  Call this for
    /// validation before the host snapshots the symbol for undo, then ApplyToSymbol() afterwards.
    bool CommitPendingChanges();

    /// Number of fixed leading columns (unit, pin number, pin name).
    static constexpr int FIXED_COLS = 3;

    /// Set every cell in pad column \a aCol back to its row's symbol pin number (1:1).
    void ResetColumnToIdentity( int aCol );

    /// Copy every pad value from column \a aSrcCol into column \a aDstCol.
    void CopyColumn( int aSrcCol, int aDstCol );

    /// Prompt for a new name for column \a aCol's map and update every reference to it.
    void RenameColumn( int aCol );

    /// @return the map name shown for pad column \a aCol, for menu captions.
    wxString GetColumnMapName( int aCol ) const;

    /// Re-evaluate the colour of pad cell (aRow, aCol) from its current contents.
    void ValidateCell( int aRow, int aCol );

protected:
    void OnAddMap( wxCommandEvent& aEvent ) override;
    void OnRemoveMap( wxCommandEvent& aEvent ) override;
    void OnSizeGrid( wxSizeEvent& aEvent ) override;

private:
    void onCellChanged( wxGridEvent& aEvent );

    /// Double-clicking a map column's header opens its rename dialog.
    void onLabelDClick( wxGridEvent& aEvent );

    /// Bind (or clear, when empty) the footprint entered in column \a aCol's footprint cell.
    void applyColumnFootprint( int aCol, const wxString& aFootprintId );

    /// Rebuild the whole grid from the working model.
    void rebuildGrid();

    /// Resize the pad columns to fill the available width.
    void adjustGridColumns();

    /// Read every pad cell back into the working pin maps.
    void harvestGrid();

    /// Re-evaluate the colour of every pad cell.
    void validateAllCells();

    /// @return a map name not already used by m_pinMaps.
    wxString makeUniqueMapName() const;

    /// @return the pad number strings of aFootprintId.
    const std::set<wxString>& padNumbersFor( const wxString& aFootprintId );

    LIB_SYMBOL*                            m_symbol;
    std::vector<wxString>                  m_pinNumbers; // one per grid row, logical numbers
    PIN_MAP_SET                            m_pinMaps;
    std::vector<ASSOCIATED_FOOTPRINT>      m_associations;
    std::unique_ptr<PIN_MAP_GRID_TRICKS>   m_gridTricks;
    std::map<wxString, std::set<wxString>> m_footprintPads;
};

#endif // PANEL_SYMBOL_PIN_MAP_H
