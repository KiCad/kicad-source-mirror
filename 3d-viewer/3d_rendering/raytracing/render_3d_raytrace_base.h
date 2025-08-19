/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef RENDER_3D_RAYTRACE_BASE_H
#define RENDER_3D_RAYTRACE_BASE_H

#include "accelerators/container_3d.h"
#include "accelerators/accelerator_3d.h"
#include "../render_3d_base.h"
#include "light.h"
#include "../post_shader_ssao.h"
#include "material.h"
#include <plugins/3dapi/c3dmodel.h>

#include <map>

/// Vector of materials.
typedef std::vector< BLINN_PHONG_MATERIAL > MODEL_MATERIALS;

/// Maps a #S3DMODEL pointer with a created BLINN_PHONG_MATERIAL vector.
typedef std::map< const S3DMODEL* , MODEL_MATERIALS > MAP_MODEL_MATERIALS;

typedef enum
{
    RT_RENDER_STATE_TRACING = 0,
    RT_RENDER_STATE_POST_PROCESS_SHADE,
    RT_RENDER_STATE_POST_PROCESS_BLUR_AND_FINISH,
    RT_RENDER_STATE_FINISH,
    RT_RENDER_STATE_MAX
} RT_RENDER_STATE;


class RENDER_3D_RAYTRACE_BASE : public RENDER_3D_BASE
{
public:
    // TODO: Take into account board thickness so that the camera won't move inside of the board
    // when facing it perpendicularly.
    static constexpr float MIN_DISTANCE_IU = 4 * PCB_IU_PER_MM;

    explicit RENDER_3D_RAYTRACE_BASE( BOARD_ADAPTER& aAdapter, CAMERA& aCamera );

    ~RENDER_3D_RAYTRACE_BASE();

    int GetWaitForEditingTimeOut() override;

    void Reload( REPORTER* aStatusReporter, REPORTER* aWarningReporter,
                 bool aOnlyLoadCopperAndShapes );

    BOARD_ITEM *IntersectBoardItem( const RAY& aRay );

protected:
    virtual void initPbo() = 0;
    virtual void deletePbo() = 0;
    void createItemsFromContainer( const BVH_CONTAINER_2D* aContainer2d, PCB_LAYER_ID aLayer_id,
                                   const MATERIAL* aMaterialLayer, const SFVEC3F& aLayerColor,
                                   float aLayerZOffset );

    void restartRenderState();
    void renderTracing( uint8_t* ptrPBO, REPORTER* aStatusReporter );
    void postProcessShading( uint8_t* ptrPBO, REPORTER* aStatusReporter );
    void postProcessBlurFinish( uint8_t* ptrPBO, REPORTER* aStatusReporter );
    void renderBlockTracing( uint8_t* ptrPBO , signed int iBlock );
    void renderFinalColor( uint8_t* ptrPBO, const SFVEC4F& rgbColor,
                           bool applyColorSpaceConversion );

    void renderRayPackets( const SFVEC4F* bgColorY, const RAY* aRayPkt, HITINFO_PACKET* aHitPacket,
                           bool is_testShadow, SFVEC4F* aOutHitColor );

    void renderAntiAliasPackets( const SFVEC4F* aBgColorY, const HITINFO_PACKET* aHitPck_X0Y0,
                                 const HITINFO_PACKET* aHitPck_AA_X1Y1, const RAY* aRayPck,
                                 SFVEC4F* aOutHitColor );

    // Materials
    void setupMaterials();

    SFVEC4F shadeHit( const SFVEC4F& aBgColor, const RAY& aRay, HITINFO& aHitInfo,
                      bool aIsInsideObject, unsigned int aRecursiveLevel,
                      bool is_testShadow ) const;

    /**
     * Create one or more 3D objects form a 2D object and Z positions.
     *
     * It tries to optimize some types of objects that will be faster to trace than the
     * LAYER_ITEM object.
     */
    void createObject( CONTAINER_3D& aDstContainer, const OBJECT_2D* aObject2D, float aZMin,
                       float aZMax, const MATERIAL* aMaterial, const SFVEC3F& aObjColor );

    void addPadsAndVias();
    void insertHole( const PCB_VIA* aVia );
    void insertHole( const PAD* aPad );
    void addCounterborePlating( const BOARD_ITEM& aSource, const SFVEC2F& aCenter,
                                float aInnerRadius, float aDepth, float aSurfaceZ,
                                bool aIsFront );
    void addCountersinkPlating( const SFVEC2F& aCenter, float aTopInnerRadius,
                                float aBottomInnerRadius, float aSurfaceZ, float aDepth,
                                bool aIsFront );
    void backfillPostMachine();
    void load3DModels( CONTAINER_3D& aDstContainer, bool aSkipMaterialInformation );
    void addModels( CONTAINER_3D& aDstContainer, const S3DMODEL* a3DModel,
                    const glm::mat4& aModelMatrix, float aFPOpacity,
                    bool aSkipMaterialInformation, BOARD_ITEM* aBoardItem );

    MODEL_MATERIALS* getModelMaterial( const S3DMODEL* a3DModel );

    void initializeBlockPositions();

    void render( uint8_t* ptrPBO, REPORTER* aStatusReporter );
    void renderPreview( uint8_t* ptrPBO );

    static SFVEC4F premultiplyAlpha( const SFVEC4F& aInput );

    struct
    {
        BLINN_PHONG_MATERIAL m_Paste;
        BLINN_PHONG_MATERIAL m_SilkS;
        BLINN_PHONG_MATERIAL m_SolderMask;
        BLINN_PHONG_MATERIAL m_EpoxyBoard;
        BLINN_PHONG_MATERIAL m_Copper;
        BLINN_PHONG_MATERIAL m_NonPlatedCopper;
        BLINN_PHONG_MATERIAL m_Floor;
    } m_materials;

    BOARD_NORMAL         m_boardMaterial;
    COPPER_NORMAL        m_copperMaterial;
    PLATED_COPPER_NORMAL m_platedCopperMaterial;
    SOLDER_MASK_NORMAL   m_solderMaskMaterial;
    PLASTIC_NORMAL       m_plasticMaterial;
    PLASTIC_SHINE_NORMAL m_shinyPlasticMaterial;
    BRUSHED_METAL_NORMAL m_brushedMetalMaterial;
    SILK_SCREEN_NORMAL   m_silkScreenMaterial;

    bool m_is_canvas_initialized;
    bool m_isPreview;

    /// State used on quality render
    RT_RENDER_STATE m_renderState;

    /// Time that the render starts
    int64_t m_renderStartTime;

    /// Save the number of blocks progress of the render
    size_t m_blockRenderProgressCount;

    POST_SHADER_SSAO m_postShaderSsao;

    std::list<LIGHT*> m_lights;

    DIRECTIONAL_LIGHT* m_cameraLight;

    /*GLuint m_pboId;
    GLuint m_pboDataSize;*/

    CONTAINER_3D m_objectContainer;

    /// Store the list of created objects special for RT that will be clear in the end.
    CONTAINER_2D m_containerWithObjectsToDelete;

    CONTAINER_2D* m_outlineBoard2dObjects;
    BVH_CONTAINER_2D* m_antioutlineBoard2dObjects;

    ACCELERATOR_3D* m_accelerator;

    SFVEC4F m_backgroundColorTop;
    SFVEC4F m_backgroundColorBottom;

    /// Used to see if the windows size changed.
    wxSize m_oldWindowsSize;

    /// Encode Morton code positions.
    std::vector< SFVEC2UI > m_blockPositions;

    /// Flag if a position was already processed (cleared each new render).
    std::vector< int > m_blockPositionsWasProcessed;

    /// Encode the Morton code positions (on fast preview mode).
    std::vector< SFVEC2UI > m_blockPositionsFast;

    SFVEC2UI m_realBufferSize;
    SFVEC2UI m_fastPreviewModeSize;

    HITINFO_PACKET* m_firstHitinfo;

    SFVEC3F* m_shaderBuffer;

    // Display Offset
    unsigned int m_xoffset;
    unsigned int m_yoffset;

    /// Stores materials of the 3D models
    MAP_MODEL_MATERIALS m_modelMaterialMap;

    // Statistics
    unsigned int m_convertedDummyBlockCount;
    unsigned int m_converted2dRoundSegmentCount;
};

#define USE_SRGB_SPACE

#ifdef USE_SRGB_SPACE
extern SFVEC3F ConvertSRGBToLinear( const SFVEC3F& aSRGBcolor );
extern SFVEC4F ConvertSRGBAToLinear( const SFVEC4F& aSRGBAcolor );
#else
#define ConvertSRGBToLinear( v ) ( v )
#define ConvertSRGBAToLinear( v ) ( v )
#endif

#endif // RENDER_3D_RAYTRACE_BASE_H
