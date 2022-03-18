#ifndef __LABEL_MANAGER_H
#define __LABEL_MANAGER_H

#include <wx/wx.h>

#include <gal/graphics_abstraction_layer.h>
#include <gal/color4d.h>
#include <math/vector2d.h>
#include <math/box2.h>
#include <view/view_overlay.h>
#include <geometry/shape_line_chain.h>

class LABEL_MANAGER
{
public:
    struct LABEL
    {
        KIGFX::COLOR4D m_color;
        wxString       m_msg;
        VECTOR2I       m_target;
        BOX2I          m_bbox;
    };

    LABEL_MANAGER( KIGFX::GAL* aGal );
    ~LABEL_MANAGER();

    void Add( VECTOR2I target, wxString msg, KIGFX::COLOR4D color );
    void Add( const SHAPE_LINE_CHAIN& aL, KIGFX::COLOR4D color );
    void Redraw( KIGFX::VIEW_OVERLAY* aOvl );

private:
    VECTOR2I nearestBoxCorner( BOX2I b, VECTOR2I p );
    VECTOR2I boxMtv( BOX2I b1, BOX2I b2 );
    void     recalculate();

    KIGFX::GAL*        m_gal;
    int                m_textSize = 100000;
    std::vector<LABEL> m_labels;
};

#endif