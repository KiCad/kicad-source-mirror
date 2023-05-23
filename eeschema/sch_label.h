/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


class SCH_LABEL_BASE : public SCH_TEXT
{
public:
    SCH_LABEL_BASE( const VECTOR2I& aPos, const wxString& aText, KICAD_T aType );

    SCH_LABEL_BASE( const SCH_LABEL_BASE& aLabel );

    // Abstract class
    virtual wxString GetClass() const override = 0;

    bool IsType( const std::vector<KICAD_T>& aScanTypes ) const override;

    void SwapData( SCH_ITEM* aItem ) override;

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

    LABEL_FLAG_SHAPE GetShape() const override        { return m_shape; }
    void SetShape( LABEL_FLAG_SHAPE aShape ) override { m_shape = aShape; }

    COLOR4D GetLabelColor() const;

    void SetLastResolvedState( const SCH_ITEM* aItem ) override
    {
        const SCH_LABEL_BASE* aLabel = dynamic_cast<const SCH_LABEL_BASE*>( aItem );

        if( aLabel )
            m_lastResolvedColor = aLabel->m_lastResolvedColor;
    }

    static const wxString GetDefaultFieldName( const wxString& aName, bool aUseDefaultName );

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
     * Increment the label text, if it ends with a number.
     *
     * @param aIncrement = the increment value to add to the number ending the text.
     */
    bool IncrementLabel( int aIncrement );

    void Move( const VECTOR2I& aMoveVector ) override;
    void Rotate( const VECTOR2I& aCenter ) override;
    void Rotate90( bool aClockwise ) override;

    void MirrorSpinStyle( bool aLeftRight ) override;

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;

    void SetPosition( const VECTOR2I& aPosition ) override;

    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    /**
     * Builds an array of { pageNumber, pageName } pairs.
     * @param pages [out] Array of { pageNumber, pageName } pairs.
     */
    void GetIntersheetRefs( std::vector<std::pair<wxString, wxString>>* pages );

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
        return GetShownText( nullptr, aAllowExtraText, aDepth );
    }

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction ) override;

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
    virtual const BOX2I GetBodyBoundingBox() const;

    /**
     * Return the bounding box of the label including its fields.
     */
    const BOX2I GetBoundingBox() const override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    std::vector<VECTOR2I> GetConnectionPoints() const override;

    void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                              const SCH_SHEET_PATH* aPath = nullptr ) override;

    bool IsDangling() const override { return m_isDangling; }
    void SetIsDangling( bool aIsDangling ) { m_isDangling = aIsDangling; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    void Plot( PLOTTER* aPlotter, bool aBackground ) const override;

    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& offset ) override;

    /**
     * @brief autoRotateOnPlacement
     * @return Returns true if the label rotation will be automatically set on the placement
     */
    bool AutoRotateOnPlacement() const;

    /**
     * @brief setAutoRotateOnPlacement
     * @param autoRotate If set to true when the label is placed in the connection to a
     * pin/net the direction will be automatically set according to the positioning of the net/pin
     */
    void SetAutoRotateOnPlacement( bool autoRotate = true );

    /**
     * @brief AutoRotateOnPlacementSupported
     * @return true if the automated rotation of the label is supported after the placement
     * At the moment it is supported for global and hierarchial labels
     */
    virtual bool AutoRotateOnPlacementSupported() const = 0;

protected:
    std::vector<SCH_FIELD>  m_fields;

    LABEL_FLAG_SHAPE        m_shape;

    CONNECTION_TYPE         m_connectionType;
    bool                    m_isDangling;
    bool                    m_autoRotateOnPlacement = false;

    mutable COLOR4D         m_lastResolvedColor;
};


class SCH_LABEL : public SCH_LABEL_BASE
{
public:
    SCH_LABEL( const VECTOR2I& aPos = VECTOR2I( 0, 0 ), const wxString& aText = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_LABEL() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_LABEL" );
    }

    const BOX2I GetBodyBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

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

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_DIRECTIVE_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_DIRECTIVE_LABEL" );
    }

    EDA_ITEM* Clone() const override
    {
        return new SCH_DIRECTIVE_LABEL( *this );
    }

    void SwapData( SCH_ITEM* aItem ) override;

    int GetPinLength() const { return m_pinLength; }
    void SetPinLength( int aLength ) { m_pinLength = aLength; }

    int GetPenWidth() const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos ) const override;

    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

    bool IsConnectable() const override { return true; }

    bool AutoRotateOnPlacementSupported() const override { return false; }

    void MirrorSpinStyle( bool aLeftRight ) override;

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;

private:
    int       m_pinLength;
    int       m_symbolSize;
};


class SCH_GLOBALLABEL : public SCH_LABEL_BASE
{
public:
    SCH_GLOBALLABEL( const VECTOR2I& aPos = VECTOR2I( 0, 0 ), const wxString& aText = wxEmptyString );

    SCH_GLOBALLABEL( const SCH_GLOBALLABEL& aGlobalLabel );

    ~SCH_GLOBALLABEL() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_GLOBAL_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_GLOBALLABEL" );
    }

    EDA_ITEM* Clone() const override
    {
        return new SCH_GLOBALLABEL( *this );
    }

    int GetMandatoryFieldCount() override { return 1; }

    void SetTextSpinStyle( TEXT_SPIN_STYLE aSpinStyle ) override;

    VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aRenderSettings, std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos ) const override;

    bool ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth ) const override;

    bool IsConnectable() const override { return true; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

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

    ~SCH_HIERLABEL() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_HIER_LABEL_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_HIERLABEL" );
    }

    void SetTextSpinStyle( TEXT_SPIN_STYLE aSpinStyle ) override;

    VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos ) const override;
    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<VECTOR2I>& aPoints,
                             const VECTOR2I& aPos, LABEL_FLAG_SHAPE aShape ) const;

    const BOX2I GetBodyBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

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
