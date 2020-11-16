/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_paste_special.h>


static bool g_keepAnnotations = true;


DIALOG_PASTE_SPECIAL::DIALOG_PASTE_SPECIAL( wxWindow* parent, bool* aKeep ) :
    DIALOG_PASTE_SPECIAL_BASE( parent ),
    m_keep( aKeep )
{
    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_PASTE_SPECIAL::TransferDataToWindow()
{
    m_keepAnnotations->SetValue( g_keepAnnotations );
    return true;
}


bool DIALOG_PASTE_SPECIAL::TransferDataFromWindow()
{
    g_keepAnnotations = *m_keep = m_keepAnnotations->GetValue();
    return true;
}
