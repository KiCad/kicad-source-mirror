/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_LABEL_H
#define SCH_LABEL_H

#include <sch_text.h>
#include <sch_item.h>
#include <sch_field.h>
#include <sch_connection.h>   // for CONNECTION_TYPE

class SCH_RULE_AREA;


/*
 * Spin style for labels of all kinds on schematics.
 *
 * Basically a higher level abstraction of rotation and justification of text.
 */
class SPIN_STYLE
{
public:
    enum SPIN : int
    {
        LEFT   = 0,
        UP     = 1,
        RIGHT  = 2,
        BOTTOM = 3
    };


    SPIN_STYLE() = default;
    constexpr SPIN_STYLE( SPIN aSpin ) : m_spin( aSpin )
    {
    }

    constexpr bool operator==( SPIN a ) const
    {
        return m_spin == a;
    }

    constexpr bool operator!=( SPIN a ) const
    {
        return m_spin != a;
    }

    operator int() const
    {
        return static_cast<int>( m_spin );
    }

    SPIN_STYLE RotateCCW();

    /**
     * Mirror the label spin style across the X axis or simply swaps up and bottom.
     */
    SPIN_STYLE MirrorX();

    /**
     * Mirror the label spin style across the Y axis or simply swaps left and right.
     */
    SPIN_STYLE MirrorY();

    /**
     * Get CCW rotation needed to get to the given spin style.
     */
    unsigned CCWRotationsTo( const SPIN_STYLE& aOther ) const;

private:
    SPIN m_spin;
};


/*
 * Label and flag shapes used with text objects.
 */
enum LABEL_FLAG_SHAPE : unsigned int
{
    L_INPUT,
    L_OUTPUT,
    L_BIDI,
    L_TRISTATE,
    L_UNSPECIFIED,

    F_FIRST,
    F_DOT = F_FIRST,
    F_ROUND,
    F_DIAMOND,
    F_RECTANGLE
};

/*
 * Specific enums for property manager (not used elsewhere)
 */
enum LABEL_SHAPE : unsigned int
{
    LABEL_INPUT    = L_INPUT,
    LABEL_OUTPUT   = L_OUTPUT,
    LABEL_BIDI     = L_BIDI,
    LABEL_TRISTATE = L_TRISTATE,
    LABEL_PASSIVE  = L_UNSPECIFIED
};

enum FLAG_SHAPE : unsigned int
{
    FLAG_DOT       = F_DOT,
    FLAG_CIRCLE    = F_ROUND,
    FLAG_DIAMOND   = F_DIAMOND,
    FLAG_RECTANGLE = F_RECTANGLE
};


class SCH_LABEL_BASE : public SCH_TEXT
{
public:
    SCH_LABEL_BASE( const VECTOR2I& aPos, const wxString& aText, KICAD_T aType );

    SCH_LABEL_BASE( const SCH_LABEL_BASE& aLabel );

    SCH_LABEL_BASE& operator=( const SCH_LABEL_BASE& aLabel );

    // Abstract class
    virtual wxString GetClass() const override = 0;

    bool IsType( const std::vector<KICAD_T>& aScanTypes ) const override;

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        switch( aItem->Type() )
        {
        case SCH_LINE_T:
            return aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS;

        case SCH_BUS_WIRE_ENTRY_T:
            return true;

        case SCH_SYMBOL_T:
            return true;

        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
        case SCH_SHEET_PIN_T:
            return true;

        default:
            return false;
        }
    }

    bool HasConnectivityChanges( const SCH_ITEM* aItem,
                                 const SCH_SHEET_PATH* aInstance = nullptr ) const override;

    // Type-specific versions for property manager
    LABEL_SHAPE GetLabelShape() const        { return (LABEL_SHAPE) m_shape; }
    void        SetLabelShape( LABEL_SHAPE aShape );

    LABEL_FLAG_SHAPE GetShape() const        { return m_shape; }
    void SetShape( LABEL_FLAG_SHAPE aShape )
    {
        // Set flags directly if a flag shape
        if( aShape >= F_FIRST )
            m_shape = aShape;
        else
            SetLabelShape( (LABEL_SHAPE) aShape );
    }

    COLOR4D GetLabelColor() const;

    virtual void SetSpinStyle( SPIN_STYLE aSpinStyle );
    SPIN_STYLE   GetSpinStyle() const;

    void SetLastResolvedState( const SCH_ITEM* aItem ) override
    {
        const SCH_LABEL_BASE* aLabel = dynamic_cast<const SCH_LABEL_BASE*>( aItem );

        if( aLabel )
            m_lastResolvedColor = aLabel->m_lastResolvedColor;
    }

    static const wxString GetDefaultFieldName( const wxString& aName, bool aUseDefaultName );

    /**
     * Return the next ordinal for a user field for this label
     */
    int GetNextFieldOrdinal() const;

    virtual int GetMandatoryFieldCount()              { return 0; }

    std::vector<SCH_FIELD>& GetFields()               { return m_fields; }
    const std::vector<SCH_FIELD>& GetFields() const   { return m_fields; }

    /**
     * Set multiple schematic fields.
     *
     * @param aFields are the fields to set in this symbol.
     */
    void SetFields( const std::vector<SCH_FIELD>& aFields )
    {
        m_fields = aFields;     // vector copying, length is changed possibly
    }

    void AddFields( const std::vector<SCH_FIELD>& aFields )
    {
        m_fields.insert( m_fields.end(), aFields.begin(), aFields.end() );
    }

    void AddField( const SCH_FIELD& aField )
    {
        m_fields.push_back( aField );
    }

    /**
     * Increment the label text if it ends with a number.
     *
     * @param aIncrement = the increment value to add to the number ending the text.
     */
    bool IncrementLabel( int aIncrement );

    void Move( const VECTOR2I& aMoveVector ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;
    void Rotate90( bool aClockwise ) override;

    void MirrorSpinStyle( bool aLeftRight ) override;

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;

    void SetPosition( const VECTOR2I& aPosition ) override;

    void AutoplaceFields( SCH_SCREEN* aScreen, AUTOPLACE_ALGO aAlgo ) override;

    /**
     * Build an array of { pageNumber, pageName } pairs.
     *
     * @param pages [out] Array of { pageNumber, pageName } pairs.
     */
    void GetIntersheetRefs( const SCH_SHEET_PATH* aPath,
                            std::vector<std::pair<wxString, wxString>>* pages );

    /**
     * Return the list of system text vars & fields for this label.
     */
    void GetContextualTextVars( wxArrayString* aVars ) const;

    /**
     * Resolve any references to system tokens supported by the label.
     *
     * @param aDepth a counter to limit recursion and circular references.
     */
    virtual bool ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth ) const;

    wxString GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                           int aDepth = 0 ) const override;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override
    {
        SCHEMATIC* schematic = Schematic();

        if( schematic )
            return GetShownText( &schematic->CurrentSheet(), aAllowExtraText, aDepth );
        else
            return GetText();
    }

    bool HasCachedDriverName() const override;
    const wxString& GetCachedDriverName() const override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction, RECURSE_MODE aMode ) override;

    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& scanTypes ) override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;
    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override;

    VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    /**
     * Calculate the graphic shape (a polygon) associated to the text.
     *
     * @param aPoints A buffer to fill with polygon corners coordinates
     * @param Pos Position of the shape, for texts and labels: do nothing
     */
    virtual void CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                     std::vector<VECTOR2I>& aPoints, const VECTOR2I& Pos ) const
    {
        aPoints.clear();
    }

    int GetLabelBoxExpansion( const RENDER_SETTINGS* aSettings = nullptr ) const;

    /**
     * Return the bounding box of the label only, without taking in account its fields.
     */
    virtual const BOX2I GetBodyBoundingBox( const RENDER_SETTINGS* aSettings ) const;

    /**
     * Return the bounding box of the label including its fields.
     */
    const BOX2I GetBoundingBox() const override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    std::vector<VECTOR2I> GetConnectionPoints() const override;

    void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                              std::vector<DANGLING_END_ITEM>& aItemListByPos,
                              const SCH_SHEET_PATH*           aPath = nullptr ) override;

    bool IsDangling() const override { return m_isDangling; }
    void SetIsDangling( bool aIsDangling ) { m_isDangling = aIsDangling; }

    std::vector<int> ViewGetLayers() const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    /**
     * @brief autoRotateOnPlacement
     * @return true if the label rotation will be automatically set on the placement.
     */
    bool AutoRotateOnPlacement() const;

    /**
     * @param autoRotate If set to true when the label is placed in the connection to a
     * pin/net the direction will be automatically set according to the positioning of the net/pin.
     */
    void SetAutoRotateOnPlacement( bool autoRotate = true );

    /**
     * @return true if the automated rotation of the label is supported after the placement
     * At the moment it is supported for global and hierarchical labels
     */
    virtual bool AutoRotateOnPlacementSupported() const = 0;

    double Similarity( const SCH_ITEM& aItem ) const override;

    bool operator==( const SCH_ITEM& aItem ) const override;

protected:
    void swapData( SCH_ITEM* aItem ) override;

    void cacheShownText() override;

protected:
    std::vector<SCH_FIELD>  m_fields;

    LABEL_FLAG_SHAPE        m_shape;

    CONNECTION_TYPE         m_connectionType;
    bool                    m_isDangling;
    bool                    m_autoRotateOnPlacement;

    mutable COLOR4D         m_lastResolvedColor;

    wxString                m_cached_driver_name;
};


class SCH_LABEL : public SCH_LABEL_BASE
{
public:
    SCH_LABEL( const VECTOR2I& aPos = VECTOR2I( 0, 0 ), const wxString& aText = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_LABEL() { }

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_LABEL" );
    }

    wxString GetFriendlyName() const override
    {
        return _( "Label" );
    }

    const BOX2I GetBodyBoundingBox( const RENDER_SETTINGS* aSettings ) const override;

    bool IsConnectable() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    bool IsReplaceable() const override { return true; }

    EDA_ITEM* Clone() const override
    {
        return new SCH_LABEL( *this );
    }

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    bool AutoRotateOnPlacementSupported() const override { return false; }

private:
    bool doIsConnected( const VECTOR2I& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};


class SCH_DIRECTIVE_LABEL : public SCH_LABEL_BASE
{
public:
    SCH_DIRECTIVE_LABEL( const VECTOR2I& aPos = VECTOR2I( 0, 0 ) );

    SCH_DIRECTIVE_LABEL( const SCH_DIRECTIVE_LABEL& aClassLabel );

    ~SCH_DIRECTIVE_LABEL() { }

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_DIRECTIVE_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_DIRECTIVE_LABEL" );
    }

    wxString GetFriendlyName() const override
    {
        return _( "Directive Label" );
    }

    EDA_ITEM* Clone() const override
    {
        return new SCH_DIRECTIVE_LABEL( *this );
    }

    void swapData( SCH_ITEM* aItem ) override;

    FLAG_SHAPE GetFlagShape() const          { return (FLAG_SHAPE) m_shape; }
    void SetFlagShape( FLAG_SHAPE aShape )   { m_shape = (LABEL_FLAG_SHAPE) aShape; }

    int GetPinLength() const         { return m_pinLength; }
    void SetPinLength( int aLength ) { m_pinLength = aLength; }

    int GetPenWidth() const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                             std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos ) const override;

    void AutoplaceFields( SCH_SCREEN* aScreen, AUTOPLACE_ALGO aAlgo ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    bool IsConnectable() const override { return true; }

    bool AutoRotateOnPlacementSupported() const override { return false; }

    void MirrorSpinStyle( bool aLeftRight ) override;

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;

    /// @brief Adds an entry to the connected rule area cache
    void AddConnectedRuleArea( SCH_RULE_AREA* aRuleArea );

    /// @brief Removes all rule areas from the cache
    void ClearConnectedRuleAreas();

    /// @brief Removes a specific rule area from the cache
    void RemoveConnectedRuleArea( SCH_RULE_AREA* aRuleArea );

    const std::unordered_set<SCH_RULE_AREA*> GetConnectedRuleAreas() const;

    /// @brief Determines dangling state from connectivity and cached connected rule areas
    virtual bool IsDangling() const override;

private:
    int       m_pinLength;
    int       m_symbolSize;

    /// Cache of any rule areas with borders which this label connects to.
    std::unordered_set<SCH_RULE_AREA*> m_connected_rule_areas;
};


class SCH_GLOBALLABEL : public SCH_LABEL_BASE
{
public:
    SCH_GLOBALLABEL( const VECTOR2I& aPos = VECTOR2I( 0, 0 ),
                     const wxString& aText = wxEmptyString );

    SCH_GLOBALLABEL( const SCH_GLOBALLABEL& aGlobalLabel );

    ~SCH_GLOBALLABEL() { }

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_GLOBAL_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_GLOBALLABEL" );
    }

    wxString GetFriendlyName() const override
    {
        return _( "Global Label" );
    }

    EDA_ITEM* Clone() const override
    {
        return new SCH_GLOBALLABEL( *this );
    }

    int GetMandatoryFieldCount() override { return 1; }

    /**
     * Return a mandatory field in this label.  The const version will return nullptr if the
     * field doesn't exist; the non-const version will create it.
     */
    SCH_FIELD* GetField( FIELD_T aFieldType );
    const SCH_FIELD* GetField( FIELD_T aFieldNdx ) const;

    void SetSpinStyle( SPIN_STYLE aSpinStyle ) override;

    VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aRenderSettings,
                             std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos ) const override;

    bool ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth ) const override;

    bool IsConnectable() const override { return true; }

    std::vector<int> ViewGetLayers() const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    bool AutoRotateOnPlacementSupported() const override { return true; }

private:
    bool doIsConnected( const VECTOR2I& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};


class SCH_HIERLABEL : public SCH_LABEL_BASE
{
public:
    SCH_HIERLABEL( const VECTOR2I& aPos = VECTOR2I( 0, 0 ), const wxString& aText = wxEmptyString,
                   KICAD_T aType = SCH_HIER_LABEL_T );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_HIERLABEL() override = default;

    void Serialize( google::protobuf::Any &aContainer ) const override;
    bool Deserialize( const google::protobuf::Any &aContainer ) override;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_HIER_LABEL_T == aItem->Type();
    }

    wxString GetFriendlyName() const override
    {
        return _( "Hierarchical Label" );
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_HIERLABEL" );
    }

    void SetSpinStyle( SPIN_STYLE aSpinStyle ) override;

    VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos ) const override;
    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos, LABEL_FLAG_SHAPE aShape ) const;

    const BOX2I GetBodyBoundingBox( const RENDER_SETTINGS* aSettings ) const override;

    bool IsConnectable() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_HIERLABEL( *this );
    }

    bool IsPointClickableAnchor( const VECTOR2I& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

    bool AutoRotateOnPlacementSupported() const override { return true; }

private:
    bool doIsConnected( const VECTOR2I& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};

#endif /* SCH_LABEL_H */
