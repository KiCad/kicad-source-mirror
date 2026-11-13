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
#include "panel_drc_rule_editor.h"

#include <bitmaps.h>

#include <wx/bmpbndl.h>
#include <wx/checkbox.h>
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>


/// Space between a field and the label beside it, in DIP. The positioning code places labels at
/// this distance and roomBeforeNextField() has to reserve the same.
static constexpr int LABEL_GAP = 4;


DRC_RE_BITMAP_OVERLAY_PANEL::DRC_RE_BITMAP_OVERLAY_PANEL( wxWindow* aParent, wxWindowID aId ) :
        wxPanel(),
        m_bitmapId( BITMAPS::INVALID_BITMAP ),
        m_logicalBitmapSize( 0, 0 )
{
    // The illustration has no background of its own, so let the page behind it show through.
    // Where the port cannot, the paint handler erases it with a buffered DC that needs wxBG_STYLE_PAINT
    m_erasesBackground = !aParent->IsTransparentBackgroundSupported();
    SetBackgroundStyle( m_erasesBackground ? wxBG_STYLE_PAINT : wxBG_STYLE_TRANSPARENT );
    Create( aParent, aId );

    Bind( wxEVT_PAINT, &DRC_RE_BITMAP_OVERLAY_PANEL::OnPaint, this );
    Bind( wxEVT_DPI_CHANGED, &DRC_RE_BITMAP_OVERLAY_PANEL::OnDPIChanged, this );
    Bind( wxEVT_SYS_COLOUR_CHANGED, &DRC_RE_BITMAP_OVERLAY_PANEL::OnThemeChange, this );
}


DRC_RE_BITMAP_OVERLAY_PANEL::~DRC_RE_BITMAP_OVERLAY_PANEL()
{
}


void DRC_RE_BITMAP_OVERLAY_PANEL::OnPaint( wxPaintEvent& aEvent )
{
    if( m_erasesBackground )
    {
        wxAutoBufferedPaintDC dc( this );

        dc.SetBackground( wxBrush( pageBackground() ) );
        dc.Clear();
        drawBitmap( dc );
    }
    else
    {
        wxPaintDC dc( this );

        drawBitmap( dc );
    }
}


void DRC_RE_BITMAP_OVERLAY_PANEL::drawBitmap( wxDC& aDC )
{
    if( m_bitmap.IsOk() )
        aDC.DrawBitmap( m_bitmap, 0, 0, true );
}


void DRC_RE_BITMAP_OVERLAY_PANEL::OnDPIChanged( wxDPIChangedEvent& aEvent )
{
    LoadBitmap();
    PositionFields();

    // Positioning the fields changed the panel's minimum size, which only reaches the screen
    // once the sizers above it run again.
    if( PANEL_DRC_RULE_EDITOR* editor = PANEL_DRC_RULE_EDITOR::FindEditor( this ) )
        editor->RelayoutHost();
    else if( wxWindow* parent = GetParent() )
        parent->Layout();

    Refresh();
    aEvent.Skip();
}


wxColour DRC_RE_BITMAP_OVERLAY_PANEL::pageBackground() const
{
    // Take the colour of whatever holds the illustration, not the default panel colour, which
    // is darker on some themes. Read it when painting, because a parent only inherits its own
    // once the hierarchy is built.
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

        // The slots were drawn for millimetre values. Give a longer one the room its text
        // needs, up to where the next field on the line begins.
        int slotLogical = FromDIP( pos.xEnd - pos.xStart );
        int widthLogical = slotLogical;

        if( wxTextCtrl* text = dynamic_cast<wxTextCtrl*>( ctrl ) )
        {
            int textWidth = 0;
            int textHeight = 0;

            text->GetTextExtent( text->GetValue(), &textWidth, &textHeight );
            widthLogical = std::max( widthLogical, textWidth + FromDIP( 24 ) );
        }

        // Where there is a neighbour the field stops short of it, and it never goes below the
        // slot the drawing was made for, however little room the neighbour leaves.
        if( std::optional<int> roomLogical = roomBeforeNextField( field.get() ) )
            widthLogical = std::min( widthLogical, std::max( *roomLogical, slotLogical ) );

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

    // A field, and the unit label after it, can reach past the right edge of the drawing, so
    // ask for the room they use rather than the room the bitmap takes.
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

    int height = m_logicalBitmapSize.y;

    if( m_minHeightDIP > 0 )
        height = std::max( height, FromDIP( m_minHeightDIP ) );

    SetMinSize( wxSize( used, height ) );
}


std::optional<int> DRC_RE_BITMAP_OVERLAY_PANEL::roomBeforeNextField( const DRC_RE_OVERLAY_FIELD* aField ) const
{
    const DRC_RE_FIELD_POSITION& pos = aField->GetPosition();
    std::optional<int>           room;

    // Leave room for what is drawn between the two fields: this field's unit label and the
    // next field's prefix.
    int reserved = FromDIP( LABEL_GAP );

    if( aField->HasLabel() && pos.labelPosition == LABEL_POSITION::RIGHT )
        reserved += aField->GetLabel()->GetBestSize().GetWidth() + FromDIP( LABEL_GAP );

    for( const auto& other : m_fields )
    {
        const DRC_RE_FIELD_POSITION& otherPos = other->GetPosition();

        if( other.get() == aField || otherPos.yCenter != pos.yCenter || otherPos.xStart <= pos.xStart )
            continue;

        int limit = FromDIP( otherPos.xStart - pos.xStart ) - reserved;

        if( other->HasPrefixLabel() )
            limit -= other->GetPrefixLabel()->GetBestSize().GetWidth() + FromDIP( LABEL_GAP );

        if( !room || limit < *room )
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
    int     GAP = FromDIP( LABEL_GAP );

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

    int GAP = FromDIP( LABEL_GAP );

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
