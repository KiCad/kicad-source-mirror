/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_export_3d_image.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/statline.h>
#include <wx/gbsizer.h>

#include <bitmaps.h>
#include <widgets/bitmap_button.h>


DIALOG_EXPORT_3D_IMAGE::DIALOG_EXPORT_3D_IMAGE( wxWindow* aParent, const wxSize& aCanvasSize,
                                                EDA_3D_VIEWER_SETTINGS::EXPORT_IMAGE_SETTINGS* aCfg ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Export 3D View" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE ),
        m_cfg( aCfg ),
        m_originalSize( aCanvasSize ),
        m_width( aCanvasSize.GetWidth() ),
        m_height( aCanvasSize.GetHeight() ),
        m_xResolution( 300.0 ),
        m_yResolution( 300.0 ),
        m_lockAspectRatio( true ),
        m_sizeUnits( SIZE_UNITS::PIXELS ),
        m_resolutionUnits( RESOLUTION_UNITS::PIXELS_PER_INCH )
{
    m_aspectRatio = static_cast<double>(m_width) / static_cast<double>(m_height);

    // State is persisted through EXPORT_IMAGE_SETTINGS, so opt out of the generic
    // control-state save and restore.
    OptOut( this );

    if( aCfg )
    {
        if( aCfg->width )
            m_width = aCfg->width;

        if( aCfg->height )
            m_height = aCfg->height;

        m_xResolution = aCfg->x_resolution;
        m_yResolution = aCfg->y_resolution;
        m_lockAspectRatio = aCfg->lock_aspect_ratio;
        m_sizeUnits = static_cast<SIZE_UNITS>( aCfg->size_units );
        m_resolutionUnits = static_cast<RESOLUTION_UNITS>( aCfg->resolution_units );
    }

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // Image Size section
    wxStaticText* imageSizeHeader = new wxStaticText( this, wxID_ANY, _( "Image Size" ) );
    imageSizeHeader->SetFont( imageSizeHeader->GetFont().Bold() );
    mainSizer->Add( imageSizeHeader, 0, wxLEFT | wxTOP, 10 );

    wxGridBagSizer* sizeGrid = new wxGridBagSizer( 5, 2 );
    sizeGrid->SetFlexibleDirection( wxHORIZONTAL );
    sizeGrid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    // Width row
    sizeGrid->Add( new wxStaticText( this, wxID_ANY, _( "Width:" ) ), {0, 0}, {1, 1}, wxALIGN_CENTER_VERTICAL );
    m_spinWidth = new wxSpinCtrlDouble( this, wxID_ANY );
    m_spinWidth->SetRange( 1, 50000 );
    m_spinWidth->SetDigits( 0 );
    sizeGrid->Add( m_spinWidth, {0, 1}, {1, 1}, wxALIGN_CENTER_VERTICAL );

    // Lock button - will span 2 rows
    m_lockButton = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap );
    m_lockButton->SetBitmap( KiBitmapBundle( BITMAPS::link ) );
	m_lockButton->SetToolTip( _( "Lock/unlock aspect ratio" ) );
    m_lockButton->SetIsCheckButton();
    sizeGrid->Add( m_lockButton, {0, 2}, {2, 1}, wxALIGN_CENTER | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 10 );

    // Size units choice
    m_choiceSizeUnits = new wxChoice( this, wxID_ANY );
    m_choiceSizeUnits->Append( _( "pixels" ) );
    m_choiceSizeUnits->Append( _( "%" ) );
    m_choiceSizeUnits->Append( _( "mm" ) );
    m_choiceSizeUnits->Append( _( "in" ) );
    m_choiceSizeUnits->SetSelection( static_cast<int>( m_sizeUnits ) );
    m_choiceSizeUnits->SetMinSize( wxSize( 100, -1 ) );     // Try to keep two units choice boxes the same size
    sizeGrid->Add( m_choiceSizeUnits, {0, 4}, {2, 1}, wxALIGN_CENTER_VERTICAL | wxLEFT, 15 );

    // Height row
    sizeGrid->Add( new wxStaticText( this, wxID_ANY, _( "Height:" ) ), {1, 0}, {1, 1}, wxALIGN_CENTER_VERTICAL );
    m_spinHeight = new wxSpinCtrlDouble( this, wxID_ANY );
    m_spinHeight->SetRange( 1, 50000 );
    m_spinHeight->SetDigits( 0 );
    sizeGrid->Add( m_spinHeight, {1, 1}, {1, 1}, wxALIGN_CENTER_VERTICAL );

    mainSizer->Add( sizeGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10 );

    // Pixel size display
    m_pixelSizeLabel = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_pixelSizeLabel->SetFont( m_pixelSizeLabel->GetFont().Smaller() );
    mainSizer->Add( m_pixelSizeLabel, 0, wxLEFT | wxTOP, 10 );

    // Separator
    mainSizer->AddSpacer( 15 );
    wxStaticLine* line = new wxStaticLine( this );
    mainSizer->Add( line, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );

    // Resolution section
    wxStaticText* resolutionHeader = new wxStaticText( this, wxID_ANY, _( "Resolution" ) );
    resolutionHeader->SetFont( resolutionHeader->GetFont().Bold() );
    mainSizer->Add( resolutionHeader, 0, wxLEFT | wxTOP, 10 );

    wxGridBagSizer* resGrid = new wxGridBagSizer( 5, 2 );
    resGrid->SetFlexibleDirection( wxHORIZONTAL );
    resGrid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    // X Resolution row
    resGrid->Add( new wxStaticText( this, wxID_ANY, _( "X resolution:" ) ), {0, 0}, {1, 1}, wxALIGN_CENTER_VERTICAL );
    m_spinXResolution = new wxSpinCtrlDouble( this, wxID_ANY );
    m_spinXResolution->SetRange( 1, 10000 );
    m_spinXResolution->SetDigits( 3 );
    resGrid->Add( m_spinXResolution, {0, 1}, {1, 1}, wxEXPAND );

    // Resolution units choice
    m_choiceResolutionUnits = new wxChoice( this, wxID_ANY );
    m_choiceResolutionUnits->Append( _( "pixels/in" ) );
    m_choiceResolutionUnits->Append( _( "pixels/mm" ) );
    m_choiceResolutionUnits->SetMinSize( wxSize( 100, -1 ) );     // Try to keep two units choice boxes the same size
    m_choiceResolutionUnits->SetSelection( static_cast<int>( m_resolutionUnits ) );
    resGrid->Add( m_choiceResolutionUnits, {0, 2}, {2, 1}, wxALIGN_CENTER_VERTICAL | wxLEFT, 15 );

    // Y Resolution row
    resGrid->Add( new wxStaticText( this, wxID_ANY, _( "Y resolution:" ) ), {1, 0}, {1, 1}, wxALIGN_CENTER_VERTICAL );
    m_spinYResolution = new wxSpinCtrlDouble( this, wxID_ANY );
    m_spinYResolution->SetRange( 1, 10000 );
    m_spinYResolution->SetDigits( 3 );
    resGrid->Add( m_spinYResolution, {1, 1}, {1, 1}, wxEXPAND );

    mainSizer->Add( resGrid, 0, wxEXPAND | wxALL, 10 );
    mainSizer->AddSpacer( 5 );

    // Dialog buttons
    wxStdDialogButtonSizer* btnSizer = CreateStdDialogButtonSizer( wxOK | wxCANCEL );
    mainSizer->Add( btnSizer, 0, wxEXPAND | wxALL, 5 );

    SetSizerAndFit( mainSizer );
    Centre();

    // Set initial values AFTER creating all controls
    m_spinWidth->SetValue( m_width );
    m_spinHeight->SetValue( m_height );
    m_lockButton->Check( m_lockAspectRatio );
    m_spinXResolution->SetValue( m_xResolution );
    m_spinYResolution->SetValue( m_yResolution );
    UpdatePixelSize();

    // Bind events AFTER setting initial values
    m_lockButton->Bind( wxEVT_BUTTON, &DIALOG_EXPORT_3D_IMAGE::OnLockToggle, this );
    m_spinWidth->Bind( wxEVT_SPINCTRLDOUBLE, &DIALOG_EXPORT_3D_IMAGE::OnWidthChange, this );
    m_spinHeight->Bind( wxEVT_SPINCTRLDOUBLE, &DIALOG_EXPORT_3D_IMAGE::OnHeightChange, this );
    m_spinXResolution->Bind( wxEVT_SPINCTRLDOUBLE, &DIALOG_EXPORT_3D_IMAGE::OnXResolutionChange, this );
    m_spinYResolution->Bind( wxEVT_SPINCTRLDOUBLE, &DIALOG_EXPORT_3D_IMAGE::OnYResolutionChange, this );
    m_choiceSizeUnits->Bind( wxEVT_CHOICE, &DIALOG_EXPORT_3D_IMAGE::OnSizeUnitChange, this );
    m_choiceResolutionUnits->Bind( wxEVT_CHOICE, &DIALOG_EXPORT_3D_IMAGE::OnResolutionUnitChange, this );
}


bool DIALOG_EXPORT_3D_IMAGE::TransferDataFromWindow()
{
    double width = m_spinWidth->GetValue();
    double height = m_spinHeight->GetValue();
    double xRes = m_spinXResolution->GetValue();
    double yRes = m_spinYResolution->GetValue();

    if( m_resolutionUnits == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        xRes *= 25.4;
        yRes *= 25.4;
    }

    wxSize pixelSize = GetPixelSize( width, height, xRes, yRes, m_sizeUnits );
    m_width = pixelSize.GetWidth();
    m_height = pixelSize.GetHeight();

    m_xResolution = m_spinXResolution->GetValue();
    m_yResolution = m_spinYResolution->GetValue();

    if( m_cfg )
    {
        m_cfg->width = m_width;
        m_cfg->height = m_height;
        m_cfg->x_resolution = m_xResolution;
        m_cfg->y_resolution = m_yResolution;
        m_cfg->size_units = static_cast<int>( m_sizeUnits );
        m_cfg->resolution_units = static_cast<int>( m_resolutionUnits );
        m_cfg->lock_aspect_ratio = m_lockAspectRatio;
    }

    return true;
}


void DIALOG_EXPORT_3D_IMAGE::OnLockToggle( wxCommandEvent& aEvent )
{
    m_lockAspectRatio = !m_lockAspectRatio;

    if( m_lockAspectRatio )
        UpdateAspectRatio();
}


void DIALOG_EXPORT_3D_IMAGE::OnWidthChange( wxSpinDoubleEvent& aEvent )
{
    if( m_lockAspectRatio )
    {
        double width = m_spinWidth->GetValue();
        double height;

        if( m_sizeUnits == SIZE_UNITS::PERCENT )
            height = m_originalSize.GetWidth() * width / 100.0 / m_aspectRatio;
        else
            height = width / m_aspectRatio;

        // Ensure height is not less than 1
        if( height < 1 )
            height = 1;

        m_spinHeight->SetValue( height );
    }

    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnHeightChange( wxSpinDoubleEvent& aEvent )
{
    if( m_lockAspectRatio )
    {
        double height = m_spinHeight->GetValue();
        double width;

        if( m_sizeUnits == SIZE_UNITS::PERCENT )
            width = m_originalSize.GetHeight() * height / 100.0 * m_aspectRatio;
        else
            width = height * m_aspectRatio;

        // Ensure width is not less than 1
        if( width < 1 )
            width = 1;

        m_spinWidth->SetValue( width );
    }

    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnXResolutionChange( wxSpinDoubleEvent& aEvent )
{
    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnYResolutionChange( wxSpinDoubleEvent& aEvent )
{
    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnSizeUnitChange( wxCommandEvent& aEvent )
{
    SIZE_UNITS oldUnits = m_sizeUnits;
    m_sizeUnits = static_cast<SIZE_UNITS>( m_choiceSizeUnits->GetSelection() );
    ConvertSizeUnits( oldUnits, m_sizeUnits );
    UpdatePixelSize();
}


void DIALOG_EXPORT_3D_IMAGE::OnResolutionUnitChange( wxCommandEvent& aEvent )
{
    RESOLUTION_UNITS oldUnits = m_resolutionUnits;
    m_resolutionUnits = static_cast<RESOLUTION_UNITS>( m_choiceResolutionUnits->GetSelection() );
    ConvertResolutionUnits( oldUnits, m_resolutionUnits );
    UpdatePixelSize();
}


wxSize DIALOG_EXPORT_3D_IMAGE::GetPixelSize( double aWidth, double aHeight, double aXResolution, double aYResolution, SIZE_UNITS aSizeUnits ) const
{
    switch( aSizeUnits )
    {
    case SIZE_UNITS::PIXELS:
        return wxSize( static_cast<int>( aWidth ), static_cast<int>( aHeight ) );
    case SIZE_UNITS::PERCENT:
        return wxSize( static_cast<int>( aWidth * m_originalSize.GetWidth() / 100.0 ),
                       static_cast<int>( aHeight * m_originalSize.GetHeight() / 100.0 ) );
    case SIZE_UNITS::MM:
        return wxSize( static_cast<int>( aWidth * aXResolution / 25.4 ),
                       static_cast<int>( aHeight * aYResolution / 25.4 ) );
    case SIZE_UNITS::INCHES:
        return wxSize( static_cast<int>( aWidth * aXResolution ),
                       static_cast<int>( aHeight * aYResolution ) );
    default:
        break;
    }

    return wxSize( 10, 10 ); // Should not happen
}


void DIALOG_EXPORT_3D_IMAGE::UpdatePixelSize()
{
    double width = m_spinWidth->GetValue();
    double height = m_spinHeight->GetValue();
    double xRes = m_spinXResolution->GetValue();
    double yRes = m_spinYResolution->GetValue();

    // Convert resolution to standard pixels/inch
    if( m_resolutionUnits == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        xRes *= 25.4; // Convert mm to inches
        yRes *= 25.4;
    }

    wxSize pixelSize = GetPixelSize( width, height, xRes, yRes, m_sizeUnits );

    m_pixelSizeLabel->SetLabel( wxString::Format( _( "%d × %d pixels" ), pixelSize.GetWidth(), pixelSize.GetHeight() ) );
}


void DIALOG_EXPORT_3D_IMAGE::UpdateAspectRatio()
{
    m_aspectRatio = m_spinWidth->GetValue() / m_spinHeight->GetValue();
}


void DIALOG_EXPORT_3D_IMAGE::ConvertSizeUnits( SIZE_UNITS aFromUnit, SIZE_UNITS aToUnit )
{
    if( aFromUnit == aToUnit )
        return;

    double width = m_spinWidth->GetValue();
    double height = m_spinHeight->GetValue();
    double xRes = m_spinXResolution->GetValue();
    double yRes = m_spinYResolution->GetValue();

    // Convert resolution to standard pixels/inch
    if( m_resolutionUnits == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        xRes *= 25.4; // Convert mm to inches
        yRes *= 25.4;
    }

    // Convert to pixels first
    wxSize pixelSize = GetPixelSize( width, height, xRes, yRes, aFromUnit );

    // Convert from pixels to target unit
    switch( aToUnit )
    {
    case SIZE_UNITS::PIXELS:
        m_spinWidth->SetValue( pixelSize.GetWidth() );
        m_spinHeight->SetValue( pixelSize.GetHeight() );
        break;
    case SIZE_UNITS::PERCENT:
        m_spinWidth->SetValue( pixelSize.GetWidth() * 100.0 / m_originalSize.GetWidth() );
        m_spinHeight->SetValue( pixelSize.GetHeight() * 100.0 / m_originalSize.GetHeight() );
        break;
    case SIZE_UNITS::MM:
        m_spinWidth->SetValue( pixelSize.GetWidth() * 25.4 / xRes );
        m_spinHeight->SetValue( pixelSize.GetHeight() * 25.4 / yRes );
        break;
    case SIZE_UNITS::INCHES:
        m_spinWidth->SetValue( pixelSize.GetWidth() / xRes );
        m_spinHeight->SetValue( pixelSize.GetHeight() / yRes );
        break;
    }
}


void DIALOG_EXPORT_3D_IMAGE::ConvertResolutionUnits( RESOLUTION_UNITS aFromUnit, RESOLUTION_UNITS aToUnit )
{
    if( aFromUnit == aToUnit )
        return;

    double xRes = m_spinXResolution->GetValue();
    double yRes = m_spinYResolution->GetValue();

    if( aFromUnit == RESOLUTION_UNITS::PIXELS_PER_INCH && aToUnit == RESOLUTION_UNITS::PIXELS_PER_MM )
    {
        // Convert from pixels/inch to pixels/mm
        m_spinXResolution->SetValue( xRes / 25.4 );
        m_spinYResolution->SetValue( yRes / 25.4 );
    }
    else if( aFromUnit == RESOLUTION_UNITS::PIXELS_PER_MM && aToUnit == RESOLUTION_UNITS::PIXELS_PER_INCH )
    {
        // Convert from pixels/mm to pixels/inch
        m_spinXResolution->SetValue( xRes * 25.4 );
        m_spinYResolution->SetValue( yRes * 25.4 );
    }
}