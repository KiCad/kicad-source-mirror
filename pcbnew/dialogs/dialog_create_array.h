/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 John Beard, john.j.beard@gmail.com
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __DIALOG_CREATE_ARRAY__
#define __DIALOG_CREATE_ARRAY__

// Include the wxFormBuider header base:
#include <dialog_create_array_base.h>

#include <array_options.h>
#include <class_board_item.h>
#include <pcb_base_frame.h>

#include <boost/bimap.hpp>
#include <widgets/unit_binder.h>
#include <widgets/widget_save_restore.h>


class DIALOG_CREATE_ARRAY : public DIALOG_CREATE_ARRAY_BASE
{
public:

    #define NUMBERING_TYPE_MAX NUMBERING_ALPHA_FULL

    // Constructor and destructor
    DIALOG_CREATE_ARRAY( PCB_BASE_FRAME* aParent, bool enableNumbering,
                         wxPoint aOrigPos );

    ~DIALOG_CREATE_ARRAY();

    /*!
     * @return the array options set by this dialogue, or NULL if they were
     * not set, or could not be set
     */
    ARRAY_OPTIONS* GetArrayOptions() const
    {
        return m_settings;
    }

private:

    /**
     * The settings object returned to the caller.
     * We retain ownership of this
     */
    ARRAY_OPTIONS* m_settings;

    UNIT_BINDER    m_hSpacing, m_vSpacing;
    UNIT_BINDER    m_hOffset, m_vOffset;
    UNIT_BINDER    m_hCentre, m_vCentre;
    UNIT_BINDER    m_circRadius;
    UNIT_BINDER    m_circAngle;

    WIDGET_SAVE_RESTORE m_cfg_persister;

    /*
     * The position of the original item(s), used for finding radius, etc
     */
    const wxPoint m_originalItemPosition;

    // Event callbacks
    void    OnParameterChanged( wxCommandEvent& event ) override;

    // Internal callback handlers
    void setControlEnablement();
    void calculateCircularArrayProperties();

    bool TransferDataFromWindow() override;

    // some uses of arrays might not allow component renumbering
    bool m_numberingEnabled;
};

#endif      // __DIALOG_CREATE_ARRAY__
