/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_BLOCK_OPTIONS_H_
#define DIALOG_BLOCK_OPTIONS_H_

#include <dialogs/dialog_block_options_base.h>

class PCB_BASE_FRAME;

class DIALOG_BLOCK_OPTIONS : public DIALOG_BLOCK_OPTIONS_BASE
{
public:

    /**
     * Struct that will be set with the result of the user choices
     * in the dialog
     */
    struct OPTIONS
    {
        bool includeModules     = true;
        bool includeLockedModules = true;
        bool includeTracks      = true;
        bool includeZones       = true;
        bool includeItemsOnTechLayers  = true;
        bool includeBoardOutlineLayer = true;
        bool includePcbTexts   = true;
        bool drawItems = true;
        bool includeItemsOnInvisibleLayers = false;
    };

    DIALOG_BLOCK_OPTIONS( PCB_BASE_FRAME* aParent, OPTIONS& aOptions,
                          bool aShowLegacyOptions,
                          const wxString& aTitle );

    ~DIALOG_BLOCK_OPTIONS()
    {
    }

private:
    void ExecuteCommand( wxCommandEvent& event ) override;

    void OnCancel( wxCommandEvent& event ) override
    {
        EndModal( wxID_CANCEL );
    }

    void checkBoxClicked( wxCommandEvent& aEvent ) override;

    ///< Reference to the options struct to fill
    OPTIONS& m_options;
};

#endif // DIALOG_EXCHANGE_MODULES_H_
