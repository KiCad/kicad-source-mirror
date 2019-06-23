/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file board_design_settings.cpp
 * BOARD_DESIGN_SETTINGS class functions.
 */

#include <fctsys.h>
#include <common.h>
#include <class_board.h>
#include <layers_id_colors_and_visibility.h>
#include <kiface_i.h>
#include <pcbnew.h>
#include <board_design_settings.h>


#define CopperLayerCountKey         wxT( "CopperLayerCount" )
#define BoardThicknessKey           wxT( "BoardThickness" )

#define LayerKeyPrefix              wxT( "Layer" )
#define LayerNameKey                wxT( "Name" )
#define LayerTypeKey                wxT( "Type" )
#define LayerEnabledKey             wxT( "Enabled" )

#define NetclassNameKey             wxT( "Name" )
#define ClearanceKey                wxT( "Clearance" )
#define TrackWidthKey               wxT( "TrackWidth" )
#define ViaDiameterKey              wxT( "ViaDiameter" )
#define ViaDrillKey                 wxT( "ViaDrill" )
#define uViaDiameterKey             wxT( "uViaDiameter" )
#define uViaDrillKey                wxT( "uViaDrill" )
#define dPairWidthKey               wxT( "dPairWidth" )
#define dPairGapKey                 wxT( "dPairGap" )
#define dPairViaGapKey              wxT( "dPairViaGap" )


//
// NOTE: layer configuration info is stored in both the BOARD and BOARD_DESIGN_SETTINGS so one
// of the two needs to read/write the config so we don't end up with order dependency issues.
//
class PARAM_CFG_LAYERS : public PARAM_CFG_BASE
{
protected:
    BOARD* m_Pt_param;   ///< Pointer to the parameter value

public:
    PARAM_CFG_LAYERS( BOARD* ptparam, const wxChar* group = nullptr ) :
            PARAM_CFG_BASE( wxEmptyString, PARAM_LAYERS, group )
    {
        m_Pt_param = ptparam;
    }

    void ReadParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        BOARD*                 board = m_Pt_param;
        BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
        LSET                   enabledLayers = bds.GetEnabledLayers();
        wxString               oldPath = aConfig->GetPath();
        wxString               layerKeyPrefix = LayerKeyPrefix;

        bds.SetCopperLayerCount( aConfig->Read( CopperLayerCountKey, 2 ) );

        double thickness = aConfig->ReadDouble( BoardThicknessKey, DEFAULT_BOARD_THICKNESS_MM );
        bds.SetBoardThickness( Millimeter2iu( thickness ) );

        for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
        {
            PCB_LAYER_ID layer = *seq;
            wxString     path = layerKeyPrefix + wxT( "." ) + board->GetStandardLayerName( layer );
            wxString     layerName;
            int          layerType;
            bool         layerEnabled;

            aConfig->SetPath( oldPath );
            aConfig->SetPath( path );

            if( aConfig->Read( LayerNameKey, &layerName ) )
                board->SetLayerName( layer, layerName );

            if( aConfig->Read( LayerTypeKey, &layerType ) )
                board->SetLayerType( layer, (LAYER_T) layerType );

            if( aConfig->Read( LayerEnabledKey, &layerEnabled ) )
                enabledLayers.set( layer, layerEnabled );
        }

        board->SetEnabledLayers( enabledLayers );

        aConfig->SetPath( oldPath );
    }

    void SaveParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        BOARD*                 board = m_Pt_param;
        BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
        wxString               oldPath = aConfig->GetPath();
        wxString               layerKeyPrefix = LayerKeyPrefix;

        aConfig->Write( CopperLayerCountKey, board->GetCopperLayerCount() );
        aConfig->Write( BoardThicknessKey, Iu2Millimeter( bds.GetBoardThickness() ) );

        for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
        {
            PCB_LAYER_ID layer = *seq;
            wxString     path = layerKeyPrefix + wxT( "." ) + board->GetStandardLayerName( layer );
            wxString     layerName = board->GetLayerName( layer );
            LAYER_T      layerType = board->GetLayerType( layer );

            aConfig->SetPath( oldPath );
            aConfig->SetPath( path );

            if( IsCopperLayer( layer ) )
            {
                aConfig->Write( LayerNameKey, layerName );
                aConfig->Write( LayerTypeKey, (int) layerType );
            }

            aConfig->Write( LayerEnabledKey, board->IsLayerEnabled( layer ) );
        }

        aConfig->SetPath( oldPath );
    }
};


class PARAM_CFG_TRACKWIDTHS : public PARAM_CFG_BASE
{
protected:
    std::vector<int>* m_Pt_param;   ///< Pointer to the parameter value

public:
    PARAM_CFG_TRACKWIDTHS( std::vector<int>* ptparam, const wxChar* group = nullptr ) :
            PARAM_CFG_BASE( wxEmptyString, PARAM_TRACKWIDTHS, group )
    {
        m_Pt_param = ptparam;
    }

    void ReadParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        m_Pt_param->clear();

        for( int index = 1; ; ++index )
        {
            wxString key = TrackWidthKey;
            double width;

            if( !aConfig->Read( key << index, &width ) )
                break;

            m_Pt_param->push_back( Millimeter2iu( width ) );
        }
    }

    void SaveParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        for( size_t index = 1; index <= m_Pt_param->size(); ++index )
        {
            wxString key = TrackWidthKey;
            aConfig->Write( key << index, Iu2Millimeter( m_Pt_param->at( index - 1 ) ) );
        }
    }
};


class PARAM_CFG_VIADIMENSIONS : public PARAM_CFG_BASE
{
protected:
    std::vector<VIA_DIMENSION>* m_Pt_param;   ///< Pointer to the parameter value

public:
    PARAM_CFG_VIADIMENSIONS( std::vector<VIA_DIMENSION>* ptparam, const wxChar* group = nullptr ) :
            PARAM_CFG_BASE( wxEmptyString, PARAM_VIADIMENSIONS, group )
    {
        m_Pt_param = ptparam;
    }

    void ReadParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        m_Pt_param->clear();

        for( int index = 1; ; ++index )
        {
            double diameter = 0.0, drill = 0.0;

            wxString key = ViaDiameterKey;

            if( !aConfig->Read( key << index, &diameter ) )
                break;

            key = ViaDrillKey;
            drill = aConfig->ReadDouble( key << index, 0.0 );

            m_Pt_param->emplace_back( VIA_DIMENSION( Millimeter2iu( diameter ),
                                                     Millimeter2iu( drill ) ) );
        }
    }

    void SaveParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        for( size_t index = 1; index <= m_Pt_param->size(); ++index )
        {
            wxString key = ViaDiameterKey;
            aConfig->Write( key << index, Iu2Millimeter( m_Pt_param->at( index - 1 ).m_Diameter ) );
            key = ViaDrillKey;
            aConfig->Write( key << index, Iu2Millimeter( m_Pt_param->at( index - 1 ).m_Drill ) );
        }
    }
};


class PARAM_CFG_DIFFPAIRDIMENSIONS : public PARAM_CFG_BASE
{
protected:
    std::vector<DIFF_PAIR_DIMENSION>* m_Pt_param;   ///< Pointer to the parameter value

public:
    PARAM_CFG_DIFFPAIRDIMENSIONS( std::vector<DIFF_PAIR_DIMENSION>* ptparam,
                                  const wxChar* group = nullptr ) :
            PARAM_CFG_BASE( wxEmptyString, PARAM_DIFFPAIRDIMENSIONS, group )
    {
        m_Pt_param = ptparam;
    }

    void ReadParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        m_Pt_param->clear();

        for( int index = 1; ; ++index )
        {
            double width, gap, viagap;

            wxString key = dPairWidthKey;

            if( !aConfig->Read( key << index, &width ) )
                break;

            key = dPairGapKey;
            gap = aConfig->ReadDouble( key << index, 0.0 );

            key = dPairViaGapKey;
            viagap = aConfig->ReadDouble( key << index, 0.0 );

            m_Pt_param->emplace_back( DIFF_PAIR_DIMENSION( Millimeter2iu( width ),
                                                           Millimeter2iu( gap ),
                                                           Millimeter2iu( viagap ) ) );
        }
    }

    void SaveParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        for( size_t index = 1; index <= m_Pt_param->size(); ++index )
        {
            wxString key = dPairWidthKey;
            aConfig->Write( key << index, Iu2Millimeter( m_Pt_param->at( index - 1 ).m_Width ) );
            key = dPairGapKey;
            aConfig->Write( key << index, Iu2Millimeter( m_Pt_param->at( index - 1 ).m_Gap ) );
            key = dPairViaGapKey;
            aConfig->Write( key << index, Iu2Millimeter( m_Pt_param->at( index - 1 ).m_ViaGap ) );
        }
    }
};


class PARAM_CFG_NETCLASSES : public PARAM_CFG_BASE
{
protected:
    NETCLASSES* m_Pt_param;     ///<  Pointer to the parameter value

public:
    PARAM_CFG_NETCLASSES( const wxChar* ident, NETCLASSES* ptparam,
                          const wxChar* group = nullptr ) :
        PARAM_CFG_BASE( ident, PARAM_NETCLASSES, group )
    {
        m_Pt_param = ptparam;
    }

    void ReadParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        wxString oldPath = aConfig->GetPath();

        m_Pt_param->Clear();

        for( int index = 0; ; ++index )
        {
            wxString    path = "";
            NETCLASSPTR netclass;
            wxString    netclassName;

            if( index == 0 )
                path = "Default";
            else
                path << index;

            aConfig->SetPath( oldPath );
            aConfig->SetPath( m_Ident );
            aConfig->SetPath( path );

            if( !aConfig->Read( NetclassNameKey, &netclassName ) )
                break;

            if( index == 0 )
                netclass = m_Pt_param->GetDefault();
            else
                netclass = std::make_shared<NETCLASS>( netclassName );

#define READ_MM( aKey, aDefault ) Millimeter2iu( aConfig->ReadDouble( aKey, aDefault ) )
            netclass->SetClearance( READ_MM( ClearanceKey, netclass->GetClearance() ) );
            netclass->SetTrackWidth( READ_MM( TrackWidthKey, netclass->GetTrackWidth() ) );
            netclass->SetViaDiameter( READ_MM( ViaDiameterKey, netclass->GetViaDiameter() ) );
            netclass->SetViaDrill( READ_MM( ViaDrillKey, netclass->GetViaDrill() ) );
            netclass->SetuViaDiameter( READ_MM( uViaDiameterKey, netclass->GetuViaDiameter() ) );
            netclass->SetuViaDrill( READ_MM( uViaDrillKey, netclass->GetuViaDrill() ) );
            netclass->SetDiffPairWidth( READ_MM( dPairWidthKey, netclass->GetDiffPairWidth() ) );
            netclass->SetDiffPairGap( READ_MM( dPairGapKey, netclass->GetDiffPairGap() ) );
            netclass->SetDiffPairViaGap( READ_MM( dPairViaGapKey, netclass->GetDiffPairViaGap() ) );

            if( index > 0 )
                m_Pt_param->Add( netclass );
        }

        aConfig->SetPath( oldPath );
    }

    void SaveParam( wxConfigBase* aConfig ) const override
    {
        if( !m_Pt_param || !aConfig )
            return;

        wxString                   oldPath = aConfig->GetPath();
        NETCLASSES::const_iterator nc = m_Pt_param->begin();

        for( unsigned index = 0; index <= m_Pt_param->GetCount(); ++index )
        {
            wxString    path = "";
            NETCLASSPTR netclass;

            if( index == 0 )
                path = "Default";
            else
                path << index;

            aConfig->SetPath( oldPath );
            aConfig->SetPath( m_Ident );
            aConfig->SetPath( path );

            if( index == 0 )
            {
                netclass = m_Pt_param->GetDefault();
            }
            else
            {
                netclass = nc->second;
                ++nc;
            }

            aConfig->Write( NetclassNameKey, netclass->GetName() );

#define WRITE_MM( aKey, aValue ) aConfig->Write( aKey, Iu2Millimeter( aValue ) )
            WRITE_MM( ClearanceKey,    netclass->GetClearance() );
            WRITE_MM( TrackWidthKey,   netclass->GetTrackWidth() );
            WRITE_MM( ViaDiameterKey,  netclass->GetViaDiameter() );
            WRITE_MM( ViaDrillKey,     netclass->GetViaDrill() );
            WRITE_MM( uViaDiameterKey, netclass->GetuViaDiameter() );
            WRITE_MM( uViaDrillKey,    netclass->GetuViaDrill() );
            WRITE_MM( dPairWidthKey,   netclass->GetDiffPairWidth() );
            WRITE_MM( dPairGapKey,     netclass->GetDiffPairGap() );
            WRITE_MM( dPairViaGapKey,  netclass->GetDiffPairViaGap() );
        }

        aConfig->SetPath( oldPath );
    }
};


BOARD_DESIGN_SETTINGS::BOARD_DESIGN_SETTINGS() :
    m_Pad_Master( NULL )
{
    m_HasStackup = false;                   // no stackup defined by default

    LSET all_set = LSET().set();
    m_enabledLayers = all_set;              // All layers enabled at first.
                                            // SetCopperLayerCount() will adjust this.
    SetVisibleLayers( all_set );

    // set all but hidden text as visible.
    m_visibleElements = ~( 1 << GAL_LAYER_INDEX( LAYER_MOD_TEXT_INVISIBLE ) );

    SetCopperLayerCount( 2 );               // Default design is a double sided board
    m_CurrentViaType = VIA_THROUGH;

    // if true, when creating a new track starting on an existing track, use this track width
    m_UseConnectedTrackWidth = false;

    m_BlindBuriedViaAllowed = false;
    m_MicroViasAllowed  = false;

    m_LineThickness[ LAYER_CLASS_SILK ] = Millimeter2iu( DEFAULT_SILK_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_SILK ] = wxSize( Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ),
                                             Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_SILK ] = Millimeter2iu( DEFAULT_SILK_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_SILK ] = false;
    m_TextUpright[ LAYER_CLASS_SILK ] = true;

    m_LineThickness[ LAYER_CLASS_COPPER ] = Millimeter2iu( DEFAULT_COPPER_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_COPPER ] = wxSize( Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ),
                                               Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_COPPER ] = Millimeter2iu( DEFAULT_COPPER_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_COPPER ] = false;
    m_TextUpright[ LAYER_CLASS_COPPER ] = true;

    // Edges & Courtyards; text properties aren't used but better to have them holding
    // reasonable values than not.
    m_LineThickness[ LAYER_CLASS_EDGES ] = Millimeter2iu( DEFAULT_EDGE_WIDTH );
    m_TextSize[ LAYER_CLASS_EDGES ] = wxSize( Millimeter2iu( DEFAULT_TEXT_SIZE ),
                                              Millimeter2iu( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_EDGES ] = Millimeter2iu( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_EDGES ] = false;
    m_TextUpright[ LAYER_CLASS_EDGES ] = true;

    m_LineThickness[ LAYER_CLASS_COURTYARD ] = Millimeter2iu( DEFAULT_COURTYARD_WIDTH );
    m_TextSize[ LAYER_CLASS_COURTYARD ] = wxSize( Millimeter2iu( DEFAULT_TEXT_SIZE ),
                                                  Millimeter2iu( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_COURTYARD ] = Millimeter2iu( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_COURTYARD ] = false;
    m_TextUpright[ LAYER_CLASS_COURTYARD ] = true;

    m_LineThickness[ LAYER_CLASS_OTHERS ] = Millimeter2iu( DEFAULT_LINE_WIDTH );
    m_TextSize[ LAYER_CLASS_OTHERS ] = wxSize( Millimeter2iu( DEFAULT_TEXT_SIZE ),
                                               Millimeter2iu( DEFAULT_TEXT_SIZE ) );
    m_TextThickness[ LAYER_CLASS_OTHERS ] = Millimeter2iu( DEFAULT_TEXT_WIDTH );
    m_TextItalic[ LAYER_CLASS_OTHERS ] = false;
    m_TextUpright[ LAYER_CLASS_OTHERS ] = true;

    m_useCustomTrackVia = false;
    m_customTrackWidth  = Millimeter2iu( DEFAULT_CUSTOMTRACKWIDTH );
    m_customViaSize.m_Diameter = Millimeter2iu( DEFAULT_VIASMINSIZE );
    m_customViaSize.m_Drill = Millimeter2iu( DEFAULT_VIASMINDRILL );

    m_useCustomDiffPair = false;
    m_customDiffPair.m_Width = Millimeter2iu( DEFAULT_CUSTOMDPAIRWIDTH );
    m_customDiffPair.m_Gap = Millimeter2iu( DEFAULT_CUSTOMDPAIRGAP );
    m_customDiffPair.m_ViaGap = Millimeter2iu( DEFAULT_CUSTOMDPAIRVIAGAP );

    m_TrackMinWidth       = Millimeter2iu( DEFAULT_TRACKMINWIDTH );
    m_ViasMinSize         = Millimeter2iu( DEFAULT_VIASMINSIZE );
    m_ViasMinDrill        = Millimeter2iu( DEFAULT_VIASMINDRILL );
    m_MicroViasMinSize    = Millimeter2iu( DEFAULT_MICROVIASMINSIZE );
    m_MicroViasMinDrill   = Millimeter2iu( DEFAULT_MICROVIASMINDRILL );
    m_CopperEdgeClearance = Millimeter2iu( DEFAULT_COPPEREDGECLEARANCE );

    m_MaxError            = ARC_HIGH_DEF;
    m_ZoneUseNoOutlineInFill = false;   // Use compatibility mode by default

    // Global mask margins:
    m_SolderMaskMargin  = Millimeter2iu( DEFAULT_SOLDERMASK_CLEARANCE );
    m_SolderMaskMinWidth = Millimeter2iu( DEFAULT_SOLDERMASK_MIN_WIDTH );
    m_SolderPasteMargin = 0;                    // Solder paste margin absolute value
    m_SolderPasteMarginRatio = 0.0;             // Solder paste margin as a ratio of pad size
                                                // The final margin is the sum of these 2 values
                                                // Usually < 0 because the mask is smaller than pad
    // Layer thickness for 3D viewer
    m_boardThickness = Millimeter2iu( DEFAULT_BOARD_THICKNESS_MM );

    m_viaSizeIndex = 0;
    m_trackWidthIndex = 0;
    m_diffPairIndex = 0;

    // Default ref text on fp creation. If empty, use footprint name as default
    m_RefDefaultText = wxT( "REF**" );
    m_RefDefaultVisibility = true;
    m_RefDefaultlayer = int( F_SilkS );
    // Default value text on fp creation. If empty, use footprint name as default
    m_ValueDefaultText = wxEmptyString;
    m_ValueDefaultVisibility = true;
    m_ValueDefaultlayer = int( F_Fab );
}

// Add parameters to save in project config.
// values are saved in mm
void BOARD_DESIGN_SETTINGS::AppendConfigs( BOARD* aBoard, PARAM_CFG_ARRAY* aResult )
{
    aResult->push_back( new PARAM_CFG_LAYERS( aBoard ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "AllowMicroVias" ),
          &m_MicroViasAllowed, false ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "AllowBlindVias" ),
          &m_BlindBuriedViaAllowed, false ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "RequireCourtyardDefinitions" ),
          &m_RequireCourtyards, false ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "ProhibitOverlappingCourtyards" ),
          &m_ProhibitOverlappingCourtyards, true ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "MinTrackWidth" ),
          &m_TrackMinWidth,
          Millimeter2iu( DEFAULT_TRACKMINWIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "MinViaDiameter" ),
          &m_ViasMinSize,
          Millimeter2iu( DEFAULT_VIASMINSIZE ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "MinViaDrill" ),
          &m_ViasMinDrill,
          Millimeter2iu( DEFAULT_VIASMINDRILL ), Millimeter2iu( 0.01 ), Millimeter2iu( 25.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "MinMicroViaDiameter" ),
          &m_MicroViasMinSize,
          Millimeter2iu( DEFAULT_MICROVIASMINSIZE ), Millimeter2iu( 0.01 ), Millimeter2iu( 10.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "MinMicroViaDrill" ),
          &m_MicroViasMinDrill,
          Millimeter2iu( DEFAULT_MICROVIASMINDRILL ), Millimeter2iu( 0.01 ), Millimeter2iu( 10.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "MinHoleToHole" ),
          &m_HoleToHoleMin,
          Millimeter2iu( DEFAULT_HOLETOHOLEMIN ), Millimeter2iu( 0.0 ), Millimeter2iu( 10.0 ),
          nullptr, MM_PER_IU ) );

    // Note: a clearance of -0.01 is a flag indicating we should use the legacy (pre-6.0) method
    // based on the edge cut thicknesses.
    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "CopperEdgeClearance" ),
          &m_CopperEdgeClearance,
          Millimeter2iu( LEGACY_COPPEREDGECLEARANCE ), Millimeter2iu( -0.01 ), Millimeter2iu( 25.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_TRACKWIDTHS( &m_TrackWidthList ) );
    aResult->push_back( new PARAM_CFG_VIADIMENSIONS( &m_ViasDimensionsList ) );
    aResult->push_back( new PARAM_CFG_DIFFPAIRDIMENSIONS( &m_DiffPairDimensionsList ) );

    aResult->push_back( new PARAM_CFG_NETCLASSES( wxT( "Netclasses" ), &m_NetClasses ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SilkLineWidth" ),
          &m_LineThickness[ LAYER_CLASS_SILK ],
          Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
          nullptr, MM_PER_IU, wxT( "ModuleOutlineThickness" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SilkTextSizeV" ),
          &m_TextSize[ LAYER_CLASS_SILK ].y,
          Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
          nullptr, MM_PER_IU, wxT( "ModuleTextSizeV" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SilkTextSizeH" ),
          &m_TextSize[ LAYER_CLASS_SILK ].x,
          Millimeter2iu( DEFAULT_SILK_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
          nullptr, MM_PER_IU, wxT( "ModuleTextSizeH" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SilkTextSizeThickness" ),
          &m_TextThickness[ LAYER_CLASS_SILK ],
          Millimeter2iu( DEFAULT_SILK_TEXT_WIDTH ), 1, TEXTS_MAX_WIDTH,
          nullptr, MM_PER_IU, wxT( "ModuleTextSizeThickness" ) ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "SilkTextItalic" ),
          &m_TextItalic[ LAYER_CLASS_SILK ], false ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "SilkTextUpright" ),
          &m_TextUpright[ LAYER_CLASS_SILK ], true ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "CopperLineWidth" ),
          &m_LineThickness[ LAYER_CLASS_COPPER ],
          Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
          nullptr, MM_PER_IU, wxT( "DrawSegmentWidth" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "CopperTextSizeV" ),
          &m_TextSize[ LAYER_CLASS_COPPER ].y,
          Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE  ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
          nullptr, MM_PER_IU, wxT( "PcbTextSizeV" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "CopperTextSizeH" ),
          &m_TextSize[ LAYER_CLASS_COPPER ].x,
          Millimeter2iu( DEFAULT_COPPER_TEXT_SIZE  ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
          nullptr, MM_PER_IU, wxT( "PcbTextSizeH" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "CopperTextThickness" ),
          &m_TextThickness[ LAYER_CLASS_COPPER ],
          Millimeter2iu( DEFAULT_COPPER_TEXT_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
          nullptr, MM_PER_IU, wxT( "PcbTextThickness" ) ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "CopperTextItalic" ),
          &m_TextItalic[ LAYER_CLASS_COPPER ], false ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "CopperTextUpright" ),
          &m_TextUpright[ LAYER_CLASS_COPPER ], true ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "EdgeCutLineWidth" ),
          &m_LineThickness[ LAYER_CLASS_EDGES ],
          Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
          nullptr, MM_PER_IU, wxT( "BoardOutlineThickness" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "CourtyardLineWidth" ),
          &m_LineThickness[ LAYER_CLASS_COURTYARD ],
          Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "OthersLineWidth" ),
          &m_LineThickness[ LAYER_CLASS_OTHERS ],
          Millimeter2iu( DEFAULT_SILK_LINE_WIDTH ), Millimeter2iu( 0.01 ), Millimeter2iu( 5.0 ),
          nullptr, MM_PER_IU, wxT( "ModuleOutlineThickness" ) ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "OthersTextSizeV" ),
          &m_TextSize[ LAYER_CLASS_OTHERS ].x,
          Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "OthersTextSizeH" ),
          &m_TextSize[ LAYER_CLASS_OTHERS ].y,
          Millimeter2iu( DEFAULT_TEXT_SIZE ), TEXTS_MIN_SIZE, TEXTS_MAX_SIZE,
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "OthersTextSizeThickness" ),
          &m_TextThickness[ LAYER_CLASS_OTHERS ],
          Millimeter2iu( DEFAULT_TEXT_WIDTH ), 1, TEXTS_MAX_WIDTH,
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "OthersTextItalic" ),
          &m_TextItalic[ LAYER_CLASS_OTHERS ], false ) );

    aResult->push_back( new PARAM_CFG_BOOL( wxT( "OthersTextUpright" ),
          &m_TextUpright[ LAYER_CLASS_OTHERS ], true ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SolderMaskClearance" ),
          &m_SolderMaskMargin,
          Millimeter2iu( DEFAULT_SOLDERMASK_CLEARANCE ), Millimeter2iu( -1.0 ), Millimeter2iu( 1.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SolderMaskMinWidth" ),
          &m_SolderMaskMinWidth,
          Millimeter2iu( DEFAULT_SOLDERMASK_MIN_WIDTH ), 0, Millimeter2iu( 1.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_INT_WITH_SCALE( wxT( "SolderPasteClearance" ),
          &m_SolderPasteMargin,
          Millimeter2iu( DEFAULT_SOLDERPASTE_CLEARANCE ), Millimeter2iu( -1.0 ), Millimeter2iu( 1.0 ),
          nullptr, MM_PER_IU ) );

    aResult->push_back( new PARAM_CFG_DOUBLE( wxT( "SolderPasteRatio" ),
          &m_SolderPasteMarginRatio,
          DEFAULT_SOLDERPASTE_RATIO, -0.5, 1.0 ) );
}


bool BOARD_DESIGN_SETTINGS::SetCurrentNetClass( const wxString& aNetClassName )
{
    NETCLASSPTR netClass = m_NetClasses.Find( aNetClassName );
    bool        lists_sizes_modified = false;

    // if not found (should not happen) use the default
    if( !netClass )
        netClass = m_NetClasses.GetDefault();

    m_currentNetClassName = netClass->GetName();

    // Initialize others values:
    if( m_TrackWidthList.size() == 0 )
    {
        lists_sizes_modified = true;
        m_TrackWidthList.push_back( 0 );
    }

    if( m_ViasDimensionsList.size() == 0 )
    {
        lists_sizes_modified = true;
        m_ViasDimensionsList.emplace_back( VIA_DIMENSION() );
    }

    if( m_DiffPairDimensionsList.size() == 0 )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList.emplace_back( DIFF_PAIR_DIMENSION() );
    }

    /* note the m_ViasDimensionsList[0] and m_TrackWidthList[0] values
     * are always the Netclass values
     */
    if( m_TrackWidthList[0] != netClass->GetTrackWidth() )
    {
        lists_sizes_modified = true;
        m_TrackWidthList[0] = netClass->GetTrackWidth();
    }

    if( m_ViasDimensionsList[0].m_Diameter != netClass->GetViaDiameter() )
    {
        lists_sizes_modified = true;
        m_ViasDimensionsList[0].m_Diameter = netClass->GetViaDiameter();
    }

    if( m_ViasDimensionsList[0].m_Drill != netClass->GetViaDrill() )
    {
        lists_sizes_modified = true;
        m_ViasDimensionsList[0].m_Drill = netClass->GetViaDrill();
    }

    if( m_DiffPairDimensionsList[0].m_Width != netClass->GetDiffPairWidth() )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList[0].m_Width = netClass->GetDiffPairWidth();
    }

    if( m_DiffPairDimensionsList[0].m_Gap != netClass->GetDiffPairGap() )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList[0].m_Gap = netClass->GetDiffPairGap();
    }

    if( m_DiffPairDimensionsList[0].m_ViaGap != netClass->GetDiffPairViaGap() )
    {
        lists_sizes_modified = true;
        m_DiffPairDimensionsList[0].m_ViaGap = netClass->GetDiffPairViaGap();
    }

    if( GetViaSizeIndex() >= m_ViasDimensionsList.size() )
        SetViaSizeIndex( m_ViasDimensionsList.size() );

    if( GetTrackWidthIndex() >= m_TrackWidthList.size() )
        SetTrackWidthIndex( m_TrackWidthList.size() );

    if( GetDiffPairIndex() >= m_DiffPairDimensionsList.size() )
        SetDiffPairIndex( m_DiffPairDimensionsList.size() );

    return lists_sizes_modified;
}


int BOARD_DESIGN_SETTINGS::GetBiggestClearanceValue()
{
    int clearance = m_NetClasses.GetDefault()->GetClearance();

    //Read list of Net Classes
    for( NETCLASSES::const_iterator nc = m_NetClasses.begin(); nc != m_NetClasses.end(); ++nc )
    {
        NETCLASSPTR netclass = nc->second;
        clearance = std::max( clearance, netclass->GetClearance() );
    }

    return clearance;
}


int BOARD_DESIGN_SETTINGS::GetSmallestClearanceValue()
{
    int clearance = m_NetClasses.GetDefault()->GetClearance();

    //Read list of Net Classes
    for( NETCLASSES::const_iterator nc = m_NetClasses.begin(); nc != m_NetClasses.end(); ++nc )
    {
        NETCLASSPTR netclass = nc->second;
        clearance = std::min( clearance, netclass->GetClearance() );
    }

    return clearance;
}


int BOARD_DESIGN_SETTINGS::GetCurrentMicroViaSize()
{
    NETCLASSPTR netclass = m_NetClasses.Find( m_currentNetClassName );

    return netclass->GetuViaDiameter();
}


int BOARD_DESIGN_SETTINGS::GetCurrentMicroViaDrill()
{
    NETCLASSPTR netclass = m_NetClasses.Find( m_currentNetClassName );

    return netclass->GetuViaDrill();
}


void BOARD_DESIGN_SETTINGS::SetViaSizeIndex( unsigned aIndex )
{
    m_viaSizeIndex = std::min( aIndex, (unsigned) m_ViasDimensionsList.size() );
    m_useCustomTrackVia = false;
}


int BOARD_DESIGN_SETTINGS::GetCurrentViaDrill() const
{
    int drill;

    if( m_useCustomTrackVia )
        drill = m_customViaSize.m_Drill;
    else
        drill = m_ViasDimensionsList[m_viaSizeIndex].m_Drill;

    return drill > 0 ? drill : -1;
}


void BOARD_DESIGN_SETTINGS::SetTrackWidthIndex( unsigned aIndex )
{
    m_trackWidthIndex = std::min( aIndex, (unsigned) m_TrackWidthList.size() );
    m_useCustomTrackVia = false;
}


void BOARD_DESIGN_SETTINGS::SetDiffPairIndex( unsigned aIndex )
{
    m_diffPairIndex = std::min( aIndex, (unsigned) 8 );
    m_useCustomDiffPair = false;
}


void BOARD_DESIGN_SETTINGS::SetMinHoleSeparation( int aDistance )
{
    m_HoleToHoleMin = aDistance;
}


void BOARD_DESIGN_SETTINGS::SetCopperEdgeClearance( int aDistance )
{
    m_CopperEdgeClearance = aDistance;
}


void BOARD_DESIGN_SETTINGS::SetRequireCourtyardDefinitions( bool aRequire )
{
    m_RequireCourtyards = aRequire;
}


void BOARD_DESIGN_SETTINGS::SetProhibitOverlappingCourtyards( bool aProhibit )
{
    m_ProhibitOverlappingCourtyards = aProhibit;
}


void BOARD_DESIGN_SETTINGS::SetVisibleAlls()
{
    SetVisibleLayers( LSET().set() );
    m_visibleElements = -1;
}


void BOARD_DESIGN_SETTINGS::SetLayerVisibility( PCB_LAYER_ID aLayer, bool aNewState )
{
    m_visibleLayers.set( aLayer, aNewState && IsLayerEnabled( aLayer ));
}


void BOARD_DESIGN_SETTINGS::SetElementVisibility( GAL_LAYER_ID aElementCategory, bool aNewState )
{
    if( aNewState )
        m_visibleElements |= 1 << GAL_LAYER_INDEX( aElementCategory );
    else
        m_visibleElements &= ~( 1 << GAL_LAYER_INDEX( aElementCategory ) );
}


void BOARD_DESIGN_SETTINGS::SetCopperLayerCount( int aNewLayerCount )
{
    // if( aNewLayerCount < 2 ) aNewLayerCount = 2;

    m_copperLayerCount = aNewLayerCount;

    // ensure consistency with the m_EnabledLayers member
#if 0
    // was:
    m_enabledLayers &= ~ALL_CU_LAYERS;
    m_enabledLayers |= LAYER_BACK;

    if( m_copperLayerCount > 1 )
        m_enabledLayers |= LAYER_FRONT;

    for( LAYER_NUM ii = LAYER_N_2; ii < aNewLayerCount - 1; ++ii )
        m_enabledLayers |= GetLayerSet( ii );
#else
    // Update only enabled copper layers mask
    m_enabledLayers &= ~LSET::AllCuMask();
    m_enabledLayers |= LSET::AllCuMask( aNewLayerCount );
#endif
}


void BOARD_DESIGN_SETTINGS::SetEnabledLayers( LSET aMask )
{
    // Back and front layers are always enabled.
    aMask.set( B_Cu ).set( F_Cu );

    m_enabledLayers = aMask;

    // A disabled layer cannot be visible
    m_visibleLayers &= aMask;

    // update m_CopperLayerCount to ensure its consistency with m_EnabledLayers
    m_copperLayerCount = ( aMask & LSET::AllCuMask() ).count();
}


// Return the layer class index { silk, copper, edges & courtyards, others } of the
// given layer.
int BOARD_DESIGN_SETTINGS::GetLayerClass( PCB_LAYER_ID aLayer ) const
{
    if( aLayer == F_SilkS || aLayer == B_SilkS )
        return LAYER_CLASS_SILK;
    else if( IsCopperLayer( aLayer ) )
        return LAYER_CLASS_COPPER;
    else if( aLayer == Edge_Cuts )
        return LAYER_CLASS_EDGES;
    else if( aLayer == F_CrtYd || aLayer == B_CrtYd )
        return LAYER_CLASS_COURTYARD;
    else
        return LAYER_CLASS_OTHERS;
}


int BOARD_DESIGN_SETTINGS::GetLineThickness( PCB_LAYER_ID aLayer ) const
{
    return m_LineThickness[ GetLayerClass( aLayer ) ];
}


wxSize BOARD_DESIGN_SETTINGS::GetTextSize( PCB_LAYER_ID aLayer ) const
{
    return m_TextSize[ GetLayerClass( aLayer ) ];
}


int BOARD_DESIGN_SETTINGS::GetTextThickness( PCB_LAYER_ID aLayer ) const
{
    return m_TextThickness[ GetLayerClass( aLayer ) ];
}


bool BOARD_DESIGN_SETTINGS::GetTextItalic( PCB_LAYER_ID aLayer ) const
{
    return m_TextItalic[ GetLayerClass( aLayer ) ];
}


bool BOARD_DESIGN_SETTINGS::GetTextUpright( PCB_LAYER_ID aLayer ) const
{
    return m_TextUpright[ GetLayerClass( aLayer ) ];
}
