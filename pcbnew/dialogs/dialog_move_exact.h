/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __DIALOG_MOVE_EXACT__
#define __DIALOG_MOVE_EXACT__

#include <vector>
#include <widgets/unit_binder.h>

#include <dialogs/dialog_move_exact_base.h>


class PCB_BASE_FRAME;


enum ROTATION_ANCHOR
{
    ROTATE_AROUND_ITEM_ANCHOR,
    ROTATE_AROUND_SEL_CENTER,
    ROTATE_AROUND_USER_ORIGIN,
    ROTATE_AROUND_AUX_ORIGIN
};


class DIALOG_MOVE_EXACT : public DIALOG_MOVE_EXACT_BASE
{
public:
    DIALOG_MOVE_EXACT( PCB_BASE_FRAME* aParent, wxPoint& aTranslate, double& aRotate,
                       ROTATION_ANCHOR& aAnchor, const EDA_RECT& aBbox );
    ~DIALOG_MOVE_EXACT() { };

private:

    /**
     * Reset a text field to be 0 if it was exited while blank.
     */
    void OnTextFocusLost( wxFocusEvent& event ) override;

    void OnPolarChanged( wxCommandEvent& event ) override;
    void OnClear( wxCommandEvent& event ) override;
    void OnTextChanged( wxCommandEvent& event ) override;

    // Automatically called when clicking on the OK button
    bool TransferDataFromWindow() override;

    /**
     * Convert a given Cartesian point into a polar representation.
     *
     * Linear units are not considered, the answer is in the same units as given
     * Angle is returned in degrees
     */
    void ToPolarDeg( double x, double y, double& r, double& q );

    /**
     * Get the (Cartesian) translation described by the text entries.
     *
     * @param val is the output translation vector.
     * @param polar set to true to interpret as polar coordinates.
     * @return false if error (though the text conversion functions don't report errors).
     */
    bool GetTranslationInIU( wxRealPoint& val, bool polar );

    void buildRotationAnchorMenu();

    // Update controls and their labels after changing the coordinates type (polar/cartesian)
    void updateDialogControls( bool aPolar );

    /**
     * Persistent dialog options
     */
    struct MOVE_EXACT_OPTIONS
    {
        bool     polarCoords;
        wxString entry1;
        wxString entry2;
        wxString entryRotation;
        size_t   entryAnchorSelection;

        MOVE_EXACT_OPTIONS():
            polarCoords( false ),
            entry1( wxT( "0" ) ),
            entry2( wxT( "0" ) ),
            entryRotation( wxT( "0" ) ),
            entryAnchorSelection( 0 )
        {
        }
    };

    // persistent settings
    static MOVE_EXACT_OPTIONS m_options;

private:
    wxPoint&          m_translation;
    double&           m_rotation;
    ROTATION_ANCHOR&  m_rotationAnchor;
    const EDA_RECT&   m_bbox;

    UNIT_BINDER       m_moveX;
    UNIT_BINDER       m_moveY;
    UNIT_BINDER       m_rotate;

    std::vector<ROTATION_ANCHOR> m_menuIDs;

    double m_stateX;
    double m_stateY;
    double m_stateRadius;
    double m_stateTheta;
};

#endif      //  __DIALOG_MOVE_EXACT__
