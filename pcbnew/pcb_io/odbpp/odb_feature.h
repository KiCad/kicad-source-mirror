/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#pragma once

#include "odb_attribute.h"
#include "odb_util.h"
#include "convert_basic_shapes_to_polygon.h"

#include <iterator>
#include <list>
#include <optional>
#include <math/vector2d.h>
#include <common.h>
#include "odb_defines.h"


enum class ODB_DIRECTION
{
    CW,
    CCW
};


class ODB_LINE;
class ODB_ARC;
class ODB_PAD;
class ODB_SURFACE;
class ODB_FEATURE;
class PCB_IO_ODBPP;
class FOOTPRINT;
class PAD;
class PCB_VIA;
class PCB_DIMENSION_BASE;
class PCB_TRACK;

class FEATURES_MANAGER : public ATTR_MANAGER
{
public:
    FEATURES_MANAGER( BOARD* aBoard, PCB_IO_ODBPP* aPlugin, const wxString& aLayerName, ODB_LAYER_ROLE aRole,
                      std::optional<ODB_AUX_LAYER_TYPE> aAuxType,
                      std::optional<DRILL_SPAN>         aDrillSpan = std::nullopt ) :
            m_board( aBoard ),
            m_plugin( aPlugin ),
            m_layerName( aLayerName ),
            m_role( aRole ),
            m_auxType( aAuxType ),
            m_drillSpan( aDrillSpan )
    {}

    void AddPad( PCB_LAYER_ID aLayer, PAD* pad );
    void AddDimension( PCB_LAYER_ID aLayer, PCB_DIMENSION_BASE* dimension );
    void AddShape( PCB_LAYER_ID aLayer, PCB_SHAPE* shape );
    void AddText( PCB_LAYER_ID aLayer, BOARD_ITEM* item, RESOLUTION_CONTEXT aContext );
    void AddZone( PCB_LAYER_ID aLayer, ZONE* zone );
    void AddTrack( PCB_LAYER_ID aLayer, PCB_TRACK* track );

    void InitFeatureList( PCB_LAYER_ID aLayer, const std::vector<BOARD_ITEM*>& aItems );

    void AddFeatureLine( const VECTOR2I& aStart, const VECTOR2I& aEnd, uint64_t aWidth );

    void AddFeatureArc( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCenter,
                        uint64_t aWidth, ODB_DIRECTION aDirection );

    void AddRoutContour( const SHAPE_LINE_CHAIN& aContour, int aChain, bool aPlated );
    void AddRoutSlot( const PAD& aPad, int aChain );

    void AddPadCircle( const VECTOR2I& aCenter, uint64_t aDiameter );

    void AddPadShape( const PAD& aPad, PCB_LAYER_ID aLayer );

    bool AddFeatureSurface( const SHAPE_POLY_SET::POLYGON& aPolygon, FILL_T aFillType = FILL_T::FILLED_SHAPE );

    size_t FeatureCount() const { return m_featuresList.size(); }

    void AddShape( const PCB_SHAPE& aShape, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    void AddVia( const PCB_VIA* aVia, PCB_LAYER_ID aLayer );

    bool AddViaDrillHole( const PCB_VIA* aVia, PCB_LAYER_ID aLayer );

    void AddViaProtection( const PCB_VIA* aVia, bool drill, PCB_LAYER_ID aLayer );

    bool AddContour( const SHAPE_POLY_SET& aPolySet, int aOutline = 0,
                     FILL_T aFillType = FILL_T::FILLED_SHAPE );

    bool AddPolygon( const SHAPE_POLY_SET::POLYGON& aPolygon, FILL_T aFillType, int aWidth,
                     LINE_STYLE aDashType );

    bool AddPolygonCutouts( const SHAPE_POLY_SET::POLYGON& aPolygon );

    void GenerateFeatureFile( std::ostream& ost ) const;

    void GenerateProfileFeatures( std::ostream& ost ) const;

private:
    inline uint32_t AddCircleSymbol( const wxString& aDiameter )
    {
        return GetSymbolIndex( m_circleSymMap, "r" + aDiameter );
    }

    uint32_t AddRoundDonutSymbol( const wxString& aOuterDim, const wxString& aInnerDim )
    {
        wxString sym = "donut_r" + aOuterDim + ODB_DIM_X + aInnerDim;
        return GetSymbolIndex( m_roundDonutSymMap, sym );
    }

    uint32_t AddRectSymbol( const wxString& aWidth, const wxString& aHeight )
    {
        wxString sym = "rect" + aWidth + ODB_DIM_X + aHeight;
        return GetSymbolIndex( m_rectSymMap, sym );
    }

    uint32_t AddOvalSymbol( const wxString& aWidth, const wxString& aHeight )
    {
        wxString sym = "oval" + aWidth + ODB_DIM_X + aHeight;
        return GetSymbolIndex( m_ovalSymMap, sym );
    }

    uint32_t AddRoundRectSymbol( const wxString& aWidth, const wxString& aHeight,
                                 const wxString& aRadius )
    {
        wxString sym = "rect" + aWidth + ODB_DIM_X + aHeight + ODB_DIM_X + ODB_DIM_R + aRadius;
        return GetSymbolIndex( m_roundRectSymMap, sym );
    }

    uint32_t AddRoundRectDonutSymbol( const wxString& aOuterWidth, const wxString& aOuterHeight,
                                      const wxString& aLineWidth, const wxString& aRadius )
    {
        wxString sym = "donut_rc" + aOuterWidth + ODB_DIM_X + aOuterHeight + ODB_DIM_X + aLineWidth
                       + ODB_DIM_X + ODB_DIM_R + aRadius;
        return GetSymbolIndex( m_roundRectDonutSymMap, sym );
    }

    uint32_t AddChamferRectSymbol( const wxString& aWidth, const wxString& aHeight,
                                   const wxString& aRadius, int aPositions )
    {
        wxString sym = "rect" + aWidth + ODB_DIM_X + aHeight + ODB_DIM_X + ODB_DIM_C + aRadius;

        if( aPositions != RECT_CHAMFER_ALL )
        {
            sym += ODB_DIM_X;
            if( aPositions & RECT_CHAMFER_TOP_RIGHT )
                sym += "1";
            if( aPositions & RECT_CHAMFER_TOP_LEFT )
                sym += "2";
            if( aPositions & RECT_CHAMFER_BOTTOM_LEFT )
                sym += "3";
            if( aPositions & RECT_CHAMFER_BOTTOM_RIGHT )
                sym += "4";
        }

        return GetSymbolIndex( m_chamRectSymMap, sym );
    }


    uint32_t GetSymbolIndex( std::map<wxString, uint32_t>& aSymMap, const wxString& aKey )
    {
        if( aSymMap.count( aKey ) )
        {
            return aSymMap.at( aKey );
        }
        else
        {
            uint32_t index = m_symIndex;
            m_symIndex++;
            aSymMap.emplace( aKey, index );
            m_allSymMap.emplace( index, aKey );
            return index;
        }
    }

    void tagRoutFeatures( size_t aFirst, int aChain, bool aPlated );
    void addViaDrillAttributes( ODB_FEATURE& aFeature, const PCB_VIA* aVia );
    void addPostMachiningAttributes( ODB_FEATURE& aFeature, const DRILL_OPERATION& aOperation );

    std::map<wxString, uint32_t> m_circleSymMap; // diameter -> symbol index
    std::map<wxString, uint32_t> m_roundDonutSymMap;
    std::map<wxString, uint32_t> m_padSymMap;  // name -> symbol index
    std::map<wxString, uint32_t> m_rectSymMap; // w,h -> symbol index
    std::map<wxString, uint32_t> m_ovalSymMap; // w,h -> symbol index
    std::map<wxString, uint32_t> m_roundRectSymMap;
    std::map<wxString, uint32_t> m_roundRectDonutSymMap;
    std::map<wxString, uint32_t> m_chamRectSymMap;

    std::map<uint32_t, wxString> m_allSymMap;

    template <typename T, typename... Args>
    void AddFeature( Args&&... args )
    {
        auto feature = std::make_unique<T>( m_featuresList.size(), std::forward<Args>( args )... );

        m_featuresList.emplace_back( std::move( feature ) );
    }

    template <typename F>
    void forEachNewFeature( size_t aFirst, F&& aFunc )
    {
        auto it = std::prev( m_featuresList.end(), m_featuresList.size() - aFirst );

        for( size_t index = aFirst; it != m_featuresList.end(); ++it, ++index )
            aFunc( **it, index );
    }

    inline PCB_IO_ODBPP* GetODBPlugin() { return m_plugin; }

    BOARD*                            m_board;
    PCB_IO_ODBPP*                     m_plugin;
    wxString                          m_layerName;
    ODB_LAYER_ROLE                    m_role;
    std::optional<ODB_AUX_LAYER_TYPE> m_auxType;
    std::optional<DRILL_SPAN>         m_drillSpan;
    uint32_t                          m_symIndex = 0;

    std::list<std::unique_ptr<ODB_FEATURE>>      m_featuresList;
};


class ODB_FEATURE : public ATTR_RECORD_WRITER
{
public:
    // template <typename T> using check_type = attribute::is_feature<T>;
    ODB_FEATURE( uint32_t aIndex ) : m_index( aIndex ) {}
    virtual void WriteFeatures( std::ostream& ost, const ODB_FORMAT& aFormat );

    virtual ~ODB_FEATURE() = default;

protected:
    enum class FEATURE_TYPE
    {
        LINE,
        ARC,
        PAD,
        SURFACE
    };

    virtual FEATURE_TYPE GetFeatureType() = 0;

    virtual void WriteRecordContent( std::ostream& ost, const ODB_FORMAT& aFormat ) = 0;

    const uint32_t m_index;
};

class ODB_LINE : public ODB_FEATURE
{
public:
    ODB_LINE( uint32_t aIndex, const std::pair<wxString, wxString>& aStart,
              const std::pair<wxString, wxString>& aEnd, uint32_t aSym ) :
            ODB_FEATURE( aIndex ),
            m_start( aStart ),
            m_end( aEnd ),
            m_symIndex( aSym )
    {}

    inline virtual FEATURE_TYPE GetFeatureType() override { return FEATURE_TYPE::LINE; }

protected:
    virtual void WriteRecordContent( std::ostream& ost, const ODB_FORMAT& aFormat ) override;

private:
    std::pair<wxString, wxString> m_start;
    std::pair<wxString, wxString> m_end;
    uint32_t                      m_symIndex;
};


class ODB_ARC : public ODB_FEATURE
{
public:
    inline virtual FEATURE_TYPE GetFeatureType() override { return FEATURE_TYPE::ARC; }


    ODB_ARC( uint32_t aIndex, const std::pair<wxString, wxString>& aStart,
             const std::pair<wxString, wxString>& aEnd,
             const std::pair<wxString, wxString>& aCenter, uint32_t aSym,
             ODB_DIRECTION aDirection ) :
            ODB_FEATURE( aIndex ),
            m_start( aStart ),
            m_end( aEnd ),
            m_center( aCenter ),
            m_symIndex( aSym ),
            m_direction( aDirection )
    {}

protected:
    virtual void WriteRecordContent( std::ostream& ost, const ODB_FORMAT& aFormat ) override;

private:
    std::pair<wxString, wxString> m_start;
    std::pair<wxString, wxString> m_end;
    std::pair<wxString, wxString> m_center;
    uint32_t                      m_symIndex;
    ODB_DIRECTION                 m_direction;
};

class ODB_PAD : public ODB_FEATURE
{
public:
    ODB_PAD( uint32_t aIndex, const std::pair<wxString, wxString>& aCenter, uint32_t aSym, EDA_ANGLE aAngle = ANGLE_0,
             double aResize = 1.0 ) :
            ODB_FEATURE( aIndex ),
            m_center( aCenter ),
            m_symIndex( aSym ),
            m_angle( aAngle ),
            m_resize( aResize )
    {
    }

    inline virtual FEATURE_TYPE GetFeatureType() override { return FEATURE_TYPE::PAD; }

protected:
    virtual void WriteRecordContent( std::ostream& ost, const ODB_FORMAT& aFormat ) override;

private:
    std::pair<wxString, wxString> m_center;
    uint32_t                      m_symIndex;
    EDA_ANGLE                     m_angle;
    double                        m_resize;
};

class ODB_SURFACE_DATA;
class ODB_SURFACE : public ODB_FEATURE
{
public:
    ODB_SURFACE( uint32_t aIndex, const SHAPE_POLY_SET::POLYGON& aPolygon,
                 FILL_T aFillType = FILL_T::FILLED_SHAPE );

    virtual ~ODB_SURFACE() = default;

    inline virtual FEATURE_TYPE GetFeatureType() override { return FEATURE_TYPE::SURFACE; }

    std::unique_ptr<ODB_SURFACE_DATA> m_surfaces;

protected:
    virtual void WriteRecordContent( std::ostream& ost, const ODB_FORMAT& aFormat ) override;
};


class ODB_SURFACE_DATA
{
public:
    ODB_SURFACE_DATA( const SHAPE_POLY_SET::POLYGON& aPolygon );

    struct SURFACE_LINE
    {
        enum class LINE_TYPE
        {
            SEGMENT,
            ARC
        };

        SURFACE_LINE() = default;

        SURFACE_LINE( const VECTOR2I& aEnd ) :
                m_end( aEnd )
        {}

        SURFACE_LINE( const VECTOR2I& aEnd, const VECTOR2I& aCenter, ODB_DIRECTION aDirection ) :
                m_end( aEnd ),
                m_type( LINE_TYPE::ARC ),
                m_center( aCenter ),
                m_direction( aDirection )
        {}

        VECTOR2I  m_end;
        LINE_TYPE m_type = LINE_TYPE::SEGMENT;

        VECTOR2I      m_center;
        ODB_DIRECTION m_direction = ODB_DIRECTION::CW;
    };

    void AddPolygonHoles( const SHAPE_POLY_SET::POLYGON& aPolygon );
    void WriteData( std::ostream& ost, const ODB_FORMAT& aFormat ) const;

    std::vector<std::vector<SURFACE_LINE>> m_polygons;

private:
    void AddContour( const SHAPE_LINE_CHAIN& aChain, bool aHole );
};
