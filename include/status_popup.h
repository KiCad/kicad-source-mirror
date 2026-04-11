/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __STATUS_POPUP_H_
#define __STATUS_POPUP_H_


#include <math/vector2d.h>
#include <wx/popupwin.h>
#include <wx/timer.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>

class EDA_DRAW_FRAME;

/**
 * A tiny, headerless popup window used to display useful status (e.g. line length
 * tuning info) next to the mouse cursor.
 */
class STATUS_POPUP: public wxPopupWindow
{
public:
    STATUS_POPUP( wxWindow* aParent );
    virtual ~STATUS_POPUP() { Hide(); }

    virtual void Popup( wxWindow* aFocus = nullptr );
    virtual void PopupFor( int aMsecs );
    virtual void Move( const wxPoint& aWhere );
    virtual void Move( const VECTOR2I& aWhere );

    /**
     * Hide the popup after a specified time.
     *
     * @param aMsecs is the time expressed in milliseconds
     */
    void Expire( int aMsecs );

    wxWindow* GetPanel() { return m_panel; }

protected:
    void updateSize();

    void onCharHook( wxKeyEvent& aEvent );

    /// Expire timer even handler.
    void onExpire( wxTimerEvent& aEvent );

protected:
    wxPanel*    m_panel;
    wxBoxSizer* m_topSizer;
    wxTimer     m_expireTimer;
};


/**
 * Extension of #STATUS_POPUP for displaying a single line text.
 */
class STATUS_TEXT_POPUP : public STATUS_POPUP
{
public:
    STATUS_TEXT_POPUP( wxWindow* aParent );
    virtual ~STATUS_TEXT_POPUP() {}

    /**
     * Display a text.
     *
     * @param aText text to be displayed.
     */
    void SetText( const wxString& aText );

    /**
     * Change text color.
     *
     * @param aColor new text color.
     */
    void SetTextColor( const wxColour& aColor );

protected:
    wxStaticText* m_statusLine;
};


#endif /* __STATUS_POPUP_H_*/
