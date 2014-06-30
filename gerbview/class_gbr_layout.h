/**
 * @file class_gbr_layout.h
 * @brief Class CLASS_GBR_LAYOUT to handle a board.
 */

#ifndef CLASS_GBR_LAYOUT_H
#define CLASS_GBR_LAYOUT_H


#include <dlist.h>

#include <class_colors_design_settings.h>
#include <common.h>                         // PAGE_INFO
#include <gerbview.h>                       // GERBER_DRAWLAYERS_COUNT
#include <class_title_block.h>
#include <class_gerber_draw_item.h>

#include <gr_basic.h>

/**
 * Class GBR_LAYOUT
 * holds list of GERBER_DRAW_ITEM currently loaded.
 */
class GBR_LAYOUT
{
private:
    EDA_RECT            m_BoundingBox;
    PAGE_INFO           m_paper;
    TITLE_BLOCK         m_titles;
    wxPoint             m_originAxisPosition;
    std::bitset <GERBER_DRAWLAYERS_COUNT> m_printLayersMask; // When printing: the list of layers to print
public:

    DLIST<GERBER_DRAW_ITEM> m_Drawings;     // linked list of Gerber Items

    GBR_LAYOUT();
    ~GBR_LAYOUT();

    const PAGE_INFO&    GetPageSettings() const { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )  { m_paper = aPageSettings; }

    const wxPoint&      GetAuxOrigin() const
    {
        return m_originAxisPosition;
    }

    void SetAuxOrigin( const wxPoint& aPosition )
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
     * @param aPrintBlackAndWhite = true to force black and white insdeat of color
     *        useful only to print/plot gebview layers
     */
    void    Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                  GR_DRAWMODE aDrawMode, const wxPoint& aOffset,
                  bool aPrintBlackAndWhite = false );
    /**
     * Function SetPrintableLayers
     * changes the list of printable layers
     * @param aLayerMask = The new bit-mask of printable layers
     */
    void    SetPrintableLayers( const std::bitset <GERBER_DRAWLAYERS_COUNT>& aLayerMask  )
    {
        m_printLayersMask = aLayerMask;
    }

    /**
     * Function GetPrintableLayers
     * @return the bit-mask of printable layers
     */
    std::bitset <GERBER_DRAWLAYERS_COUNT> GetPrintableLayers()
    {
        return m_printLayersMask;
    }

     /**
     * Function IsLayerPrintable
     * tests whether a given layer is visible
     * @param aLayer = The layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool    IsLayerPrintable( int aLayer ) const
    {
        return m_printLayersMask[ aLayer ];
    }

#if defined(DEBUG)
    void    Show( int nestLevel, std::ostream& os ) const;  // overload

#endif
};

#endif      // #ifndef CLASS_GBR_LAYOUT_H
