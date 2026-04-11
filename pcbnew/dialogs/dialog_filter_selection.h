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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <dialogs/dialog_filter_selection_base.h>

class PCB_BASE_FRAME;

class DIALOG_FILTER_SELECTION : public DIALOG_FILTER_SELECTION_BASE
{
public:
    /**
     * Struct that will be set with the result of the user choices in the dialog
     */
    struct OPTIONS
    {
        bool includeFootprints        = true;
        bool includeLockedFootprints  = true;
        bool includeTracks            = true;
        bool includeVias              = true;
        bool includeZones             = true;
        bool includeItemsOnTechLayers = true;
        bool includeBoardOutlineLayer = true;
        bool includePcbTexts          = true;
    };

    /**
     * Create the filter selection dialog.
     *
     * @param[in] aParent is the parent window that called the dialog.
     * @param[in,out] aOptions is the options to populate the dialog with and contains the
     *                         changes made by the dialog on exit.
     */
    DIALOG_FILTER_SELECTION( PCB_BASE_FRAME* aParent, OPTIONS& aOptions );
    ~DIALOG_FILTER_SELECTION() = default;

protected:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    void checkBoxClicked( wxCommandEvent& aEvent ) override;
    void allItemsClicked( wxCommandEvent& aEvent ) override;

    void forceCheckboxStates( bool aNewState );
    wxCheckBoxState GetSuggestedAllItemsState( void );

    ///< Reference to the options struct to fill.
    OPTIONS& m_options;
};
