/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
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

/*
 * Edit properties of Lines, Circles, Arcs and Polygons for PCBNew and Footprint Editor
 */
#include "dialog_shape_properties_base.h"

#include <wx/valnum.h>

#include <pcb_base_edit_frame.h>
#include <pcb_edit_frame.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <pcb_layer_box_selector.h>
#include <dialogs/html_message_box.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <pcb_shape.h>
#include <macros.h>
#include <algorithm>
#include <widgets/unit_binder.h>

#include <tools/drawing_tool.h>


struct BOUND_CONTROL
{
    std::unique_ptr<UNIT_BINDER> m_Binder;
    wxTextCtrl*                  m_Ctrl;
};


/**
 * A class that operates over a list of BOUND_CONTROLs
 * and keeps them in sync with a PCB_SHAPE. Exactly how that is done
 * depends on the kind of shape.
 *
 * Inherit from this class and implement the relvant update functions
 * and listen for changes on the right controls for each mode
 * (e.g. edit line segment by endpoints).
 */
class GEOM_SYNCER : public wxEvtHandler
{
public:
    GEOM_SYNCER( PCB_SHAPE& aShape, std::vector<BOUND_CONTROL>& aBoundCtrls ) :
            m_shape( aShape ),
            m_boundCtrls( aBoundCtrls )
    {
    }

    void BindCtrls( size_t aFrom, size_t aTo, std::function<void()> aCb )
    {
        wxCHECK( aFrom < m_boundCtrls.size(), /* void */ );
        wxCHECK( aTo < m_boundCtrls.size(), /* void */ );

        for( size_t i = aFrom; i <= aTo; ++i )
        {
            m_boundCtrls[i].m_Ctrl->Bind( wxEVT_TEXT,
                                          [aCb]( wxCommandEvent& aEvent )
                                          {
                                              aCb();
                                          } );
        }
    }

    void SetShape( PCB_SHAPE& aShape )
    {
        m_shape = aShape;
        updateAll();
    }

    virtual bool Validate( wxArrayString& aErrs ) const { return true; }

protected:
    virtual void updateAll() = 0;

    wxTextCtrl* GetCtrl( size_t aIndex ) const
    {
        wxCHECK( aIndex < m_boundCtrls.size(), nullptr );
        return m_boundCtrls[aIndex].m_Ctrl;
    }

    int GetIntValue( size_t aIndex ) const
    {
        wxCHECK( aIndex < m_boundCtrls.size(), 0.0 );
        return static_cast<int>( m_boundCtrls[aIndex].m_Binder->GetValue() );
    }

    EDA_ANGLE GetAngleValue( size_t aIndex ) const
    {
        wxCHECK( aIndex < m_boundCtrls.size(), EDA_ANGLE() );
        return m_boundCtrls[aIndex].m_Binder->GetAngleValue();
    }

    void ChangeValue( size_t aIndex, int aValue )
    {
        wxCHECK( aIndex < m_boundCtrls.size(), /* void */ );
        m_boundCtrls[aIndex].m_Binder->ChangeValue( aValue );
    }

    void ChangeAngleValue( size_t aIndex, const EDA_ANGLE& aValue )
    {
        wxCHECK( aIndex < m_boundCtrls.size(), /* void */ );
        m_boundCtrls[aIndex].m_Binder->ChangeAngleValue( aValue );
    }

    PCB_SHAPE& GetShape() { return m_shape; }

    const PCB_SHAPE& GetShape() const { return m_shape; }

private:
    PCB_SHAPE&                  m_shape;
    std::vector<BOUND_CONTROL>& m_boundCtrls;
};


/**
 * Class that keeps a rectangle's various fields all up to date.
 */
class RECTANGLE_GEOM_SYNCER : public GEOM_SYNCER
{
public:
    enum CTRL_IDX
    {
        START_X = 0,
        START_Y,
        END_X,
        END_Y,
        CORNER_X,
        CORNER_Y,
        CORNER_W,
        CORNER_H,
        CENTER_X,
        CENTER_Y,
        CENTER_W,
        CENTER_H,

        NUM_CTRLS,
    };

    RECTANGLE_GEOM_SYNCER( PCB_SHAPE& aShape, std::vector<BOUND_CONTROL>& aBoundCtrls ) :
            GEOM_SYNCER( aShape, aBoundCtrls )
    {
        wxASSERT( aBoundCtrls.size() == NUM_CTRLS );
        wxASSERT( GetShape().GetShape() == SHAPE_T::RECTANGLE );

        BindCtrls( START_X, END_Y,
                   [this]()
                   {
                       OnCornersChange();
                   } );

        BindCtrls( CORNER_X, CORNER_H,
                   [this]()
                   {
                       OnCornerSizeChange();
                   } );

        BindCtrls( CENTER_X, CENTER_H,
                   [this]()
                   {
                       OnCenterSizeChange();
                   } );
    }

    bool Validate( wxArrayString& aErrs ) const override
    {
        const VECTOR2I p0{ GetIntValue( START_X ), GetIntValue( START_Y ) };
        const VECTOR2I p1{ GetIntValue( END_X ), GetIntValue( END_Y ) };

        if( p0 == p1 )
        {
            aErrs.push_back( _( "Rectangle cannot be zero-sized." ) );
            return false;
        }

        return true;
    }

    void updateAll() override
    {
        updateCorners();
        updateCornerSize();
        updateCenterSize();
    }

    void OnCornersChange()
    {
        const VECTOR2I p0{ GetIntValue( START_X ), GetIntValue( START_Y ) };
        const VECTOR2I p1{ GetIntValue( END_X ), GetIntValue( END_Y ) };

        GetShape().SetStart( p0 );
        GetShape().SetEnd( p1 );

        updateCenterSize();
        updateCornerSize();
    }

    void updateCorners()
    {
        const VECTOR2I p0 = GetShape().GetStart();
        const VECTOR2I p1 = GetShape().GetEnd();

        ChangeValue( START_X, p0.x );
        ChangeValue( START_Y, p0.y );
        ChangeValue( END_X, p1.x );
        ChangeValue( END_Y, p1.y );
    }

    void OnCornerSizeChange()
    {
        const VECTOR2I p0{ GetIntValue( CORNER_X ), GetIntValue( CORNER_Y ) };
        const VECTOR2I size{ GetIntValue( CORNER_W ), GetIntValue( CORNER_H ) };

        GetShape().SetStart( p0 );
        GetShape().SetEnd( p0 + size );

        updateCorners();
        updateCenterSize();
    }

    void updateCornerSize()
    {
        const VECTOR2I p0 = GetShape().GetStart();

        ChangeValue( CORNER_X, p0.x );
        ChangeValue( CORNER_Y, p0.y );
        ChangeValue( CORNER_W, GetShape().GetRectangleWidth() );
        ChangeValue( CORNER_H, GetShape().GetRectangleHeight() );
    }

    void OnCenterSizeChange()
    {
        const VECTOR2I center = { GetIntValue( CENTER_X ), GetIntValue( CENTER_Y ) };
        const VECTOR2I size = { GetIntValue( CENTER_W ), GetIntValue( CENTER_H ) };

        GetShape().SetStart( center - size / 2 );
        GetShape().SetEnd( center + size / 2 );

        updateCorners();
        updateCornerSize();
    }

    void updateCenterSize()
    {
        const VECTOR2I c = GetShape().GetCenter();

        ChangeValue( CENTER_X, c.x );
        ChangeValue( CENTER_Y, c.y );
        ChangeValue( CENTER_W, GetShape().GetRectangleWidth() );
        ChangeValue( CENTER_H, GetShape().GetRectangleHeight() );
    }
};


class LINE_GEOM_SYNCER : public GEOM_SYNCER
{
public:
    enum CTRL_IDX
    {
        START_X = 0,
        START_Y,
        END_X,
        END_Y,

        POLAR_START_X,
        POLAR_START_Y,
        LENGTH,
        ANGLE,

        MID_START_X,
        MID_START_Y,
        MID_X,
        MID_Y,

        NUM_CTRLS,
    };

    LINE_GEOM_SYNCER( PCB_SHAPE& aShape, std::vector<BOUND_CONTROL>& aBoundCtrls ) :
            GEOM_SYNCER( aShape, aBoundCtrls )
    {
        wxASSERT( aBoundCtrls.size() == NUM_CTRLS );
        wxASSERT( GetShape().GetShape() == SHAPE_T::SEGMENT );

        BindCtrls( START_X, END_Y,
                   [this]()
                   {
                       OnEndsChange();
                   } );

        BindCtrls( POLAR_START_X, ANGLE,
                   [this]()
                   {
                       OnPolarChange();
                   } );

        BindCtrls( MID_START_X, MID_Y,
                   [this]()
                   {
                       OnStartMidpointChange();
                   } );
    }

    void updateAll() override
    {
        updateEnds();
        updatePolar();
        updateStartMidpoint();
    }

    void OnEndsChange()
    {
        const VECTOR2I p0{ GetIntValue( START_X ), GetIntValue( START_Y ) };
        const VECTOR2I p1{ GetIntValue( END_X ), GetIntValue( END_Y ) };

        GetShape().SetStart( p0 );
        GetShape().SetEnd( p1 );

        updatePolar();
        updateStartMidpoint();
    }

    void updateEnds()
    {
        const VECTOR2I p0 = GetShape().GetStart();
        const VECTOR2I p1 = GetShape().GetEnd();

        ChangeValue( START_X, p0.x );
        ChangeValue( START_Y, p0.y );
        ChangeValue( END_X, p1.x );
        ChangeValue( END_Y, p1.y );
    }

    void OnPolarChange()
    {
        const VECTOR2I  p0{ GetIntValue( POLAR_START_X ), GetIntValue( POLAR_START_Y ) };
        const int       length = GetIntValue( LENGTH );
        const EDA_ANGLE angle = GetAngleValue( ANGLE );

        const VECTOR2I polar = GetRotated( VECTOR2I{ length, 0 }, angle );

        GetShape().SetStart( p0 );
        GetShape().SetEnd( p0 + polar );

        updateEnds();
        updateStartMidpoint();
    }

    void updatePolar()
    {
        const VECTOR2I p0 = GetShape().GetStart();
        const VECTOR2I p1 = GetShape().GetEnd();

        ChangeValue( POLAR_START_X, p0.x );
        ChangeValue( POLAR_START_Y, p0.y );
        ChangeValue( LENGTH, p0.Distance( p1 ) );
        ChangeAngleValue( ANGLE, -EDA_ANGLE( p1 - p0 ) );
    }

    void OnStartMidpointChange()
    {
        const VECTOR2I start{ GetIntValue( MID_START_X ), GetIntValue( MID_START_Y ) };
        const VECTOR2I mid{ GetIntValue( MID_X ), GetIntValue( MID_Y ) };

        GetShape().SetStart( start );
        GetShape().SetEnd( mid - ( start - mid ) );

        updateEnds();
        updatePolar();
    }

    void updateStartMidpoint()
    {
        const VECTOR2I s = GetShape().GetStart();
        const VECTOR2I c = GetShape().GetCenter();

        ChangeValue( MID_X, c.x );
        ChangeValue( MID_Y, c.y );
        ChangeValue( MID_START_X, s.x );
        ChangeValue( MID_START_Y, s.y );
    }
};


class ARC_GEOM_SYNCER : public GEOM_SYNCER
{
public:
    enum CTRL_IDX
    {
        //CSA
        CSA_CENTER_X = 0,
        CSA_CENTER_Y,
        CSA_START_X,
        CSA_START_Y,
        CSA_ANGLE,

        SME_START_X,
        SME_START_Y,
        SME_MID_X,
        SME_MID_Y,
        SME_END_X,
        SME_END_Y,

        NUM_CTRLS
    };

    ARC_GEOM_SYNCER( PCB_SHAPE& aShape, std::vector<BOUND_CONTROL>& aBoundCtrls ) :
            GEOM_SYNCER( aShape, aBoundCtrls )
    {
        wxASSERT( aBoundCtrls.size() == NUM_CTRLS );
        wxASSERT( GetShape().GetShape() == SHAPE_T::ARC );

        BindCtrls( CSA_CENTER_X, CSA_ANGLE,
                   [this]()
                   {
                       OnCSAChange();
                   } );

        BindCtrls( SME_START_X, SME_END_Y,
                   [this]()
                   {
                       OnSMEChange();
                   } );
    }

    bool Validate( wxArrayString& aErrs ) const override
    {
        const EDA_ANGLE angle = GetAngleValue( CSA_ANGLE );

        if( angle == ANGLE_0 )
        {
            aErrs.push_back( _( "Arc angle must be greater than 0" ) );
            return false;
        }

        const VECTOR2I start{ GetIntValue( SME_START_X ), GetIntValue( SME_START_Y ) };
        const VECTOR2I mid{ GetIntValue( SME_MID_X ), GetIntValue( SME_MID_Y ) };
        const VECTOR2I end{ GetIntValue( SME_END_X ), GetIntValue( SME_END_Y ) };

        if( start == mid || mid == end || start == end )
        {
            aErrs.push_back( _( "Arc must have 3 distinct points" ) );
            return false;
        }
        else
        {
            const VECTOR2D center = CalcArcCenter( start, end, angle );

            double radius = ( center - start ).EuclideanNorm();
            double max_offset = std::max( std::abs( center.x ), std::abs( center.y ) ) + radius;
            VECTOR2I center_i = VECTOR2I( center.x, center.y );

            if( max_offset >= ( std::numeric_limits<VECTOR2I::coord_type>::max() / 2.0 )
                || center_i == start || center_i == end )
            {
                aErrs.push_back( wxString::Format( _( "Invalid Arc with radius %f and angle %f." ),
                                                   radius, angle.AsDegrees() ) );
                return false;
            }
        }

        return true;
    }

    void updateAll() override
    {
        updateCSA();
        updateSME();
    }

    void OnCSAChange()
    {
        const VECTOR2I  center{ GetIntValue( CSA_CENTER_X ), GetIntValue( CSA_CENTER_Y ) };
        const VECTOR2I  start{ GetIntValue( CSA_START_X ), GetIntValue( CSA_START_Y ) };
        const EDA_ANGLE angle{ GetAngleValue( CSA_ANGLE ) };

        GetShape().SetCenter( center );
        GetShape().SetStart( start );
        GetShape().SetArcAngleAndEnd( angle );

        updateSME();
    }

    void updateCSA()
    {
        const VECTOR2I center = GetShape().GetCenter();
        const VECTOR2I start = GetShape().GetStart();

        ChangeValue( CSA_CENTER_X, center.x );
        ChangeValue( CSA_CENTER_Y, center.y );
        ChangeValue( CSA_START_X, start.x );
        ChangeValue( CSA_START_Y, start.y );
        ChangeAngleValue( CSA_ANGLE, GetShape().GetArcAngle() );
    }

    void OnSMEChange()
    {
        const VECTOR2I p0{ GetIntValue( SME_START_X ), GetIntValue( SME_START_Y ) };
        const VECTOR2I p1{ GetIntValue( SME_MID_X ), GetIntValue( SME_MID_Y ) };
        const VECTOR2I p2{ GetIntValue( SME_END_X ), GetIntValue( SME_END_Y ) };

        GetShape().SetArcGeometry( p0, p1, p2 );

        updateCSA();
    }

    void updateSME()
    {
        const VECTOR2I p0 = GetShape().GetStart();
        const VECTOR2I p1 = GetShape().GetArcMid();
        const VECTOR2I p2 = GetShape().GetEnd();

        ChangeValue( SME_START_X, p0.x );
        ChangeValue( SME_START_Y, p0.y );
        ChangeValue( SME_MID_X, p1.x );
        ChangeValue( SME_MID_Y, p1.y );
        ChangeValue( SME_END_X, p2.x );
        ChangeValue( SME_END_Y, p2.y );
    }
};


class CIRCLE_GEOM_SYNCER : public GEOM_SYNCER
{
public:
    enum CTRL_IDX
    {
        CENTER_X = 0,
        CENTER_Y,
        RADIUS,
        CENTER_PT_X,
        CENTER_PT_Y,
        PT_PT_X,
        PT_PT_Y,

        NUM_CTRLS,
    };

    CIRCLE_GEOM_SYNCER( PCB_SHAPE& aShape, std::vector<BOUND_CONTROL>& aBoundCtrls ) :
            GEOM_SYNCER( aShape, aBoundCtrls )
    {
        wxASSERT( aBoundCtrls.size() == NUM_CTRLS );
        wxASSERT( GetShape().GetShape() == SHAPE_T::CIRCLE );

        BindCtrls( CENTER_X, RADIUS,
                   [this]()
                   {
                       OnCenterRadiusChange();
                   } );

        BindCtrls( CENTER_PT_X, PT_PT_Y,
                   [this]()
                   {
                       OnCenterPointChange();
                   } );
    }

    void updateAll() override
    {
        updateCenterRadius();
        updateCenterPoint();
    }

    bool Validate( wxArrayString& aErrs ) const override
    {
        if( GetIntValue( RADIUS ) <= 0 )
        {
            aErrs.push_back( _( "Radius must be greater than 0" ) );
            return false;
        }

        return true;
    }

    void OnCenterRadiusChange()
    {
        const VECTOR2I center{ GetIntValue( CENTER_X ), GetIntValue( CENTER_Y ) };
        const int      radius = GetIntValue( RADIUS );

        GetShape().SetCenter( center );
        GetShape().SetRadius( radius );

        updateCenterPoint();
    }

    void updateCenterRadius()
    {
        const VECTOR2I center = GetShape().GetCenter();

        ChangeValue( CENTER_X, center.x );
        ChangeValue( CENTER_Y, center.y );
        ChangeValue( RADIUS, GetShape().GetRadius() );
    }

    void OnCenterPointChange()
    {
        const VECTOR2I center{ GetIntValue( CENTER_PT_X ), GetIntValue( CENTER_PT_Y ) };
        const VECTOR2I pt{ GetIntValue( PT_PT_X ), GetIntValue( PT_PT_Y ) };

        GetShape().SetCenter( center );
        GetShape().SetEnd( pt );

        updateCenterRadius();
    }

    void updateCenterPoint()
    {
        const VECTOR2I center = GetShape().GetCenter();
        const VECTOR2I pt = GetShape().GetEnd();

        ChangeValue( CENTER_PT_X, center.x );
        ChangeValue( CENTER_PT_Y, center.y );
        ChangeValue( PT_PT_X, pt.x );
        ChangeValue( PT_PT_Y, pt.y );
    }
};


class BEZIER_GEOM_SYNCER : public GEOM_SYNCER
{
public:
    enum CTRL_IDX
    {
        START_X = 0,
        START_Y,
        END_X,
        END_Y,
        CTRL1_X,
        CTRL1_Y,
        CTRL2_X,
        CTRL2_Y,

        NUM_CTRLS,
    };

    BEZIER_GEOM_SYNCER( PCB_SHAPE& aShape, std::vector<BOUND_CONTROL>& aBoundCtrls ) :
            GEOM_SYNCER( aShape, aBoundCtrls )
    {
        wxASSERT( aBoundCtrls.size() == NUM_CTRLS );
        wxASSERT( GetShape().GetShape() == SHAPE_T::BEZIER );

        BindCtrls( START_X, CTRL2_Y,
                   [this]()
                   {
                       OnBezierChange();
                   } );
    }

    void updateAll() override
    {
        updateBezier();
    }

    void OnBezierChange()
    {
        const VECTOR2I p0{ GetIntValue( START_X ), GetIntValue( START_Y ) };
        const VECTOR2I p1{ GetIntValue( END_X ), GetIntValue( END_Y ) };
        const VECTOR2I c1{ GetIntValue( CTRL1_X ), GetIntValue( CTRL1_Y ) };
        const VECTOR2I c2{ GetIntValue( CTRL2_X ), GetIntValue( CTRL2_Y ) };

        GetShape().SetStart( p0 );
        GetShape().SetEnd( p1 );
        GetShape().SetBezierC1( c1 );
        GetShape().SetBezierC2( c2 );
    }

    void updateBezier()
    {
        const VECTOR2I p0 = GetShape().GetStart();
        const VECTOR2I p1 = GetShape().GetEnd();
        const VECTOR2I c1 = GetShape().GetBezierC1();
        const VECTOR2I c2 = GetShape().GetBezierC2();

        ChangeValue( START_X, p0.x );
        ChangeValue( START_Y, p0.y );
        ChangeValue( END_X, p1.x );
        ChangeValue( END_Y, p1.y );
        ChangeValue( CTRL1_X, c1.x );
        ChangeValue( CTRL1_Y, c1.y );
        ChangeValue( CTRL2_X, c2.x );
        ChangeValue( CTRL2_Y, c2.y );
    }
};


class DIALOG_SHAPE_PROPERTIES : public DIALOG_SHAPE_PROPERTIES_BASE
{
public:
    DIALOG_SHAPE_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_SHAPE* aShape );
    ~DIALOG_SHAPE_PROPERTIES() override = default;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onRoundedRectChanged( wxCommandEvent& event ) override;
    void onCornerRadius( wxCommandEvent& event ) override;
    void onLayerSelection( wxCommandEvent& event ) override;
    void onTechLayersChanged( wxCommandEvent& event ) override;

    bool Validate() override;

    void enableNetInfo()
    {
        bool isCopper = IsCopperLayer( m_LayerSelectionCtrl->GetLayerSelection() );

        m_netSelector->Enable( isCopper );
        m_netLabel->Enable( isCopper );
    }

    void enableTechLayers()
    {
        bool isExtCopper = IsExternalCopperLayer( m_LayerSelectionCtrl->GetLayerSelection() );

        m_techLayersLabel->Enable( isExtCopper );
        m_hasSolderMask->Enable( isExtCopper );

        bool showMaskMargin = isExtCopper && m_hasSolderMask->GetValue();

        m_solderMaskMarginLabel->Enable( showMaskMargin );
        m_solderMaskMarginCtrl->Enable( showMaskMargin );
        m_solderMaskMarginUnit->Enable( showMaskMargin );
    }

private:
    PCB_BASE_EDIT_FRAME*  m_parent;
    PCB_SHAPE*            m_item;

    UNIT_BINDER           m_cornerRadius;
    UNIT_BINDER           m_thickness;
    UNIT_BINDER           m_solderMaskMargin;

    std::vector<BOUND_CONTROL>   m_boundCtrls;
    std::unique_ptr<GEOM_SYNCER> m_geomSync;
    PCB_SHAPE                    m_workingCopy;
};


static void AddXYPointToSizer( EDA_DRAW_FRAME& aFrame, wxGridBagSizer& aSizer, int row, int col,
                               const wxString& aName, bool aRelative, std::vector<BOUND_CONTROL>& aBoundCtrls )
{
    //    Name
    // X [Ctrl] mm
    // Y [Ctrl] mm
    wxWindow* parent = aSizer.GetContainingWindow();

    wxStaticText* titleLabel = new wxStaticText( parent, wxID_ANY, aName );
    aSizer.Add( titleLabel, wxGBPosition( row, col ), wxGBSpan( 1, 3 ),
                wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL | wxALL | wxEXPAND );
    row++;

    for( size_t coord = 0; coord < 2; ++coord )
    {
        wxStaticText* label = new wxStaticText( parent, wxID_ANY, coord == 0 ? _( "X:" ) : _( "Y:" ) );
        aSizer.Add( label, wxGBPosition( row, col ), wxDefaultSpan,
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, col > 0 ? 20 : 5 );

        wxTextCtrl* ctrl = new wxTextCtrl( parent, wxID_ANY, "" );
        aSizer.Add( ctrl, wxGBPosition( row, col + 1 ), wxDefaultSpan,
                    wxEXPAND | wxALIGN_CENTER_VERTICAL, 5 );

        wxStaticText* units = new wxStaticText( parent, wxID_ANY, _( "mm" ) );
        aSizer.Add( units, wxGBPosition( row, col + 2 ), wxDefaultSpan,
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

        auto binder = std::make_unique<UNIT_BINDER>( &aFrame, label, ctrl, units );

        if( aRelative )
            binder->SetCoordType( coord == 0 ? ORIGIN_TRANSFORMS::REL_X_COORD : ORIGIN_TRANSFORMS::REL_Y_COORD );
        else
            binder->SetCoordType( coord == 0 ? ORIGIN_TRANSFORMS::ABS_X_COORD : ORIGIN_TRANSFORMS::ABS_Y_COORD );

        aBoundCtrls.push_back( BOUND_CONTROL{ std::move( binder ), ctrl } );
        row++;
    }

    if( !aSizer.IsColGrowable( col + 1 ) )
        aSizer.AddGrowableCol( col + 1 );
}


void AddFieldToSizer( EDA_DRAW_FRAME& aFrame, wxGridBagSizer& aSizer, int row, int col,
                      const wxString& aName, ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType,
                      bool aIsAngle, std::vector<BOUND_CONTROL>& aBoundCtrls )
{
    // Name: [Ctrl] mm
    wxWindow* parent = aSizer.GetContainingWindow();

    wxStaticText* label = new wxStaticText( parent, wxID_ANY, aName + wxS( ":" ) );
    aSizer.Add( label, wxGBPosition( row, col ), wxDefaultSpan,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxLEFT, col > 0 ? 20 : 5 );

    wxTextCtrl* ctrl = new wxTextCtrl( parent, wxID_ANY );
    aSizer.Add( ctrl, wxGBPosition( row, col + 1 ), wxDefaultSpan,
                wxEXPAND | wxALIGN_CENTER_VERTICAL, 5 );

    wxStaticText* units = new wxStaticText( parent, wxID_ANY, _( "mm" ) );
    aSizer.Add( units, wxGBPosition( row, col + 2 ), wxDefaultSpan,
                wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxRIGHT, 5 );

    auto binder = std::make_unique<UNIT_BINDER>( &aFrame, label, ctrl, units );
    binder->SetCoordType( aCoordType );

    if( aIsAngle )
    {
        binder->SetPrecision( 4 );
        binder->SetUnits( EDA_UNITS::DEGREES );
    }

    aBoundCtrls.push_back( BOUND_CONTROL{ std::move( binder ), ctrl } );

    if( !aSizer.IsColGrowable( col + 1 ) )
        aSizer.AddGrowableCol( col + 1 );
}


static std::map<SHAPE_T, int> s_lastTabForShape;


DIALOG_SHAPE_PROPERTIES::DIALOG_SHAPE_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_SHAPE* aShape ):
        DIALOG_SHAPE_PROPERTIES_BASE( aParent ),
        m_parent( aParent ),
        m_item( aShape ),
        m_cornerRadius( aParent, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits ),
        m_thickness( aParent, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits ),
        m_solderMaskMargin( aParent, m_solderMaskMarginLabel, m_solderMaskMarginCtrl, m_solderMaskMarginUnit ),
        m_workingCopy( *m_item )
{
    SetTitle( wxString::Format( GetTitle(), m_item->GetFriendlyName() ) );
    m_hash_key = TO_UTF8( GetTitle() );

    wxFont infoFont = KIUI::GetSmallInfoFont( this );
    m_techLayersLabel->SetFont( infoFont );

    // All the pages exist in the WxFB template, but we'll scrap the ones we don't
    // use. Constructing on-demand would work fine too.
    std::set<int> shownPages;

    const auto showPage =
            [&]( wxSizer& aMainSizer, bool aSelect = false )
            {
                // Get the parent of the sizer, which is the panel
                wxWindow* page = aMainSizer.GetContainingWindow();
                wxCHECK( page, /* void */ );
                page->Layout();

                const int pageIdx = m_notebookShapeDefs->FindPage( page );
                shownPages.insert( pageIdx );

                if( aSelect )
                    m_notebookShapeDefs->SetSelection( pageIdx );
            };

    switch( m_item->GetShape() )
    {
    case SHAPE_T::RECTANGLE:
        // For all these functions, it's very important that the fields are added in the same order
        // as the CTRL_IDX enums in the GEOM_SYNCER classes.
        AddXYPointToSizer( *aParent, *m_gbsRectangleByCorners, 0, 0, _( "Start Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsRectangleByCorners, 0, 3, _( "End Point" ), false, m_boundCtrls );

        AddXYPointToSizer( *aParent, *m_gbsRectangleByCornerSize, 0, 0, _( "Start Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsRectangleByCornerSize, 0, 3, _( "Size" ), true, m_boundCtrls );

        AddXYPointToSizer( *aParent, *m_gbsRectangleByCenterSize, 0, 0, _( "Center" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsRectangleByCenterSize, 0, 3, _( "Size" ), true, m_boundCtrls );

        m_geomSync = std::make_unique<RECTANGLE_GEOM_SYNCER>( m_workingCopy, m_boundCtrls );

        showPage( *m_gbsRectangleByCorners, true );
        showPage( *m_gbsRectangleByCornerSize );
        showPage( *m_gbsRectangleByCenterSize );
        break;

    case SHAPE_T::SEGMENT:

        AddXYPointToSizer( *aParent, *m_gbsLineByEnds, 0, 0, _( "Start Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsLineByEnds, 0, 3, _( "End Point" ), false, m_boundCtrls );

        AddXYPointToSizer( *aParent, *m_gbsLineByLengthAngle, 0, 0, _( "Start Point" ), false, m_boundCtrls);
        AddFieldToSizer( *aParent, *m_gbsLineByLengthAngle, 1, 3, _( "Length" ), ORIGIN_TRANSFORMS::NOT_A_COORD, false, m_boundCtrls );
        AddFieldToSizer( *aParent, *m_gbsLineByLengthAngle, 2, 3, _( "Angle" ), ORIGIN_TRANSFORMS::NOT_A_COORD, true, m_boundCtrls );

        AddXYPointToSizer( *aParent, *m_gbsLineByStartMid, 0, 0, _( "Start Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsLineByStartMid, 0, 3, _( "Mid Point" ), false, m_boundCtrls );

        m_geomSync = std::make_unique<LINE_GEOM_SYNCER>( m_workingCopy, m_boundCtrls );

        showPage( *m_gbsLineByEnds, true );
        showPage( *m_gbsLineByLengthAngle );
        showPage( *m_gbsLineByStartMid );

        m_cbRoundRect->Show( false );
        m_cornerRadius.Show( false );
        break;

    case SHAPE_T::ARC:
        AddXYPointToSizer( *aParent, *m_gbsArcByCSA, 0, 0, _( "Center" ), false, m_boundCtrls);
        AddXYPointToSizer( *aParent, *m_gbsArcByCSA, 0, 3, _( "Start Point" ), false, m_boundCtrls);
        AddFieldToSizer( *aParent, *m_gbsArcByCSA, 3, 0, _( "Included Angle" ), ORIGIN_TRANSFORMS::NOT_A_COORD, true, m_boundCtrls );

        AddXYPointToSizer( *aParent, *m_gbsArcBySME, 0, 0, _( "Start Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsArcBySME, 0, 3, _( "Mid Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsArcBySME, 3, 0, _( "End Point" ), false, m_boundCtrls );

        m_geomSync = std::make_unique<ARC_GEOM_SYNCER>( m_workingCopy, m_boundCtrls );

        showPage( *m_gbsArcByCSA, true );
        showPage( *m_gbsArcBySME );

        m_cbRoundRect->Show( false );
        m_cornerRadius.Show( false );
        break;

    case SHAPE_T::CIRCLE:
        AddXYPointToSizer( *aParent, *m_gbsCircleCenterRadius, 0, 0, _( "Center" ), false, m_boundCtrls);
        AddFieldToSizer( *aParent, *m_gbsCircleCenterRadius, 3, 0, _( "Radius" ), ORIGIN_TRANSFORMS::NOT_A_COORD, false, m_boundCtrls );

        AddXYPointToSizer( *aParent, *m_gbsCircleCenterPoint, 0, 0, _( "Center" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsCircleCenterPoint, 0, 3, _( "Point on Circle" ), false, m_boundCtrls );

        m_geomSync = std::make_unique<CIRCLE_GEOM_SYNCER>( m_workingCopy, m_boundCtrls );

        showPage( *m_gbsCircleCenterRadius, true );
        showPage( *m_gbsCircleCenterPoint );

        m_cbRoundRect->Show( false );
        m_cornerRadius.Show( false );
        break;

    case SHAPE_T::BEZIER:
        AddXYPointToSizer( *aParent, *m_gbsBezier, 0, 0, _( "Start Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsBezier, 0, 3, _( "End Point" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsBezier, 3, 0, _( "Control Point 1" ), false, m_boundCtrls );
        AddXYPointToSizer( *aParent, *m_gbsBezier, 3, 3, _( "Control Point 2" ), false, m_boundCtrls );

        m_geomSync = std::make_unique<BEZIER_GEOM_SYNCER>( m_workingCopy, m_boundCtrls );

        showPage( *m_gbsBezier, TRUE );
        break;

    case SHAPE_T::POLY:
        m_notebookShapeDefs->Hide();
        // Nothing to do here...yet

        m_cbRoundRect->Show( false );
        m_cornerRadius.Show( false );
        break;

    case SHAPE_T::UNDEFINED:
        wxFAIL_MSG( "Undefined shape" );
        break;
    }

    // Remove any tabs not used (Hide() doesn't work on Windows)
    for( int i = (int) m_notebookShapeDefs->GetPageCount() - 1; i >= 0; --i )
    {
        if( shownPages.count( i ) == 0 )
            m_notebookShapeDefs->RemovePage( i );
    }

    // Used the last saved tab if any
    if( s_lastTabForShape.count( m_item->GetShape() ) > 0
            && s_lastTabForShape[m_item->GetShape()] < (int) m_notebookShapeDefs->GetPageCount()
            && s_lastTabForShape[m_item->GetShape()] >= 0 )
    {
        m_notebookShapeDefs->SetSelection( s_lastTabForShape[m_item->GetShape()] );
    }

    // Find the first control in the shown tab
    wxWindow* tabPanel = m_notebookShapeDefs->GetCurrentPage();

    for( size_t i = 0; i < m_boundCtrls.size(); ++i )
    {
        if( m_boundCtrls[i].m_Ctrl->IsDescendant( tabPanel ) )
        {
            m_boundCtrls[i].m_Ctrl->SetFocus();
            break;
        }
    }

    // Do not allow locking items in the footprint editor
    m_locked->Show( dynamic_cast<PCB_EDIT_FRAME*>( aParent ) != nullptr );

    // Configure the layers list selector
    if( m_parent->GetFrameType() == FRAME_FOOTPRINT_EDITOR )
    {
        // In the footprint editor, turn off the layers that the footprint doesn't have
        const LSET& brdLayers = aParent->GetBoard()->GetEnabledLayers();
        LSET        forbiddenLayers = LSET::AllLayersMask() & ~brdLayers;

        m_LayerSelectionCtrl->SetNotAllowedLayerSet( forbiddenLayers );
    }

    for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        m_lineStyleCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    m_netSelector->SetNetInfo( &aParent->GetBoard()->GetNetInfo() );

    if( m_parent->GetFrameType() == FRAME_FOOTPRINT_EDITOR )
    {
        m_netLabel->Hide();
        m_netSelector->Hide();
    }

    if( m_item->GetShape() == SHAPE_T::ARC
        || m_item->GetShape() == SHAPE_T::SEGMENT
        || m_item->GetShape() == SHAPE_T::BEZIER )
    {
        m_fillLabel->Show( false );
        m_fillCtrl->Show( false );
    }

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void PCB_BASE_EDIT_FRAME::ShowGraphicItemPropertiesDialog( PCB_SHAPE* aShape )
{
    wxCHECK_RET( aShape, wxT( "ShowGraphicItemPropertiesDialog() error: NULL item" ) );

    DIALOG_SHAPE_PROPERTIES dlg( this, aShape );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        if( aShape->IsOnLayer( GetActiveLayer() ) )
        {
            DRAWING_TOOL* drawingTool = m_toolManager->GetTool<DRAWING_TOOL>();
            drawingTool->SetStroke( aShape->GetStroke(), GetActiveLayer() );
        }
    }
}


void DIALOG_SHAPE_PROPERTIES::onRoundedRectChanged( wxCommandEvent &event )
{
    if( !m_cbRoundRect->GetValue() )
        m_cornerRadius.ChangeValue( wxEmptyString );
}


void DIALOG_SHAPE_PROPERTIES::onCornerRadius( wxCommandEvent &event )
{
    m_cbRoundRect->SetValue( true );
}


void DIALOG_SHAPE_PROPERTIES::onLayerSelection( wxCommandEvent& event )
{
    if( m_LayerSelectionCtrl->GetLayerSelection() >= 0 )
        enableNetInfo();

    enableTechLayers();
}


void DIALOG_SHAPE_PROPERTIES::onTechLayersChanged( wxCommandEvent& event )
{
    enableTechLayers();
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !m_item )
        return false;

    // Not all shapes have a syncer (e.g. polygons)
    if( m_geomSync )
        m_geomSync->SetShape( *m_item );

    m_fillCtrl->SetSelection( m_item->GetFillModeProp() );
    m_locked->SetValue( m_item->IsLocked() );

    if( m_item->GetShape() == SHAPE_T::RECTANGLE )
    {
        if( m_item->GetCornerRadius() > 0 )
        {
            m_cbRoundRect->SetValue( true );
            m_cornerRadius.ChangeValue( m_item->GetCornerRadius() );
        }
        else
        {
            m_cbRoundRect->SetValue( false );
            m_cornerRadius.ChangeValue( wxEmptyString );
        }
    }

    m_thickness.SetValue( m_item->GetStroke().GetWidth() );

    int style = static_cast<int>( m_item->GetStroke().GetLineStyle() );

    if( style >= 0 && style < (int) lineTypeNames.size() )
        m_lineStyleCombo->SetSelection( style );
    else
        m_lineStyleCombo->SetSelection( 0 );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );

    m_hasSolderMask->SetValue( m_item->HasSolderMask() );

    if( m_item->GetLocalSolderMaskMargin().has_value() )
        m_solderMaskMargin.SetValue( m_item->GetLocalSolderMaskMargin().value() );
    else
        m_solderMaskMargin.SetValue( wxEmptyString );

    if( m_parent->GetFrameType() == FRAME_PCB_EDITOR )
    {
        int net = m_item->GetNetCode();

        if( net >= 0 )
        {
            m_netSelector->SetSelectedNetcode( net );
        }
        else
        {
            m_netSelector->SetIndeterminateString( INDETERMINATE_STATE );
            m_netSelector->SetIndeterminate();
        }
    }

    enableNetInfo();
    enableTechLayers();

    return DIALOG_SHAPE_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_SHAPE_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    if( !m_item )
        return true;

    int layer = m_LayerSelectionCtrl->GetLayerSelection();

    BOARD_COMMIT commit( m_parent );
    commit.Modify( m_item );

    bool pushCommit = ( m_item->GetEditFlags() == 0 );

    // Set IN_EDIT flag to force undo/redo/abort proper operation and avoid new calls to
    // SaveCopyInUndoList for the same text if is moved, and then rotated, edited, etc....
    if( !pushCommit )
        m_item->SetFlags( IN_EDIT );

    *m_item = m_workingCopy;

    bool wasLocked = m_item->IsLocked();

    if( m_item->GetShape() == SHAPE_T::RECTANGLE )
        m_item->SetCornerRadius( m_cbRoundRect->GetValue() ? m_cornerRadius.GetIntValue() : 0 );

    m_item->SetFillModeProp( (UI_FILL_MODE) m_fillCtrl->GetSelection() );
    m_item->SetLocked( m_locked->GetValue() );

    m_item->SetWidth( m_thickness.GetIntValue() );

    auto it = lineTypeNames.begin();
    std::advance( it, m_lineStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        m_item->SetLineStyle( LINE_STYLE::SOLID );
    else
        m_item->SetLineStyle( it->first );

    m_item->SetLayer( ToLAYER_ID( layer ) );

    m_item->SetHasSolderMask( m_hasSolderMask->GetValue() );

    if( m_solderMaskMargin.IsNull() )
        m_item->SetLocalSolderMaskMargin( {} );
    else
        m_item->SetLocalSolderMaskMargin( m_solderMaskMargin.GetIntValue() );

    m_item->RebuildBezierToSegmentsPointsList( m_item->GetMaxError() );

    if( m_item->IsOnCopperLayer() )
        m_item->SetNetCode( m_netSelector->GetSelectedNetcode() );
    else
        m_item->SetNetCode( -1 );

    if( pushCommit )
        commit.Push( _( "Edit Shape Properties" ) );

    // Save the tab
    s_lastTabForShape[m_item->GetShape()] = m_notebookShapeDefs->GetSelection();

    // Notify clients which treat locked and unlocked items differently (ie: POINT_EDITOR)
    if( wasLocked != m_item->IsLocked() )
        m_parent->GetToolManager()->PostEvent( EVENTS::SelectedEvent );

    return true;
}


bool DIALOG_SHAPE_PROPERTIES::Validate()
{
    wxArrayString errors;

    if( !DIALOG_SHAPE_PROPERTIES_BASE::Validate() )
        return false;

    if( m_geomSync )
        m_geomSync->Validate( errors );

    // Type specific checks.
    switch( m_item->GetShape() )
    {
    case SHAPE_T::ARC:
        if( m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero." ) );
        break;

    case SHAPE_T::CIRCLE:
        if( m_fillCtrl->GetSelection() != UI_FILL_MODE::SOLID && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled circle." ) );

        break;

    case SHAPE_T::RECTANGLE:
    {
        if( m_fillCtrl->GetSelection() != UI_FILL_MODE::SOLID && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled rectangle." ) );

        int shortSide = std::min( m_item->GetRectangleWidth(), m_item->GetRectangleHeight() );

        if( m_cbRoundRect->GetValue() && m_cornerRadius.GetIntValue() * 2 > shortSide )
        {
            errors.Add( _( "Corner radius must be less than or equal to half the smaller side." ) );
            m_cornerRadius.SetValue( KiROUND( shortSide / 2.0 ) );
        }

        break;
    }

    case SHAPE_T::POLY:
        if( m_fillCtrl->GetSelection() != UI_FILL_MODE::SOLID && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled polygon." ) );

        break;

    case SHAPE_T::SEGMENT:
        if( m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero." ) );

        break;

    case SHAPE_T::BEZIER:
        if( m_fillCtrl->GetSelection() != UI_FILL_MODE::SOLID && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled curve." ) );

        break;

    default:
        UNIMPLEMENTED_FOR( m_item->SHAPE_T_asString() );
        break;
    }

    if( errors.GetCount() )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error List" ) );
        dlg.ListSet( errors );
        dlg.ShowModal();
    }

    return errors.GetCount() == 0;
}
