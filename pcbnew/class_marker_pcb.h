/**
 * @file class_marker_pcb.h
 * @brief Markers used to show a drc problem on boards.
 */

#ifndef CLASS_MARKER_PCB_H
#define CLASS_MARKER_PCB_H


#include <class_board_item.h>
#include <class_marker_base.h>


class MSG_PANEL_ITEM;


class MARKER_PCB : public BOARD_ITEM, public MARKER_BASE
{

public:

    MARKER_PCB( BOARD_ITEM* aParent );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER_PCB on the BOARD
     * @param aText Text describing the first of two objects
     * @param aPos The position of the first of two objects
     * @param bText Text describing the second of the two conflicting objects
     * @param bPos The position of the second of two objects
     */
    MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                const wxString& aText, const wxPoint& aPos,
                const wxString& bText, const wxPoint& bPos );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER_PCB on the BOARD
     * @param aText Text describing the object
     * @param aPos The position of the object
     */
    MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                const wxString& aText, const wxPoint& aPos );

    ~MARKER_PCB();

    void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

    void Rotate( const wxPoint& aRotCentre, double aAngle );

    void Flip( const wxPoint& aCentre );

    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset )
    {
        DrawMarker( aPanel, aDC, aDrawMode, aOffset );
    }

    const wxPoint& GetPosition() const          { return m_Pos; }
    void SetPosition( const wxPoint& aPos )     { m_Pos = aPos; }

    void SetItem( const BOARD_ITEM* aItem )
    {
        m_item = aItem;
    }

    const BOARD_ITEM* GetItem() const
    {
        return m_item;
    }

    bool HitTest( const wxPoint& aPosition ) const
    {
        return HitTestMarker( aPosition );
    }

    bool IsOnLayer( LAYER_NUM aLayer ) const;

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    wxString GetSelectMenuText() const;

    BITMAP_DEF GetMenuImage() const { return  drc_xpm; }

    ///> @copydoc VIEW_ITEM::ViewBBox()
    virtual const BOX2I ViewBBox() const
    {
        return GetParent()->ViewBBox();
    }

    ///> @copydoc VIEW_ITEM::ViewGetLayers()
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
#endif

protected:
    ///> Pointer to BOARD_ITEM that causes DRC error.
    const BOARD_ITEM* m_item;
};

#endif      //  CLASS_MARKER_PCB_H
