/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#ifndef MODEL_IMPORT_H
#define MODEL_IMPORT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <plugins/3dapi/c3dmodel.h>
#include <plugins/3dapi/ifsg_defs.h>

#ifndef KICAD_3D_IMPORT_API
#if defined( COMPILE_3D_IMPORT )
#define KICAD_3D_IMPORT_API APIEXPORT
#else
#define KICAD_3D_IMPORT_API APIIMPORT
#endif
#endif

namespace S3D
{
enum class MODEL_IMPORT_FORMAT
{
    AUTO,
    STEP,
    IGES,
    VRML,
    X3D
};


enum class MODEL_IMPORT_ERROR
{
    NONE,
    UNSUPPORTED,
    UNREADABLE,
    INVALID,
    TOO_LARGE,
    CANCELED,
    IMPORT_FAILED,
    OUT_OF_MEMORY,
    INTERNAL
};


struct MODEL_IMPORT_OPTIONS
{
    MODEL_IMPORT_FORMAT format = MODEL_IMPORT_FORMAT::AUTO;
    double              stepLinearDeflection = 0.14;
    double              stepAngularDeflectionDegrees = 30.0;
    bool                vrmlEnableInline = true;
    bool                vrmlApplyLegacyUnitConversion = true;
    std::uint64_t       maxCompressedBytes = 64U * 1024U * 1024U;
    std::uint64_t       maxExpandedBytes = 512U * 1024U * 1024U;
    std::uint64_t       maxMeshBytes = 1024U * 1024U * 1024U;
    bool ( *isCanceled )( void* aContext ) noexcept = nullptr;
    void* cancellationContext = nullptr;

    bool IsCanceled() const noexcept { return isCanceled && isCanceled( cancellationContext ); }
};


class KICAD_3D_IMPORT_API MODEL_IMPORT_RESULT
{
public:
    MODEL_IMPORT_RESULT( MODEL_IMPORT_RESULT&& aOther ) noexcept;
    MODEL_IMPORT_RESULT& operator=( MODEL_IMPORT_RESULT&& aOther ) noexcept;
    ~MODEL_IMPORT_RESULT();

    MODEL_IMPORT_RESULT( const MODEL_IMPORT_RESULT& ) = delete;
    MODEL_IMPORT_RESULT& operator=( const MODEL_IMPORT_RESULT& ) = delete;

    explicit           operator bool() const noexcept;
    const S3DMODEL*    GetModel() const noexcept;
    MODEL_IMPORT_ERROR GetError() const noexcept;
    std::string_view   GetDiagnostic() const noexcept;

private:
    MODEL_IMPORT_RESULT() noexcept = default;

    S3DMODEL*          m_model = nullptr;
    MODEL_IMPORT_ERROR m_error = MODEL_IMPORT_ERROR::INTERNAL;
    std::string        m_diagnostic;

    friend KICAD_3D_IMPORT_API MODEL_IMPORT_RESULT ImportModel( const std::string&          aFileName,
                                                                const MODEL_IMPORT_OPTIONS& aOptions ) noexcept;
};


/**
 * Options for previewing a file the user has not opened.
 *
 * Inline following is off because merely selecting or hovering a model does not grant access to
 * its siblings, and the byte caps are tightened because a preview host has no way to report
 * progress or to be interrupted by the user.
 */
inline MODEL_IMPORT_OPTIONS PreviewImportOptions() noexcept
{
    MODEL_IMPORT_OPTIONS options;
    options.vrmlEnableInline = false;
    options.maxCompressedBytes = 32U * 1024U * 1024U;
    options.maxExpandedBytes = 256U * 1024U * 1024U;
    options.maxMeshBytes = 512U * 1024U * 1024U;

    return options;
}


/**
 * Import and flatten one model without invoking GUI facilities.
 *
 * Source and expanded temporary data are admitted against maxExpandedBytes, and the flattened
 * arrays are admitted against maxMeshBytes using checked arithmetic.  OCCT can allocate its
 * document and tessellation before those flattened-array checks, so these limits are not a hard
 * process-memory cap.  Cancellation is checked before and after file readers and tessellation,
 * and at parser, label, face, vertex and triangle loop boundaries.  Individual OCCT read and
 * BRepMesh_IncrementalMesh calls are noninterruptible.
 */
KICAD_3D_IMPORT_API MODEL_IMPORT_RESULT ImportModel( const std::string&          aFileName,
                                                     const MODEL_IMPORT_OPTIONS& aOptions = {} ) noexcept;
} // namespace S3D

#endif // MODEL_IMPORT_H
