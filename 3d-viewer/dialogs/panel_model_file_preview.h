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

#ifndef PANEL_MODEL_FILE_PREVIEW_H
#define PANEL_MODEL_FILE_PREVIEW_H

#include <cstdint>
#include <functional>
#include <memory>

#include <plugins/3dapi/model_import.h>

#include <wx/panel.h>
#include <wx/string.h>

class EDA_3D_MODEL_VIEWER;
class S3DMODEL;
class wxCheckBox;
class wxFileDialog;

class PANEL_MODEL_FILE_PREVIEW : public wxPanel
{
public:
    struct RESULT
    {
        S3D::MODEL_IMPORT_ERROR status = S3D::MODEL_IMPORT_ERROR::INTERNAL;

        /// Keeps the imported model alive for as long as the panel shows it.
        std::shared_ptr<const void> owner;
        const S3DMODEL*             model = nullptr;
        wxString                    message;
    };

    using COMPLETION = std::function<void( uint64_t, RESULT )>;
    using REQUEST = std::function<void( wxString, uint64_t, COMPLETION )>;
    using CANCEL = std::function<void()>;

    PANEL_MODEL_FILE_PREVIEW( wxWindow* aParent, REQUEST aRequest, CANCEL aCancel, bool aEmbedFile );
    ~PANEL_MODEL_FILE_PREVIEW() override;

    void SetSelectedFile( const wxString& aPath );
    bool GetEmbedFile() const;

private:
    struct LIFETIME;

    void ApplyResult( uint64_t aGeneration, RESULT aResult );

    REQUEST                     m_request;
    CANCEL                      m_cancel;
    std::shared_ptr<LIFETIME>   m_lifetime;
    std::shared_ptr<const void> m_modelOwner;
    EDA_3D_MODEL_VIEWER*        m_viewer = nullptr;
    wxCheckBox*                 m_embedFile = nullptr;
    wxFileDialog*               m_dialog = nullptr;
    wxString                    m_selectedFile;
    uint64_t                    m_generation = 0;
};

#endif // PANEL_MODEL_FILE_PREVIEW_H
