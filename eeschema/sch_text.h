/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CLASS_TEXT_LABEL_H
#define CLASS_TEXT_LABEL_H


#include <eda_text.h>
#include <sch_item.h>
#include <sch_field.h>
#include <sch_connection.h>   // for CONNECTION_TYPE


class NETLIST_OBJECT_LIST;
class HTML_MESSAGE_BOX;

/*
 * Spin style for text items of all kinds on schematics
 * Basically a higher level abstraction of rotation and justification of text
 */
class LABEL_SPIN_STYLE
{
public:
    enum SPIN : int
    {
        LEFT   = 0,
        UP     = 1,
        RIGHT  = 2,
        BOTTOM = 3
    };


    LABEL_SPIN_STYLE() = default;
    constexpr LABEL_SPIN_STYLE( SPIN aSpin ) : m_spin( aSpin )
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

    LABEL_SPIN_STYLE RotateCW();

    LABEL_SPIN_STYLE RotateCCW();

    /**
     * Mirror the label spin style across the X axis or simply swaps up and bottom.
     */
    LABEL_SPIN_STYLE MirrorX();

    /**
     * Mirror the label spin style across the Y axis or simply swaps left and right.
     */
    LABEL_SPIN_STYLE MirrorY();

private:
    SPIN m_spin;
};

/*
 * Shape/Type of #SCH_HIERLABEL, #SCH_GLOBALLABEL and #SCH_NETCLASS_FLAG.
 */
enum LABEL_FLAG_SHAPE
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


extern const char* SheetLabelType[];    /* names of types of labels */


class SCH_TEXT : public SCH_ITEM, public EDA_TEXT
{
public:
    SCH_TEXT( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString,
              KICAD_T aType = SCH_TEXT_T );

    SCH_TEXT( const SCH_TEXT& aText );

    ~SCH_TEXT() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_TEXT_T == aItem->Type();
    }

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_TEXT" );
    }

    wxString GetShownText( int aDepth = 0 ) const override;

    /**
     * Increment the label text, if it ends with a number.
     *
     * @param aIncrement = the increment value to add to the number ending the text.
     */
    bool IncrementLabel( int aIncrement );

    /**
     * Set a spin or rotation angle, along with specific horizontal and vertical justification
     * styles with each angle.
     *
     * @param aSpinStyle Spin style as per #LABEL_SPIN_STYLE storage class, may be the enum
     *                   values or int value
     */
    virtual void     SetLabelSpinStyle( LABEL_SPIN_STYLE aSpinStyle );
    LABEL_SPIN_STYLE GetLabelSpinStyle() const  { return m_spin_style; }

    virtual LABEL_FLAG_SHAPE GetShape() const        { return L_UNSPECIFIED; }
    virtual void SetShape( LABEL_FLAG_SHAPE aShape ) { }

    /**
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     *
     * @return the offset between the SCH_TEXT position and the text itself position
     */
    virtual wxPoint GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const;

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& offset ) override;

    void SwapData( SCH_ITEM* aItem ) override;

    const EDA_RECT GetBoundingBox() const override;

    bool operator<( const SCH_ITEM& aItem ) const override;

    int GetTextOffset( const RENDER_SETTINGS* aSettings = nullptr ) const;

    int GetPenWidth() const override;

    void Move( const wxPoint& aMoveVector ) override
    {
        EDA_TEXT::Offset( aMoveVector );
    }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const wxPoint& aCenter ) override;

    virtual void Rotate90( bool aClockwise );
    virtual void MirrorSpinStyle( bool aLeftRight );

    bool Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const override
    {
        return SCH_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( const wxFindReplaceData& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    virtual bool IsReplaceable() const override { return true; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    wxPoint GetPosition() const override { return (wxPoint)EDA_TEXT::GetTextPos(); }
    void SetPosition( const wxPoint& aPosition ) override { EDA_TEXT::SetTextPos( aPosition ); }

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) const override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_TEXT( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static HTML_MESSAGE_BOX* ShowSyntaxHelp( wxWindow* aParentWindow );

protected:
    /**
     * The orientation of text and any associated drawing elements of derived objects.
     *  - 0 is the horizontal and left justified.
     *  - 1 is vertical and top justified.
     *  - 2 is horizontal and right justified.  It is the equivalent of the mirrored 0 orientation.
     *  - 3 is vertical and bottom justified. It is the equivalent of the mirrored 1 orientation.
     *
     * This is a duplication of m_Orient, m_HJustified, and m_VJustified in #EDA_TEXT but is
     * easier to handle than 3 parameters when editing and reading and saving files.
     */
    LABEL_SPIN_STYLE m_spin_style;
};


class SCH_LABEL_BASE : public SCH_TEXT
{
public:
    SCH_LABEL_BASE( const wxPoint& aPos, const wxString& aText, KICAD_T aType );

    SCH_LABEL_BASE( const SCH_LABEL_BASE& aLabel );

    // Abstract class
    virtual wxString GetClass() const override = 0;

    bool IsType( const KICAD_T aScanTypes[] ) const override;

    void SwapData( SCH_ITEM* aItem ) override;

    LABEL_FLAG_SHAPE GetShape() const override        { return m_shape; }
    void SetShape( LABEL_FLAG_SHAPE aShape ) override { m_shape = aShape; }

    static const wxString GetDefaultFieldName( const wxString& aName, bool aUseDefaultName );

    virtual int GetMandatoryFieldCount() { return 0; }

    std::vector<SCH_FIELD>& GetFields() { return m_fields; }
    const std::vector<SCH_FIELD>& GetFields() const { return m_fields; }

    /**
     * Set multiple schematic fields.
     *
     * @param aFields are the fields to set in this symbol.
     */
    void SetFields( const std::vector<SCH_FIELD>& aFields )
    {
        m_fields = aFields;     // vector copying, length is changed possibly
    }

    void Move( const wxPoint& aMoveVector ) override
    {
        SCH_TEXT::Move( aMoveVector );

        for( SCH_FIELD& field : m_fields )
            field.Offset( aMoveVector );
    }

    void Rotate( const wxPoint& aCenter ) override;
    void Rotate90( bool aClockwise ) override;

    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    virtual bool ResolveTextVar( wxString* token, int aDepth ) const;

    wxString GetShownText( int aDepth = 0 ) const override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction ) override;

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    /**
     * Calculate the graphic shape (a polygon) associated to the text.
     *
     * @param aPoints A buffer to fill with polygon corners coordinates
     * @param Pos Position of the shape, for texts and labels: do nothing
     */
    virtual void CreateGraphicShape( const RENDER_SETTINGS* aSettings,
                                     std::vector<wxPoint>& aPoints, const wxPoint& Pos ) const
    {
        aPoints.clear();
    }

    int GetLabelBoxExpansion( const RENDER_SETTINGS* aSettings = nullptr ) const;

    /**
     * Return the bounding box of the label only, without taking in account its fields.
     */
    virtual const EDA_RECT GetBodyBoundingBox() const;

    /**
     * Return the bounding box of the label including its fields.
     */
    const EDA_RECT GetBoundingBox() const override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    std::vector<wxPoint> GetConnectionPoints() const override;

    void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                              const SCH_SHEET_PATH* aPath = nullptr ) override;

    bool IsDangling() const override { return m_isDangling; }
    void SetIsDangling( bool aIsDangling ) { m_isDangling = aIsDangling; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    void Plot( PLOTTER* aPlotter ) const override;

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& offset ) override;

protected:
    std::vector<SCH_FIELD>  m_fields;

    LABEL_FLAG_SHAPE        m_shape;

    CONNECTION_TYPE         m_connectionType;
    bool                    m_isDangling;
};


class SCH_LABEL : public SCH_LABEL_BASE
{
public:
    SCH_LABEL( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString );

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

    const EDA_RECT GetBodyBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T &&
                ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    bool IsReplaceable() const override { return true; }

    EDA_ITEM* Clone() const override
    {
        return new SCH_LABEL( *this );
    }

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

private:
    bool doIsConnected( const wxPoint& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};


class SCH_NETCLASS_FLAG : public SCH_LABEL_BASE
{
public:
    SCH_NETCLASS_FLAG( const wxPoint& aPos = wxPoint( 0, 0 ) );

    SCH_NETCLASS_FLAG( const SCH_NETCLASS_FLAG& aClassLabel );

    ~SCH_NETCLASS_FLAG() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_NETCLASS_FLAG_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_NETCLASS_FLAG" );
    }

    EDA_ITEM* Clone() const override
    {
        return new SCH_NETCLASS_FLAG( *this );
    }

    int GetPinLength() const { return m_pinLength; }
    void SetPinLength( int aLength ) { m_pinLength = aLength; }

    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<wxPoint>& aPoints,
                             const wxPoint& aPos ) const override;

    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T &&
                ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

private:
    int       m_pinLength;
    int       m_symbolSize;
};


class SCH_GLOBALLABEL : public SCH_LABEL_BASE
{
public:
    SCH_GLOBALLABEL( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString );

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

    void MirrorSpinStyle( bool aLeftRight ) override;

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;

    void SetLabelSpinStyle( LABEL_SPIN_STYLE aSpinStyle ) override;

    wxPoint GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aRenderSettings,
                             std::vector<wxPoint>& aPoints, const wxPoint& aPos ) const override;

    bool ResolveTextVar( wxString* token, int aDepth ) const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T &&
                ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

private:
    bool doIsConnected( const wxPoint& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};


class SCH_HIERLABEL : public SCH_LABEL_BASE
{
public:
    SCH_HIERLABEL( const wxPoint& aPos = wxPoint( 0, 0 ), const wxString& aText = wxEmptyString,
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

    void SetLabelSpinStyle( LABEL_SPIN_STYLE aSpinStyle ) override;

    wxPoint GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const override;

    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<wxPoint>& aPoints,
                             const wxPoint& aPos ) const override;
    void CreateGraphicShape( const RENDER_SETTINGS* aSettings, std::vector<wxPoint>& aPoints,
                             const wxPoint& aPos, LABEL_FLAG_SHAPE aShape ) const;

    const EDA_RECT GetBodyBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T &&
                ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_HIERLABEL( *this );
    }

    bool IsPointClickableAnchor( const wxPoint& aPos ) const override
    {
        return m_isDangling && GetPosition() == aPos;
    }

private:
    bool doIsConnected( const wxPoint& aPosition ) const override
    {
        return EDA_TEXT::GetTextPos() == aPosition;
    }
};

#endif /* CLASS_TEXT_LABEL_H */
