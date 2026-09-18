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

#ifndef KICAD_3D_PREVIEW_HANDLER_H
#define KICAD_3D_PREVIEW_HANDLER_H

#include <windows.h>
#include <shlobj.h>
#include <AIS_ViewController.hxx>
#include <atomic>
#include <mutex>
#include <memory>
#include <optional>

#include <plugins/3dapi/model_import.h>
#include <plugins/3dapi/model_import_queue.h>

class KICAD_3D_PREVIEW_HANDLER : public IObjectWithSite,
                                 public IPreviewHandler,
                                 public IOleWindow,
                                 public IInitializeWithFile,
                                 public AIS_ViewController
{
public:
    IFACEMETHODIMP QueryInterface( REFIID aRiid, void** aPpv );
    IFACEMETHODIMP_( ULONG ) AddRef();
    IFACEMETHODIMP_( ULONG ) Release();

    IFACEMETHODIMP Initialize( LPCWSTR aFilePath, DWORD aMode );

    IFACEMETHODIMP SetWindow( HWND aHwnd, const RECT* aPrc );
    IFACEMETHODIMP SetFocus();
    IFACEMETHODIMP QueryFocus( HWND* aHwnd );
    IFACEMETHODIMP TranslateAccelerator( MSG* aMsg );
    IFACEMETHODIMP SetRect( const RECT* aPrc );
    IFACEMETHODIMP DoPreview();
    IFACEMETHODIMP Unload();

    IFACEMETHODIMP GetWindow( HWND* aHwnd );
    IFACEMETHODIMP ContextSensitiveHelp( BOOL aEnterMode );

    IFACEMETHODIMP SetSite( IUnknown* aPunkSite );
    IFACEMETHODIMP GetSite( REFIID aRiid, void** aPpv );

    KICAD_3D_PREVIEW_HANDLER();

protected:
    ~KICAD_3D_PREVIEW_HANDLER();

    virtual void ProcessExpose() override;
    virtual void ProcessConfigure( bool aIsResized ) override;
    virtual void ProcessInput() override;

private:
    long m_cRef;

    LPWSTR m_PreviewFilePath;

    HWND m_hwndParent;

    RECT m_rcParent;

    HWND m_hwndPreview;

    IUnknown* m_punkSite;

    Handle( V3d_View ) m_view;

    Handle( AIS_InteractiveContext ) m_context;

    S3D::MODEL_IMPORT_QUEUE m_importQueue;

    /// Handed from the import worker to the window procedure, which is the only consumer.
    std::mutex                              m_resultMutex;
    std::optional<S3D::MODEL_IMPORT_RESULT> m_result;
    std::uint64_t                           m_resultGeneration = 0;

    HRESULT InitializeOpenCascadeComponents();

    HRESULT CreatePreviewWindow();

    HRESULT RegisterPreviewWindowClass();

    HRESULT CreatePreviewMainWindow();

    HRESULT             LoadAndDisplay3DContent();
    void FinishImport( uint64_t aGeneration );
    void CancelImport();

    static LRESULT WINAPI PreviewWindowProcWrapper( HWND aHwnd, UINT aMsg, WPARAM aParamW, LPARAM aParamL );
    LRESULT WINAPI        PreviewWindowProc( HWND aHwnd, UINT aMsg, WPARAM aParamW, LPARAM aParamL );
};

#endif // KICAD_3D_PREVIEW_HANDLER_H
