/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <kiplatform/ui.h>
#include <dialogs/dialog_color_picker.h>
#include <cmath>
#include <algorithm>
#include <kiface_base.h>
#include <settings/app_settings.h>
#include <widgets/color_swatch.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>

#define ALPHA_MAX 100   // the max value returned by the alpha (opacity) slider
#define SLOPE_AXIS ( bmsize.y / 5.28 ) // was 50 at 264 size

using KIGFX::COLOR4D;

// Configure the spin controls contained inside the dialog
void configureSpinCtrl( wxSpinCtrl* aCtrl )
{
    wxSize textLength = aCtrl->GetTextExtent( "999" );
    wxSize ctrlSize   = aCtrl->GetSizeFromTextSize( textLength );

    aCtrl->SetMinSize( ctrlSize );
    aCtrl->SetSize( ctrlSize );
}


DIALOG_COLOR_PICKER::DIALOG_COLOR_PICKER( wxWindow* aParent, const COLOR4D& aCurrentColor,
                                          bool aAllowOpacityControl,
                                          CUSTOM_COLORS_LIST* aUserColors,
                                          const COLOR4D& aDefaultColor ) :
	DIALOG_COLOR_PICKER_BASE( aParent )
{
    m_allowMouseEvents = false;
    m_allowOpacityCtrl = aAllowOpacityControl;
    m_previousColor4D = aCurrentColor;
    m_newColor4D = aCurrentColor;
    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    m_bitmapRGB = nullptr;
    m_bitmapHSV = nullptr;
    m_selectedCursor = nullptr;
    m_defaultColor = aDefaultColor;

    updateHandleSize();

    m_OldColorRect->SetMinSize( FromDIP( wxSize( 20, 20 ) ) );
    m_NewColorRect->SetMinSize( FromDIP( wxSize( 20, 20 ) ) );

    m_colorValue->SetMinSize( wxSize( GetTextExtent( wxS( "@{color(DEVICE_BACKGROUND)}" ) ).x + FromDIP( 8 ), -1 ) );

    if( !m_allowOpacityCtrl )
    {
        m_SizerTransparency->Show( false );

        if( aCurrentColor != COLOR4D::UNSPECIFIED )
        {
            m_previousColor4D.a = 1.0;
            m_newColor4D.a      = 1.0;
        }
    }

    // UNSPECIFIED is ( 0, 0, 0, 0 ) but that is unfriendly for editing because you have to notice
    // first that the value slider is all the way down before you get any color
    if( aCurrentColor == COLOR4D::UNSPECIFIED )
        m_val = 1.0;

    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();
    wxASSERT( cfg );

    m_notebook->SetSelection( cfg->m_ColorPicker.default_tab );

    // Build the defined colors panel:
    initDefinedColors( aUserColors );

    /**
     * There are two types of color settings: theme colors and local overrides.
     * Theme colors have a default value, and the Reset to Default button reverts to it.
     * Local override colors have a default of UNSPECIFIED, which means "use the theme color".
     * The underlying action is the same, but we change the label here because the action from
     * the point of view of the user is slightly different.
     */
    if( aDefaultColor == COLOR4D::UNSPECIFIED )
        m_resetToDefault->SetLabel( _( "Clear Color" ) );

    SetupStandardButtons();
}


DIALOG_COLOR_PICKER::~DIALOG_COLOR_PICKER()
{
    APP_SETTINGS_BASE* cfg = Kiface().KifaceSettings();
    wxASSERT( cfg );

    if( cfg )
        cfg->m_ColorPicker.default_tab = m_notebook->GetSelection();

    delete m_bitmapRGB;
    delete m_bitmapHSV;

    for( wxStaticBitmap* swatch : m_colorSwatches )
    {
        swatch->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED,
                            wxMouseEventHandler( DIALOG_COLOR_PICKER::buttColorClick ),
                            nullptr, this );
    }
}


void DIALOG_COLOR_PICKER::updatePreview( wxStaticBitmap* aStaticBitmap, COLOR4D& aColor4D )
{
    wxSize  swatchSize = aStaticBitmap->GetSize();
    wxSize  checkerboardSize = ConvertDialogToPixels( CHECKERBOARD_SIZE_DU );

    wxBitmap newBm = COLOR_SWATCH::MakeBitmap( aColor4D, COLOR4D::WHITE,
                                               ToPhys( swatchSize ),
                                               ToPhys( checkerboardSize ),
                                               aStaticBitmap->GetParent()->GetBackgroundColour() );

    newBm.SetScaleFactor( GetDPIScaleFactor() );
    aStaticBitmap->SetBitmap( newBm );
}


bool DIALOG_COLOR_PICKER::TransferDataToWindow()
{
    SetEditVals( INIT, false );

    // Configure the spin control sizes
    configureSpinCtrl( m_spinCtrlGreen );
    configureSpinCtrl( m_spinCtrlBlue );
    configureSpinCtrl( m_spinCtrlRed );
    configureSpinCtrl( m_spinCtrlHue );
    configureSpinCtrl( m_spinCtrlSaturation );

    m_notebook->GetPage( 0 )->Layout();
    m_notebook->GetPage( 1 )->Layout();

    finishDialogSettings();

    // Draw all bitmaps, with colors according to the color 4D
    updatePreview( m_OldColorRect, m_previousColor4D );
    drawAll();

    return true;
}


void DIALOG_COLOR_PICKER::initDefinedColors( CUSTOM_COLORS_LIST* aPredefinedColors )
{
    #define ID_COLOR_BLACK 2000 // colors_id = ID_COLOR_BLACK a ID_COLOR_BLACK + NBCOLORS-1

    // Colors are built from the colorRefs() table (size NBCOLORS).
    // The look is better when colorRefs() order is displayed in a grid matrix
    // of 7 row and 5 columns, first filling a row, and after the next column.
    // But the wxFlexGrid used here must be filled by columns, then next row
    // the best interval colorRefs() from a matrix row to the next row is 6
    // So when have to reorder the index used to explore colorRefs()
    int grid_col = 0;
    int grid_row = 0;
    int table_row_count = 7;

    wxSize  swatchSize = ConvertDialogToPixels( SWATCH_SIZE_LARGE_DU );
    wxSize  checkerboardSize = ConvertDialogToPixels( CHECKERBOARD_SIZE_DU );
    COLOR4D checkboardBackground = m_OldColorRect->GetParent()->GetBackgroundColour();

    auto addSwatch =
            [&]( int aId, COLOR4D aColor, const wxString& aColorName )
            {
                wxBitmap bm = COLOR_SWATCH::MakeBitmap( aColor, COLOR4D::WHITE,
                                                        ToPhys( swatchSize ),
                                                        ToPhys( checkerboardSize ),
                                                        checkboardBackground );

                bm.SetScaleFactor( GetDPIScaleFactor() );
                wxStaticBitmap* swatch = new wxStaticBitmap( m_panelDefinedColors, aId, bm );

                m_fgridColor->Add( swatch, 0, wxALIGN_CENTER_VERTICAL, 5 );

                wxStaticText* label = new wxStaticText( m_panelDefinedColors, wxID_ANY, aColorName,
                                                        wxDefaultPosition, wxDefaultSize, 0 );
                m_fgridColor->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15 );

                m_colorSwatches.push_back( swatch );

                swatch->Connect( wxEVT_LEFT_DOWN,
                                 wxMouseEventHandler( DIALOG_COLOR_PICKER::buttColorClick ),
                                 nullptr, this );
                swatch->Connect( wxEVT_LEFT_DCLICK,
                                 wxMouseEventHandler( DIALOG_COLOR_PICKER::colorDClick ),
                                 nullptr, this );
            };

    // If no predefined list is given, build the default predefined colors:
    if( aPredefinedColors )
    {
        for( unsigned jj = 0; jj < aPredefinedColors->size() && jj < NBCOLORS; ++jj )
        {
            CUSTOM_COLOR_ITEM* item = & *aPredefinedColors->begin() + jj;
            int                butt_ID = ID_COLOR_BLACK + jj;

            addSwatch( butt_ID, item->m_Color, item->m_ColorName );
            m_Color4DList.push_back( item->m_Color );
        }
    }
    else
    {
        m_Color4DList.assign( NBCOLORS, COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );

        for( int jj = 0; jj < NBCOLORS; ++jj, grid_col++ )
        {
            if( grid_col * table_row_count >= NBCOLORS )
            {
                // the current grid row is filled, and we must fill the next grid row
                grid_col = 0;
                grid_row++;
            }

            int     ii = grid_row + ( grid_col * table_row_count ); // The index in colorRefs()
            int     butt_ID = ID_COLOR_BLACK + ii;
            COLOR4D buttcolor = COLOR4D( colorRefs()[ii].m_Numcolor );

            addSwatch( butt_ID, buttcolor, wxGetTranslation( colorRefs()[ii].m_ColorName ) );
            m_Color4DList[ butt_ID - ID_COLOR_BLACK ] = buttcolor;
        }
    }
}


void DIALOG_COLOR_PICKER::createRGBBitmap()
{
    wxSize bmsize = ToPhys( m_RgbBitmap->GetSize() );
    int half_size = std::min( bmsize.x, bmsize.y )/2;

    // We use here a Y axis from bottom to top and origin to center, So we need to map
    // coordinated to write pixel in a wxImage.  MAPX and MAPY are defined above so they
    // must be undefined here to prevent compiler warnings.
#undef MAPX
#undef MAPY
#define MAPX( xx ) bmsize.x / 2 + ( xx )
#define MAPY( yy ) bmsize.y / 2 - ( yy )

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    COLOR4D color;

    // Red blue area in X Z 3d axis
    double inc = 255.0 / half_size;
    double slope = SLOPE_AXIS/half_size;
    color.g = 0.0;

    wxImage img( bmsize );  // a temporary buffer to build the color map

    // clear background (set the window bg color)
    wxColor bg = GetBackgroundColour();

    // Don't do standard-color lookups on OSX each time through the loop
    wxColourBase::ChannelType bgR = bg.Red();
    wxColourBase::ChannelType bgG = bg.Green();
    wxColourBase::ChannelType bgB = bg.Blue();

    for( int xx = 0; xx < bmsize.x; xx++ ) // blue axis
    {
        for( int yy = 0; yy < bmsize.y; yy++ )  // Red axis
            img.SetRGB( xx, yy, bgR, bgG, bgB );
    }

    // Build the palette
    for( int xx = 0; xx < half_size; xx++ ) // blue axis
    {
        color.b = inc * xx;

        for( int yy = 0; yy < half_size; yy++ )  // Red axis
        {
            color.r = inc * yy;
            img.SetRGB( MAPX( xx ), MAPY( yy - (slope*xx) ), color.r, color.g, color.b );
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
            img.SetRGB( MAPX( -xx ), MAPY( yy - (slope*xx) ), color.r, color.g, color.b );
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

            // Mapping the xx, yy color axis to draw coordinates is more tricky than previously
            // in DC coordinates:
            // the blue axis is the (0, 0) to half_size, (-yy - SLOPE_AXIS)
            // the green axis is the (0, 0) to - half_size, (-yy - SLOPE_AXIS)
            int drawX = -xx + yy;
            int drawY = - std::min( xx,yy ) * 0.9;
            img.SetRGB( MAPX( drawX ),  MAPY( drawY - std::abs( slope*drawX ) ),
                        color.r,  color.g,  color.b );
        }
    }

    delete m_bitmapRGB;
    m_bitmapRGB = new wxBitmap( img, 24 );

    m_bitmapRGB->SetScaleFactor( GetDPIScaleFactor() );
    m_RgbBitmap->SetBitmap( *m_bitmapRGB );
}


void DIALOG_COLOR_PICKER::createHSVBitmap()
{
    wxSize bmsize = ToPhys( m_HsvBitmap->GetSize() );
    int half_size = std::min( bmsize.x, bmsize.y )/2;

    // We use here a Y axis from bottom to top and origin to center, So we need to map
    // coordinated to write pixel in a wxImage
    #define MAPX( xx ) bmsize.x / 2 + ( xx )
    #define MAPY( yy ) bmsize.y / 2 - ( yy )

    wxImage img( bmsize );  // a temporary buffer to build the color map

    // clear background (set the window bg color)
    wxColor bg = GetBackgroundColour();

    // Don't do standard-color lookups on OSX each time through the loop
    wxColourBase::ChannelType bgR = bg.Red();
    wxColourBase::ChannelType bgG = bg.Green();
    wxColourBase::ChannelType bgB = bg.Blue();

    for( int xx = 0; xx < bmsize.x; xx++ ) // blue axis
    {
        for( int yy = 0; yy < bmsize.y; yy++ )  // Red axis
            img.SetRGB( xx, yy, bgR, bgG, bgB );
    }

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize/2;

    double  hue, sat;
    COLOR4D color;
    int     sq_radius = half_size*half_size;

    // Build the palette
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

            img.SetRGB( MAPX( xx ), MAPY( yy ), color.r * 255, color.g * 255, color.b * 255 );
        }
    }

    delete m_bitmapHSV;
    m_bitmapHSV = new wxBitmap( img, 24 );

    m_bitmapHSV->SetScaleFactor( GetDPIScaleFactor() );
    m_HsvBitmap->SetBitmap( *m_bitmapHSV );
}


void DIALOG_COLOR_PICKER::drawRGBPalette()
{
    if( !m_bitmapRGB || m_bitmapRGB->GetSize() != ToPhys( m_RgbBitmap->GetSize() ) )
        createRGBBitmap();

    wxMemoryDC bitmapDC;
    wxSize     bmsize = m_bitmapRGB->GetSize();
    int        half_size = std::min( bmsize.x, bmsize.y ) / 2;

    wxBitmap newBm( *m_bitmapRGB );
    newBm.SetScaleFactor( 1.0 );

    bitmapDC.SelectObject( newBm );

    // Use Y axis from bottom to top and origin to center
    bitmapDC.SetAxisOrientation( true, true );

#if defined( __WXMSW__ ) && !wxCHECK_VERSION( 3, 3, 0 )
    // For some reason, SetDeviceOrigin has changed in wxWidgets 3.1.6 or 3.1.7
    // It was fixed in wx >= 3.3
    bitmapDC.SetDeviceOrigin( half_size, -half_size );
#else
    bitmapDC.SetDeviceOrigin( half_size, half_size );
#endif

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize / 2;

    // Draw the 3 RGB cursors, using white color to make them always visible:
    wxPen pen( wxColor( 255, 255, 255 ), 2 );       // use 2 pixels for pen size
    wxBrush brush( wxColor( 0, 0, 0 ), wxBRUSHSTYLE_TRANSPARENT );
    bitmapDC.SetPen( pen );
    bitmapDC.SetBrush( brush );
    int half_csize = m_cursorsSize / 2;

    double slope = SLOPE_AXIS / half_size;

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

    newBm.SetScaleFactor( GetDPIScaleFactor() );
    m_RgbBitmap->SetBitmap( newBm );

    /* Deselect the Tool Bitmap from DC,
     *  in order to delete the MemoryDC safely without deleting the bitmap */
    bitmapDC.SelectObject( wxNullBitmap );
}


void DIALOG_COLOR_PICKER::drawHSVPalette()
{
    if( !m_bitmapHSV || m_bitmapHSV->GetSize() != ToPhys( m_HsvBitmap->GetSize() ) )
        createHSVBitmap();

    wxMemoryDC bitmapDC;
    wxSize     bmsize = m_bitmapHSV->GetSize();
    int        half_size = std::min( bmsize.x, bmsize.y ) / 2;

    wxBitmap newBm( *m_bitmapHSV );
    newBm.SetScaleFactor( 1.0 );

    bitmapDC.SelectObject( newBm );

    // Use Y axis from bottom to top and origin to center
    bitmapDC.SetAxisOrientation( true, true );
#if defined( __WXMSW__ ) && !wxCHECK_VERSION(3,3,0)
    // For some reason, SetDeviceOrigin has changed in wxWidgets 3.1.6 or 3.1.7
    // It was fixed in wx >= 3.3
    bitmapDC.SetDeviceOrigin( half_size, -half_size );
#else
    bitmapDC.SetDeviceOrigin( half_size, half_size );
#endif

    // Reserve room to draw cursors inside the bitmap
    half_size -= m_cursorsSize / 2;

    // Draw the HSB cursor:
    m_cursorBitmapHSV.x = cos( m_hue * M_PI / 180.0 ) * half_size * m_sat;
    m_cursorBitmapHSV.y = sin( m_hue * M_PI / 180.0 ) * half_size * m_sat;

    wxPen pen( wxColor( 0, 0, 0 ), 2 );     // Use 2 pixels as pensize
    wxBrush brush( wxColor( 0, 0, 0 ), wxBRUSHSTYLE_TRANSPARENT );
    bitmapDC.SetPen( pen );
    bitmapDC.SetBrush( brush );

    bitmapDC.DrawRectangle( m_cursorBitmapHSV.x - ( m_cursorsSize / 2 ),
                            m_cursorBitmapHSV.y - ( m_cursorsSize / 2 ),
                            m_cursorsSize, m_cursorsSize );

    newBm.SetScaleFactor( GetDPIScaleFactor() );
    m_HsvBitmap->SetBitmap( newBm );

    /* Deselect the Tool Bitmap from DC,
     * in order to delete the MemoryDC safely without deleting the bitmap
     */
    bitmapDC.SelectObject( wxNullBitmap );
}


void DIALOG_COLOR_PICKER::SetEditVals( CHANGED_COLOR aChanged, bool aCheckTransparency )
{
    if( aCheckTransparency )
    {
        // If they've changed the color, they probably don't want it to remain 100% transparent,
        // and it looks like a bug when changing the color has no effect.
        if( m_newColor4D.a == 0.0 )
            m_newColor4D.a = 1.0;
    }

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
        m_sliderBrightness->SetValue(normalizeToInt( m_val ) );

    if( aChanged == INIT )
    {
        if( m_newColor4D.m_text.has_value() )
            m_colorValue->ChangeValue( m_newColor4D.m_text.value() );
        else
            m_colorValue->ChangeValue( m_newColor4D.ToHexString() );
    }
    else if( aChanged != HEX_CHANGED )
    {
        m_newColor4D.m_text = std::nullopt;
        m_colorValue->ChangeValue( m_newColor4D.ToHexString() );
    }
}


void DIALOG_COLOR_PICKER::updateHandleSize()
{
    m_cursorsSize = ToPhys( FromDIP( 8 ) ); // Size of square cursors drawn on color bitmaps
}


void DIALOG_COLOR_PICKER::drawAll()
{
    updateHandleSize();
    m_NewColorRect->Freeze();   // Avoid flicker
    m_HsvBitmap->Freeze();
    m_RgbBitmap->Freeze();
    updatePreview( m_NewColorRect, m_newColor4D );
    drawHSVPalette();
    drawRGBPalette();
    m_NewColorRect->Thaw();
    m_HsvBitmap->Thaw();
    m_RgbBitmap->Thaw();
    m_NewColorRect->Refresh();
    m_HsvBitmap->Refresh();
    m_RgbBitmap->Refresh();
}


void DIALOG_COLOR_PICKER::colorDClick( wxMouseEvent& event )
{
    wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
}


void DIALOG_COLOR_PICKER::buttColorClick( wxMouseEvent& event )
{
    int id = event.GetId();
    COLOR4D color( m_Color4DList[id - ID_COLOR_BLACK] );
    m_newColor4D.r = color.r;
    m_newColor4D.g = color.g;
    m_newColor4D.b = color.b;
    m_newColor4D.a = color.a;

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED, false );

    drawAll();

    event.Skip();
}


void DIALOG_COLOR_PICKER::onRGBMouseClick( wxMouseEvent& event )
{
    m_allowMouseEvents = true;

    // The cursor position is relative to the m_bitmapHSV wxBitmap center
    wxPoint mousePos = ToPhys( event.GetPosition() );
    wxSize  bmsize = m_bitmapRGB->GetSize();
    int     half_size = std::min( bmsize.x, bmsize.y ) / 2;

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

    if( std::abs( dist.x ) <= m_cursorsSize / 2 && std::abs( dist.y ) <= m_cursorsSize / 2 )
    {
        m_selectedCursor = &m_cursorBitmapGreen;
        return;
    }

    dist = m_cursorBitmapBlue - mousePos;

    if( std::abs( dist.x ) <= m_cursorsSize / 2 && std::abs( dist.y ) <= m_cursorsSize / 2 )
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

    if( m_selectedCursor != &m_cursorBitmapRed
     && m_selectedCursor != &m_cursorBitmapGreen
     && m_selectedCursor != &m_cursorBitmapBlue )
    {
        return;
    }

    // Adjust the HSV cursor position to follow the mouse cursor
    // The cursor position is relative to the m_bitmapHSV wxBitmap center
    wxPoint mousePos = ToPhys( event.GetPosition() );
    wxSize  bmsize = m_bitmapRGB->GetSize();
    int     half_size = std::min( bmsize.x, bmsize.y ) / 2;

    mousePos.x -= half_size;
    mousePos.y -= half_size;
    mousePos.y = -mousePos.y;           // Use the bottom to top vertical axis

    half_size -= m_cursorsSize / 2;     // the actual half_size of the palette area

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
    SetEditVals( ALL_CHANGED, true );

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


void DIALOG_COLOR_PICKER::onSize( wxSizeEvent& event )
{
    drawAll();

    event.Skip();
}


void DIALOG_COLOR_PICKER::OnColorValueText( wxCommandEvent& event )
{
    if( m_newColor4D.SetFromHexString( m_colorValue->GetValue() ) )
        m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );

    SetEditVals( HEX_CHANGED, false );
    drawAll();
}


bool DIALOG_COLOR_PICKER::setHSvaluesFromCursor( const wxPoint& aMouseCursor )
{
    wxPoint mousePos = ToPhys( aMouseCursor );
    wxSize  bmsize = m_bitmapHSV->GetSize();
    int     half_size = std::min( bmsize.x, bmsize.y ) / 2;

    // Make the cursor position relative to the m_bitmapHSV wxBitmap center
    mousePos.x -= half_size;
    mousePos.y -= half_size;
    mousePos.y = -mousePos.y;   // Use the bottom to top vertical axis

    // The HS cursor position is restricted to a circle of radius half_size
    double dist_from_centre = hypot( (double)mousePos.x, (double)mousePos.y );

    if( dist_from_centre > half_size )
    {
        // Saturation cannot be calculated:
        return false;
    }

    m_cursorBitmapHSV = mousePos;

    // Set saturation and hue from new cursor position:
    half_size -= m_cursorsSize / 2;       // the actual half_size of the palette area
    m_sat = dist_from_centre / half_size;

    if( m_sat > 1.0 )
        m_sat = 1.0;

    m_hue = atan2( mousePos.y, mousePos.x ) / M_PI * 180.0;

    if( m_hue < 0 )
        m_hue += 360.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );
    SetEditVals( ALL_CHANGED, true );

    return true;
}


void DIALOG_COLOR_PICKER::OnChangeAlpha( wxScrollEvent& event )
{
    double alpha = (double)event.GetPosition() / ALPHA_MAX;
    m_newColor4D.a = alpha;
    m_NewColorRect->Freeze();   // Avoid flicker
    updatePreview( m_NewColorRect, m_newColor4D );
    m_NewColorRect->Thaw();
    m_NewColorRect->Refresh();
    SetEditVals( ALPHA_CHANGED, false );
}


void DIALOG_COLOR_PICKER::OnChangeEditRed( wxSpinEvent& event )
{
    double val = (double)event.GetPosition() / 255.0;
    m_newColor4D.r = val;
    SetEditVals( RED_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditGreen( wxSpinEvent& event )
{
    double val = (double)event.GetPosition() / 255.0;
    m_newColor4D.g = val;
    SetEditVals( GREEN_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditBlue( wxSpinEvent& event )
{
    double val = (double)event.GetPosition() / 255.0;
    m_newColor4D.b = val;
    SetEditVals( BLUE_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditHue( wxSpinEvent& event )
{
    m_hue = (double)event.GetPosition();

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( HUE_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeEditSat( wxSpinEvent& event )
{
    m_sat = (double)event.GetPosition() / 255.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( SAT_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnChangeBrightness( wxScrollEvent& event )
{
    m_val = (double)event.GetPosition() / 255.0;

    m_newColor4D.FromHSV( m_hue, m_sat, m_val );

    SetEditVals( VAL_CHANGED, true );

    drawAll();
}


void DIALOG_COLOR_PICKER::OnResetButton( wxCommandEvent& aEvent )
{
    m_newColor4D.r = m_defaultColor.r;
    m_newColor4D.g = m_defaultColor.g;
    m_newColor4D.b = m_defaultColor.b;
    m_newColor4D.a = m_defaultColor.a;

    m_newColor4D.ToHSV( m_hue, m_sat, m_val, true );
    SetEditVals( ALL_CHANGED, false );

    drawAll();
}
