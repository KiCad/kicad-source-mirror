/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * This file is part of the common library.
 *
 * @file  kidialog.h
 * @see   common.h
 */

#ifndef KIDIALOG_H_
#define KIDIALOG_H_

#include <kicommon.h>
#include <wx/richmsgdlg.h>


/**
 * Helper class to create more flexible dialogs, including 'do not show again' checkbox handling.
 */
class KICOMMON_API KIDIALOG : public wxRichMessageDialog
{
public:
    ///< Dialog type. Selects appropriate icon and default dialog title
    enum KD_TYPE { KD_NONE, KD_INFO, KD_QUESTION, KD_WARNING, KD_ERROR };

    KIDIALOG( wxWindow* aParent, const wxString& aMessage, const wxString& aCaption,
              long aStyle = wxOK );
    KIDIALOG( wxWindow* aParent, const wxString& aMessage, KD_TYPE aType,
              const wxString& aCaption = "" );

    bool SetOKCancelLabels( const ButtonLabel& ok, const ButtonLabel& cancel ) override
    {
        m_cancelMeansCancel = false;
        return wxRichMessageDialog::SetOKCancelLabels( ok, cancel );
    }

    ///< Shows the 'do not show again' checkbox
    void DoNotShowCheckbox( wxString file, int line );

    ///< Checks the 'do not show again' setting for the dialog
    bool DoNotShowAgain() const;
    void ForceShowAgain();

    bool Show( bool aShow = true ) override;
    int ShowModal() override;

protected:
    // Helper functions for wxRichMessageDialog constructor
    static wxString getCaption( KD_TYPE aType, const wxString& aCaption );
    static long getStyle( KD_TYPE aType );

protected:
    unsigned long m_hash;               // Unique id
    bool          m_cancelMeansCancel;  // If the Cancel button is renamed then it should be
                                        // saved by the DoNotShowAgain checkbox.  If it's really
                                        // a cancel then it should not.
};


#endif /* KIDIALOG_H_ */
