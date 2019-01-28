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

class CONFIG_SAVE_RESTORE_WINDOW
{
private:

    enum CONFIG_CTRL_TYPE_T
    {
        CFG_CTRL_TEXT,
        CFG_CTRL_UNIT_BINDER,
        CFG_CTRL_CHECKBOX,
        CFG_CTRL_RADIOBOX,
        CFG_CTRL_CHOICE,
        CFG_CTRL_TAB
    };

    struct CONFIG_CTRL_T
    {
        void* control;
        CONFIG_CTRL_TYPE_T type;
        void* dest;
    };

    std::vector<CONFIG_CTRL_T> ctrls;
    bool& valid;

protected:
    CONFIG_SAVE_RESTORE_WINDOW( bool& validFlag ) :
        valid( validFlag )
    {}

    void Add( wxRadioBox* ctrl, int& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_RADIOBOX, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( wxCheckBox* ctrl, bool& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_CHECKBOX, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( wxTextCtrl* ctrl, wxString& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_TEXT, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( UNIT_BINDER& ctrl, int& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { &ctrl, CFG_CTRL_UNIT_BINDER, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }


    void Add( wxChoice* ctrl, int& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_CHOICE, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void Add( wxNotebook* ctrl, int& dest )
    {
        CONFIG_CTRL_T ctrlInfo = { ctrl, CFG_CTRL_TAB, (void*) &dest };

        ctrls.push_back( ctrlInfo );
    }

    void ReadConfigFromControls()
    {
        for( std::vector<CONFIG_CTRL_T>::const_iterator iter = ctrls.begin(), iend = ctrls.end();
             iter != iend; ++iter )
        {
            switch( iter->type )
            {
            case CFG_CTRL_CHECKBOX:
                *(bool*) iter->dest = static_cast<wxCheckBox*>( iter->control )->GetValue();
                break;

            case CFG_CTRL_TEXT:
                *(wxString*) iter->dest = static_cast<wxTextCtrl*>( iter->control )->GetValue();
                break;

            case CFG_CTRL_UNIT_BINDER:
                *(int*) iter->dest = static_cast<UNIT_BINDER*>( iter->control )->GetValue();
                break;

            case CFG_CTRL_CHOICE:
                *(int*) iter->dest = static_cast<wxChoice*>( iter->control )->GetSelection();
                break;

            case CFG_CTRL_RADIOBOX:
                *(int*) iter->dest = static_cast<wxRadioBox*>( iter->control )->GetSelection();
                break;

            case CFG_CTRL_TAB:
                *(int*) iter->dest = static_cast<wxNotebook*>( iter->control )->GetSelection();
                break;

            default:
                wxASSERT_MSG( false, wxString(
                                "Unhandled control type for config store: " ) << iter->type );
            }
        }

        valid = true;
    }

    void RestoreConfigToControls()
    {
        if( !valid )
            return;

        for( std::vector<CONFIG_CTRL_T>::const_iterator iter = ctrls.begin(), iend = ctrls.end();
             iter != iend; ++iter )
        {
            switch( iter->type )
            {
            case CFG_CTRL_CHECKBOX:
                static_cast<wxCheckBox*>( iter->control )->SetValue( *(bool*) iter->dest );
                break;

            case CFG_CTRL_TEXT:
                static_cast<wxTextCtrl*>( iter->control )->SetValue( *(wxString*) iter->dest );
                break;

            case CFG_CTRL_UNIT_BINDER:
                static_cast<UNIT_BINDER*>( iter->control )->SetValue( *(int*) iter->dest );
                break;

            case CFG_CTRL_CHOICE:
                static_cast<wxChoice*>( iter->control )->SetSelection( *(int*) iter->dest );
                break;

            case CFG_CTRL_RADIOBOX:
                static_cast<wxRadioBox*>( iter->control )->SetSelection( *(int*) iter->dest );
                break;

            case CFG_CTRL_TAB:
                static_cast<wxNotebook*>( iter->control )->SetSelection( *(int*) iter->dest );
                break;

            default:
                wxASSERT_MSG( false, wxString(
                                "Unhandled control type for config restore: " ) << iter->type );
            }
        }
    }
};

class DIALOG_CREATE_ARRAY : public DIALOG_CREATE_ARRAY_BASE,
    public CONFIG_SAVE_RESTORE_WINDOW
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
