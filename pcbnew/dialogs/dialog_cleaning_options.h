/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#ifndef DIALOG_CLEANING_OPTIONS_H_
#define DIALOG_CLEANING_OPTIONS_H_

#include <dialog_cleaning_options_base.h>

class DIALOG_CLEANING_OPTIONS: public DIALOG_CLEANING_OPTIONS_BASE
{
public:
    static bool m_cleanVias;
    static bool m_mergeSegments;
    static bool m_deleteUnconnectedSegm;

public:
    DIALOG_CLEANING_OPTIONS( wxWindow* parent );

    ~DIALOG_CLEANING_OPTIONS()
    {
        GetOpts( );
    }

private:
        void OnCancelClick( wxCommandEvent& event )
        {
            EndModal( wxID_CANCEL );
        }
        void OnOKClick( wxCommandEvent& event )
        {
            GetOpts( );
            EndModal( wxID_OK );
        }

        void GetOpts( )
        {
            m_cleanVias = m_cleanViasOpt->GetValue( );
            m_mergeSegments = m_mergeSegmOpt->GetValue( );
            m_deleteUnconnectedSegm = m_deleteUnconnectedOpt->GetValue( );
        }
};

#endif
    // DIALOG_CLEANING_OPTIONS_H_
