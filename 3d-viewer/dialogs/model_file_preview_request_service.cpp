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

#include "model_file_preview_request_service.h"

#include <plugins/3dapi/model_import_queue.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>


struct MODEL_FILE_PREVIEW_REQUEST_SERVICE::STATE
{
    S3D::MODEL_IMPORT_QUEUE queue;
};


MODEL_FILE_PREVIEW_REQUEST_SERVICE::MODEL_FILE_PREVIEW_REQUEST_SERVICE() :
        m_state( std::make_shared<STATE>() )
{
}


MODEL_FILE_PREVIEW_REQUEST_SERVICE::~MODEL_FILE_PREVIEW_REQUEST_SERVICE()
{
    Shutdown();
}


void MODEL_FILE_PREVIEW_REQUEST_SERVICE::Cancel()
{
    m_state->queue.Cancel();
}


void MODEL_FILE_PREVIEW_REQUEST_SERVICE::Shutdown()
{
    m_state->queue.Shutdown();
}


PANEL_MODEL_FILE_PREVIEW::REQUEST MODEL_FILE_PREVIEW_REQUEST_SERVICE::MakeRequest()
{
    std::weak_ptr<STATE> state = m_state;

    return [state]( wxString aPath, uint64_t aGeneration, PANEL_MODEL_FILE_PREVIEW::COMPLETION aCompletion )
    {
        std::shared_ptr<STATE> locked = state.lock();

        if( !locked )
            return;

        // The panel stamps its own generation, so the queue's is ignored here.
        locked->queue.Submit( std::string( aPath.utf8_str() ), S3D::PreviewImportOptions(),
                              [aGeneration, completion = std::move( aCompletion )]( std::uint64_t,
                                                                                    S3D::MODEL_IMPORT_RESULT aImported )
                              {
                                  PANEL_MODEL_FILE_PREVIEW::RESULT result;
                                  result.status = aImported.GetError();

                                  const std::string_view diagnostic = aImported.GetDiagnostic();
                                  result.message = wxString::FromUTF8( diagnostic.data(), diagnostic.size() );

                                  if( result.message.IsEmpty() && !diagnostic.empty() )
                                      result.message = wxString::From8BitData( diagnostic.data(), diagnostic.size() );

                                  if( aImported )
                                  {
                                      auto owner = std::make_shared<S3D::MODEL_IMPORT_RESULT>( std::move( aImported ) );
                                      result.model = owner->GetModel();
                                      result.owner = std::move( owner );
                                  }

                                  completion( aGeneration, std::move( result ) );
                              } );
    };
}


PANEL_MODEL_FILE_PREVIEW::CANCEL MODEL_FILE_PREVIEW_REQUEST_SERVICE::MakeCancel()
{
    std::weak_ptr<STATE> state = m_state;

    return [state]
    {
        if( std::shared_ptr<STATE> locked = state.lock() )
            locked->queue.Cancel();
    };
}
