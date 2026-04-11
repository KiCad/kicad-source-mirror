/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef SPNAV_VIEWER_PLUGIN_H
#define SPNAV_VIEWER_PLUGIN_H

#include <memory>
#include <wx/timer.h>
#include "common/spacenav/spacenav_driver.h"
#include "common/spacenav/libspnav_driver.h"

class EDA_3D_CANVAS;
class TRACK_BALL;

class SPNAV_VIEWER_PLUGIN : public wxEvtHandler, public SPACEMOUSE_HANDLER
{
public:
    explicit SPNAV_VIEWER_PLUGIN( EDA_3D_CANVAS* aCanvas );
    ~SPNAV_VIEWER_PLUGIN();

    void SetFocus( bool aFocus = true );

    void OnPan( double x, double y, double z ) override;
    void OnRotate( double rx, double ry, double rz ) override;
    void OnButton( int button, bool pressed ) override;

private:
    void onPollTimer( wxTimerEvent& evt );

    std::unique_ptr<SPACENAV_DRIVER> m_driver;
    wxTimer                          m_timer;
    EDA_3D_CANVAS*                   m_canvas;
    TRACK_BALL*                      m_camera;
    bool                             m_focused;
};

#endif // SPNAV_VIEWER_PLUGIN_H