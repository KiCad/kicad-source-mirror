/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "dialog_color_picker.h"
#include <cmath>

#define ALPHA_MAX 100   // the max value returned by the alpha (opacity) slider

DIALOG_COLOR_PICKER::DIALOG_COLOR_PICKER( wxWindow* aParent, KIGFX::COLOR4D& aCurrentColor,
                                          bool aAllowOpacityControl, CUSTOM_COLORS_LIST* aUserColors,
                                          const KIGFX::COLOR4D& aDefaultColor )
	: DIALOG_COLOR_PICKER_BASE( aParent )
{
    m_allowMouseEvents = false;
    m_allowOpacityCtrl = aAllowOpacityControl;
    m_previousColor4D = aCurrentColor;
    m_newColor4D = aCurrentColor;
    m_cursorsSize = 8;      // Size of square cursors drawn on color bitmaps
    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    m_bitmapRGB = nullptr;
    m_bitmapHSV = nullptr;
    m_selectedCursor = nullptr;
    m_defaultColor = aDefaultColor;

    if( !m_allowOpacityCtrl )
    {
        m_SizerTransparency->Show( false );
        m_previousColor4D.a = 1.0;
        m_newColor4D.a = 1.0;
    }

    if( m_ActivePage >= 0 )
        m_notebook->SetSelection( (unsigned) m_ActivePage );

    // Build the defined colors panel:
    initDefinedColors( aUserColors );

    // If there is no default color, don't give the option to reset to default
    if( aDefaultColor == KIGFX::COLOR4D::UNSPECIFIED )
        m_resetToDefault->Hide();

    m_sdbSizerOK->SetDefault();
}

int DIALOG_COLOR_PICKER::m_ActivePage = 0;    // the active notebook page, stored during a session


DIALOG_COLOR_PICKER::~DIALOG_COLOR_PICKER()
{
    delete m_bitmapRGB;
    delete m_bitmapHSV;

    m_ActivePage = m_notebook->GetSelection();

    for( auto button : m_buttonsColor )
        button->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                            wxCommandEventHandler( DIALOG_COLOR_PICKER::buttColorClick ), NULL, this );
}


void DIALOG_COLOR_PICKER::setIconColor( wxStaticBitmap* aStaticBitmap, KIGFX::COLOR4D& aColor4D )
{
    // Draw the icon that shows the aColor4D,
    // with colors according to the color 4D rgb and alpha
    // for alpha = 1 (no tranparency, the icon is a full rgb color rect
    // for alpha = 0 (100% tranparency, the icon is a grid of rgb color
    // and background color small sub rect
    wxMemoryDC bitmapDC;
    wxSize size = aStaticBitmap->GetSize();
    wxBitmap newBm( size );
    bitmapDC.SelectObject( newBm );
    wxPen pen( aColor4D.ToColour() );
    wxBrush brush( aColor4D.ToColour() );

    // clear background (set bg color to aColor4D )
    bitmapDC.SetBackground( brush );
    bitmapDC.Clear();


    // Draw the alpha subrect
    int stepx = size.x/8;
    int stepy = size.y/8;

    // build the alpha color for icon:
    // the alpha color is the initial color modified to be
    // the initial color for transparency = 0 ( alpha = 1 )
    // and white color for transparency = 1( alpha = 0 )
    KIGFX::COLOR4D bgcolor( GetBackgroundColour() );
    KIGFX::COLOR4D alphacolor = aColor4D;
    alphacolor.r = ( alphacolor.r * aColor4D.a ) + ( bgcolor.r * (1-aColor4D.a) );
    alphacolor.g = ( alphacolor.g * aColor4D.a ) + ( bgcolor.g * (1-aColor4D.a) );
    alphacolor.b = ( alphacolor.b * aColor4D.a ) + ( bgcolor.b * (1-aColor4D.a) );

    pen.SetColour( alphacolor.ToColour() );
    brush.SetColour( alphacolor.ToColour() );
    bitmapDC.SetPen( pen );
    bitmapDC.SetBrush( brush );

    for( int ii = 0; ii < size.x/stepx; ii+=2 )
    {
        for( int jj = 0; jj < size.y/stepy; jj+= 2 )
        {
            wxPoint pos( stepx*ii + stepx/2, stepy*jj + stepy/2 );
            bitmapDC.DrawRectangle( pos, wxSize( stepx, stepy ) );
        }
    }

    aStaticBitmap->SetBitmap( newBm );

    // Deselect the Tool Bitmap from DC, in order to delete the MemoryDC
    // safely without deleting the bitmap
    bitmapDC.SelectObject( wxNullBitmap );
}


bool DIALOG_COLOR_PICKER::TransferDataToWindow()
{
    // Draw all bitmaps, with colors according to the color 4D
    setIconColor( m_OldColorRect, m_previousColor4D );
    SetEditVals( ALL_CHANGED );
    drawAll();

    m_notebook->GetPage( 0 )->Layout();
    m_notebook->GetPage( 1 )->Layout();

    FinishDialogSettings();

    return true;
}


void DIALOG_COLOR_PICKER::initDefinedColors( CUSTOM_COLORS_LIST* aPredefinedColors )
{
    #define ID_COLOR_BLACK 2000 // colors_id = ID_COLOR_BLACK a ID_COLOR_BLACK + NBCOLORS-1

    // Size of color swatches
    const int w = 32, h = 32;

    // Colors are built from the g_ColorRefs table (size NBCOLORS).
    // The look is better when g_ColorRefs order is displayed in a grid matrix
    // of 6 row and 5 columns, first filling a row, and after the next column.
    // But the wxFlexGrid used here must be filled by columns, then next row
    // the best interval g_ColorRefs from a matrix row to the next row is 6
    // So when have to reorder the index used to explore g_ColorRefs
    int grid_col = 0;
    int grid_row = 0;
    int table_row_count = 6;

    // If no predefined list is given, build the default predefined colors:
    if( aPredefinedColors )
    {
        for( unsigned jj = 0; jj < aPredefinedColors->size() && jj < NBCOLORS; ++jj )
        {
            CUSTOM_COLOR_ITEM* item = & *aPredefinedColors->begin() + jj;
            int butt_ID = ID_COLOR_BLACK + jj;
            wxMemoryDC iconDC;
            wxBitmap   ButtBitmap( w, h );
            wxBrush    brush;

            iconDC.SelectObject( ButtBitmap );

            KIGFX::COLOR4D buttcolor = item->m_Color;

            iconDC.SetPen( *wxBLACK_PEN );
            brush.SetColour( buttcolor.ToColour() );
            brush.SetStyle( wxBRUSHSTYLE_SOLID );

            iconDC.SetBrush( brush );
            iconDC.SetBackground( *wxGREY_BRUSH );
            iconDC.Clear();
            iconDC.DrawRoundedRectangle( 0, 0, w, h, (double) h / 3 );

            wxBitmapButton* bitmapButton = new wxBitmapButton( m_panelDefinedColors, butt_ID, ButtBitmap,
                                               wxDefaultPosition, wxSize( w+8, h+6 ) );
            m_fgridColor->Add( bitmapButton, 0,
                               wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                               wxLEFT | wxBOTTOM, 5 );

            wxStaticText* label = new wxStaticText( m_panelDefinedColors, -1,
                                                    item->m_ColorName,
                                                    wxDefaultPosition, wxDefaultSize, 0 );
            m_fgridColor->Add( label, 1,
                               wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                               wxLEFT | wxRIGHT | wxBOTTOM, 5 );
            m_buttonsColor.push_back( bitmapButton );

            m_Color4DList.push_back( buttcolor );

            bitmapButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                   wxCommandEventHandler( DIALOG_COLOR_PICKER::buttColorClick ), NULL, this );
        }
    }
    else
    {
        m_Color4DList.assign( NBCOLORS, KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );

        for( int jj = 0; jj < NBCOLORS; ++jj, grid_col++ )
        {
            if( grid_col*table_row_count >= NBCOLORS )
            {   // the current grid row is filled, and we must fill the next grid row
                grid_col = 0;
                grid_row++;
            }

            int ii = grid_row + (grid_col*table_row_count); // The index in g_ColorRefs

            int butt_ID = ID_COLOR_BLACK + ii;
            wxMemoryDC iconDC;
            wxBitmap   ButtBitmap( w, h );
            wxBrush    brush;

            iconDC.SelectObject( ButtBitmap );

            KIGFX::COLOR4D buttcolor = KIGFX::COLOR4D( g_ColorRefs[ii].m_Numcolor );
            m_Color4DList[ butt_ID - ID_COLOR_BLACK ] = buttcolor;

            iconDC.SetPen( *wxBLACK_PEN );
            brush.SetColour( buttcolor.ToColour() );
            brush.SetStyle( wxBRUSHSTYLE_SOLID );

            iconDC.SetBrush( brush );
            iconDC.SetBackground( *wxGREY_BRUSH );
            iconDC.Clear();
            iconDC.DrawRoundedRectangle( 0, 0, w, h, (double) h / 3 );

            wxBitmapButton* bitmapButton = new wxBitmapButton( m_panelDefinedColors, butt_ID, ButtBitmap,
                                               wxDefaultPosition, wxSize( w+8, h+6 ) );
            m_fgridColor->Add( bitmapButton, 0,
                               wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                               wxLEFT | wxBOTTOM, 5 );

            wxStaticText* label = new wxStaticText( m_panelDefinedColors, -1,
                                                    wxGetTranslation( g_ColorRefs[ii].m_ColorName ),
                                                    wxDefaultPosition, wxDefaultSize, 0 );
            m_fgridColor->Add( label, 1,
                               wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                               wxLEFT | wxRIGHT | wxBOTTOM, 5 );
            m_buttonsColor.push_back( bitmapButton );
            bitmapButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED,
                                   wxCommandEventHandler( DIALOG_COLOR_PICKER::buttColorClick ), NULL, this );
        }
    }
}


void DIALOG_COLOR_PICKER::createRGBBitmap()
{
    wxMemoryDC bitmapDC;
    wxSize bmsize = m_RgbBitmap->GetSize();
    int half_size = std::min( bmsize.x, bmsize.y )/2;
    m_bitmapRGB = new wxBitmap( bmsize );
    bitmapDC.SelectObject( *m_bitmapRGB );
    wxPen pen;

    // clear background (set the window bg color)
    wxBrush bgbrush( GetBackgroundColour() );
    bitmapDC.SetBackground( bgbrush );
    bitmapDC.Clear();

    // Use Y axis from bottom to top and origin to center
    bitmapDC.SetAxisOrientation( true, true );
    bitmapDC.SetDeviceOrigin( half_size, half_size );

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    KIGFX::COLOR4D color;

    // Red blue area in X Z 3d axis
    double inc = 1.0 / half_size;
    #define SLOPE_AXIS 50.0
    double slope = SLOPE_AXIS/half_size;
    color.g = 0.0;

    for( int xx = 0; xx < half_size; xx++ ) // blue axis
    {
        color.b = inc * xx;

        for( int yy = 0; yy < half_size; yy++ )  // Red axis
        {
            color.r = inc * yy;

            pen.SetColour( color.ToColour() );
            bitmapDC.SetPen( pen );
            bitmapDC.DrawPoint( xx, yy - (slope*xx) );
        }
    }

    // Red green area in y Z 3d axis
    color.b = 0.0;
    for( int xx = 0; xx < half_size; xx++ )     // green axis
    {
        color.g = inc * xx;

        for( int yy = 0; yy < half_size; yy++ ) // Red axis
        {
            color.r = inc * yy;

            pen.SetColour( color.ToColour() );
            bitmapDC.SetPen( pen );
            bitmapDC.DrawPoint( -xx, yy - (slope*xx) );
        }
    }

    // Blue green area in x y 3d axis
    color.r = 0.0;
    for( int xx = 0; xx < half_size; xx++ )     // green axis
    {
        color.g = inc * xx;

        for( int yy = 0; yy < half_size; yy++ ) // blue axis
        {
            color.b = inc * yy;

            pen.SetColour( color.ToColour() );
            bitmapDC.SetPen( pen );

            // Mapping the xx, yy color axis to draw coordinates is more tricky than previously
            // in DC coordinates:
            // the blue axis is the (0, 0) to half_size, (-yy - SLOPE_AXIS)
            // the green axis is the (0, 0) to - half_size, (-yy - SLOPE_AXIS)
            int drawX = -xx + yy;
            int drawY = - std::min( xx,yy ) * 0.9;
            bitmapDC.DrawPoint( drawX, drawY - std::abs( slope*drawX ) );
        }
    }
}


void DIALOG_COLOR_PICKER::createHSVBitmap()
{
    wxMemoryDC bitmapDC;
    wxSize bmsize = m_HsvBitmap->GetSize();
    int half_size = std::min( bmsize.x, bmsize.y )/2;
    delete m_bitmapHSV;
    m_bitmapHSV = new wxBitmap( bmsize );
    bitmapDC.SelectObject( *m_bitmapHSV );
    wxPen pen;

    // clear background (set the window bd color)
    wxBrush bgbrush( GetBackgroundColour() );
    bitmapDC.SetBackground( bgbrush );
    bitmapDC.Clear();

    // Use Y axis from bottom to top and origin to center
    bitmapDC.SetAxisOrientation( true, true );
    bitmapDC.SetDeviceOrigin( half_size, half_size );

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    double hue, sat;
    KIGFX::COLOR4D color;
    int sq_radius = half_size*half_size;

    for( int xx = -half_size; xx < half_size; xx++ )
    {
        for( int yy = -half_size; yy < half_size; yy++ )
        {
            sat = double(xx*xx + yy*yy) / sq_radius;

            // sat is <= 1.0
            // any value > 1.0 is not a valid HSB color:
            if( sat > 1.0 )
                continue;

            // sat is the distance from center
            sat = sqrt( sat );
            hue = atan2( (double)yy, (double)xx ) * 180 / M_PI;

            if( hue < 0.0 )
                hue += 360.0;

            color.FromHSV( hue, sat, 1.0 );

            pen.SetColour( color.ToColour() );
            bitmapDC.SetPen( pen );
            bitmapDC.DrawPoint( xx, yy );
        }
    }

    /* Deselect the Tool Bitmap from DC,
     * in order to delete the MemoryDC safely without deleting the bitmap
     */
    bitmapDC.SelectObject( wxNullBitmap );
}


void DIALOG_COLOR_PICKER::drawRGBPalette()
{
    if( !m_bitmapRGB || m_bitmapRGB->GetSize() != m_RgbBitmap->GetSize() )
        createRGBBitmap();

    wxMemoryDC bitmapDC;
    wxSize bmsize = m_bitmapRGB->GetSize();
    int half_size = std::min( bmsize.x, bmsize.y )/2;
    wxBitmap newBm( *m_bitmapRGB );
    bitmapDC.SelectObject( newBm );

    // Use Y axis from bottom to top and origin to center
    bitmapDC.SetAxisOrientation( true, true );
    bitmapDC.SetDeviceOrigin( half_size, half_size );

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    // Draw the 3 RGB cursors, usiing white color to make them always visible:
    wxPen pen( wxColor( 255, 255, 255 ) );
    wxBrush brush( wxColor( 0, 0, 0 ), wxBRUSHSTYLE_TRANSPARENT );
    bitmapDC.SetPen( pen );
    bitmapDC.SetBrush( brush );
    int half_csize = m_cursorsSize/2;

    #define SLOPE_AXIS 50.0
    double slope = SLOPE_AXIS/half_size;

    // Red axis cursor (Z 3Daxis):
    m_cursorBitmapRed.x = 0;
    m_cursorBitmapRed.y = m_newColor4D.r * half_size;
    bitmapDC.DrawRectangle( m_cursorBitmapRed.x - half_csize,
                            m_cursorBitmapRed.y - half_csize,
                            m_cursorsSize, m_cursorsSize );

    // Blue axis cursor (X 3Daxis):
    m_cursorBitmapBlue.x = m_newColor4D.b * half_size;
    m_cursorBitmapBlue.y = - slope*m_cursorBitmapBlue.x;
    bitmapDC.DrawRectangle( m_cursorBitmapBlue.x - half_csize,
                            m_cursorBitmapBlue.y - half_csize,
                            m_cursorsSize, m_cursorsSize );

    // Green axis cursor (Y 3Daxis):
    m_cursorBitmapGreen.x = m_newColor4D.g * half_size;
    m_cursorBitmapGreen.y = - slope * m_cursorBitmapGreen.x;
    m_cursorBitmapGreen.x = -m_cursorBitmapGreen.x;

    bitmapDC.DrawRectangle( m_cursorBitmapGreen.x - half_csize,
                            m_cursorBitmapGreen.y - half_csize,
                            m_cursorsSize, m_cursorsSize );

    // Draw the 3 RGB axis:
    half_size += half_size/5;
    bitmapDC.DrawLine( 0, 0, 0, half_size );                    // Red axis (Z 3D axis)
    bitmapDC.DrawLine( 0, 0, half_size, - half_size*slope );    // Blue axis (X 3D axis)
    bitmapDC.DrawLine( 0, 0, -half_size, - half_size*slope );   // green axis (Y 3D axis)

    m_RgbBitmap->SetBitmap( newBm );
    /* Deselect the Tool Bitmap from DC,
     *  in order to delete the MemoryDC safely without deleting the bitmap */
    bitmapDC.SelectObject( wxNullBitmap );
}


void DIALOG_COLOR_PICKER::drawHSVPalette()
{
    if( !m_bitmapHSV || m_bitmapHSV->GetSize() != m_HsvBitmap->GetSize() )
        createHSVBitmap();

    wxMemoryDC bitmapDC;
    wxSize bmsize = m_bitmapHSV->GetSize();
    int half_size = std::min( bmsize.x, bmsize.y )/2;
    wxBitmap newBm( *m_bitmapHSV );
    bitmapDC.SelectObject( newBm );

    // Use Y axis from bottom to top and origin to center
    bitmapDC.SetAxisOrientation( true, true );
    bitmapDC.SetDeviceOrigin( half_size, half_size );

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    // Draw the HSB cursor:
    m_cursorBitmapHSV.x = cos( m_hue * M_PI / 180.0 ) * half_size * m_sat;
    m_cursorBitmapHSV.y = sin( m_hue * M_PI / 180.0 ) * half_size * m_sat;

    wxPen pen( wxColor( 0, 0, 0 ) );
    wxBrush brush( wxColor( 0, 0, 0 ), wxBRUSHSTYLE_TRANSPARENT );
    bitmapDC.SetPen( pen );
    bitmapDC.SetBrush( brush );

    int half_csize = m_cursorsSize/2;
    bitmapDC.DrawRectangle( m_cursorBitmapHSV.x- half_csize,
                            m_cursorBitmapHSV.y-half_csize,
                            m_cursorsSize, m_cursorsSize );

    m_HsvBitmap->SetBitmap( newBm );
    /* Deselect the Tool Bitmap from DC,
     * in order to delete the MemoryDC safely without deleting the bitmap
     */
    bitmapDC.SelectObject( wxNullBitmap );
}


void DIALOG_COLOR_PICKER::SetEditVals( CHANGED_COLOR aChanged )
{
    m_sliderTransparency->SetValue( normalizeToInt( m_newColor4D.a, ALPHA_MAX ) );

    if( aChanged == RED_CHANGED || aChanged == GREEN_CHANGED || aChanged == BLUE_CHANGED )
        m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );

    if( aChanged != RED_CHANGED )
        m_spinCtrlRed->SetValue( normalizeToInt( m_newColor4D.r ) );

    if( aChanged != GREEN_CHANGED )
        m_spinCtrlGreen->SetValue( normalizeToInt( m_newColor4D.g ) );

    if( aChanged != BLUE_CHANGED )
        m_spinCtrlBlue->SetValue( normalizeToInt( m_newColor4D.b  ) );

    if( aChanged != HUE_CHANGED )
        m_spinCtrlHue->SetValue( (int)m_hue );

    if( aChanged != SAT_CHANGED )
        m_spinCtrlSaturation->SetValue( m_sat * 255 );

    if( aChanged != VAL_CHANGED )
    {
        m_sliderBrightness->SetValue(normalizeToInt( m_val ) );
    }
}


void DIALOG_COLOR_PICKER::drawAll()
{
    m_NewColorRect->Freeze();   // Avoid flicker
    m_HsvBitmap->Freeze();
    m_RgbBitmap->Freeze();
    setIconColor( m_NewColorRect, m_newColor4D );
    drawHSVPalette();
    drawRGBPalette();
    m_NewColorRect->Thaw();
    m_HsvBitmap->Thaw();
    m_RgbBitmap->Thaw();
    m_NewColorRect->Refresh();
    m_HsvBitmap->Refresh();
    m_RgbBitmap->Refresh();
}


void DIALOG_COLOR_PICKER::buttColorClick( wxCommandEvent& event )
{
    int id = event.GetId();
    KIGFX::COLOR4D color( m_Color4DList[id - ID_COLOR_BLACK] );//EDA_COLOR_T( id - ID_COLOR_BLACK ) );
    m_newColor4D.r = color.r;
    m_newColor4D.g = color.g;
    m_newColor4D.b = color.b;

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED );

    drawAll();

    event.Skip();
}


void DIALOG_COLOR_PICKER::onRGBMouseClick( wxMouseEvent& event )
{
    m_allowMouseEvents = true;
    wxPoint mousePos = event.GetPosition();

    // The cursor position is relative to the m_bitmapHSV wxBitmap center
    wxSize bmsize = m_bitmapRGB->GetSize();
    int half_size = std::min( bmsize.x, bmsize.y )/2;
    mousePos.x -= half_size;
    mousePos.y -= half_size;
    mousePos.y = -mousePos.y;       // Use the bottom to top vertical axis

    wxPoint dist = m_cursorBitmapRed - mousePos;

    if( std::abs( dist.x ) <= m_cursorsSize/2 && std::abs( dist.y ) <= m_cursorsSize/2 )
    {
        m_selectedCursor = &m_cursorBitmapRed;
        return;
    }

    dist = m_cursorBitmapGreen - mousePos;

    if( std::abs( dist.x ) <= m_cursorsSize/2 && std::abs( dist.y ) <= m_cursorsSize/2 )
    {
        m_selectedCursor = &m_cursorBitmapGreen;
        return;
    }

    dist = m_cursorBitmapBlue - mousePos;

    if( std::abs( dist.x ) <= m_cursorsSize/2 && std::abs( dist.y ) <= m_cursorsSize/2 )
    {
        m_selectedCursor = &m_cursorBitmapBlue;
        return;
    }

    m_selectedCursor = nullptr;
}


void DIALOG_COLOR_PICKER::onRGBMouseDrag( wxMouseEvent& event )
{
    if( !event.Dragging() || !m_allowMouseEvents )
    {
        m_selectedCursor = nullptr;
        return;
    }

    if( m_selectedCursor != &m_cursorBitmapRed &&
        m_selectedCursor != &m_cursorBitmapGreen &&
        m_selectedCursor != &m_cursorBitmapBlue )
        return;

    // Adjust the HSV cursor position to follow the mouse cursor
    // The cursor position is relative to the m_bitmapHSV wxBitmap center
    wxPoint mousePos = event.GetPosition();
    wxSize bmsize = m_bitmapRGB->GetSize();
    int half_size = std::min( bmsize.x, bmsize.y )/2;
    mousePos.x -= half_size;
    mousePos.y -= half_size;
    mousePos.y = -mousePos.y;       // Use the bottom to top vertical axis

    half_size -= m_cursorsSize/2;       // the actual half_size of the palette area

    // Change colors according to the selected cursor:
    if( m_selectedCursor == &m_cursorBitmapRed )
    {
        if( mousePos.y >= 0 && mousePos.y <= half_size )
            m_newColor4D.r = (double)mousePos.y / half_size;
        else
            return;
    }

    if( m_selectedCursor == &m_cursorBitmapGreen )
    {
        mousePos.x = -mousePos.x;

        if( mousePos.x >= 0 && mousePos.x <= half_size )
            m_newColor4D.g = (double)mousePos.x / half_size;
        else
            return;
    }

    if( m_selectedCursor == &m_cursorBitmapBlue )
    {
        if( mousePos.x >= 0 && mousePos.x <= half_size )
            m_newColor4D.b = (double)mousePos.x / half_size;
        else
            return;
    }

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED );

    drawAll();
}


void DIALOG_COLOR_PICKER::onHSVMouseClick( wxMouseEvent& event )
{
    m_allowMouseEvents = true;

    if( setHSvaluesFromCursor( event.GetPosition() ) )
        drawAll();
}


void DIALOG_COLOR_PICKER::onHSVMouseDrag( wxMouseEvent& event )
{
    if( !event.Dragging() || !m_allowMouseEvents )
        return;

    if( setHSvaluesFromCursor( event.GetPosition() ) )
        drawAll();
}


bool DIALOG_COLOR_PICKER::setHSvaluesFromCursor( wxPoint aMouseCursor )
{
    wxPoint mousePos = aMouseCursor;
    wxSize bmsize = m_bitmapHSV->GetSize();
    int half_size = std::min( bmsize.x, bmsize.y )/2;
    // Make the cursor position relative to the m_bitmapHSV wxBitmap center
    mousePos.x -= half_size;
    mousePos.y -= half_size;
    mousePos.y = -mousePos.y;   // Use the bottom to top vertical axis

    // The HS cursor position is restricted to a circle of radius half_size
    double dist_from_centre = hypot( (double)mousePos.x, (double)mousePos.y );

    if( dist_from_centre > half_size )
        // Saturation cannot be calculated:
        return false;

    m_cursorBitmapHSV = mousePos;

    // Set saturation and hue from new cursor position:
    half_size -= m_cursorsSize/2;       // the actual half_size of the palette area
    m_sat = dist_from_centre / half_size;

    if( m_sat > 1.0 )
        m_sat = 1.0;

    m_hue = atan2( mousePos.y, mousePos.x ) / M_PI * 180.0;

    if( m_hue < 0 )
        m_hue += 360.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );
    SetEditVals( ALL_CHANGED );

    return true;
}


void DIALOG_COLOR_PICKER::OnChangeAlpha( wxScrollEvent& event )
{
    double alpha = (double)event.GetPosition() / ALPHA_MAX;
    m_newColor4D.a = alpha;
    m_NewColorRect->Freeze();   // Avoid flicker
    setIconColor( m_NewColorRect, m_newColor4D );
    m_NewColorRect->Thaw();
    m_NewColorRect->Refresh();
}


void DIALOG_COLOR_PICKER::OnChangeEditRed( wxSpinEvent& event )
{
    double val = (double)event.GetPosition() / 255.0;
    m_newColor4D.r = val;
    SetEditVals( RED_CHANGED );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditGreen( wxSpinEvent& event )
{
    double val = (double)event.GetPosition() / 255.0;
    m_newColor4D.g = val;
    SetEditVals( GREEN_CHANGED );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditBlue( wxSpinEvent& event )
{
    double val = (double)event.GetPosition() / 255.0;
    m_newColor4D.b = val;
    SetEditVals( BLUE_CHANGED );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditHue( wxSpinEvent& event )
{
    m_hue = (double)event.GetPosition();

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( HUE_CHANGED );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditSat( wxSpinEvent& event )
{
    m_sat = (double)event.GetPosition() / 255.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( SAT_CHANGED );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeBrightness( wxScrollEvent& event )
{
    m_val = (double)event.GetPosition() / 255.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( VAL_CHANGED );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnResetButton( wxCommandEvent& aEvent )
{
    m_newColor4D.r = m_defaultColor.r;
    m_newColor4D.g = m_defaultColor.g;
    m_newColor4D.b = m_defaultColor.b;

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED );

    drawAll();
}
