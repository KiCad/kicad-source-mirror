/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2009 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _ANNOTATE_DIALOG_H_
#define _ANNOTATE_DIALOG_H_

#include "dialog_annotate_base.h"


class WinEDA_SchematicFrame;
class wxConfig;


/*!
 * DIALOG_ANNOTATE class declaration
 */

class DIALOG_ANNOTATE: public DIALOG_ANNOTATE_BASE
{
private:
    WinEDA_SchematicFrame * m_Parent;
    wxConfig* m_Config;

public:
    DIALOG_ANNOTATE( WinEDA_SchematicFrame* parent );
    ~DIALOG_ANNOTATE(){};

    /// Initialises member variables
    void InitValues();
    void OnCancelClick( wxCommandEvent& event );
    void OnClearAnnotationCmpClick( wxCommandEvent& event );
    void OnApplyClick( wxCommandEvent& event );

    // User functions:
    bool GetLevel( void );
    bool GetResetItems( void );
    int GetSortOrder( void );

};

#endif
    // _ANNOTATE_DIALOG_H_
