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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "drc_re_bitmap_overlay_panel.h"
#include "drc_re_overlay_field.h"

#include <bitmaps.h>

#include <wx/bmpbndl.h>
#include <wx/checkbox.h>
#include <wx/dcclient.h>
#include <wx/stattext.h>


DRC_RE_BITMAP_OVERLAY_PANEL::DRC_RE_BITMAP_OVERLAY_PANEL( wxWindow* aParent, wxWindowID aId ) :
        wxPanel(),
        m_bitmapId( BITMAPS::INVALID_BITMAP ),
        m_baseBitmapSize( 0, 0 )
{
    // Must set background style BEFORE creating the window
    SetBackgroundStyle( wxBG_STYLE_PAINT );
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
    wxPaintDC dc( this );

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

    m_bitmap = bundle.GetBitmapFor( this );
    m_baseBitmapSize = bundle.GetDefaultSize();

    Refresh();
}


void DRC_RE_BITMAP_OVERLAY_PANEL::SetBackgroundBitmap( BITMAPS aBitmap )
{
    m_bitmapId = aBitmap;
    LoadBitmap();

    if( m_bitmap.IsOk() )
        SetMinSize( m_baseBitmapSize );
}


void DRC_RE_BITMAP_OVERLAY_PANEL::PositionFields()
{
    for( const auto& field : m_fields )
    {
        const DRC_RE_FIELD_POSITION& pos = field->GetPosition();
        wxControl* ctrl = field->GetControl();

        if( !ctrl )
            continue;

        // Calculate scaled position and size
        wxPoint scaledPos( pos.xStart, pos.yTop );
        int width = pos.xEnd - pos.xStart;
        wxSize scaledSize( width, ctrl->GetBestSize().GetHeight() );

        ctrl->SetPosition( scaledPos );
        ctrl->SetSize( scaledSize );

        // Position label if present
        if( field->HasLabel() )
            PositionLabel( field.get() );
    }
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
    constexpr int GAP = 4;

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
        field->ShowError( false );
}


void DRC_RE_BITMAP_OVERLAY_PANEL::ShowFieldError( const wxString& aFieldId )
{
    auto it = m_fieldIdMap.find( aFieldId );

    if( it != m_fieldIdMap.end() )
        it->second->ShowError( true );
}


DRC_RE_OVERLAY_FIELD* DRC_RE_BITMAP_OVERLAY_PANEL::AddCheckbox( const wxString& aId,
                                                                 const DRC_RE_FIELD_POSITION& aPosition )
{
    wxCheckBox* checkbox = new wxCheckBox( this, wxID_ANY, wxEmptyString );

    auto field = std::make_unique<DRC_RE_OVERLAY_FIELD>( this, aId, checkbox, aPosition );
    DRC_RE_OVERLAY_FIELD* fieldPtr = field.get();

    SetupFieldStyling( checkbox );

    wxPoint pos( aPosition.xStart, aPosition.yTop );
    checkbox->SetPosition( pos );

    // Create label if specified
    fieldPtr->CreateLabel();

    if( fieldPtr->HasLabel() )
        PositionLabel( fieldPtr );

    m_fieldIdMap[aId] = fieldPtr;
    m_fields.push_back( std::move( field ) );

    return fieldPtr;
}
