/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_text.h
 * @brief Definitions of the SCH_TEXT class and derivatives for Eeschema.
 */

#ifndef CLASS_TEXT_LABEL_H
#define CLASS_TEXT_LABEL_H


#include <macros.h>
#include <eda_text.h>
#include <sch_item.h>
#include <sch_connection.h>   // for CONNECTION_TYPE


class LINE_READER;
class NETLIST_OBJECT_LIST;


/* Shape/Type of SCH_HIERLABEL and SCH_GLOBALLABEL
 * mainly used to handle the graphic associated shape
 */
enum PINSHEETLABEL_SHAPE {
    NET_INPUT,
    NET_OUTPUT,
    NET_BIDI,
    NET_TRISTATE,
    NET_UNSPECIFIED
};


extern const char* SheetLabelType[];    /* names of types of labels */


class SCH_TEXT : public SCH_ITEM, public EDA_TEXT
{
protected:
    PINSHEETLABEL_SHAPE m_shape;

    /// True if not connected to another object if the object derive from SCH_TEXT
    /// supports connections.
    bool m_isDangling;

    CONNECTION_TYPE m_connectionType;

    /**
     * The orientation of text and any associated drawing elements of derived objects.
     * 0 is the horizontal and left justified.
     * 1 is vertical and top justified.
     * 2 is horizontal and right justified.  It is the equivalent of the mirrored 0 orentation.
     * 3 is veritcal and bottom justifiend. It is the equivalent of the mirrored 1 orentation.
     * This is a duplicattion of m_Orient, m_HJustified, and m_VJustified in #EDA_TEXT but is
     * easier to handle than 3 parameters when editing and reading and saving files.
     */
    int m_spin_style;

public:
    SCH_TEXT( const wxPoint& pos = wxPoint( 0, 0 ),
              const wxString& text = wxEmptyString,
              KICAD_T aType = SCH_TEXT_T );

    /**
     * Clones \a aText into a new object.  All members are copied as is except
     * for the #m_isDangling member which is set to false.  This prevents newly
     * copied objects derived from #SCH_TEXT from having their connection state
     * improperly set.
     */
    SCH_TEXT( const SCH_TEXT& aText );

    ~SCH_TEXT() { }

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_TEXT" );
    }

    /**
     * Increment the label text, if it ends with a number.
     *
     * @param aIncrement = the increment value to add to the number ending the text.
     */
    void IncrementLabel( int aIncrement );

    /**
     * Set a spin or rotation angle, along with specific horizontal and vertical justification
     * styles with each angle.
     *
     * @param aSpinStyle =
     *  0 = normal (horizontal, left justified).
     *  1 = up (vertical)
     *  2 = (horizontal, right justified). This can be seen as the mirrored position of 0
     *  3 = bottom . This can be seen as the mirrored position of up
     */
    virtual void SetLabelSpinStyle( int aSpinStyle );
    int GetLabelSpinStyle() const               { return m_spin_style; }

    PINSHEETLABEL_SHAPE GetShape() const        { return m_shape; }

    void SetShape( PINSHEETLABEL_SHAPE aShape ) { m_shape = aShape; }

    /**
     * @return the offset between the SCH_TEXT position and the text itself position
     *
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     */
    virtual wxPoint GetSchematicTextOffset() const;

    void Print( wxDC* DC, const wxPoint& offset ) override;

    /**
     * Calculate the graphic shape (a polygon) associated to the text.
     *
     * @param aPoints A buffer to fill with polygon corners coordinates
     * @param Pos Position of the shape, for texts and labels: do nothing
     * Mainly for derived classes (SCH_SHEET_PIN and Hierarchical labels)
     */
    virtual void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
    {
        aPoints.clear();
    }

    void SwapData( SCH_ITEM* aItem ) override;

    const EDA_RECT GetBoundingBox() const override;

    int GetPenSize() const override;

    // Geometric transforms (used in block operations):

    void Move( const wxPoint& aMoveVector ) override
    {
        EDA_TEXT::Offset( aMoveVector );
    }

    void MirrorY( int aYaxis_position ) override;
    void MirrorX( int aXaxis_position ) override;
    void Rotate( wxPoint aPosition ) override;

    bool Matches( wxFindReplaceData& aSearchData, void* aAuxData ) override
    {
        return SCH_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( wxFindReplaceData& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    virtual bool IsReplaceable() const override { return true; }

    void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList ) override;

    bool IsDangling() const override { return m_isDangling; }
    void SetIsDangling( bool aIsDangling ) { m_isDangling = aIsDangling; }

    void GetConnectionPoints( std::vector< wxPoint >& aPoints ) const override;

    bool CanIncrementLabel() const override { return true; }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    void GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems, SCH_SHEET_PATH* aSheetPath ) override;

    wxPoint GetPosition() const override { return EDA_TEXT::GetTextPos(); }
    void SetPosition( const wxPoint& aPosition ) override { EDA_TEXT::SetTextPos( aPosition ); }

    bool HitTest( const wxPoint& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) override;

    EDA_ITEM* Clone() const override;

    void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif
};


class SCH_LABEL : public SCH_TEXT
{
public:
    SCH_LABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_LABEL() { }

    wxString GetClass() const override
    {
        return wxT( "SCH_LABEL" );
    }

    bool IsType( const KICAD_T aScanTypes[] ) override ;

    const EDA_RECT GetBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T &&
                ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    bool IsReplaceable() const override { return true; }

    EDA_ITEM* Clone() const override;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override { return EDA_TEXT::GetTextPos() == aPosition; }
};


class SCH_GLOBALLABEL : public SCH_TEXT
{
public:
    SCH_GLOBALLABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_GLOBALLABEL() { }

    void Print( wxDC* DC, const wxPoint& offset ) override;

    wxString GetClass() const override
    {
        return wxT( "SCH_GLOBALLABEL" );
    }

    void SetLabelSpinStyle( int aSpinStyle ) override;

    wxPoint GetSchematicTextOffset() const override;

    const EDA_RECT GetBoundingBox() const override;

    void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& aPos ) override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T &&
                ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override { return EDA_TEXT::GetTextPos() == aPosition; }
};


class SCH_HIERLABEL : public SCH_TEXT
{
public:
    SCH_HIERLABEL( const wxPoint& pos = wxPoint( 0, 0 ),
                   const wxString& text = wxEmptyString,
                   KICAD_T aType = SCH_HIER_LABEL_T );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_HIERLABEL() { }

    void Print( wxDC* DC, const wxPoint& offset ) override;

    wxString GetClass() const override
    {
        return wxT( "SCH_HIERLABEL" );
    }

    void SetLabelSpinStyle( int aSpinStyle ) override;

    wxPoint GetSchematicTextOffset() const override;

    void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos ) override;

    const EDA_RECT GetBoundingBox() const override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return aItem->Type() == SCH_LINE_T &&
                ( aItem->GetLayer() == LAYER_WIRE || aItem->GetLayer() == LAYER_BUS );
    }

    wxString GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    BITMAP_DEF GetMenuImage() const override;

    EDA_ITEM* Clone() const override;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override { return EDA_TEXT::GetTextPos() == aPosition; }
};

#endif /* CLASS_TEXT_LABEL_H */
