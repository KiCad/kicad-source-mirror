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

#include "native_model_file_picker_gtk.h"

#ifdef __WXGTK3__

#include "model_file_preview_request_service.h"
#include "panel_model_file_preview.h"

#include <trace_helpers.h>

#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/thread.h>

namespace
{
struct PICKER_CONTEXT
{
    MODEL_FILE_PREVIEW_REQUEST_SERVICE* service;
    bool                                embedFile;
};

thread_local PICKER_CONTEXT* currentContext = nullptr;

// Held as a raw pointer because shutdown must run while wx and the importer are still alive,
// which a static object's destructor cannot guarantee.  pcbnew's IFACE::OnKifaceEnd is the sole
// owner of that teardown, so any other kiface that starts showing this picker has to call
// ShutdownNativeModelFilePickerGtk too or it leaks a joinable worker.
MODEL_FILE_PREVIEW_REQUEST_SERVICE* requestService = nullptr;

wxWindow* createExtraControl( wxWindow* aParent )
{
    if( !currentContext )
        return nullptr;

    try
    {
        return new PANEL_MODEL_FILE_PREVIEW( aParent, currentContext->service->MakeRequest(),
                                             currentContext->service->MakeCancel(), currentContext->embedFile );
    }
    catch( ... )
    {
        return nullptr;
    }
}

class CONTEXT_SCOPE
{
public:
    explicit CONTEXT_SCOPE( PICKER_CONTEXT* aContext ) { currentContext = aContext; }

    ~CONTEXT_SCOPE() { currentContext = nullptr; }

    CONTEXT_SCOPE( const CONTEXT_SCOPE& ) = delete;
    CONTEXT_SCOPE& operator=( const CONTEXT_SCOPE& ) = delete;
};
} // namespace


NATIVE_MODEL_FILE_PICKER_RESULT
ShowNativeModelFilePickerGtk( const NATIVE_MODEL_FILE_PICKER_OPTIONS& aOptions )
{
    wxASSERT_MSG( wxIsMainThread(), wxS( "model picker must run on the UI thread" ) );

    if( currentContext )
    {
        wxFAIL_MSG( wxS( "nested model pickers are unsupported" ) );
        return {};
    }

    if( !requestService )
        requestService = new MODEL_FILE_PREVIEW_REQUEST_SERVICE;

    const long style = ( aOptions.style & ~( wxFD_MULTIPLE | wxFD_SAVE ) ) | wxFD_OPEN | wxFD_FILE_MUST_EXIST;
    wxASSERT_MSG( !( aOptions.style & ( wxFD_MULTIPLE | wxFD_SAVE ) ),
                  wxS( "model picker supports single-file open only" ) );

    // S3D::ImportModel needs a real filesystem path, so the chooser stays local-only.
    wxFileDialog dialog( aOptions.parent, aOptions.title, aOptions.directory, aOptions.filename, aOptions.wildcard,
                         style );
    dialog.SetFilterIndex( aOptions.filterIndex );
    PICKER_CONTEXT context{ requestService, aOptions.embedFile };
    CONTEXT_SCOPE  contextScope( &context );

    if( !dialog.SetExtraControlCreator( &createExtraControl ) )
        wxLogTrace( traceModelPreview, wxS( "extra control unsupported" ) );

    const int modalResult = dialog.ShowModal();

    NATIVE_MODEL_FILE_PICKER_RESULT result;
    result.embedFile = aOptions.embedFile;

    if( modalResult == wxID_OK )
    {
        result.path = dialog.GetPath();
        result.filterIndex = dialog.GetFilterIndex();
        result.accepted = !result.path.IsEmpty();

        if( auto* panel = dynamic_cast<PANEL_MODEL_FILE_PREVIEW*>( dialog.GetExtraControl() ) )
            result.embedFile = panel->GetEmbedFile();
    }

    return result;
}


void ShutdownNativeModelFilePickerGtk()
{
    wxASSERT_MSG( wxIsMainThread(), wxS( "model picker shutdown must run on the UI thread" ) );

    if( requestService )
    {
        requestService->Shutdown();
        delete requestService;
        requestService = nullptr;
    }
}

#endif // __WXGTK3__
