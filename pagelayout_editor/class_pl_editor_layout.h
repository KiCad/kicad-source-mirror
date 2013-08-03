/**
 * @file class_pl_editor_layout.h
 */

#ifndef CLASS_PL_EDITOR_LAYOUT_H
#define CLASS_PL_EDITOR_LAYOUT_H

#include <base_struct.h>                         // PAGE_INFO
#include <common.h>                         // PAGE_INFO
#include <class_title_block.h>

class EDA_DRAW_PANEL;


/**
 * Class PL_EDITOR_LAYOUT
 * holds list of GERBER_DRAW_ITEM currently loaded.
 */
class PL_EDITOR_LAYOUT
{
private:
    EDA_RECT                m_BoundingBox;
    PAGE_INFO               m_paper;
    TITLE_BLOCK             m_titles;

public:
    PL_EDITOR_LAYOUT();
    ~PL_EDITOR_LAYOUT();

    const PAGE_INFO&    GetPageSettings() const { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )
    {
        m_paper = aPageSettings;
    }

    const wxPoint&      GetAuxOrigin() const
    {
        static wxPoint zero( 0, 0 );
        return zero;
    }

    const TITLE_BLOCK& GetTitleBlock() const
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
     * as long as the CLASS_PL_EDITOR_LAYOUT has not changed.
     */
    EDA_RECT GetBoundingBox() const { return m_BoundingBox; }    // override

    void SetBoundingBox( const EDA_RECT& aBox ) { m_BoundingBox = aBox; }

#if defined(DEBUG)
    void    Show( int nestLevel, std::ostream& os ) const;  // overload

#endif
};

#endif      // #ifndef CLASS_PL_EDITOR_LAYOUT_H
