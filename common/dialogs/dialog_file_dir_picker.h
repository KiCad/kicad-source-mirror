/*
 * Copyright (C) 2018 CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// inspired by David Hart's FileDirDlg widget (4Pane project)

#ifndef __DIALOG_FILE_DIR_PICKER_H__
#define __DIALOG_FILE_DIR_PICKER_H__

#include <dialog_shim.h>
#include "wx/dirctrl.h"


enum FILE_DIR_PICKER_STYLE
{
    FD_MULTIPLE         = 0x0001,
    FD_SHOW_HIDDEN      = 0x0002,
    FD_RETURN_FILESONLY = 0x0004
};

/**
 * @brief Dialog that can select both files and directories.
 */
class DIALOG_FILE_DIR_PICKER : public DIALOG_SHIM
{
public:
    DIALOG_FILE_DIR_PICKER( wxWindow* parent, const wxString& message, const wxString& defaultPath,
            const wxString& wildcard, int style = FD_MULTIPLE | FD_SHOW_HIDDEN );

    void SetDirectory( const wxString& aDirectory ) const;

    wxString GetDirectory() const;

    /**
     * Sets the wildcard filter.
     * @param aFilter is the new filter
     */
    void SetFilter( const wxString& aFilter )
    {
        m_GDC->SetFilter( aFilter );
    }

    // Get multiple filepaths. Returns number found
    size_t GetFilenames( wxArrayString& aFilepaths );

protected:
    void onHidden( wxCommandEvent& event );

    bool m_filesOnly;
    wxGenericDirCtrl* m_GDC;
    wxCheckBox* m_showHidden;

};

#endif //__DIALOG_FILE_DIR_PICKER_H__
