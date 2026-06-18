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

#include <memory>
#include <vector>
#include <wx/string.h>

#include <pin_map.h>
#include <panel_symbol_pin_map_base.h>

class LIB_SYMBOL;
class PIN_MAP_GRID_TRICKS;


/**
 * Composed widget that edits a symbol's named pin maps and the footprints they
 * are associated with.
 *
 * The grid lists every logical pin of the symbol as a row.  The leading columns
 * describe the pin (unit, number, name) and one trailing column per associated
 * footprint carries the pad that the pin resolves to under that footprint's map.
 * Editing a pad cell rewrites the corresponding PIN_MAP entry; the leading
 * columns are read-only.
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

    /// Point the association behind \a aCol at the existing named map \a aMapName.
    void BindColumnToMap( int aCol, const wxString& aMapName );

    /// @return the footprint label shown for pad column \a aCol, for menu captions.
    wxString GetColumnFootprintLabel( int aCol ) const;

    /// @return the names of every map currently in the working set, for the bind menu.
    std::vector<wxString> GetMapNames() const;

    /// Re-evaluate the colour of pad cell (aRow, aCol) from its current contents.
    void ValidateCell( int aRow, int aCol );

protected:
    void OnAddFootprint( wxCommandEvent& aEvent ) override;
    void OnRemoveFootprint( wxCommandEvent& aEvent ) override;
    void OnSizeGrid( wxSizeEvent& aEvent ) override;

private:
    void onCellChanged( wxGridEvent& aEvent );

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

    LIB_SYMBOL*                          m_symbol;
    std::vector<wxString>                m_pinNumbers; // one per grid row, logical numbers
    PIN_MAP_SET                          m_pinMaps;
    std::vector<ASSOCIATED_FOOTPRINT>    m_associations;
    std::unique_ptr<PIN_MAP_GRID_TRICKS> m_gridTricks;
};

#endif // PANEL_SYMBOL_PIN_MAP_H
