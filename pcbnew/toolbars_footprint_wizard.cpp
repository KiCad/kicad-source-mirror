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

#include <tools/pcb_actions.h>
#include <toolbars_footprint_wizard.h>


std::optional<TOOLBAR_CONFIGURATION> FOOTPRINT_WIZARD_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    case TOOLBAR_LOC::LEFT:
    case TOOLBAR_LOC::RIGHT:
    case TOOLBAR_LOC::TOP_AUX:
        return std::nullopt;

    case TOOLBAR_LOC::TOP_MAIN:
        /* TODO (ISM): Convert to action
        m_tbTopMain->AddTool( ID_FOOTPRINT_WIZARD_SELECT_WIZARD, wxEmptyString,
            KiBitmap( BITMAPS::module_wizard ),
            _( "Select wizard script to run" ) );
    */

        config.AppendSeparator();
        /* TODO (ISM): Convert to action
        m_tbTopMain->AddTool( ID_FOOTPRINT_WIZARD_RESET_TO_DEFAULT, wxEmptyString,
                KiBitmap( BITMAPS::reload ),
                _( "Reset wizard parameters to default") );
    */
        config.AppendSeparator();
        /* TODO (ISM): Convert to action
        m_tbTopMain->AddTool( ID_FOOTPRINT_WIZARD_PREVIOUS, wxEmptyString,
                KiBitmap( BITMAPS::lib_previous ),
                _( "Select previous parameters page" ) );
        m_tbTopMain->AddTool( ID_FOOTPRINT_WIZARD_NEXT, wxEmptyString,
                KiBitmap( BITMAPS::lib_next ),
                _( "Select next parameters page" ) );
    */

        config.AppendSeparator()
            .AppendAction( ACTIONS::zoomRedraw )
            .AppendAction( ACTIONS::zoomInCenter )
            .AppendAction( ACTIONS::zoomOutCenter )
            .AppendAction( ACTIONS::zoomFitScreen );

        // The footprint wizard always can export the current footprint
        config.AppendSeparator();
        /* TODO (ISM): Convert to action
        m_tbTopMain->AddTool( ID_FOOTPRINT_WIZARD_DONE,
                wxEmptyString, KiBitmap( BITMAPS::export_footprint_names ),
                _( "Export footprint to editor" ) );
    */
        break;
    }

    // clang-format on
    return config;
}
