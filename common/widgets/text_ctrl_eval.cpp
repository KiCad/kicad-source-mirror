/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/text_ctrl_eval.h>

TEXT_CTRL_EVAL::TEXT_CTRL_EVAL( wxWindow* aParent, wxWindowID aId, const
        wxString& aValue, const wxPoint& aPos, const wxSize& aSize, long aStyle,
        const wxValidator& aValidator, const wxString& aName )
    : wxTextCtrl( aParent, aId, aValue, aPos, aSize, aStyle, aValidator, aName )
{
    Connect( wxEVT_SET_FOCUS,
            wxFocusEventHandler( TEXT_CTRL_EVAL::onTextFocusGet ), NULL, this );
    Connect( wxEVT_KILL_FOCUS,
            wxFocusEventHandler( TEXT_CTRL_EVAL::onTextFocusLost ), NULL, this );
}


void TEXT_CTRL_EVAL::onTextFocusGet( wxFocusEvent& aEvent )
{
    auto oldStr = m_eval.textInput( this );

    if( oldStr )
        SetValue( wxString::FromUTF8( oldStr ) );

    aEvent.Skip();
}


void TEXT_CTRL_EVAL::onTextFocusLost( wxFocusEvent& aEvent )
{
    if( GetValue().IsEmpty() )
        SetValue( "0" );

    if( m_eval.process( GetValue().mb_str(), this ) )
        SetValue( wxString::FromUTF8( m_eval.result() ) );

    aEvent.Skip();
}
