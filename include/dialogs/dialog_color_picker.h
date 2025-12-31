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

#ifndef DIALOG_COLOR_PICKER_H
#define DIALOG_COLOR_PICKER_H


#include <gal/color4d.h>
#include "../../common/dialogs/dialog_color_picker_base.h"

class COLOR_SWATCH;


/**
 * A class to handle a custom color (predefined color) for the color picker dialog.
 */
struct CUSTOM_COLOR_ITEM
{
    KIGFX::COLOR4D m_Color;
    wxString m_ColorName;

    CUSTOM_COLOR_ITEM( double red, double green, double blue, const wxString& aName )
    {
        m_Color.r = red;
        m_Color.g = green;
        m_Color.b = blue;
        m_ColorName = aName;
    }

    CUSTOM_COLOR_ITEM( double red, double green, double blue, double alpha, const wxString& aName )
    {
        m_Color.r = red;
        m_Color.g = green;
        m_Color.b = blue;
        m_Color.a = alpha;
        m_ColorName = aName;
    }

    CUSTOM_COLOR_ITEM( const KIGFX::COLOR4D& aColor, const wxString& aName )
        : m_Color( aColor ), m_ColorName( aName)
    {}
};


typedef std::vector<CUSTOM_COLOR_ITEM> CUSTOM_COLORS_LIST;


enum CHANGED_COLOR
{
    ALL_CHANGED,
    RED_CHANGED,
    BLUE_CHANGED,
    GREEN_CHANGED,
    ALPHA_CHANGED,
    HUE_CHANGED,
    SAT_CHANGED,
    VAL_CHANGED,
    HEX_CHANGED,
    INIT
};

class DIALOG_COLOR_PICKER : public DIALOG_COLOR_PICKER_BASE
{
public:
    /**
     * Dialog constructor
     * @param aParent is the caller
     * @param aCurrentColor is the current color, used to show it in dialog
     * @param aAllowOpacityControl true to allow opacity (alpha channel) setting
     * false to not show this setting (opacity = 1.0 always)
     * @param aUserColors if not null is a list of defined colors replacing the dialog
     *                    predefined colors
     */
	DIALOG_COLOR_PICKER( wxWindow* aParent, const KIGFX::COLOR4D& aCurrentColor,
                         bool aAllowOpacityControl, CUSTOM_COLORS_LIST* aUserColors = nullptr,
                         const KIGFX::COLOR4D& aDefaultColor = KIGFX::COLOR4D::UNSPECIFIED );
	~DIALOG_COLOR_PICKER();

	KIGFX::COLOR4D GetColor()
    {
        return KIGFX::COLOR4D( m_colorValue->GetValue() );
    };

private:
    /* When the dialog is created, the mouse cursor can be on the RGB or HSV palette selector
     * Because this dialog is created by clicking double clicking on a widget, the left mouse
     * button is down, thus creating a not wanted mouse event inside this dialog
     * m_allowMouseEvents is first set to false, and then set to true on the first left mouse
     * clicking inside this dialog to prevent not wanted mouse drag event
     */
    bool m_allowMouseEvents;
    bool m_allowOpacityCtrl;            ///< true to show the widget,
                                        ///< false to keep alpha channel = 1.0
    KIGFX::COLOR4D m_previousColor4D;   ///< the initial color4d
    KIGFX::COLOR4D m_newColor4D;        ///< the current color4d
    KIGFX::COLOR4D m_defaultColor;      ///< The default color4d

    /// the list of color4d ordered by button ID, for predefined colors
    std::vector<KIGFX::COLOR4D> m_Color4DList;
    int m_cursorsSize;

    wxPoint m_cursorBitmapRed;          ///< the red cursor on the RGB bitmap palette.
    wxPoint m_cursorBitmapGreen;        ///< the green cursor on the RGB bitmap palette.
    wxPoint m_cursorBitmapBlue;         ///< the blue cursor on the RGB bitmap palette.
    wxPoint m_cursorBitmapHSV;          ///< the cursor on the HSV bitmap palette.
    wxPoint* m_selectedCursor;          ///< the ref cursor to the selected cursor, if any, or null.

    double m_hue;                       ///< the current hue, in degrees (0 ... 360)
    double m_sat;                       ///< the current saturation (0 ... 1.0)
    double m_val;                       ///< the current value (0 ... 1.0)

    wxBitmap* m_bitmapRGB;              ///< the basic RGB palette
    wxBitmap* m_bitmapHSV;              ///< the basic HUV palette

    std::vector<wxStaticBitmap*> m_colorSwatches;    ///< list of defined colors buttons

    void SetEditVals( CHANGED_COLOR aChanged, bool aCheckTransparency );
	void drawAll();

	void createHSVBitmap();             ///< generate the bitmap that shows the HSV color circle
	void drawHSVPalette();              ///< draws the HSV color circle
    void createRGBBitmap();             ///< generate the bitmap that shows the RVB color space
    void drawRGBPalette();              ///< draws the RVB color space

    ///< repaint a static bitmap with the aColor4D color
    void updatePreview( wxStaticBitmap* aStaticBitmap, KIGFX::COLOR4D& aColor4D );

    ///< Event handler from wxSlider: brightness (value) control
	void OnChangeBrightness( wxScrollEvent& event ) override;

    ///< Event handler from wxSlider: alpha (transparency) control
    void OnChangeAlpha( wxScrollEvent& event ) override;

    ///< Event handlers from wxSpinControl
    void OnChangeEditRed( wxSpinEvent& event ) override;
    void OnChangeEditGreen( wxSpinEvent& event ) override;
    void OnChangeEditBlue( wxSpinEvent& event ) override;
    void OnChangeEditHue( wxSpinEvent& event ) override;
    void OnChangeEditSat( wxSpinEvent& event ) override;

    ///< mouse handlers, when clicking on a palette bitmap
	void onRGBMouseClick( wxMouseEvent& event ) override;
	void onRGBMouseDrag( wxMouseEvent& event ) override;
	void onHSVMouseClick( wxMouseEvent& event ) override;
	void onHSVMouseDrag( wxMouseEvent& event ) override;

    void onSize( wxSizeEvent& event ) override;

    void OnColorValueText( wxCommandEvent& event ) override;

    ///< Event handler for the reset button press
    void OnResetButton( wxCommandEvent& aEvent ) override;

    /**
     * Manage the Hue and Saturation settings when the mouse cursor is at aMouseCursor.
     *
     * @param aMouseCursor is the mouse cursor position on the HSV bitmap
     * @return true if the Hue and Saturation can be set from aMouseCursor,
     * if Saturation value computed from aMouseCursor is <= 1.0,
     * and false if aMouseCursor is outside this area.
     */
	bool setHSvaluesFromCursor( const wxPoint& aMouseCursor );

    ///< Event handler for defined color buttons
    void buttColorClick( wxMouseEvent& event );

    ///< Event handler for double click on color buttons
    void colorDClick( wxMouseEvent& event );

    ///< called when creating the dialog
    bool TransferDataToWindow() override;

    /**
     * Create the bitmap buttons for each defined colors.
     *
     * If aPredefinedColors is nullptr, a internal predefined list will be used.
     */
    void initDefinedColors( CUSTOM_COLORS_LIST* aPredefinedColors );

    void updateHandleSize();

    // convert double value 0 ... 1 to int 0 ... aValMax
    int normalizeToInt( double aValue, int aValMax = 255 )
    {
        return ( aValue * aValMax ) + 0.5;
    }

};

#endif  // #define DIALOG_COLOR_PICKER_H
