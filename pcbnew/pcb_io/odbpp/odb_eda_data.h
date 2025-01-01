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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ODB_EDA_DATA_H_
#define _ODB_EDA_DATA_H_


#include <list>
#include <memory>

#include "odb_attribute.h"
#include "odb_feature.h"


class PKG_OUTLINE;
class EDA_DATA : public ATTR_MANAGER
{
public:
    EDA_DATA();

    void                                    Write( std::ostream& ost ) const;
    size_t                                  GetLyrIdx( const wxString& aLayerName );
    std::vector<std::shared_ptr<FOOTPRINT>> GetEdaFootprints() const { return m_eda_footprints; }

    class FEATURE_ID
    {
        friend EDA_DATA;

    public:
        enum class TYPE
        {
            COPPER,
            LAMINATE,
            HOLE
        };

        FEATURE_ID( TYPE t, size_t l, size_t fid ) : type( t ), layer( l ), feature_id( fid ) {}

        TYPE   type;
        size_t layer;
        size_t feature_id;

        void Write( std::ostream& ost ) const;
    };

    class SUB_NET
    {
    public:
        SUB_NET( size_t aIndex, EDA_DATA* aEda ) : m_index( aIndex ), m_edadata( aEda ) {}
        const size_t m_index;
        void         Write( std::ostream& ost ) const;

        std::list<FEATURE_ID> feature_ids;
        void AddFeatureID( FEATURE_ID::TYPE type, const wxString& layer, size_t feature_id );

        virtual ~SUB_NET() {}

    protected:
        virtual void WriteSubnet( std::ostream& ost ) const = 0;
        EDA_DATA*    m_edadata;
    };

    class SUB_NET_VIA : public SUB_NET
    {
    public:
        SUB_NET_VIA( size_t aIndex, EDA_DATA* aEda ) : SUB_NET( aIndex, aEda ) {}
        void WriteSubnet( std::ostream& ost ) const override;
    };

    class SUB_NET_TRACE : public SUB_NET
    {
    public:
        SUB_NET_TRACE( size_t aIndex, EDA_DATA* aEda ) : SUB_NET( aIndex, aEda ) {}
        void WriteSubnet( std::ostream& ost ) const override;
    };

    class SUB_NET_PLANE : public SUB_NET
    {
    public:
        enum class FILL_TYPE
        {
            SOLID,
            OUTLINE
        };

        enum class CUTOUT_TYPE
        {
            CIRCLE,
            RECT,
            OCTAGON,
            EXACT
        };

        SUB_NET_PLANE( size_t aIndex, EDA_DATA* aEda, FILL_TYPE aFill, CUTOUT_TYPE aCutout,
                       size_t aFillSize ) :
                SUB_NET( aIndex, aEda ), fill_type( aFill ), cutout_type( aCutout ),
                fill_size( aFillSize )
        {
        }

        FILL_TYPE   fill_type;
        CUTOUT_TYPE cutout_type;
        size_t      fill_size;

        void WriteSubnet( std::ostream& ost ) const override;
    };

    class SUB_NET_TOEPRINT : public SUB_NET
    {
    public:
        enum class SIDE
        {
            TOP,
            BOTTOM
        };

        SUB_NET_TOEPRINT( size_t aIndex, EDA_DATA* aEda, SIDE aSide, size_t aCompNum,
                          size_t aToepNum ) :
                SUB_NET( aIndex, aEda ), side( aSide ), comp_num( aCompNum ), toep_num( aToepNum )
        {
        }

        ~SUB_NET_TOEPRINT() {}

        SIDE side;

        size_t comp_num;
        size_t toep_num;

        void WriteSubnet( std::ostream& ost ) const override;
    };

    class NET : public ATTR_RECORD_WRITER
    {
    public:
        NET( size_t aIndex, const wxString& aName ) : m_index( aIndex ), m_name( aName ) {}

        const size_t                        m_index;
        wxString                            m_name;
        std::list<std::unique_ptr<SUB_NET>> subnets;

        template <typename T, typename... Args>
        T& AddSubnet( Args&&... args )
        {
            auto  f = std::make_unique<T>( subnets.size(), std::forward<Args>( args )... );
            auto& r = *f;
            subnets.push_back( std::move( f ) );
            return r;
        }

        void Write( std::ostream& ost ) const;
    };

    void AddNET( const NETINFO_ITEM* aNet );
    NET& GetNet( size_t aNetcode ) { return nets_map.at( aNetcode ); }

    class PIN
    {
    public:
        PIN( const size_t aIndex, const wxString& aName ) : m_index( aIndex ), m_name( aName ) {}

        const size_t m_index;
        wxString     m_name;

        std::pair<wxString, wxString> m_center;

        enum class TYPE
        {
            THROUGH_HOLE,
            BLIND,
            SURFACE
        };

        TYPE type = TYPE::SURFACE;

        enum class ELECTRICAL_TYPE
        {
            ELECTRICAL,
            MECHANICAL,
            UNDEFINED
        };

        ELECTRICAL_TYPE etype = ELECTRICAL_TYPE::UNDEFINED;

        enum class MOUNT_TYPE
        {
            SMT,
            SMT_RECOMMENDED,
            THROUGH_HOLE,
            THROUGH_RECOMMENDED,
            PRESSFIT,
            NON_BOARD,
            HOLE,
            UNDEFINED
        };
        MOUNT_TYPE mtype = MOUNT_TYPE::UNDEFINED;

        std::list<std::unique_ptr<PKG_OUTLINE>> m_pinOutlines;

        void Write( std::ostream& ost ) const;
    };

    class PACKAGE : public ATTR_RECORD_WRITER
    {
    public:
        PACKAGE( const size_t aIndex, const wxString& afpName ) :
                m_index( aIndex ), m_name( afpName ),
                m_pitch( 0 ),
                m_xmin( 0 ), m_ymin( 0 ),
                m_xmax( 0 ), m_ymax( 0 )

        {
        }

        const size_t m_index; /// <! Reference number of the package to be used in CMP.
        wxString     m_name;

        uint64_t  m_pitch;
        int64_t m_xmin, m_ymin, m_xmax, m_ymax; // Box points: leftlow, rightup

        std::list<std::unique_ptr<PKG_OUTLINE>> m_pkgOutlines;

        void                       AddPin( const PAD* aPad, size_t aPinNum );
        const std::shared_ptr<PIN> GetEdaPkgPin( size_t aPadIndex ) const
        {
            return m_pinsVec.at( aPadIndex );
        }

        void Write( std::ostream& ost ) const;

    private:
        std::vector<std::shared_ptr<PIN>> m_pinsVec;
    };

    void           AddPackage( const FOOTPRINT* aFp );
    const PACKAGE& GetPackage( size_t aHash ) const { return packages_map.at( aHash ); }

private:
    std::map<size_t, NET> nets_map;
    std::list<const NET*> nets;

    std::map<size_t, PACKAGE> packages_map; //hash value, package
    std::list<const PACKAGE*> packages;

    std::map<wxString, size_t>              layers_map;
    std::vector<wxString>                   layers;
    std::vector<std::shared_ptr<FOOTPRINT>> m_eda_footprints;
};

class PKG_OUTLINE
{
public:
    virtual void Write( std::ostream& ost ) const = 0;

    virtual ~PKG_OUTLINE() = default;
};

class OUTLINE_RECT : public PKG_OUTLINE
{
public:
    OUTLINE_RECT( const VECTOR2I& aLowerLeft, size_t aWidth, size_t aHeight ) :
            m_lower_left( aLowerLeft ), m_width( aWidth ), m_height( aHeight )
    {
    }

    OUTLINE_RECT( const BOX2I& aBox ) :
            OUTLINE_RECT( aBox.GetPosition(), aBox.GetWidth(), aBox.GetHeight() )
    {
    }

    VECTOR2I m_lower_left;
    size_t   m_width;
    size_t   m_height;

    void Write( std::ostream& ost ) const override;
};

class ODB_SURFACE_DATA;
class OUTLINE_CONTOUR : public PKG_OUTLINE
{
public:
    OUTLINE_CONTOUR( const SHAPE_POLY_SET::POLYGON& aPolygon,
                     FILL_T                         aFillType = FILL_T::FILLED_SHAPE )
    {
        if( !aPolygon.empty() && aPolygon[0].PointCount() >= 3 )
        {
            m_surfaces = std::make_unique<ODB_SURFACE_DATA>( aPolygon );
            if( aFillType != FILL_T::NO_FILL )
            {
                m_surfaces->AddPolygonHoles( aPolygon );
            }
        }
    }

    std::unique_ptr<ODB_SURFACE_DATA> m_surfaces;

    void Write( std::ostream& ost ) const override;
};

class OUTLINE_SQUARE : public PKG_OUTLINE
{
public:
    OUTLINE_SQUARE( const VECTOR2I& aCenter, size_t aHalfSide ) :
            m_center( aCenter ), m_halfSide( aHalfSide )
    {
    }
    VECTOR2I m_center;
    size_t   m_halfSide;

    void Write( std::ostream& ost ) const override;
};

class OUTLINE_CIRCLE : public PKG_OUTLINE
{
public:
    OUTLINE_CIRCLE( const VECTOR2I& aCenter, size_t aRadius ) :
            m_center( aCenter ), m_radius( aRadius )
    {
    }
    VECTOR2I m_center;
    size_t   m_radius;

    void Write( std::ostream& ost ) const override;
};


#endif // _ODB_EDA_DATA_H_
