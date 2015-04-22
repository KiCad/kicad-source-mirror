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


/* Type of SCH_HIERLABEL and SCH_GLOBALLABEL
 * mainly used to handle the graphic associated shape
 */
typedef enum {
    NET_INPUT,
    NET_OUTPUT,
    NET_BIDI,
    NET_TRISTATE,
    NET_UNSPECIFIED,
    NET_TMAX        /* Last value */
} TypeSheetLabel;


extern const char* SheetLabelType[];    /* names of types of labels */

class SCH_TEXT : public SCH_ITEM, public EDA_TEXT
{
protected:
    int m_shape;

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

    virtual wxString GetClass() const
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

    int GetShape() const { return m_shape; }

    void SetShape( int aShape ) { m_shape = aShape; }

    /**
     * Function GetSchematicTextOffset (virtual)
     * @return the offset between the SCH_TEXT position and the text itself position
     *
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     */
    virtual wxPoint GetSchematicTextOffset() const;

    virtual void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                       GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR );

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

    virtual void SwapData( SCH_ITEM* aItem );

    virtual const EDA_RECT GetBoundingBox() const;

    virtual bool Save( FILE* aFile ) const;

    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    virtual int GetPenSize() const;

    // Geometric transforms (used in block operations):

    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    virtual void MirrorY( int aYaxis_position );

    virtual void MirrorX( int aXaxis_position );

    virtual void Rotate( wxPoint aPosition );

    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation );

    virtual bool Replace( wxFindReplaceData& aSearchData, void* aAuxData = NULL )
    {
        return EDA_ITEM::Replace( aSearchData, m_Text );
    }

    virtual bool IsReplaceable() const { return true; }

    virtual void GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDangling() const { return m_isDangling; }

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( std::vector< wxPoint >& aPoints ) const;

    virtual bool CanIncrementLabel() const { return true; }

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_text_xpm; }

    virtual void GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                                 SCH_SHEET_PATH*      aSheetPath );

    virtual wxPoint GetPosition() const { return m_Pos; }

    virtual void SetPosition( const wxPoint& aPosition ) { m_Pos = aPosition; }

    virtual bool HitTest( const wxPoint& aPosition, int aAccuracy ) const;

    virtual bool HitTest( const EDA_RECT& aRect, bool aContained = false,
                          int aAccuracy = 0 ) const;

    virtual void Plot( PLOTTER* aPlotter );

    virtual EDA_ITEM* Clone() const;

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // override
#endif
};


class SCH_LABEL : public SCH_TEXT
{
public:
    SCH_LABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_LABEL() { }

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
               GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR );

    wxString GetClass() const
    {
        return wxT( "SCH_LABEL" );
    }

    void SetOrientation( int aSchematicOrientation );

    wxPoint GetSchematicTextOffset() const;

    void MirrorX( int aXaxis_position );

    void Rotate( wxPoint aPosition );

    const EDA_RECT GetBoundingBox() const;  // Virtual

    bool Save( FILE* aFile ) const;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    bool IsConnectable() const { return true; }

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  add_line_label_xpm; }

    bool IsReplaceable() const { return true; }

    EDA_ITEM* Clone() const;

private:
    bool doIsConnected( const wxPoint& aPosition ) const { return m_Pos == aPosition; }
};


class SCH_GLOBALLABEL : public SCH_TEXT
{
public:
    SCH_GLOBALLABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~SCH_GLOBALLABEL() { }

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
               GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR );

    wxString GetClass() const
    {
        return wxT( "SCH_GLOBALLABEL" );
    }

    void SetOrientation( int aSchematicOrientation );

    wxPoint GetSchematicTextOffset() const;

    bool Save( FILE* aFile ) const;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    const EDA_RECT GetBoundingBox() const;  // Virtual

    void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& aPos );

    void MirrorY( int aYaxis_position );

    void MirrorX( int aXaxis_position );

    void Rotate( wxPoint aPosition );

    bool IsConnectable() const { return true; }

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  add_glabel_xpm; }

    EDA_ITEM* Clone() const;

private:
    bool doIsConnected( const wxPoint& aPosition ) const { return m_Pos == aPosition; }
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
               GR_DRAWMODE draw_mode, EDA_COLOR_T Color = UNSPECIFIED_COLOR );

    wxString GetClass() const
    {
        return wxT( "SCH_HIERLABEL" );
    }

    void SetOrientation( int aSchematicOrientation );

    wxPoint GetSchematicTextOffset() const;

    void CreateGraphicShape( std::vector <wxPoint>& aPoints, const wxPoint& Pos );

    bool Save( FILE* aFile ) const;

    bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    const EDA_RECT GetBoundingBox() const;      // Virtual

    void MirrorY( int aYaxis_position );

    void MirrorX( int aXaxis_position );

    void Rotate( wxPoint aPosition );

    bool IsConnectable() const { return true; }

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  add_hierarchical_label_xpm; }

    EDA_ITEM* Clone() const;

private:
    bool doIsConnected( const wxPoint& aPosition ) const { return m_Pos == aPosition; }
};

#endif /* CLASS_TEXT_LABEL_H */
