/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_item_struct.h>


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

    /**
     * The orientation of text and any associated drawing elements of derived objects.
     * 0 is the horizontal and left justified.
     * 1 is vertical and top justified.
     * 2 is horizontal and right justified.  It is the equivalent of the mirrored 0 orentation.
     * 3 is veritcal and bottom justifiend. It is the equivalent of the mirrored 1 orentation.
     * This is a duplicattion of m_Orient, m_HJustified, and m_VJustified in #EDA_TEXT but is
     * easier to handle that 3 parameters when editing and reading and saving files.
     */
    int m_schematicOrientation;

public:
    SCH_TEXT( const wxPoint& pos = wxPoint( 0, 0 ),
              const wxString& text = wxEmptyString,
              KICAD_T aType = SCH_TEXT_T );

    /**
     * Copy Constructor
     * clones \a aText into a new object.  All members are copied as is except
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
     * Function IncrementLabel
     * increments the label text, if it ends with a number.
     * @param aIncrement = the increment value to add to the number
     * ending the text
     */
    void IncrementLabel( int aIncrement );

    /**
     * Function SetOrientation
     * Set m_schematicOrientation, and initialize
     * m_orient,m_HJustified and m_VJustified, according to the value of
     * m_schematicOrientation (for a text )
     * must be called after changing m_schematicOrientation
     * @param aSchematicOrientation =
     *  0 = normal (horizontal, left justified).
     *  1 = up (vertical)
     *  2 = (horizontal, right justified). This can be seen as the mirrored position of 0
     *  3 = bottom . This can be seen as the mirrored position of up
     */
    virtual void SetOrientation( int aSchematicOrientation );

    int GetOrientation() { return m_schematicOrientation; }

    PINSHEETLABEL_SHAPE GetShape() const { return m_shape; }

    void SetShape( PINSHEETLABEL_SHAPE aShape ) { m_shape = aShape; }

    /**
     * Function GetSchematicTextOffset (virtual)
     * @return the offset between the SCH_TEXT position and the text itself position
     *
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     */
    virtual wxPoint GetSchematicTextOffset() const;

    virtual void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                       GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR ) override;

    /**
     * Function CreateGraphicShape
     * Calculates the graphic shape (a polygon) associated to the text
     * @param aPoints A buffer to fill with polygon corners coordinates
     * @param Pos Position of the shape, for texts and labels: do nothing
     * Mainly for derived classes (SCH_SHEET_PIN and Hierarchical labels)
     */
    virtual void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos )
    {
        aPoints.clear();
    }

    virtual void SwapData( SCH_ITEM* aItem ) override;

    virtual const EDA_RECT GetBoundingBox() const override;

    virtual bool Save( FILE* aFile ) const override;

    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg ) override;

    virtual int GetPenSize() const override;

    // Geometric transforms (used in block operations):

    virtual void Move( const wxPoint& aMoveVector ) override
    {
        m_Pos += aMoveVector;
    }

    virtual void MirrorY( int aYaxis_position ) override;

    virtual void MirrorX( int aXaxis_position ) override;

    virtual void Rotate( wxPoint aPosition ) override;

    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation ) override;

    virtual bool Replace( wxFindReplaceData& aSearchData, void* aAuxData = NULL ) override
    {
        return EDA_ITEM::Replace( aSearchData, m_Text );
    }

    virtual bool IsReplaceable() const override { return true; }

    virtual void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList ) override;

    virtual bool IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList ) override;

    virtual bool IsDangling() const override { return m_isDangling; }

    virtual bool IsSelectStateChanged( const wxRect& aRect ) override;

    virtual void GetConnectionPoints( std::vector< wxPoint >& aPoints ) const override;

    virtual bool CanIncrementLabel() const override { return true; }

    virtual wxString GetSelectMenuText() const override;

    virtual BITMAP_DEF GetMenuImage() const override { return  add_text_xpm; }

    virtual void GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                                 SCH_SHEET_PATH*      aSheetPath ) override;

    virtual wxPoint GetPosition() const override { return m_Pos; }

    virtual void SetPosition( const wxPoint& aPosition ) override { m_Pos = aPosition; }

    virtual bool HitTest( const wxPoint& aPosition, int aAccuracy ) const override;

    virtual bool HitTest( const EDA_RECT& aRect, bool aContained = false,
                          int aAccuracy = 0 ) const override;

    virtual void Plot( PLOTTER* aPlotter ) override;

    virtual EDA_ITEM* Clone() const override;

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList ) override;

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

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
               GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR ) override;

    wxString GetClass() const override
    {
        return wxT( "SCH_LABEL" );
    }

    void SetOrientation( int aSchematicOrientation ) override;

    wxPoint GetSchematicTextOffset() const override;

    void MirrorX( int aXaxis_position ) override;

    void Rotate( wxPoint aPosition ) override;

    const EDA_RECT GetBoundingBox() const override;

    bool Save( FILE* aFile ) const override;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg ) override;

    bool IsConnectable() const override { return true; }

    wxString GetSelectMenuText() const override;

    BITMAP_DEF GetMenuImage() const override { return  add_line_label_xpm; }

    bool IsReplaceable() const override { return true; }

    EDA_ITEM* Clone() const override;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override { return m_Pos == aPosition; }
};


class SCH_GLOBALLABEL : public SCH_TEXT
{
public:
    SCH_GLOBALLABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_GLOBALLABEL() { }

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
               GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR ) override;

    wxString GetClass() const override
    {
        return wxT( "SCH_GLOBALLABEL" );
    }

    void SetOrientation( int aSchematicOrientation ) override;

    wxPoint GetSchematicTextOffset() const override;

    bool Save( FILE* aFile ) const override;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg ) override;

    const EDA_RECT GetBoundingBox() const override;

    void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& aPos ) override;

    void MirrorY( int aYaxis_position ) override;

    void MirrorX( int aXaxis_position ) override;

    void Rotate( wxPoint aPosition ) override;

    bool IsConnectable() const override { return true; }

    wxString GetSelectMenuText() const override;

    BITMAP_DEF GetMenuImage() const override { return  add_glabel_xpm; }

    EDA_ITEM* Clone() const override;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override { return m_Pos == aPosition; }
};


class SCH_HIERLABEL : public SCH_TEXT
{
public:
    SCH_HIERLABEL( const wxPoint& pos = wxPoint( 0, 0 ),
                   const wxString& text = wxEmptyString,
                   KICAD_T aType = SCH_HIERARCHICAL_LABEL_T );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_HIERLABEL() { }

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
               GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR ) override;

    wxString GetClass() const override
    {
        return wxT( "SCH_HIERLABEL" );
    }

    void SetOrientation( int aSchematicOrientation ) override;

    wxPoint GetSchematicTextOffset() const override;

    void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos ) override;

    bool Save( FILE* aFile ) const override;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg ) override;

    const EDA_RECT GetBoundingBox() const override;

    void MirrorY( int aYaxis_position ) override;

    void MirrorX( int aXaxis_position ) override;

    void Rotate( wxPoint aPosition ) override;

    bool IsConnectable() const override { return true; }

    wxString GetSelectMenuText() const override;

    BITMAP_DEF GetMenuImage() const override { return  add_hierarchical_label_xpm; }

    EDA_ITEM* Clone() const override;

private:
    bool doIsConnected( const wxPoint& aPosition ) const override { return m_Pos == aPosition; }
};

#endif /* CLASS_TEXT_LABEL_H */
