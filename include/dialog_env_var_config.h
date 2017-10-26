/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2015 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef _DIALOG_ENV_VAR_CONFIG_H_
#define _DIALOG_ENV_VAR_CONFIG_H_

class EDA_DRAW_FRAME;

#include <../common/dialogs/dialog_env_var_config_base.h>

#include <pgm_base.h>


/**
 * DIALOG_ENV_VAR_CONFIG class declaration
 */

class DIALOG_ENV_VAR_CONFIG: public DIALOG_ENV_VAR_CONFIG_BASE
{
private:
    ENV_VAR_MAP  m_envVarMap;
    bool         m_extDefsChanged = false;

protected:

    /**
     * Update the displayed list of ENV_VAR paths
     */
    void PopulatePathList();

    /**
     * Edit the currently selected ENV_VAR entry
     */
    void EditSelectedEntry();

    // Various button callbacks
    virtual void OnAddButton( wxCommandEvent& event ) override;
    virtual void OnEditButton( wxCommandEvent& event ) override;
    virtual void OnRemoveButton( wxCommandEvent& event ) override;
    virtual void OnHelpButton( wxCommandEvent& event ) override;

    virtual void OnPathSelected( wxListEvent& event ) override;
    virtual void OnPathActivated( wxListEvent& event ) override;

    // Variable for keeping track of currently selected list index
    unsigned int m_pathIndex = 0;

    /**
     * Extract the NAME and PATH data from the ENV_VAR at the provided index
     * @param aIndex is the index to extract data from
     * @return true if data was extracted else false
     */
    bool GetPathAtIndex( unsigned int aIndex, wxString& aEnvVar, wxString& aEnvPath );

    /**
     * Determine if a particular ENV_VAR is protected
     * @param aEnvVar is the name of the ENV_VAR
     */
    bool IsEnvVarImmutable( const wxString aEnvVar );

    /**
     * Select the ENV_VAR at the provided index
     */
    void SelectListIndex( unsigned int aIndex );

public:
    DIALOG_ENV_VAR_CONFIG( wxWindow* parent, const ENV_VAR_MAP& aEnvVarMap );
    virtual ~DIALOG_ENV_VAR_CONFIG() {}

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool ExternalDefsChanged() const { return m_extDefsChanged; }

    const ENV_VAR_MAP& GetEnvVarMap() const
    {
        return m_envVarMap;
    }
};

#endif    // _DIALOG_ENV_VAR_CONFIG_H_
