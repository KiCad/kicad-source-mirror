/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  EDA_VIEW_SWITCHER_H
#define  EDA_VIEW_SWITCHER_H

#include <eda_view_switcher_base.h>


#ifdef __WXMAC__
    #define PRESET_SWITCH_KEY WXK_RAW_CONTROL
    #define VIEWPORT_SWITCH_KEY WXK_ALT
#else
    #define PRESET_SWITCH_KEY WXK_CONTROL
    #define VIEWPORT_SWITCH_KEY WXK_SHIFT
#endif


class EDA_VIEW_SWITCHER : public EDA_VIEW_SWITCHER_BASE
{
public:
    EDA_VIEW_SWITCHER( wxWindow* aParent, const wxArrayString& aItems, wxKeyCode aCtrlKey );

    int GetSelection() const { return m_listBox->GetSelection(); }

protected:
    bool TryBefore( wxEvent& aEvent ) override;
    bool Show( bool show ) override;

protected:
    bool      m_tabState;
    bool      m_receivingEvents;
    wxKeyCode m_ctrlKey;
};

#endif    // EDA_VIEW_SWITCHER_H
