/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jaen-pierre.charras at wanadoo.fr
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHOR.txt for contributors.
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
#include <vector>

#include <pin_type.h>
#include <sch_item.h>

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

    /**
     * Create a proxy pin from an alternate pin designation.
     *
     * The #SCH_PIN data will be filled in when the pin is resolved.
     *
     * @see SCH_SYMBOL::UpdatePins().
     */
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
    void SetType( ELECTRICAL_PINTYPE aType );
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
    std::vector<wxString> GetStackedPinNumbers( bool* aValid = nullptr ) const;

    /**
     * Return the count of logical pins represented by this pin's stacked notation.
     *
     * This is a fast alternative to GetStackedPinNumbers().size() that avoids
     * allocating and populating a vector of strings.
     *
     * @param aValid Optional pointer to bool that will be set to indicate if the
     *               stacked notation is valid (true) or malformed (false).
     * @return The number of logical pins represented (always >= 1).
     */
    int GetStackedPinCount( bool* aValid = nullptr ) const;

    /**
     * Return the smallest logical pin number if this pin uses stacked
     * notation and it is valid.  Otherwise returns std::nullopt.
     */
    std::optional<wxString> GetSmallestLogicalNumber() const;

    /**
     * Return the pin number to be used for deterministic operations such as
     * auto‑generated net names.  For stacked pins this is the smallest logical
     * number; otherwise it is the shown number.
     */
    wxString GetEffectivePadNumber() const;
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

    /**
     * Set the name of the alternate pin.
     *
     * @note If the alternate pin is the same as the default pin name or does not exist in the
     *       list of pin alternates, it's set to an empty string which results in the alternate
     *       being set to the default pin.
     *
     * @param is the name of the pin alternate in #m_alternates.
     */
    void SetAlt( const wxString& aAlt );

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

    std::vector<int> ViewGetLayers() const override;

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
    bool IsGlobalPower() const;

    /**
     * Local power pin is the same except that it is sheet-local and it does not support the legacy
     * hidden pin mode
     */
    bool IsLocalPower() const;

    /**
     * Check if the pin is _either_ a global or local power pin.
     * @see IsGlobalPower() and IsLocalPower()
     */
    bool IsPower() const;

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
     * These transforms have effect only if the pin has a #LIB_SYMBOL as parent.
     */
    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW = true ) override;

    /**
     * These transforms have always effects.
     */
    void MirrorHorizontallyPin( int aCenter );
    void MirrorVerticallyPin( int aCenter );
    void RotatePin( const VECTOR2I& aCenter, bool aRotateCCW = true );

    /**
     * Plot the pin name and number.
     *
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

    bool IsDangling() const override;
    void SetIsDangling( bool aIsDangling );

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
     *
     * #SCH_PIN doesn't *have* to own this, it could be part a central cache.
     */
    mutable std::unique_ptr<PIN_LAYOUT_CACHE> m_layoutCache;

    /// The name that this pin connection will drive onto a net.
    std::recursive_mutex                                      m_netmap_mutex;
    std::map<const SCH_SHEET_PATH, std::pair<wxString, bool>> m_net_name_map;
};
