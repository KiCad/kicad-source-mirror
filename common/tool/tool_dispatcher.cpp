/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <wx/wx.h>
#include <wx/event.h>

#include <wxPcbStruct.h>
#include <wxBasePcbFrame.h>

#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <view/view.h>

#include <class_drawpanel_gal.h>

#include <pcbnew_id.h>

#include <boost/optional.hpp>
#include <boost/foreach.hpp>

using boost::optional;

struct TOOL_DISPATCHER::ButtonState
{
	ButtonState( TOOL_MouseButtons aButton, const wxEventType& aDownEvent,
	             const wxEventType& aUpEvent, bool aTriggerMenu = false ) :
		button( aButton ),
		downEvent( aDownEvent ),
		upEvent( aUpEvent ),
		triggerContextMenu( aTriggerMenu )
	{};

	bool dragging;
	bool pressed;
	
	VECTOR2D dragOrigin;
	double dragMaxDelta;
	
	TOOL_MouseButtons button;
	wxEventType downEvent;
	wxEventType upEvent;
	bool triggerContextMenu;
	
	wxLongLong downTimestamp;

	void Reset()
	{
		dragging = false;
		pressed = false;
	}
};


TOOL_DISPATCHER::TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr, PCB_BASE_FRAME* aEditFrame ) :
	m_toolMgr( aToolMgr ), m_editFrame( aEditFrame )
{
    m_buttons.push_back( new ButtonState( MB_Left, wxEVT_LEFT_DOWN, wxEVT_LEFT_UP ) );
    m_buttons.push_back( new ButtonState( MB_Right, wxEVT_RIGHT_DOWN, wxEVT_RIGHT_UP, true ) );
    m_buttons.push_back( new ButtonState( MB_Middle, wxEVT_MIDDLE_DOWN, wxEVT_MIDDLE_UP ) );

    ResetState();
}


TOOL_DISPATCHER::~TOOL_DISPATCHER()
{
	BOOST_FOREACH( ButtonState* st, m_buttons )
		delete st;
}


void TOOL_DISPATCHER::ResetState()
{
	BOOST_FOREACH( ButtonState* st, m_buttons )
		st->Reset();
}


KiGfx::VIEW* TOOL_DISPATCHER::getView()
{
	return m_editFrame->GetGalCanvas()->GetView();
}


int TOOL_DISPATCHER::decodeModifiers( const wxKeyboardState* aState ) const
{
    int mods = 0;

    if( aState->ControlDown() )
        mods |= MD_ModCtrl;
    if( aState->AltDown() )
        mods |= MD_ModAlt;
    if( aState->ShiftDown() )
        mods |= MD_ModShift;

    return mods;
}


bool TOOL_DISPATCHER::handleMouseButton( wxEvent& aEvent, int aIndex, bool aMotion )
{
	ButtonState* st = m_buttons[aIndex];
	wxEventType type = aEvent.GetEventType();
	optional<TOOL_EVENT> evt;

	bool up = type == st->upEvent;
	bool down = type == st->downEvent;
	
	int mods = decodeModifiers( static_cast<wxMouseEvent*>( &aEvent ) );
	int args = st->button | mods;

	if( down )
	{
		st->downTimestamp = wxGetLocalTimeMillis();
		st->dragOrigin = m_lastMousePos;
		st->dragMaxDelta = 0;
		st->pressed = true;
		evt = TOOL_EVENT( TC_Mouse, TA_MouseDown, args );
	}
	else if( up )
	{
		bool isClick = false;
		st->pressed = false;

		if( st->dragging )
		{
			wxLongLong t = wxGetLocalTimeMillis();

			if( t - st->downTimestamp < DragTimeThreshold &&
			        st->dragMaxDelta < DragDistanceThreshold )
				isClick = true;
			else
				evt = TOOL_EVENT( TC_Mouse, TA_MouseUp, args );
		}
		else
			isClick = true;

		
		if( isClick )
		{
			if( st->triggerContextMenu && !mods )
			{}
			//	evt = TOOL_EVENT( TC_Command, TA_ContextMenu );
			else
				evt = TOOL_EVENT( TC_Mouse, TA_MouseClick, args );
		}

		st->dragging = false;
	}

	if( st->pressed && aMotion )
	{
		st->dragging = true;
		double dragPixelDistance = getView()->ToScreen( m_lastMousePos - st->dragOrigin, false ).EuclideanNorm();
		st->dragMaxDelta = std::max( st->dragMaxDelta, dragPixelDistance );

		wxLongLong t = wxGetLocalTimeMillis();

		if( t - st->downTimestamp > DragTimeThreshold || st->dragMaxDelta > DragDistanceThreshold )
		{			
			evt = TOOL_EVENT( TC_Mouse, TA_MouseDrag, args );
			evt->SetMouseDragOrigin( st->dragOrigin );
			evt->SetMouseDelta( m_lastMousePos - st->dragOrigin );
		}
	}

	if( evt )
	{
		evt->SetMousePosition( m_lastMousePos );
		m_toolMgr->ProcessEvent( *evt );

		return true;
	}

	return false;
}


void TOOL_DISPATCHER::DispatchWxEvent( wxEvent& aEvent )
{
	bool motion = false, buttonEvents = false;
	VECTOR2D pos;
	optional<TOOL_EVENT> evt;
	
	int type = aEvent.GetEventType();

	// Mouse handling
	if( type == wxEVT_MOTION || type == wxEVT_MOUSEWHEEL ||
	        type == wxEVT_LEFT_DOWN || type == wxEVT_LEFT_UP ||
	        type == wxEVT_MIDDLE_DOWN || type == wxEVT_MIDDLE_UP ||
	        type == wxEVT_RIGHT_DOWN || type == wxEVT_RIGHT_UP )
	{
		wxMouseEvent* me = static_cast<wxMouseEvent*>( &aEvent );
		pos = getView()->ToWorld( VECTOR2D( me->GetX(), me->GetY() ) );
		if( pos != m_lastMousePos )
		{
			motion = true;
			m_lastMousePos = pos;
		}

	    for( unsigned int i = 0; i < m_buttons.size(); i++ )
	        buttonEvents |= handleMouseButton( aEvent, i, motion );

	    if( !buttonEvents && motion )
	    {
	        evt = TOOL_EVENT( TC_Mouse, TA_MouseMotion );
	        evt->SetMousePosition( pos );
	    }
	}

	// Keyboard handling
	else if( type == wxEVT_KEY_UP || type == wxEVT_KEY_DOWN )
    {
        wxKeyEvent* ke = static_cast<wxKeyEvent*>( &aEvent );
        int key = ke->GetKeyCode();
        int mods = decodeModifiers( ke );

        if( type == wxEVT_KEY_UP )
        {
            if( key == WXK_ESCAPE )
                evt = TOOL_EVENT( TC_Command, TA_CancelTool );
            else
                evt = TOOL_EVENT( TC_Keyboard, TA_KeyUp, key | mods );
        }
        else
        {
            evt = TOOL_EVENT( TC_Keyboard, TA_KeyDown, key | mods );
        }
    }

	if( evt )
		m_toolMgr->ProcessEvent( *evt );

	aEvent.Skip();
}


void TOOL_DISPATCHER::DispatchWxCommand( wxCommandEvent &aEvent )
{
	bool activateTool = false;
	std::string toolName;
	
	switch( aEvent.GetId() )
	{
		case ID_SELECTION_TOOL:
			toolName = "pcbnew.InteractiveSelection";
			activateTool = true;
			break;
	}

	if( activateTool && m_editFrame->IsGalCanvasActive() )
		m_toolMgr->InvokeTool( toolName );
}
