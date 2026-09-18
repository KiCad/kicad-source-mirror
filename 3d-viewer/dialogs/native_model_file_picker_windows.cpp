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

#include "native_model_file_picker_windows.h"

#include <trace_helpers.h>

#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/translation.h>
#include <wx/utils.h>
#include <wx/window.h>

#include <shobjidl.h>

#include <vector>

namespace
{
constexpr DWORD EMBED_FILE_CONTROL_ID = 0x4B33444D;

template <typename T>
class COM_PTR
{
public:
    COM_PTR() = default;
    COM_PTR( const COM_PTR& ) = delete;
    COM_PTR& operator=( const COM_PTR& ) = delete;

    ~COM_PTR()
    {
        if( m_pointer )
            m_pointer->Release();
    }

    T*  Get() const { return m_pointer; }
    T** Put()
    {
        if( m_pointer )
        {
            m_pointer->Release();
            m_pointer = nullptr;
        }

        return &m_pointer;
    }
    T*       operator->() const { return m_pointer; }
    explicit operator bool() const { return m_pointer != nullptr; }

private:
    T* m_pointer = nullptr;
};

class COTASKMEM_STRING
{
public:
    COTASKMEM_STRING() = default;
    COTASKMEM_STRING( const COTASKMEM_STRING& ) = delete;
    COTASKMEM_STRING& operator=( const COTASKMEM_STRING& ) = delete;

    ~COTASKMEM_STRING() { CoTaskMemFree( m_pointer ); }

    PWSTR* Put() { return &m_pointer; }
    PWSTR  Get() const { return m_pointer; }

private:
    PWSTR m_pointer = nullptr;
};

class COM_APARTMENT
{
public:
    COM_APARTMENT() :
            m_result( CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED ) )
    {
    }

    ~COM_APARTMENT()
    {
        if( SUCCEEDED( m_result ) )
            CoUninitialize();
    }

    HRESULT Result() const { return m_result; }

private:
    HRESULT m_result;
};

} // namespace


NATIVE_MODEL_FILE_PICKER_RESULT
ShowNativeModelFilePickerWindows( const NATIVE_MODEL_FILE_PICKER_OPTIONS& aOptions )
{
    NATIVE_MODEL_FILE_PICKER_RESULT result;
    COM_APARTMENT                           apartment;

    if( FAILED( apartment.Result() ) )
    {
        wxLogTrace( traceModelPreview, wxS( "Unable to enter a COM apartment for the model picker: 0x%08lx" ),
                    static_cast<unsigned long>( apartment.Result() ) );
        return result;
    }

    COM_PTR<IFileOpenDialog> dialog;
    HRESULT hr = CoCreateInstance( CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( dialog.Put() ) );

    if( FAILED( hr ) )
    {
        wxLogTrace( traceModelPreview, wxS( "Unable to create the Windows model picker: 0x%08lx" ),
                    static_cast<unsigned long>( hr ) );
        return result;
    }

    FILEOPENDIALOGOPTIONS options =
            FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEPREVIEWPANEON;

    if( !( aOptions.style & wxFD_CHANGE_DIR ) )
        options |= FOS_NOCHANGEDIR;

    if( aOptions.style & wxFD_SHOW_HIDDEN )
        options |= FOS_FORCESHOWHIDDEN;

    if( aOptions.style & wxFD_NO_FOLLOW )
        options |= FOS_NODEREFERENCELINKS;

    hr = dialog->SetOptions( options );

    if( SUCCEEDED( hr ) && !aOptions.title.empty() )
        hr = dialog->SetTitle( aOptions.title.wc_str() );

    if( SUCCEEDED( hr ) && !aOptions.filename.empty() )
        hr = dialog->SetFileName( aOptions.filename.wc_str() );

    COM_PTR<IShellItem> initialFolder;

    if( SUCCEEDED( hr ) && !aOptions.directory.empty() )
    {
        HRESULT folderResult = SHCreateItemFromParsingName( aOptions.directory.wc_str(), nullptr,
                                                            IID_PPV_ARGS( initialFolder.Put() ) );

        if( SUCCEEDED( folderResult ) )
            dialog->SetFolder( initialFolder.Get() );
    }

    // wxParseCommonDialogsFilter is what the rest of KiCad parses these strings with, and it
    // already falls back to a single "all files" entry for anything it cannot split.
    wxArrayString descriptions;
    wxArrayString patterns;
    wxParseCommonDialogsFilter( aOptions.wildcard, descriptions, patterns );

    std::vector<COMDLG_FILTERSPEC> filterSpecs;
    filterSpecs.reserve( descriptions.GetCount() );

    for( size_t i = 0; i < descriptions.GetCount(); ++i )
        filterSpecs.push_back( { descriptions[i].wc_str(), patterns[i].wc_str() } );

    if( SUCCEEDED( hr ) )
        hr = dialog->SetFileTypes( static_cast<UINT>( filterSpecs.size() ), filterSpecs.data() );

    if( SUCCEEDED( hr ) )
    {
        UINT filterIndex = aOptions.filterIndex >= 0 && static_cast<size_t>( aOptions.filterIndex ) < filterSpecs.size()
                                   ? static_cast<UINT>( aOptions.filterIndex ) + 1
                                   : 1;
        hr = dialog->SetFileTypeIndex( filterIndex );
    }

    COM_PTR<IFileDialogCustomize> customize;
    bool                          hasEmbedControl = false;

    if( SUCCEEDED( hr ) )
    {
        HRESULT customizeResult = dialog->QueryInterface( IID_PPV_ARGS( customize.Put() ) );

        if( SUCCEEDED( customizeResult ) )
        {
            customizeResult = customize->AddCheckButton( EMBED_FILE_CONTROL_ID, _( "Embed file" ).wc_str(),
                                                          aOptions.embedFile ? TRUE : FALSE );
            hasEmbedControl = SUCCEEDED( customizeResult );
        }

        if( FAILED( customizeResult ) )
        {
            wxLogTrace( traceModelPreview, wxS( "Unable to add the Embed file control: 0x%08lx" ),
                        static_cast<unsigned long>( customizeResult ) );
        }
    }

    wxWindow* ownerWindow = aOptions.parent ? wxGetTopLevelParent( aOptions.parent ) : nullptr;
    HWND      owner = ownerWindow ? static_cast<HWND>( ownerWindow->GetHandle() ) : nullptr;

    if( SUCCEEDED( hr ) )
    {
        wxWindowDisabler disableOtherWindows( nullptr, ownerWindow );
        hr = dialog->Show( owner );
    }

    if( owner && IsWindow( owner ) )
        SetForegroundWindow( owner );

    if( hr == HRESULT_FROM_WIN32( ERROR_CANCELLED ) )
        return result;

    if( FAILED( hr ) )
    {
        wxLogTrace( traceModelPreview, wxS( "Windows model picker failed: 0x%08lx" ),
                    static_cast<unsigned long>( hr ) );
        return result;
    }

    COM_PTR<IShellItem> selectedItem;
    hr = dialog->GetResult( selectedItem.Put() );

    COTASKMEM_STRING selectedPath;

    if( SUCCEEDED( hr ) )
        hr = selectedItem->GetDisplayName( SIGDN_FILESYSPATH, selectedPath.Put() );

    BOOL embedFile = aOptions.embedFile ? TRUE : FALSE;

    if( SUCCEEDED( hr ) && hasEmbedControl )
    {
        HRESULT checkboxResult = customize->GetCheckButtonState( EMBED_FILE_CONTROL_ID, &embedFile );

        if( FAILED( checkboxResult ) )
        {
            wxLogTrace( traceModelPreview, wxS( "Unable to read the Embed file control: 0x%08lx" ),
                        static_cast<unsigned long>( checkboxResult ) );
            return result;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        UINT filterIndex = 1;

        if( SUCCEEDED( dialog->GetFileTypeIndex( &filterIndex ) ) && filterIndex > 0 )
            result.filterIndex = static_cast<int>( filterIndex - 1 );

        result.accepted = true;
        result.path = selectedPath.Get();
        result.embedFile = embedFile != FALSE;
    }

    return result;
}
