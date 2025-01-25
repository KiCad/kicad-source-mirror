/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#ifndef KISTATUSBAR_H
#define KISTATUSBAR_H

#include <kicommon.h>
#include <optional>

class wxGauge;
class wxButton;
class wxStaticText;
class BITMAP_BUTTON;

/**
 * KISTATUSBAR is a wxStatusBar suitable for Kicad manager.
 * It displays the fields needed by the caller, and room for 4 other fields (see kistatusbar.cpp)
 * Background text (FIELD_OFFSET_BGJOB_TEXT offset id)
 * Background gauge widget (FIELD_OFFSET_BGJOB_GAUGE offset id)
 * Background background stop button (FIELD_OFFSET_BGJOB_CANCEL offset id)
 * Background notifications button (FIELD_OFFSET_NOTIFICATION_BUTTON  offset id)
 */

class KICOMMON_API KISTATUSBAR : public wxStatusBar
{
public:
    enum STYLE_FLAGS : int
    {
        NONE              = 0x00,
        NOTIFICATION_ICON = 0x01,
        CANCEL_BUTTON     = 0x02,
    };

    static constexpr auto DEFAULT_STYLE =
            static_cast<STYLE_FLAGS>( NOTIFICATION_ICON | CANCEL_BUTTON );

    KISTATUSBAR( int aNumberFields, wxWindow* parent, wxWindowID id,
                 STYLE_FLAGS aFlags = DEFAULT_STYLE );

    ~KISTATUSBAR();

    /**
     * Set the text in a field using wxELLIPSIZE_MIDDLE option to adjust the text size
     * to the field size.
     *
     * @note Unfortunately, setting the wxStatusBar style to wxELLIPSIZE_MIDDLE does not work.
     */
    void SetEllipsedTextField( const wxString& aText, int aFieldId );

    /**
     * Show the background progress bar.
     */
    void ShowBackgroundProgressBar( bool aCancellable = false );

    /**
     * Hide the background progress bar.
     */
    void HideBackgroundProgressBar();

    /**
     * Set the current progress of the progress bar.
     */
    void SetBackgroundProgress( int aAmount );

    /**
     * Set the max progress of the progress bar.
     */
    void SetBackgroundProgressMax( int aAmount );

    /**
     * Set the status text that displays next to the progress bar.
     */
    void SetBackgroundStatusText( const wxString& aTxt );

    /**
     * Set the notification count on the notifications button.
     *
     * A value of 0 will hide the count.
     */
    void SetNotificationCount( int aCount );

private:
    void onSize( wxSizeEvent& aEvent );
    void onBackgroundProgressClick( wxMouseEvent& aEvent );
    void onNotificationsIconClick( wxCommandEvent& aEvent );

    enum class FIELD
    {
        BGJOB_LABEL,
        BGJOB_GAUGE,
        BGJOB_CANCEL,
        NOTIFICATION
    };

    std::optional<int> fieldIndex( FIELD aField ) const;

private:
    wxGauge*       m_backgroundProgressBar;
    wxButton*      m_backgroundStopButton;
    wxStaticText*  m_backgroundTxt;
    BITMAP_BUTTON* m_notificationsButton;
    int            m_normalFieldsCount;
    STYLE_FLAGS    m_styleFlags;
};

#endif
