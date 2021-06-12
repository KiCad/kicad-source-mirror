/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef CLASS_PIN_H
#define CLASS_PIN_H

#include <eda_rect.h>
#include <lib_item.h>
#include <pin_type.h>
#include <lib_symbol.h>

// Circle diameter drawn at the active end of pins:
#define TARGET_PIN_RADIUS   Mils2iu( 15 )

// Pin visibility flag bit:
#define PIN_INVISIBLE 1    // Set makes pin invisible


/**
 *  The component library pin object orientations.
 */
enum DrawPinOrient {
    PIN_RIGHT = 'R',
    PIN_LEFT  = 'L',
    PIN_UP    = 'U',
    PIN_DOWN  = 'D'
};


class LIB_PIN : public LIB_ITEM
{
public:
    struct ALT
    {
        wxString            m_Name;
        GRAPHIC_PINSHAPE    m_Shape;         // Shape drawn around pin
        ELECTRICAL_PINTYPE  m_Type;          // Electrical type of the pin.
    };

    ~LIB_PIN() { }

    wxString GetClass() const override
    {
        return wxT( "LIB_PIN" );
    }

    wxString GetTypeName() const override
    {
        return _( "Pin" );
    }

    int GetOrientation() const { return m_orientation; }
    void SetOrientation( int aOrientation ) { m_orientation = aOrientation; }

    GRAPHIC_PINSHAPE GetShape() const { return m_shape; }
    void SetShape( GRAPHIC_PINSHAPE aShape ) { m_shape = aShape; }

    int GetLength() const { return m_length; }
    void SetLength( int aLength ) { m_length = aLength; }

    ELECTRICAL_PINTYPE GetType() const { return m_type; }
    void SetType( ELECTRICAL_PINTYPE aType ) { m_type = aType; }

    wxString const GetCanonicalElectricalTypeName() const
    {
        return GetCanonicalElectricalTypeName( m_type );
    }

    wxString const GetElectricalTypeName() const
    {
        return ElectricalPinTypeGetText( m_type );
    }

    bool IsVisible() const { return ( m_attributes & PIN_INVISIBLE ) == 0; }
    void SetVisible( bool aVisible )
    {
        if( aVisible )
            m_attributes &= ~PIN_INVISIBLE;
        else
            m_attributes |= PIN_INVISIBLE;
    }

    const wxString& GetName() const { return m_name; }
    wxString GetShownName() const;
    void SetName( const wxString& aName )
    {
        m_name = aName;

        // pin name string does not support spaces
        m_name.Replace( wxT( " " ), wxT( "_" ) );
    }

    const wxString& GetNumber() const { return m_number; }
    wxString GetShownNumber() const { return m_number; }
    void SetNumber( const wxString& aNumber )
    {
        m_number = aNumber;

        // pin number string does not support spaces
        m_number.Replace( wxT( " " ), wxT( "_" ) );
    }

    int GetNameTextSize() const { return m_nameTextSize; }
    void SetNameTextSize( int aSize ) { m_nameTextSize = aSize; }

    int GetNumberTextSize() const { return m_numTextSize; }
    void SetNumberTextSize( int aSize ) { m_numTextSize = aSize; }

    std::map<wxString, ALT>& GetAlternates() { return m_alternates; }

    ALT GetAlt( const wxString& aAlt ) { return m_alternates[ aAlt ]; }

    /**
     * Print a pin, with or without the pin texts
     *
     * @param aOffset Offset to draw
     * @param aData = used here as a boolean indicating whether or not to draw the pin
     *                electrical types
     * @param aTransform Transform Matrix (rotation, mirror ..)
     */
    void print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset, void* aData,
                const TRANSFORM& aTransform ) override;

    /**
     * Return the pin real orientation (PIN_UP, PIN_DOWN, PIN_RIGHT, PIN_LEFT),
     * according to its orientation and the matrix transform (rot, mirror) \a aTransform.
     *
     * @param aTransform Transform matrix
     */
    int PinDrawOrient( const TRANSFORM& aTransform ) const;

    LIB_PIN( LIB_PART* aParent );

    LIB_PIN( LIB_PART* aParent, const wxString& aName, const wxString& aNumber, int aOrientation,
             ELECTRICAL_PINTYPE aPinType, int aLength, int aNameTextSize, int aNumTextSize,
             int aConvert, const wxPoint& aPos, int aUnit );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;

    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /* Cannot use a default parameter here as it will not be compatible with the virtual. */
    const EDA_RECT GetBoundingBox() const override { return GetBoundingBox( false ); }

    /**
     * @param aIncludeInvisibles - if false, do not include labels for invisible pins
     *      in the calculation.
     */
    const EDA_RECT GetBoundingBox( bool aIncludeInvisibles, bool aPinOnly = false ) const;

    /**
     * Return whether this pin forms an implicit power connection: i.e., is hidden
     * and of type POWER_IN.
     */
    bool IsPowerConnection() const
    {
        return GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN
               && ( !IsVisible() || (LIB_PART*) GetParent()->IsPower() );
    }

    int GetPenWidth() const override;

    /**
     * Plot the pin number and pin text info, given the pin line coordinates.
     * Same as DrawPinTexts((), but output is the plotter
     * The line must be vertical or horizontal.
     * If TextInside then the text is been put inside (moving from x1, y1 in
     * the opposite direction to x2,y2), otherwise all is drawn outside.
     */
    void PlotPinTexts( PLOTTER *aPlotter, const wxPoint& aPinPos, int aPinOrient,
                       int aTextInside, bool aDrawPinNum, bool aDrawPinName ) const;

    void PlotSymbol( PLOTTER* aPlotter, const wxPoint& aPosition, int aOrientation ) const;

    void Offset( const wxPoint& aOffset ) override;

    void MoveTo( const wxPoint& aNewPosition ) override;

    wxPoint GetPosition() const override { return m_position; }
    void SetPosition( const wxPoint& aPos ) override { m_position = aPos; }

    void MirrorHorizontal( const wxPoint& aCenter ) override;
    void MirrorVertical( const wxPoint& aCenter ) override;
    void Rotate( const wxPoint& aCenter, bool aRotateCCW = true ) override;

    void Plot( PLOTTER* aPlotter, const wxPoint& aPffset, bool aFill,
               const TRANSFORM& aTransform ) const override;

    // Get/SetWidth() not used for pins.  Use GetPenWidth() for drawing.
    int GetWidth() const override { return 1; }
    void SetWidth( int aWidth ) override { };

    BITMAPS GetMenuImage() const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    EDA_ITEM* Clone() const override;

    void CalcEdit( const wxPoint& aPosition ) override;

    /**
     * Return a string giving the electrical type of a pin.
     *
     * Can be used when a known, not translated name is needed (for instance in net lists)
     *
     * @param aType is the electrical type (see enum ELECTRICAL_PINTYPE )
     * @return The electrical name for a pin type (see enun MsgPinElectricType for names).
     */
    static const wxString GetCanonicalElectricalTypeName( ELECTRICAL_PINTYPE aType );

protected:
    /**
     * Print the pin symbol without text.
     * If \a aColor != 0, draw with \a aColor, else with the normal pin color.
     */
    void printPinSymbol( const RENDER_SETTINGS* aSettings, const wxPoint& aPos, int aOrientation );

    /**
     * Put the pin number and pin text info, given the pin line coordinates.
     * The line must be vertical or horizontal.
     * If aDrawPinName == false the pin name is not printed.
     * If aDrawPinNum = false the pin number is not printed.
     * If aTextInside then the text is been put inside,otherwise all is drawn outside.
     * Pin Name:    substring between '~' is negated
     */
    void printPinTexts( const RENDER_SETTINGS* aSettings, wxPoint& aPinPos, int aPinOrient,
                        int aTextInside, bool aDrawPinNum, bool aDrawPinName );

    /**
     * Draw the electrical type text of the pin (only for the footprint editor)
     */
    void printPinElectricalTypeName( const RENDER_SETTINGS* aSettings, wxPoint& aPosition,
                                     int aOrientation );

private:
    /**
     * @copydoc LIB_ITEM::compare()
     *
     * The pin specific sort order is as follows:
     *      - Pin number.
     *      - Pin name, case insensitive compare.
     *      - Pin horizontal (X) position.
     *      - Pin vertical (Y) position.
     */
    int compare( const LIB_ITEM& aOther,
            LIB_ITEM::COMPARE_FLAGS aCompareFlags = LIB_ITEM::COMPARE_FLAGS::NORMAL ) const override;

protected:
    wxPoint                 m_position;      // Position of the pin.
    int                     m_length;        // Length of the pin.
    int                     m_orientation;   // Pin orientation (Up, Down, Left, Right)
    GRAPHIC_PINSHAPE        m_shape;         // Shape drawn around pin
    ELECTRICAL_PINTYPE      m_type;          // Electrical type of the pin.
    int                     m_attributes;    // Set bit 0 to indicate pin is invisible.
    wxString                m_name;
    wxString                m_number;
    int                     m_numTextSize;   // Pin num and Pin name sizes
    int                     m_nameTextSize;

    std::map<wxString, ALT> m_alternates;    // Map of alternate name to ALT structure
};


#endif  //  CLASS_PIN_H
