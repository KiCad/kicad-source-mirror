/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_item.h>
#include <pcb_base_edit_frame.h>
#include <router/pns_meander.h>
#include <router/pns_meander_placer_base.h>
#include <router/pns_router.h>
#include <router/pns_routing_settings.h>


namespace KIGFX
{
    class VIEW;
    class RENDER_SETTINGS;
}

enum LENGTH_TUNING_MODE
{
    SINGLE,
    DIFF_PAIR,
    DIFF_PAIR_SKEW
};


class TUNING_STATUS_VIEW_ITEM : public EDA_ITEM
{
public:
    TUNING_STATUS_VIEW_ITEM( PCB_BASE_EDIT_FRAME* aFrame );

    wxString GetClass() const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    VECTOR2I GetPosition() const override;
    void     SetPosition( const VECTOR2I& aPos ) override;

    void SetMinMax( const double aMin, const double aMax );

    void ClearMinMax();

    void SetCurrent( const double aCurrent, const wxString& aLabel );

    void SetIsTimeDomain( const bool aIsTimeDomain );

    const BOX2I ViewBBox() const override;

    std::vector<int> ViewGetLayers() const override;

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

protected:
    EDA_DRAW_FRAME* m_frame;
    VECTOR2I        m_pos;
    double          m_min;
    double          m_max;
    double          m_current;
    wxString        m_currentLabel;
    wxString        m_currentText;
    wxString        m_minText;
    wxString        m_maxText;
    bool            m_isTimeDomain;
};


class PCB_TUNING_PATTERN : public PCB_GENERATOR
{
public:
    static const wxString GENERATOR_TYPE;
    static const wxString DISPLAY_NAME;

    PCB_TUNING_PATTERN( BOARD_ITEM* aParent = nullptr, PCB_LAYER_ID aLayer = F_Cu,
                        LENGTH_TUNING_MODE aMode = LENGTH_TUNING_MODE::SINGLE );

    wxString GetGeneratorType() const override { return wxS( "tuning_pattern" ); }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override
    {
        return wxString( _( "Tuning Pattern" ) );
    }

    wxString GetFriendlyName() const override
    {
        return wxString( _( "Tuning Pattern" ) );
    }

    wxString GetPluralName() const override
    {
        return wxString( _( "Tuning Patterns" ) );
    }

    BITMAPS GetMenuImage() const override
    {
        switch( m_tuningMode )
        {
        case SINGLE:         return BITMAPS::ps_tune_length;           break;
        case DIFF_PAIR:      return BITMAPS::ps_diff_pair_tune_length; break;
        case DIFF_PAIR_SKEW: return BITMAPS::ps_diff_pair_tune_phase;  break;
        }

        return BITMAPS::unknown;
    }

    static PCB_TUNING_PATTERN* CreateNew( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                                          BOARD_CONNECTED_ITEM* aStartItem,
                                          LENGTH_TUNING_MODE aMode );

    void EditStart( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;

    bool Update( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;

    void EditPush( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit,
                   const wxString& aCommitMsg = wxEmptyString, int aCommitFlags = 0 ) override;

    void EditRevert( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;

    void Remove( GENERATOR_TOOL* aTool, BOARD* aBoard, BOARD_COMMIT* aCommit ) override;

    bool MakeEditPoints( EDIT_POINTS& points ) const override;

    bool UpdateFromEditPoints( EDIT_POINTS& aEditPoints ) override;

    bool UpdateEditPoints( EDIT_POINTS& aEditPoints ) override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        m_origin += aMoveVector;
        m_end += aMoveVector;

        if( !this->HasFlag( IN_EDIT ) )
        {
            PCB_GROUP::Move( aMoveVector );

            if( m_baseLine )
                m_baseLine->Move( aMoveVector );

            if( m_baseLineCoupled )
                m_baseLineCoupled->Move( aMoveVector );
        }
    }

    void Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle ) override
    {
        if( !this->HasFlag( IN_EDIT ) )
        {
            PCB_GENERATOR::Rotate( aRotCentre, aAngle );
            RotatePoint( m_end, aRotCentre, aAngle );

            if( m_baseLine )
                m_baseLine->Rotate( aAngle, aRotCentre );

            if( m_baseLineCoupled )
                m_baseLineCoupled->Rotate( aAngle, aRotCentre );
        }
    }

    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override
    {
        if( !this->HasFlag( IN_EDIT ) )
        {
            PCB_GENERATOR::Flip( aCentre, aFlipDirection );

            baseMirror( aCentre, aFlipDirection );
        }
    }

    void Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override
    {
        if( !this->HasFlag( IN_EDIT ) )
        {
            PCB_GENERATOR::Mirror( aCentre, aFlipDirection );

            baseMirror( aCentre, aFlipDirection );
        }
    }

    void SetLayer( PCB_LAYER_ID aLayer ) override
    {
        PCB_GENERATOR::SetLayer( aLayer );

        for( BOARD_ITEM* item : GetBoardItems() )
            item->SetLayer( aLayer );
    }

    PCB_LAYER_ID GetLayer() const override
    {
        return PCB_GENERATOR::GetLayer();
    }

    const BOX2I GetBoundingBox() const override
    {
        return getOutline().BBox();
    }

    std::vector<int> ViewGetLayers() const override
    {
        return { LAYER_ANCHOR, GetLayer() };
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override
    {
        return getOutline().Collide( aPosition, aAccuracy );
    }

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const override
    {
        BOX2I sel = aRect;

        if ( aAccuracy )
            sel.Inflate( aAccuracy );

        if( aContained )
            return sel.Contains( GetBoundingBox() );

        return sel.Intersects( GetBoundingBox() );
    }

    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override
    {
        return KIGEOM::ShapeHitTest( aPoly, getOutline(), aContained );
    }

    const BOX2I ViewBBox() const override { return GetBoundingBox(); }

    EDA_ITEM* Clone() const override { return new PCB_TUNING_PATTERN( *this ); }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

    const VECTOR2I& GetEnd() const { return m_end; }
    void            SetEnd( const VECTOR2I& aValue ) { m_end = aValue; }

    int  GetEndX() const { return m_end.x; }
    void SetEndX( int aValue ) { m_end.x = aValue; }

    int  GetEndY() const { return m_end.y; }
    void SetEndY( int aValue ) { m_end.y = aValue; }

    int GetWidth() const
    {
        for( BOARD_ITEM* item : GetBoardItems() )
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                return track->GetWidth();

        return m_trackWidth;
    }

    void SetWidth( int aValue )
    {
        m_trackWidth = aValue;

        for( BOARD_ITEM* item : GetBoardItems() )
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                track->SetWidth( aValue );
    }

    int GetNetCode() const
    {
        for( BOARD_ITEM* item : GetBoardItems() )
            if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                return bci->GetNetCode();

        return 0;
    }

    void SetNetCode( int aNetCode )
    {
        if( BOARD* board = GetBoard() )
        {
            if( NETINFO_ITEM* net = board->FindNet( aNetCode ) )
                m_lastNetName = net->GetNetname();
            else
                m_lastNetName.clear();
        }

        for( BOARD_ITEM* item : GetBoardItems() )
            if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
                bci->SetNetCode( aNetCode );
    }

    bool HasSolderMask() const
    {
        for( BOARD_ITEM* item : GetBoardItems() )
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                return track->HasSolderMask();

        return true;
    }

    void SetHasSolderMask( bool aVal )
    {
        for( BOARD_ITEM* item : GetBoardItems() )
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                track->SetHasSolderMask( aVal );
    }

    std::optional<int> GetLocalSolderMaskMargin() const
    {
        for( BOARD_ITEM* item : GetBoardItems() )
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                return track->GetLocalSolderMaskMargin();

        return std::optional<int>();
    }

    void SetLocalSolderMaskMargin( std::optional<int> aMargin )
    {
        for( BOARD_ITEM* item : GetBoardItems() )
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
                track->SetLocalSolderMaskMargin( aMargin );
    }

    LENGTH_TUNING_MODE GetTuningMode() const { return m_tuningMode; }

    PNS::ROUTER_MODE GetPNSMode()
    {
        switch( m_tuningMode )
        {
        case LENGTH_TUNING_MODE::SINGLE:         return PNS::PNS_MODE_TUNE_SINGLE;
        case LENGTH_TUNING_MODE::DIFF_PAIR:      return PNS::PNS_MODE_TUNE_DIFF_PAIR;
        case LENGTH_TUNING_MODE::DIFF_PAIR_SKEW: return PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW;
        default:                                 return PNS::PNS_MODE_TUNE_SINGLE;
        }
    }

    PNS::MEANDER_SETTINGS& GetSettings() { return m_settings; }

    int  GetMinAmplitude() const { return m_settings.m_minAmplitude; }
    void SetMinAmplitude( int aValue )
    {
        aValue = std::max( aValue, 0 );

        m_settings.m_minAmplitude = aValue;

        if( m_settings.m_maxAmplitude < m_settings.m_minAmplitude )
            m_settings.m_maxAmplitude = m_settings.m_minAmplitude;
    }

    int  GetMaxAmplitude() const { return m_settings.m_maxAmplitude; }
    void SetMaxAmplitude( int aValue )
    {
        aValue = std::max( aValue, 0 );

        m_settings.m_maxAmplitude = aValue;

        if( m_settings.m_maxAmplitude < m_settings.m_minAmplitude )
            m_settings.m_minAmplitude = m_settings.m_maxAmplitude;
    }

    // Update the initial side one time at EditStart based on m_end.
    void UpdateSideFromEnd() { m_updateSideFromEnd = true; }

    PNS::MEANDER_SIDE GetInitialSide() const { return m_settings.m_initialSide; }
    void              SetInitialSide( PNS::MEANDER_SIDE aValue ) { m_settings.m_initialSide = aValue; }

    int  GetSpacing() const { return m_settings.m_spacing; }
    void SetSpacing( int aValue ) { m_settings.m_spacing = aValue; }

    std::optional<int> GetTargetLength() const
    {
        if( m_settings.m_targetLength.Opt() == PNS::MEANDER_SETTINGS::LENGTH_UNCONSTRAINED )
            return std::optional<int>();
        else
            return m_settings.m_targetLength.Opt();
    }

    void SetTargetLength( std::optional<int> aValue )
    {
        m_settings.m_isTimeDomain = false;

        if( aValue.has_value() )
            m_settings.SetTargetLength( aValue.value() );
        else
            m_settings.SetTargetLength( PNS::MEANDER_SETTINGS::LENGTH_UNCONSTRAINED );
    }

    std::optional<int> GetTargetDelay() const
    {
        if( m_settings.m_targetLengthDelay.Opt() == PNS::MEANDER_SETTINGS::DELAY_UNCONSTRAINED )
            return std::optional<int>();
        else
            return m_settings.m_targetLengthDelay.Opt();
    }

    void SetTargetDelay( std::optional<int> aValue )
    {
        m_settings.m_isTimeDomain = true;

        if( aValue.has_value() )
            m_settings.SetTargetLengthDelay( aValue.value() );
        else
            m_settings.SetTargetLengthDelay( PNS::MEANDER_SETTINGS::DELAY_UNCONSTRAINED );
    }

    int  GetTargetSkew() const { return m_settings.m_targetSkew.Opt(); }
    void SetTargetSkew( int aValue ) { m_settings.SetTargetSkew( aValue ); }

    int  GetTargetSkewDelay() const { return m_settings.m_targetSkewDelay.Opt(); }
    void SetTargetSkewDelay( int aValue ) { m_settings.SetTargetSkewDelay( aValue ); }

    bool GetOverrideCustomRules() const { return m_settings.m_overrideCustomRules; }
    void SetOverrideCustomRules( bool aOverride ) { m_settings.m_overrideCustomRules = aOverride; }

    int  GetCornerRadiusPercentage() const { return m_settings.m_cornerRadiusPercentage; }
    void SetCornerRadiusPercentage( int aValue ) { m_settings.m_cornerRadiusPercentage = aValue; }

    bool IsSingleSided() const { return m_settings.m_singleSided; }
    void SetSingleSided( bool aValue ) { m_settings.m_singleSided = aValue; }

    bool IsRounded() const { return m_settings.m_cornerStyle == PNS::MEANDER_STYLE_ROUND; }
    void SetRounded( bool aFlag ) { m_settings.m_cornerStyle = aFlag ? PNS::MEANDER_STYLE_ROUND
                                                                     : PNS::MEANDER_STYLE_CHAMFER; }

    std::vector<std::pair<wxString, wxVariant>> GetRowData() override
    {
        std::vector<std::pair<wxString, wxVariant>> data = PCB_GENERATOR::GetRowData();
        data.emplace_back( _HKI( "Net" ), m_lastNetName );
        data.emplace_back( _HKI( "Tuning" ), m_tuningInfo );
        return data;
    }

    const STRING_ANY_MAP GetProperties() const override;
    void SetProperties( const STRING_ANY_MAP& aProps ) override;

    void ShowPropertiesDialog( PCB_BASE_EDIT_FRAME* aEditFrame ) override;

    std::vector<EDA_ITEM*> GetPreviewItems( GENERATOR_TOOL* aTool, PCB_BASE_EDIT_FRAME* aFrame,
                                            bool aStatusItemsOnly = false ) override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    void swapData( BOARD_ITEM* aImage ) override
    {
        wxASSERT( aImage->Type() == PCB_GENERATOR_T );

        std::swap( *this, *static_cast<PCB_TUNING_PATTERN*>( aImage ) );
    }

    bool recoverBaseline( PNS::ROUTER* aRouter );

    bool baselineValid();

    bool initBaseLine( PNS::ROUTER* aRouter, int aPNSLayer, BOARD* aBoard, VECTOR2I& aStart,
                       VECTOR2I& aEnd, NETINFO_ITEM* aNet,
                       std::optional<SHAPE_LINE_CHAIN>& aBaseLine );

    bool initBaseLines( PNS::ROUTER* aRouter, int aPNSLayer, BOARD* aBoard );

    bool removeToBaseline( PNS::ROUTER* aRouter, int aPNSLayer, SHAPE_LINE_CHAIN& aBaseLine );

    bool resetToBaseline( GENERATOR_TOOL* aTool, int aPNSLayer, SHAPE_LINE_CHAIN& aBaseLine,
                          bool aPrimary );

    SHAPE_LINE_CHAIN getOutline() const;

    void baseMirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
    {
        PCB_GENERATOR::baseMirror( aCentre, aFlipDirection );

        if( m_baseLine )
        {
            m_baseLine->Mirror( aCentre, aFlipDirection );
            m_origin = m_baseLine->CPoint( 0 );
            m_end = m_baseLine->CLastPoint();
        }

        if( m_baseLineCoupled )
            m_baseLineCoupled->Mirror( aCentre, aFlipDirection );

        if( m_settings.m_initialSide == PNS::MEANDER_SIDE_RIGHT )
            m_settings.m_initialSide = PNS::MEANDER_SIDE_LEFT;
        else
            m_settings.m_initialSide = PNS::MEANDER_SIDE_RIGHT;
    }

protected:
    VECTOR2I              m_end;

    PNS::MEANDER_SETTINGS m_settings;

    std::optional<SHAPE_LINE_CHAIN> m_baseLine;
    std::optional<SHAPE_LINE_CHAIN> m_baseLineCoupled;

    int                   m_trackWidth;
    int                   m_diffPairGap;

    LENGTH_TUNING_MODE    m_tuningMode;

    wxString              m_lastNetName;
    wxString              m_tuningInfo;

    PNS::MEANDER_PLACER_BASE::TUNING_STATUS m_tuningStatus;

    bool                  m_updateSideFromEnd;
};