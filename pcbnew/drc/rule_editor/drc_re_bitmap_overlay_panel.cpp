/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>

#include "drc_re_bitmap_overlay_panel.h"
#include "drc_re_overlay_field.h"

#include <bitmaps.h>

#include <wx/bmpbndl.h>
#include <wx/checkbox.h>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/stattext.h>


DRC_RE_BITMAP_OVERLAY_PANEL::DRC_RE_BITMAP_OVERLAY_PANEL( wxWindow* aParent, wxWindowID aId ) :
        wxPanel(),
        m_bitmapId( BITMAPS::INVALID_BITMAP ),
        m_logicalBitmapSize( 0, 0 )
{
    // The illustration has no background of its own. Letting whatever holds it draw through
    // keeps it the same shade as the rest of the page, which matters where the platform
    // composites the window background rather than filling it with a flat colour.
    SetBackgroundStyle( wxBG_STYLE_TRANSPARENT );
    Create( aParent, aId );

    if( !IsTransparentBackgroundSupported() )
        SetBackgroundStyle( wxBG_STYLE_PAINT );

    Bind( wxEVT_PAINT, &DRC_RE_BITMAP_OVERLAY_PANEL::OnPaint, this );
    Bind( wxEVT_DPI_CHANGED, &DRC_RE_BITMAP_OVERLAY_PANEL::OnDPIChanged, this );
    Bind( wxEVT_SYS_COLOUR_CHANGED, &DRC_RE_BITMAP_OVERLAY_PANEL::OnThemeChange, this );
}


DRC_RE_BITMAP_OVERLAY_PANEL::~DRC_RE_BITMAP_OVERLAY_PANEL()
{
}


void DRC_RE_BITMAP_OVERLAY_PANEL::OnPaint( wxPaintEvent& aEvent )
{
    wxPaintDC dc( this );

    if( GetBackgroundStyle() == wxBG_STYLE_PAINT )
    {
        dc.SetBackground( wxBrush( pageBackground() ) );
        dc.Clear();
    }

    if( !m_bitmap.IsOk() )
        return;

    dc.DrawBitmap( m_bitmap, 0, 0, true );
}


void DRC_RE_BITMAP_OVERLAY_PANEL::OnDPIChanged( wxDPIChangedEvent& aEvent )
{
    LoadBitmap();
    PositionFields();
    Refresh();
    aEvent.Skip();
}


wxColour DRC_RE_BITMAP_OVERLAY_PANEL::pageBackground() const
{
    // The illustration sits on the page, so it takes the colour of whatever holds it rather
    // than the default panel one, which is a shade darker on some themes. The colour is read
    // when painting because a parent inherits its own only once the hierarchy is built.
    if( GetParent() )
        return GetParent()->GetBackgroundColour();

    return GetBackgroundColour();
}


void DRC_RE_BITMAP_OVERLAY_PANEL::OnThemeChange( wxSysColourChangedEvent& aEvent )
{
    LoadBitmap();
    Refresh();
    aEvent.Skip();
}


void DRC_RE_BITMAP_OVERLAY_PANEL::OnFieldFocus( wxFocusEvent& aEvent )
{
    // Platform handles focus ring automatically.
    // Could add custom highlighting here if needed.
    aEvent.Skip();
}


void DRC_RE_BITMAP_OVERLAY_PANEL::OnFieldBlur( wxFocusEvent& aEvent )
{
    // Platform handles focus ring automatically.
    aEvent.Skip();
}


void DRC_RE_BITMAP_OVERLAY_PANEL::SetupFieldStyling( wxControl* aControl )
{
    if( !aControl )
        return;

    // Bind focus events to track active field
    aControl->Bind( wxEVT_SET_FOCUS, &DRC_RE_BITMAP_OVERLAY_PANEL::OnFieldFocus, this );
    aControl->Bind( wxEVT_KILL_FOCUS, &DRC_RE_BITMAP_OVERLAY_PANEL::OnFieldBlur, this );
}


void DRC_RE_BITMAP_OVERLAY_PANEL::LoadBitmap()
{
    if( m_bitmapId == BITMAPS::INVALID_BITMAP )
        return;

    wxBitmapBundle bundle = KiBitmapBundle( m_bitmapId );

    m_logicalBitmapSize = FromDIP( bundle.GetDefaultSize() );
    m_bitmap = bundle.GetBitmap( ToPhys( m_logicalBitmapSize ) );

    Refresh();
}


void DRC_RE_BITMAP_OVERLAY_PANEL::SetBackgroundBitmap( BITMAPS aBitmap )
{
    m_bitmapId = aBitmap;
    LoadBitmap();

    if( m_bitmap.IsOk() )
        SetMinSize( m_logicalBitmapSize );
}


void DRC_RE_BITMAP_OVERLAY_PANEL::PositionFields()
{
    for( const auto& field : m_fields )
    {
        const DRC_RE_FIELD_POSITION& pos = field->GetPosition();
        wxControl* ctrl = field->GetControl();

        if( !ctrl )
            continue;

        int heightLogical = ctrl->GetBestSize().GetHeight();

        // The slots were drawn for millimetre values. Inch values need more room, so a field
        // grows up to where its neighbour on the same line begins.
        int widthLogical = std::max( FromDIP( pos.xEnd - pos.xStart ), FromDIP( 96 ) );
        int roomLogical = roomBeforeNextField( field.get() );

        if( roomLogical > 0 )
            widthLogical = std::min( widthLogical, roomLogical );

        widthLogical = std::max( widthLogical, FromDIP( pos.xEnd - pos.xStart ) );

        int xLogical = FromDIP( pos.xStart );
        int yLogical = FromDIP( pos.yCenter ) - heightLogical / 2;

        ctrl->SetPosition( wxPoint( xLogical, yLogical ) );
        ctrl->SetSize( wxSize( widthLogical, heightLogical ) );

        // Position labels if present
        if( field->HasLabel() )
            PositionLabel( field.get() );

        if( field->HasPrefixLabel() )
            PositionPrefixLabel( field.get() );
    }

    // A field can be widened past the right edge of the drawing, and its unit label sits beyond
    // that again, so the panel has to ask for the room it actually uses rather than the room the
    // bitmap takes.  Where the page is wide enough the difference never shows.
    int used = m_logicalBitmapSize.x;

    for( const auto& field : m_fields )
    {
        if( wxControl* ctrl = field->GetControl() )
            used = std::max( used, ctrl->GetPosition().x + ctrl->GetSize().x );

        if( field->HasLabel() )
            used = std::max( used, field->GetLabel()->GetPosition().x + field->GetLabel()->GetSize().x );

        if( field->HasPrefixLabel() )
            used = std::max( used, field->GetPrefixLabel()->GetPosition().x + field->GetPrefixLabel()->GetSize().x );
    }

    SetMinSize( wxSize( used, GetMinSize().y ) );
}


int DRC_RE_BITMAP_OVERLAY_PANEL::roomBeforeNextField( const DRC_RE_OVERLAY_FIELD* aField ) const
{
    const DRC_RE_FIELD_POSITION& pos = aField->GetPosition();
    int                          room = 0;

    // Anything drawn between this field and the next one has to keep its place: this field's
    // own unit label on the right, and the next field's prefix.
    int reserved = FromDIP( 4 );

    if( aField->HasLabel() && pos.labelPosition == LABEL_POSITION::RIGHT )
        reserved += aField->GetLabel()->GetBestSize().GetWidth() + FromDIP( 4 );

    for( const auto& other : m_fields )
    {
        const DRC_RE_FIELD_POSITION& otherPos = other->GetPosition();

        if( other.get() == aField || otherPos.yCenter != pos.yCenter || otherPos.xStart <= pos.xStart )
            continue;

        int limit = FromDIP( otherPos.xStart - pos.xStart ) - reserved;

        if( other->HasPrefixLabel() )
            limit -= other->GetPrefixLabel()->GetBestSize().GetWidth() + FromDIP( 4 );

        if( room == 0 || limit < room )
            room = limit;
    }

    return room;
}


void DRC_RE_BITMAP_OVERLAY_PANEL::PositionLabel( DRC_RE_OVERLAY_FIELD* aField )
{
    wxStaticText* label = aField->GetLabel();
    wxControl* ctrl = aField->GetControl();

    if( !label || !ctrl )
        return;

    const DRC_RE_FIELD_POSITION& pos = aField->GetPosition();
    wxPoint ctrlPos = ctrl->GetPosition();
    wxSize ctrlSize = ctrl->GetSize();
    wxSize labelSize = label->GetBestSize();

    wxPoint labelPos;
    int     GAP = FromDIP( 4 );

    switch( pos.labelPosition )
    {
    case LABEL_POSITION::LEFT:
        labelPos.x = ctrlPos.x - labelSize.GetWidth() - GAP;
        labelPos.y = ctrlPos.y + ( ctrlSize.GetHeight() - labelSize.GetHeight() ) / 2;
        break;

    case LABEL_POSITION::RIGHT:
        labelPos.x = ctrlPos.x + ctrlSize.GetWidth() + GAP;
        labelPos.y = ctrlPos.y + ( ctrlSize.GetHeight() - labelSize.GetHeight() ) / 2;
        break;

    case LABEL_POSITION::TOP:
        labelPos.x = ctrlPos.x + ( ctrlSize.GetWidth() - labelSize.GetWidth() ) / 2;
        labelPos.y = ctrlPos.y - labelSize.GetHeight() - GAP;
        break;

    case LABEL_POSITION::BOTTOM:
        labelPos.x = ctrlPos.x + ( ctrlSize.GetWidth() - labelSize.GetWidth() ) / 2;
        labelPos.y = ctrlPos.y + ctrlSize.GetHeight() + GAP;
        break;

    case LABEL_POSITION::NONE:
    default:
        return;
    }

    label->SetPosition( labelPos );
    label->SetSize( labelSize );
}


void DRC_RE_BITMAP_OVERLAY_PANEL::PositionPrefixLabel( DRC_RE_OVERLAY_FIELD* aField )
{
    wxStaticText* prefix = aField->GetPrefixLabel();
    wxControl*    ctrl = aField->GetControl();

    if( !prefix || !ctrl )
        return;

    wxPoint ctrlPos = ctrl->GetPosition();
    wxSize  ctrlSize = ctrl->GetSize();
    wxSize prefixSize = prefix->GetBestSize();

    int GAP = FromDIP( 4 );

    wxPoint prefixPos;
    prefixPos.x = ctrlPos.x - prefixSize.GetWidth() - GAP;
    prefixPos.y = ctrlPos.y + ( ctrlSize.GetHeight() - prefixSize.GetHeight() ) / 2;

    prefix->SetPosition( prefixPos );
    prefix->SetSize( prefixSize );
}


bool DRC_RE_BITMAP_OVERLAY_PANEL::TransferDataToWindow()
{
    // Base implementation does nothing; derived classes override to populate fields
    return true;
}


bool DRC_RE_BITMAP_OVERLAY_PANEL::TransferDataFromWindow()
{
    // Base implementation does nothing; derived classes override to read fields
    return true;
}


void DRC_RE_BITMAP_OVERLAY_PANEL::ClearFieldErrors()
{
    for( const auto& field : m_fields )
        field->ClearError();
}


void DRC_RE_BITMAP_OVERLAY_PANEL::ShowFieldError( const wxString& aFieldId, const wxString& aMessage )
{
    auto it = m_fieldIdMap.find( aFieldId );

    if( it != m_fieldIdMap.end() )
        it->second->ShowError( aMessage );
}


DRC_RE_OVERLAY_FIELD* DRC_RE_BITMAP_OVERLAY_PANEL::AddCheckbox( const wxString&              aId,
                                                                const DRC_RE_FIELD_POSITION& aPosition )
{
    long style = wxALIGN_CENTER_VERTICAL;

    if( aPosition.labelPosition == LABEL_POSITION::LEFT )
        style |= wxALIGN_RIGHT;
    else
        style |= wxALIGN_LEFT;

    wxCheckBox* checkbox =
            new wxCheckBox( this, wxID_ANY, aPosition.labelText, wxDefaultPosition, wxDefaultSize, style );

    return AddControl( aId, aPosition, checkbox );
}
