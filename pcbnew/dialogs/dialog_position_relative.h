/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __DIALOG_POSITION_RELATIVE__
#define __DIALOG_POSITION_RELATIVE__

// Include the wxFormBuider header base:
#include <vector>
#include <dialogs/dialog_position_relative_base.h>

#include <tool/tool_manager.h>
#include "tools/position_relative_tool.h"

class DIALOG_POSITION_RELATIVE : public DIALOG_POSITION_RELATIVE_BASE
{
private:

    TOOL_MANAGER* m_toolMgr;
    wxPoint&    m_translation;
    double&     m_rotation;
    wxPoint&    m_anchor_position;

public:
    // Constructor and destructor
    DIALOG_POSITION_RELATIVE( PCB_BASE_FRAME* aParent, TOOL_MANAGER* toolMgr, wxPoint& translation,
            double& rotation, wxPoint& anchorposition );
    ~DIALOG_POSITION_RELATIVE();

    void UpdateAnchor( BOARD_ITEM* aBoardItem );

private:

    /*!
     * Reset a text field to be 0 if it was exited while blank
     */
    void OnTextFocusLost( wxFocusEvent& event ) override;

    void    OnPolarChanged( wxCommandEvent& event ) override;
    void    OnClear( wxCommandEvent& event ) override;

    void    OnSelectItemClick( wxCommandEvent& event ) override;
    void    OnOkClick( wxCommandEvent& event ) override;

    /**
     * Convert a given Cartesian point into a polar representation.
     *
     * Linear units are not considered, the answer is in the same units as given
     * Angle is returned in degrees
     */
    void ToPolarDeg( double x, double y, double& r, double& q );

    /**
     * Get the (Cartesian) translation described by the text entries
     * @param val output translation vector
     * @param polar interpret as polar coords
     * @return false if error (though the text conversion functions don't report errors)
     */
    bool GetTranslationInIU( wxPoint& val, bool polar );

    // Update texts (comments) after changing the coordinates type (polar/cartesian)
    void updateDlgTexts( bool aPolar );

    /**
     * Persistent dialog options
     */
    struct POSITION_RELATIVE_OPTIONS
    {
        bool polarCoords;
        double  entry1;
        double  entry2;
        double  entryRotation;

        POSITION_RELATIVE_OPTIONS() :
            polarCoords( false ),
            entry1( 0 ),
            entry2( 0 ),
            entryRotation( 0 )
        {
        }
    };

    // persistent settings
    static POSITION_RELATIVE_OPTIONS m_options;
};

#endif      // __DIALOG_POSITION_RELATIVE__
