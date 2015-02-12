/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

// Include the wxFormBuider header base:
#include <vector>
#include <dialog_move_exact_base.h>

class DIALOG_MOVE_EXACT : public DIALOG_MOVE_EXACT_BASE
{
private:

    wxPoint& m_translation;
    double& m_rotation;

public:

    enum MOVE_EDIT_T
    {
        MOVE_ABORT,     ///< if not changed or error
        MOVE_OK,        ///< if successfully changed
    };

    // Constructor and destructor
    DIALOG_MOVE_EXACT( PCB_BASE_FRAME* aParent, wxPoint& translation,
                       double& rotation );
    ~DIALOG_MOVE_EXACT();

private:

    void OnTextFocusLost( wxFocusEvent& event );
    void OnPolarChanged( wxCommandEvent& event );
    void OnClear( wxCommandEvent& event );

    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );

    void ToPolarDeg( double x, double y, double& r, double& q );
    bool GetTranslationInIU ( wxPoint& val, bool polar );

    /**
     * Persistent dialog options
     */
    struct MOVE_EXACT_OPTIONS
    {
        bool    polarCoords;
        double  entry1;
        double  entry2;
        double  entryRotation;

        MOVE_EXACT_OPTIONS():
            polarCoords(false),
            entry1(0),
            entry2(0),
            entryRotation(0)
        {
        }
    };

    // persistent settings
    static MOVE_EXACT_OPTIONS m_options;
};

#endif      //  __DIALOG_MOVE_EXACT__
