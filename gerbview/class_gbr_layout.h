/**
 * @file class_gbr_layout.h
 * @brief Class CLASS_GBR_LAYOUT to handle a board.
 */

#ifndef CLASS_GBR_LAYOUT_H
#define CLASS_GBR_LAYOUT_H


#include <dlist.h>

// #include <layers_id_colors_and_visibility.h>
#include <class_colors_design_settings.h>
#include <common.h>                         // PAGE_INFO
#include <class_title_block.h>
#include <class_gerber_draw_item.h>


/**
 * Class GBR_LAYOUT
 * holds list of GERBER_DRAW_ITEM currently loaded.
 */
class GBR_LAYOUT
{
private:
    EDA_RECT                m_BoundingBox;
    PAGE_INFO               m_paper;
    TITLE_BLOCK             m_titles;
    wxPoint                 m_originAxisPosition;
    int                     m_printLayersMask; // When printing: the list of layers to print
public:

    DLIST<GERBER_DRAW_ITEM> m_Drawings;     // linked list of Gerber Items

    GBR_LAYOUT();
    ~GBR_LAYOUT();

    const PAGE_INFO&    GetPageSettings() const { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )  { m_paper = aPageSettings; }

    const wxPoint&      GetOriginAxisPosition() const
    {
        return m_originAxisPosition;
    }

    void SetOriginAxisPosition( const wxPoint& aPosition )
    {
        m_originAxisPosition = aPosition;
    }

    TITLE_BLOCK& GetTitleBlock()
    {
        return m_titles;
    }

    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
    {
        m_titles = aTitleBlock;
    }

    /**
     * Function ComputeBoundingBox
     * calculates the bounding box containing all Gerber items.
     * @return EDA_RECT - the full item list bounding box
     */
    EDA_RECT ComputeBoundingBox();

    /**
     * Function GetBoundingBox
     * may be called soon after ComputeBoundingBox() to return the same EDA_RECT,
     * as long as the CLASS_GBR_LAYOUT has not changed.
     */
    EDA_RECT GetBoundingBox() const { return m_BoundingBox; }    // override

    void SetBoundingBox( const EDA_RECT& aBox ) { m_BoundingBox = aBox; }

    /**
     * Function Draw.
     * Redraw the CLASS_GBR_LAYOUT items but not cursors, axis or grid.
     * @param aPanel = the panel relative to the board
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ... (not always used)
     * @param aOffset = an draw offset value
     */
    void    Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                  int aDrawMode, const wxPoint& aOffset );

    /**
     * Function SetVisibleLayers
     * changes the bit-mask of visible layers
     * @param aLayerMask = The new bit-mask of visible layers
     */
    void    SetVisibleLayers( int aLayerMask )
    {
        m_printLayersMask = aLayerMask;
    }

    /**
     * Function IsLayerVisible
     * tests whether a given layer is visible
     * @param aLayerIndex = The index of the layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool    IsLayerVisible( int aLayerIndex ) const;

#if defined(DEBUG)
    void    Show( int nestLevel, std::ostream& os ) const;  // overload

#endif
};

#endif      // #ifndef CLASS_GBR_LAYOUT_H
