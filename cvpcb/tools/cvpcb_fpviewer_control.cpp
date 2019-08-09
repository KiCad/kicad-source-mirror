/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2007-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
using namespace std::placeholders;

#include <tool/actions.h>
#include <tools/cvpcb_fpviewer_control.h>


CVPCB_FOOTPRINT_VIEWER_CONTROL::CVPCB_FOOTPRINT_VIEWER_CONTROL() :
        TOOL_INTERACTIVE( "cvpcb.FootprintViewerControl" ),
        m_frame( nullptr )
{
}


void CVPCB_FOOTPRINT_VIEWER_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<DISPLAY_FOOTPRINTS_FRAME>();
}


int CVPCB_FOOTPRINT_VIEWER_CONTROL::Show3DViewer( const TOOL_EVENT& aEvent )
{
    m_frame->CreateAndShow3D_Frame();
    return 0;
}


void CVPCB_FOOTPRINT_VIEWER_CONTROL::setTransitions()
{
    Go( &CVPCB_FOOTPRINT_VIEWER_CONTROL::Show3DViewer, ACTIONS::show3DViewer.MakeEvent() );
}
