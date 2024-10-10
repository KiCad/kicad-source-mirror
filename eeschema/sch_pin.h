/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2018 CERN
 * Copyright (C) 2004-2024 KiCad Developers, see AUTHOR.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>

#include <pin_type.h>
#include <sch_item.h>
#include <symbol.h>

class LIB_SYMBOL;
class SCH_SYMBOL;
class PIN_LAYOUT_CACHE;

// Circle diameter drawn at the active end of pins:
#define TARGET_PIN_RADIUS   schIUScale.MilsToIU( 15 )


class SCH_PIN : public SCH_ITEM
{
public:
    struct ALT
    {
        wxString            m_Name;
        GRAPHIC_PINSHAPE    m_Shape;         // Shape drawn around pin
        ELECTRICAL_PINTYPE  m_Type;          // Electrical type of the pin.
    };

    SCH_PIN( LIB_SYMBOL* aParentSymbol );

    SCH_PIN( LIB_SYMBOL* aParentSymbol, const wxString& aName, const wxString& aNumber,
             PIN_ORIENTATION aOrientation, ELECTRICAL_PINTYPE aPinType, int aLength,
             int aNameTextSize, int aNumTextSize, int aBodyStyle, const VECTOR2I& aPos, int aUnit );

    SCH_PIN( SCH_SYMBOL* aParentSymbol, SCH_PIN* aLibPin );

    SCH_PIN( SCH_SYMBOL* aParentSymbol, const wxString& aNumber, const wxString& aAlt,
             const KIID& aUuid );

    SCH_PIN( const SCH_PIN& aPin );

    ~SCH_PIN();

    SCH_PIN& operator=( const SCH_PIN& aPin );

    wxString GetClass() const override
    {
        return wxT( "SCH_PIN" );
    }

    static bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == SCH_PIN_T;
    }

    wxString GetFriendlyName() const override
    {
        return _( "Pin" );
    }

    SCH_PIN* GetLibPin() const { return m_libPin; }
    void SetLibPin( SCH_PIN* aLibPin ) { m_libPin = aLibPin; }

    PIN_ORIENTATION GetOrientation() const;
    void SetOrientation( PIN_ORIENTATION aOrientation ) { m_orientation = aOrientation; }

    GRAPHIC_PINSHAPE GetShape() const;
    void SetShape( GRAPHIC_PINSHAPE aShape ) { m_shape = aShape; }

    int GetLength() const;
    void SetLength( int aLength ) { m_length = aLength; }

    /**
     * Change the length of a pin and adjust its position based on orientation.
     *
     * @param aLength New length of pin
     */
    void ChangeLength( int aLength );

    ELECTRICAL_PINTYPE GetType() const;
    void SetType( ELECTRICAL_PINTYPE aType ) { m_type = aType; }
    wxString GetCanonicalElectricalTypeName() const;
    wxString GetElectricalTypeName() const;

    bool IsVisible() const;
    void SetVisible( bool aVisible ) { m_hidden = !aVisible; }

    const wxString& GetName() const;
    wxString GetShownName() const;
    void SetName( const wxString& aName );
    /**
     * Get the name without any alternates
     */
    const wxString& GetBaseName() const;

    const wxString& GetNumber() const { return m_number; }
    wxString GetShownNumber() const;
    void SetNumber( const wxString& aNumber );

    int GetNameTextSize() const;
    void SetNameTextSize( int aSize );

    int GetNumberTextSize() const;
    void SetNumberTextSize( int aSize );

    const std::map<wxString, ALT>& GetAlternates() const
    {
        if( m_libPin )
            return m_libPin->GetAlternates();

        return m_alternates;
    }

    std::map<wxString, ALT>& GetAlternates()
    {
        return const_cast<std::map<wxString, ALT>&>(
                static_cast<const SCH_PIN*>( this )->GetAlternates() );
    }

    ALT GetAlt( const wxString& aAlt )
    {
        return GetAlternates()[ aAlt ];
    }

    wxString GetAlt() const { return m_alt; }
    void SetAlt( const wxString& aAlt ) { m_alt = aAlt; }

    void Print( const SCH_RENDER_SETTINGS* aSettings, int aUnit, int aBodyStyle,
                const VECTOR2I& aOffset, bool aForceNoFill, bool aDimmed ) override;

    /**
     * Return the pin real orientation (PIN_UP, PIN_DOWN, PIN_RIGHT, PIN_LEFT),
     * according to its orientation and the matrix transform (rot, mirror) \a aTransform.
     *
     * @param aTransform Transform matrix
     */
    PIN_ORIENTATION PinDrawOrient( const TRANSFORM& aTransform ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    const BOX2I ViewBBox() const override;

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    /* Cannot use a default parameter here as it will not be compatible with the virtual. */
    const BOX2I GetBoundingBox() const override
    {
        return GetBoundingBox( false, true, m_flags & SHOW_ELEC_TYPE );
    }

    /**
     * @param aIncludeLabelsOnInvisiblePins - if false, do not include labels for invisible pins
     *                                       in the calculation.
     */
    BOX2I GetBoundingBox( bool aIncludeLabelsOnInvisiblePins, bool aIncludeNameAndNumber,
                          bool aIncludeElectricalType ) const;

    /**
     * Return whether this pin forms a global power connection: i.e., is part of a power symbol
     * and of type POWER_IN, or is a legacy invisible global power pin on a symbol.
     */
    bool IsGlobalPower() const
    {
        return GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN
               && ( !IsVisible() || GetParentSymbol()->IsPower() );
    }

    int GetPenWidth() const override { return 0; }

    void Move( const VECTOR2I& aOffset ) override;

    VECTOR2I GetPosition() const override;
    VECTOR2I GetLocalPosition() const { return m_position; }
    void SetPosition( const VECTOR2I& aPos ) override { m_position = aPos; }

    // For properties system
    int GetX() const { return m_position.x; }
    void SetX( int aX ) { m_position.x = aX; }
    int GetY() const { return m_position.y; }
    void SetY( int aY ) { m_position.y = aY; }

    VECTOR2I GetPinRoot() const;

    /**
     * these transforms have effect only if the pin has a LIB_SYMBOL as parent
     */
    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW = true ) override;

    /**
     * these transforms have always effects
     */
    void MirrorHorizontallyPin( int aCenter );
    void MirrorVerticallyPin( int aCenter );
    void RotatePin( const VECTOR2I& aCenter, bool aRotateCCW = true );

    /**
     * Plot the pin name and number.
     * @param aTextInside - draw the names & numbers inside the symbol body (ie: in the opposite
     *                      direction of \a aPinOrient).
     */
    void PlotPinTexts( PLOTTER *aPlotter, const VECTOR2I &aPinPos, PIN_ORIENTATION aPinOrient,
                       int aTextInside, bool aDrawPinNum, bool aDrawPinName, bool aDimmed ) const;

    void PlotPinType( PLOTTER *aPlotter, const VECTOR2I &aPosition, PIN_ORIENTATION aOrientation,
                      bool aDimmed ) const;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    BITMAPS GetMenuImage() const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;
    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, ALT* aAlt ) const;

    EDA_ITEM* Clone() const override;

    void CalcEdit( const VECTOR2I& aPosition ) override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override;

    /**
     * Return a string giving the electrical type of a pin.
     *
     * Can be used when a known, not translated name is needed (for instance in net lists)
     *
     * @param aType is the electrical type (see enum ELECTRICAL_PINTYPE )
     * @return The electrical name for a pin type (see enun MsgPinElectricType for names).
     */
    static wxString GetCanonicalElectricalTypeName( ELECTRICAL_PINTYPE aType );

    bool IsConnectable() const override { return true; }

    bool HasConnectivityChanges( const SCH_ITEM* aItem,
                                 const SCH_SHEET_PATH* aInstance = nullptr ) const override;

    void ClearDefaultNetName( const SCH_SHEET_PATH* aPath );
    wxString GetDefaultNetName( const SCH_SHEET_PATH& aPath, bool aForceNoConnect = false );

    bool IsDangling() const override
    {
        if( GetType() == ELECTRICAL_PINTYPE::PT_NC || GetType() == ELECTRICAL_PINTYPE::PT_NIC )
            return false;

        return m_isDangling;
    }

    void SetIsDangling( bool isDangling ) { m_isDangling = isDangling; }

    /**
     * @param aPin Comparison Pin
     * @return True if aPin is stacked with this pin
     */
    bool IsStacked( const SCH_PIN* aPin ) const;

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    bool ConnectionPropagatesTo( const EDA_ITEM* aItem ) const override;

    const wxString& GetOperatingPoint() const { return m_operatingPoint; }
    void SetOperatingPoint( const wxString& aText ) { m_operatingPoint = aText; }

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator>( const SCH_ITEM& aRhs ) const { return compare( aRhs, EQUALITY ) > 0; }

    /**
     * Get the layout cache associated with this pin.
     *
     * If you need more information about how elements of the pin are physically
     * laid out than just the bounding box, you can use this. The SCH_PAINTER,
     * for example, can use this to avoid having to duplicate text extent calcs.
     */
    PIN_LAYOUT_CACHE& GetLayoutCache() const { return *m_layoutCache; }

protected:
    wxString getItemDescription( ALT* aAlt ) const;

    struct EXTENTS_CACHE
    {
        KIFONT::FONT* m_Font = nullptr;
        int           m_FontSize = 0;
        VECTOR2I      m_Extents;
    };

    void validateExtentsCache( KIFONT::FONT* aFont, int aSize, const wxString& aText,
                               EXTENTS_CACHE* aCache ) const;

    /**
     * Print the pin symbol without text.
     * If \a aColor != 0, draw with \a aColor, else with the normal pin color.
     */
    void printPinSymbol( const SCH_RENDER_SETTINGS *aSettings, const VECTOR2I &aPos,
                         PIN_ORIENTATION aOrientation, bool aDimmed );

    /**
     * Put the pin number and pin text info, given the pin line coordinates.
     * The line must be vertical or horizontal.
     * If aDrawPinName == false the pin name is not printed.
     * If aDrawPinNum = false the pin number is not printed.
     * If aTextInside then the text is been put inside,otherwise all is drawn outside.
     * Pin Name:    substring between '~' is negated
     */
    void printPinTexts( const RENDER_SETTINGS* aSettings, const VECTOR2I& aPinPos,
                        PIN_ORIENTATION aPinOrient, int aTextInside, bool aDrawPinNum,
                        bool aDrawPinName, bool aDimmed );

    /**
     * Draw the electrical type text of the pin (only for the footprint editor)
     */
    void printPinElectricalTypeName( const RENDER_SETTINGS* aSettings, const VECTOR2I& aPosition,
                                     PIN_ORIENTATION aOrientation, bool aDimmed );
    std::ostream& operator<<( std::ostream& aStream );

private:
    /**
     * The pin specific sort order is as follows:
     *      - The result of #SCH_ITEM::compare()
     *      - Number
     *      - Name, case sensitive compare
     *      - Horizontal (X) position
     *      - Vertical (Y) position
     *      - Length
     *      - Orientation
     *      - Shape
     *      - Electrical type
     *      - Visibility (true > false)
     *      - Number text size
     *      - Name text size
     *      - Alternates, name, type shape
     */
    int compare( const SCH_ITEM& aOther, int aCompareFlags = 0 ) const override;

protected:
    SCH_PIN*                m_libPin;          // The corresponding pin in the LIB_SYMBOL
                                               //   (nullptr for a pin *in* the LIB_SYMBOL)

    std::map<wxString, ALT> m_alternates;      // Map of alternate name to ALT structure
                                               //   (only valid for pins in LIB_SYMBOLS)

    VECTOR2I                m_position;        // Position of the pin.
    std::optional<int>      m_length;          // Length of the pin.
    PIN_ORIENTATION         m_orientation;     // Pin orientation (Up, Down, Left, Right)
    GRAPHIC_PINSHAPE        m_shape;           // Shape drawn around pin
    ELECTRICAL_PINTYPE      m_type;            // Electrical type of the pin.
    std::optional<bool>     m_hidden;
    wxString                m_name;
    wxString                m_number;
    std::optional<int>      m_numTextSize;     // Pin num and Pin name sizes
    std::optional<int>      m_nameTextSize;
    wxString                m_alt;             // The current alternate for an instance

    wxString                m_operatingPoint;

    bool                    m_isDangling;

    /**
     * The layout cache for this pin.
     * SCH_PIN doesn't *have* to own this, it could be part a central cache.
     */
    mutable std::unique_ptr<PIN_LAYOUT_CACHE> m_layoutCache;

    /// The name that this pin connection will drive onto a net.
    std::recursive_mutex                                      m_netmap_mutex;
    std::map<const SCH_SHEET_PATH, std::pair<wxString, bool>> m_net_name_map;
};
