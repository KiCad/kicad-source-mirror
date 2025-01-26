/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
#ifndef DIALOG_SYMBOL_CHOOSER_H
#define DIALOG_SYMBOL_CHOOSER_H

#include "dialog_shim.h"
#include <template_fieldnames.h>
#include <symbol_library_common.h>
#include <symbol_tree_model_adapter.h>
#include <footprint_info.h>
#include <widgets/html_window.h>
#include <wx/checkbox.h>


class SCH_BASE_FRAME;
class PANEL_SYMBOL_CHOOSER;
struct PICKED_SYMBOL;


class DIALOG_SYMBOL_CHOOSER : public DIALOG_SHIM
{
public:
    /**
     * Create dialog to choose symbol.
     *
     * @param aParent   a SCH_BASE_FRAME parent window.
     * @param aAllowFieldEdits  if false, all functions that allow the user to edit fields
     *                          (currently just footprint selection) will not be available.
     * @param aShowFootprints   if false, all footprint preview and selection features are
     *                          disabled. This forces aAllowFieldEdits false too.
     */
    DIALOG_SYMBOL_CHOOSER( SCH_BASE_FRAME* aParent, const LIB_ID* aPreselect,
                           const SYMBOL_LIBRARY_FILTER* aFilter,
                           std::vector<PICKED_SYMBOL>& aHistoryList,
                           std::vector<PICKED_SYMBOL>& aAlreadyPlaced, bool aAllowFieldEdits,
                           bool aShowFootprints, bool& aCancelled );

    ~DIALOG_SYMBOL_CHOOSER();

    /**
     * To be called after this dialog returns from ShowModal().
     *
     * For multi-unit symbols, if the user selects the symbol itself rather than picking
     * an individual unit, 0 will be returned in aUnit.
     * Beware that this is an invalid unit number - this should be replaced with whatever
     * default is desired (usually 1).
     *
     * @param aUnit if not NULL, the selected unit is filled in here.
     * @return the #LIB_ID of the symbol that has been selected.
     */
    LIB_ID GetSelectedLibId( int* aUnit = nullptr ) const;

    /**
     * Get a list of fields edited by the user.
     *
     * @return vector of pairs; each.first = field ID, each.second = new value.
     */
    std::vector<std::pair<FIELD_T, wxString>> GetFields() const;

    bool GetKeepSymbol() { return m_keepSymbol->GetValue(); }
    bool GetPlaceAllUnits() { return m_useUnits->GetValue(); }

public:
    static std::mutex         g_Mutex;

protected:
    PANEL_SYMBOL_CHOOSER*     m_chooserPanel;
    wxCheckBox*               m_keepSymbol;
    wxCheckBox*               m_useUnits;

private:
    void onLazyLoadUpdate();

    wxString                  m_originalTitle;
};

#endif /* DIALOG_SYMBOL_CHOOSER_H */
