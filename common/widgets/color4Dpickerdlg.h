/**
 * @file color4Dpickerdlg.h
 */

#ifndef COLOR4DPICKERDLG_H
#define COLOR4DPICKERDLG_H


#include <gal/color4d.h>
#include "color4Dpickerdlg_base.h"

enum CHANGED_COLOR
{
    ALL_CHANGED,
    RED_CHANGED,
    BLUE_CHANGED,
    GREEN_CHANGED,
    HUE_CHANGED,
    SAT_CHANGED,
    VAL_CHANGED,
};

class COLOR4D_PICKER_DLG : public COLOR4D_PICKER_DLG_BASE
{
public:
	COLOR4D_PICKER_DLG( wxWindow* aParent, KIGFX::COLOR4D& aCurrentColor );
	~COLOR4D_PICKER_DLG();

	KIGFX::COLOR4D GetColor() { return m_newColor4D; };

    static int m_ActivePage;            ///< the active notebook page, stored during a session

private:
    KIGFX::COLOR4D m_previousColor4D;   ///< the inital color4d
    KIGFX::COLOR4D m_newColor4D;        ///< the current color4d
    int m_cursorsSize;

    wxPoint m_cursorBitmapRed;          ///< the red cursor on the RGB bitmap palette.
    wxPoint m_cursorBitmapGreen;        ///< the green cursor on the RGB bitmap palette.
    wxPoint m_cursorBitmapBlue;         ///< the blue cursor on the RGB bitmap palette.
    wxPoint m_cursorBitmapHSV;          ///< the cursor on the HSV bitmap palette.
    wxPoint* m_selectedCursor;          ///< the ref cursor to the selected curor, if any, or null.

    double m_hue;                       ///< the current hue, in degrees (0 ... 360)
    double m_sat;                       ///< the current saturation (0 ... 1.0)
    double m_val;                       ///< the current value (0 ... 1.0)

    wxBitmap* m_bitmapRGB;              ///< the basic RGB palette
    wxBitmap* m_bitmapHSV;              ///< the basic HUV palette

    std::vector<wxBitmapButton*> m_buttonsColor;    ///< list of defined colors buttons

    void SetEditVals( CHANGED_COLOR aChanged );
	void drawAll();

	void createHSVBitmap();             ///< generate the bitmap that shows the HSV color circle
	void drawHSVPalette();              ///< draws the HSV color circle
    void createRGBBitmap();             ///< generate the bitmap that shows the RVB color space
    void drawRGBPalette();              ///< draws the RVB color space
    void drawRGBCursors();

    ///> repaint a static bitmap with the aColor4D color
    void setIconColor( wxStaticBitmap* aStaticBitmap, KIGFX::COLOR4D& aColor4D );

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

    ///> mouse handlers, when clicking on a palette bitmap
	void onRGBMouseClick( wxMouseEvent& event ) override;
	void onRGBMouseDrag( wxMouseEvent& event ) override;
	void onHSVMouseClick( wxMouseEvent& event ) override;
	void onHSVMouseDrag( wxMouseEvent& event ) override;

    ///> Event handler for defined color buttons
    void buttColorClick( wxCommandEvent& event );

    ///> called when creating the dialog
    bool TransferDataToWindow() override;

    ///> creates the bitmap buttons for each defined colors
    void initDefinedColors();

    // convert double value 0 ... 1 to int 0 ... aValMax
    int normalizeToInt( double aValue, int aValMax = 255 )
    {
        return ( aValue * aValMax ) + 0.5;
    }

};

#endif  // #define COLOR4DPICKERDLG_H
