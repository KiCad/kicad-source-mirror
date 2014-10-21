/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2001 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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


#ifndef _DIALOG_LIB_EDIT_TEXT_H_
#define _DIALOG_LIB_EDIT_TEXT_H_


#include <dialog_lib_edit_text_base.h>


class LIB_EDIT_FRAME;
class LIB_TEXT;


class DIALOG_LIB_EDIT_TEXT : public DIALOG_LIB_EDIT_TEXT_BASE
{
private:
    LIB_EDIT_FRAME* m_parent;
    LIB_TEXT* m_graphicText;

public:
    DIALOG_LIB_EDIT_TEXT( LIB_EDIT_FRAME* aParent, LIB_TEXT* aText );
    ~DIALOG_LIB_EDIT_TEXT() {};

private:
    void initDlg( );
    void OnOkClick( wxCommandEvent& aEvent );
    void OnCancelClick( wxCommandEvent& aEvent );
};


#endif    // _DIALOG_LIB_EDIT_TEXT_H_
