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

#include <kicad_3d_preview_handler.h>
#include <kicad_3d_model_to_occ.h>

#include <plugins/3dapi/model_import.h>

#include <new>

#include <winuser.h>
#include <shlwapi.h>

#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <WNT_Window.hxx>
#include <AIS_ViewController.hxx>
#include <OSD.hxx>
#include <V3d_Viewer.hxx>

extern HINSTANCE g_hInst;
extern long      g_cRefModule;
const wchar_t*   g_szKiCad3dPreviewClassName = L"KICAD_3D_PREVIEW_CLASS";
constexpr UINT   WM_KICAD_IMPORT_COMPLETE = WM_APP + 1;


inline int RECTWIDTH( const RECT& rc )
{
    return ( rc.right - rc.left );
}

inline int RECTHEIGHT( const RECT& rc )
{
    return ( rc.bottom - rc.top );
}

KICAD_3D_PREVIEW_HANDLER::KICAD_3D_PREVIEW_HANDLER() :
        m_cRef( 1 ),
        m_PreviewFilePath( NULL ),
        m_hwndParent( NULL ),
        m_hwndPreview( NULL ),
        m_punkSite( NULL ),
        m_view( nullptr ),
        m_context( nullptr )
{
    InterlockedIncrement( &g_cRefModule );
}

KICAD_3D_PREVIEW_HANDLER::~KICAD_3D_PREVIEW_HANDLER()
{
    try
    {
        Unload();
        m_importQueue.Shutdown();

        if( m_punkSite )
        {
            m_punkSite->Release();
            m_punkSite = NULL;
        }

        m_hwndParent = NULL;
    }
    catch( ... )
    {
    }

    InterlockedDecrement( &g_cRefModule );
}

#pragma region IUnknown

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::QueryInterface( REFIID aRiid, void** aPpv )
{
    static const QITAB qit[] = {
        QITABENT( KICAD_3D_PREVIEW_HANDLER, IObjectWithSite ),
        QITABENT( KICAD_3D_PREVIEW_HANDLER, IOleWindow ),
        QITABENT( KICAD_3D_PREVIEW_HANDLER, IInitializeWithFile ),
        QITABENT( KICAD_3D_PREVIEW_HANDLER, IPreviewHandler ),
        { 0 },
    };
    return QISearch( this, qit, aRiid, aPpv );
}

IFACEMETHODIMP_( ULONG ) KICAD_3D_PREVIEW_HANDLER::AddRef()
{
    return InterlockedIncrement( &m_cRef );
}

IFACEMETHODIMP_( ULONG ) KICAD_3D_PREVIEW_HANDLER::Release()
{
    ULONG cRef = InterlockedDecrement( &m_cRef );

    if( 0 == cRef )
    {
        delete this;
    }

    return cRef;
}

#pragma endregion

#pragma region IInitializeWithFile

HRESULT KICAD_3D_PREVIEW_HANDLER::Initialize( LPCWSTR aFilePath, DWORD )
{
    HRESULT hr = E_INVALIDARG;

    if( m_PreviewFilePath )
    {
        delete[] m_PreviewFilePath;
        m_PreviewFilePath = nullptr;
    }

    if( aFilePath )
    {
        size_t length = wcslen( aFilePath ) + 1;
        m_PreviewFilePath = new WCHAR[length];
        wcscpy_s( m_PreviewFilePath, length, aFilePath );
        hr = S_OK;
    }

    return hr;
}

#pragma endregion

#pragma region IPreviewHandler

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::SetWindow( HWND aHwnd, const RECT* aPrc )
{
    if( aHwnd && aPrc )
    {
        m_hwndParent = aHwnd;
        m_rcParent = *aPrc;

        if( m_hwndPreview )
        {
            SetParent( m_hwndPreview, m_hwndParent );
            SetWindowPos( m_hwndPreview, NULL, m_rcParent.left, m_rcParent.top, RECTWIDTH( m_rcParent ),
                          RECTHEIGHT( m_rcParent ), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
        }
    }
    return S_OK;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::SetFocus()
{
    HRESULT hr = S_FALSE;

    if( m_hwndPreview )
    {
        ::SetFocus( m_hwndPreview );
        hr = S_OK;
    }

    return hr;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::QueryFocus( HWND* aHwnd )
{
    HRESULT hr = E_INVALIDARG;

    if( aHwnd )
    {
        *aHwnd = ::GetFocus();

        if( *aHwnd )
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    return hr;
}

HRESULT KICAD_3D_PREVIEW_HANDLER::TranslateAccelerator( MSG* aMsg )
{
    HRESULT               hr = S_FALSE;
    IPreviewHandlerFrame* pFrame = NULL;

    if( m_punkSite && SUCCEEDED( m_punkSite->QueryInterface( &pFrame ) ) )
    {
        // The preview has no tab stops, so the host handles keyboard navigation.
        hr = pFrame->TranslateAccelerator( aMsg );

        pFrame->Release();
    }

    return hr;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::SetRect( const RECT* aPrc )
{
    HRESULT hr = E_INVALIDARG;

    if( aPrc != NULL )
    {
        m_rcParent = *aPrc;

        if( m_hwndPreview )
        {
            SetWindowPos( m_hwndPreview, NULL, m_rcParent.left, m_rcParent.top,
                          ( m_rcParent.right - m_rcParent.left ),
                          ( m_rcParent.bottom - m_rcParent.top ),
                          SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
        }

        hr = S_OK;
    }

    return hr;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::DoPreview()
{
    try
    {
        HRESULT hr = S_OK;

        if( m_hwndPreview != NULL || !m_PreviewFilePath )
        {
            return E_FAIL;
        }

        CancelImport();

        OSD::SetSignal( false );

        hr = InitializeOpenCascadeComponents();

        if( FAILED( hr ) )
            return hr;

        hr = CreatePreviewWindow();

        if( FAILED( hr ) )
            return hr;

        hr = LoadAndDisplay3DContent();

        if( FAILED( hr ) )
            return hr;

        return hr;
    }
    catch( ... )
    {
        return E_FAIL;
    }
}

HRESULT KICAD_3D_PREVIEW_HANDLER::Unload()
{
    try
    {
        CancelImport();
        if( m_PreviewFilePath )
        {
            delete[] m_PreviewFilePath;
            m_PreviewFilePath = nullptr;
        }

        if( m_hwndPreview )
        {
            ::SetWindowLongPtrW( m_hwndPreview, GWLP_USERDATA, 0 );

            if( !m_view.IsNull() )
            {
                m_view->Remove();
                m_view.Nullify();
            }

            if( !m_context.IsNull() )
            {
                m_context->RemoveAll( false );
                m_context.Nullify();
            }

            DestroyWindow( m_hwndPreview );
            m_hwndPreview = NULL;
        }
    }
    catch( ... )
    {
    }

    return S_OK;
}


void KICAD_3D_PREVIEW_HANDLER::CancelImport()
{
    m_importQueue.Cancel();

    std::lock_guard<std::mutex> lock( m_resultMutex );
    m_result.reset();
    m_resultGeneration = 0;
}

#pragma endregion

#pragma region IOleWindow

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::GetWindow( HWND* aHwnd )
{
    HRESULT hr = E_INVALIDARG;

    if( aHwnd )
    {
        *aHwnd = m_hwndParent;
        hr = S_OK;
    }
    return hr;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::ContextSensitiveHelp( BOOL aEnterMode )
{
    return E_NOTIMPL;
}

#pragma endregion

#pragma region IObjectWithSite

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::SetSite( IUnknown* aPunkSite )
{
    if( m_punkSite )
    {
        m_punkSite->Release();
        m_punkSite = NULL;
    }

    return aPunkSite ? aPunkSite->QueryInterface( &m_punkSite ) : S_OK;
}

IFACEMETHODIMP KICAD_3D_PREVIEW_HANDLER::GetSite( REFIID aRiid, void** aPpv )
{
    *aPpv = NULL;
    return m_punkSite ? m_punkSite->QueryInterface( aRiid, aPpv ) : E_FAIL;
}

#pragma endregion

#pragma region AIS_PreviewHandler

void KICAD_3D_PREVIEW_HANDLER::ProcessExpose()
{
    if( !m_view.IsNull() )
    {
        FlushViewEvents( m_context, m_view, true );
    }
}

void KICAD_3D_PREVIEW_HANDLER::ProcessConfigure( bool aIsResized )
{
    if( !m_view.IsNull() && aIsResized )
    {
        m_view->Window()->DoResize();
        m_view->MustBeResized();
        m_view->Invalidate();
        FlushViewEvents( m_context, m_view, true );
    }
}

void KICAD_3D_PREVIEW_HANDLER::ProcessInput()
{
    if( !m_view.IsNull() )
    {
        ProcessExpose();
    }
}

#pragma endregion

#pragma region OpenCascade components initialization

HRESULT KICAD_3D_PREVIEW_HANDLER::InitializeOpenCascadeComponents()
{
    Handle( Aspect_DisplayConnection ) aDisplayConnection = new Aspect_DisplayConnection();
    Handle( OpenGl_GraphicDriver ) aGraphicDriver = new OpenGl_GraphicDriver( aDisplayConnection );
    Handle( V3d_Viewer ) aViewer = new V3d_Viewer( aGraphicDriver );
    aViewer->SetDefaultLights();
    aViewer->SetLightOn();
    m_view = aViewer->CreateView();
    m_context = new AIS_InteractiveContext( aViewer );

    return S_OK;
}

#pragma endregion

#pragma region Window creation and handling

HRESULT KICAD_3D_PREVIEW_HANDLER::CreatePreviewWindow()
{
    HRESULT hr = S_OK;

    hr = RegisterPreviewWindowClass();

    if( FAILED( hr ) )
    {
        return hr;
    }

    hr = CreatePreviewMainWindow();

    if( FAILED( hr ) )
    {
        return hr;
    }

    return hr;
}

HRESULT KICAD_3D_PREVIEW_HANDLER::RegisterPreviewWindowClass()
{
    WNDCLASSEXW wcex = { sizeof( WNDCLASSEXW ) };
    wcex.lpfnWndProc = reinterpret_cast<WNDPROC>( PreviewWindowProcWrapper );
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = g_hInst;
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH) ( COLOR_WINDOW + 1 );
    wcex.style = 0;
    wcex.lpszClassName = g_szKiCad3dPreviewClassName;

    if( !RegisterClassExW( &wcex ) )
    {
        DWORD dwError = GetLastError();

        if( dwError != ERROR_CLASS_ALREADY_EXISTS )
        {
            return HRESULT_FROM_WIN32( dwError );
        }
    }

    return S_OK;
}


HRESULT KICAD_3D_PREVIEW_HANDLER::CreatePreviewMainWindow()
{
    m_hwndPreview = CreateWindowExW( 0, g_szKiCad3dPreviewClassName, L"OpenCASCADE Viewer", WS_CHILD | WS_VISIBLE,
                                     m_rcParent.left, m_rcParent.top, RECTWIDTH( m_rcParent ), RECTHEIGHT( m_rcParent ),
                                     m_hwndParent, NULL, g_hInst, NULL );

    if( !m_hwndPreview )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    ::SetWindowLongPtrW( m_hwndPreview, GWLP_USERDATA, (LONG_PTR) this );
    return S_OK;
}

#pragma endregion

#pragma region 3D content loading and display

HRESULT KICAD_3D_PREVIEW_HANDLER::LoadAndDisplay3DContent()
{
    ShowWindow( m_hwndPreview, SW_SHOW );

    Handle( WNT_Window ) aWindow = new WNT_Window( (Aspect_Handle) m_hwndPreview );

    m_view->SetWindow( aWindow );

    if( !aWindow->IsMapped() )
        aWindow->Map();

    m_view->SetBackgroundColor( Quantity_NOC_GRAY50 );
    m_view->TriedronDisplay( Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.1 );
    m_view->MustBeResized();

    SetLastError( ERROR_SUCCESS );
    int pathLength =
            WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, m_PreviewFilePath, -1, nullptr, 0, nullptr, nullptr );

    if( pathLength <= 1 )
    {
        DWORD error = GetLastError();
        return error == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32( error );
    }

    std::string inputFilePath( static_cast<size_t>( pathLength ), '\0' );

    SetLastError( ERROR_SUCCESS );

    if( !WideCharToMultiByte( CP_UTF8, WC_ERR_INVALID_CHARS, m_PreviewFilePath, -1, inputFilePath.data(), pathLength,
                              nullptr, nullptr ) )
    {
        DWORD error = GetLastError();
        return error == ERROR_SUCCESS ? E_FAIL : HRESULT_FROM_WIN32( error );
    }

    inputFilePath.resize( static_cast<size_t>( pathLength - 1 ) );

    const HWND previewWindow = m_hwndPreview;

    // The queue joins its worker in Shutdown(), which the destructor runs before any member of
    // this object dies, so the completion's captured `this` stays valid.
    m_importQueue.Submit( std::move( inputFilePath ), S3D::PreviewImportOptions(),
                          [this, previewWindow]( std::uint64_t aGeneration, S3D::MODEL_IMPORT_RESULT aResult )
                          {
                              std::lock_guard<std::mutex> lock( m_resultMutex );
                              m_result.emplace( std::move( aResult ) );
                              m_resultGeneration = aGeneration;

                              if( !PostMessageW( previewWindow, WM_KICAD_IMPORT_COMPLETE,
                                                 static_cast<WPARAM>( aGeneration ), 0 ) )
                              {
                                  m_result.reset();
                                  m_resultGeneration = 0;
                              }
                          } );

    return S_OK;
}


void KICAD_3D_PREVIEW_HANDLER::FinishImport( uint64_t aGeneration )
{
    try
    {
        std::optional<S3D::MODEL_IMPORT_RESULT> imported;

        {
            std::lock_guard<std::mutex> lock( m_resultMutex );

            if( !m_result || m_resultGeneration != aGeneration
                || aGeneration != m_importQueue.CurrentGeneration() )
            {
                return;
            }

            imported.emplace( std::move( *m_result ) );
            m_result.reset();
            m_resultGeneration = 0;
        }

        if( m_view.IsNull() || m_context.IsNull() || !*imported || !imported->GetModel()
            || !DisplayS3DModel( *imported->GetModel(), m_context ) )
            return;

        m_view->FitAll( 0.01, false );
        m_view->Invalidate();

        if( m_hwndPreview )
            ::InvalidateRect( m_hwndPreview, nullptr, FALSE );
    }
    catch( ... )
    {
        try
        {
            if( !m_context.IsNull() )
                m_context->RemoveAll( false );
        }
        catch( ... )
        {
        }
    }
}

#pragma endregion

#pragma region Window Procedures

LRESULT WINAPI KICAD_3D_PREVIEW_HANDLER::PreviewWindowProcWrapper( HWND aHwnd, UINT aMsg, WPARAM aParamW,
                                                                   LPARAM aParamL )
{
    KICAD_3D_PREVIEW_HANDLER* aThis = (KICAD_3D_PREVIEW_HANDLER*) ::GetWindowLongPtrW( aHwnd, GWLP_USERDATA );
    return aThis != NULL ? aThis->PreviewWindowProc( aHwnd, aMsg, aParamW, aParamL )
                         : ::DefWindowProcW( aHwnd, aMsg, aParamW, aParamL );
}

LRESULT WINAPI KICAD_3D_PREVIEW_HANDLER::PreviewWindowProc( HWND aHwnd, UINT aMsg, WPARAM aParamW, LPARAM aParamL )
{
    switch( aMsg )
    {
    case WM_KICAD_IMPORT_COMPLETE: FinishImport( static_cast<uint64_t>( aParamW ) ); return 0;

    case WM_CLOSE: return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT aPaint;
        ::BeginPaint( aHwnd, &aPaint );
        ::EndPaint( aHwnd, &aPaint );
        if( !m_view.IsNull() )
            m_view->Redraw();
        break;
    }
    case WM_SIZE:
    {
        if( !m_view.IsNull() && !m_context.IsNull() )
        {
            m_view->MustBeResized();
            AIS_ViewController::FlushViewEvents( m_context, m_view, true );
        }
        break;
    }
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        const Graphic3d_Vec2i  aPos( LOWORD( aParamL ), HIWORD( aParamL ) );
        const Aspect_VKeyFlags aFlags = WNT_Window::MouseKeyFlagsFromEvent( aParamW );
        Aspect_VKeyMouse       aButton = Aspect_VKeyMouse_NONE;
        switch( aMsg )
        {
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN: aButton = Aspect_VKeyMouse_LeftButton; break;
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN: aButton = Aspect_VKeyMouse_MiddleButton; break;
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN: aButton = Aspect_VKeyMouse_RightButton; break;
        }

        if( aMsg == WM_LBUTTONDOWN || aMsg == WM_MBUTTONDOWN || aMsg == WM_RBUTTONDOWN )
        {
            ::SetFocus( aHwnd );
            ::SetCapture( aHwnd );
            AIS_ViewController::PressMouseButton( aPos, aButton, aFlags, false );
        }
        else
        {
            ::ReleaseCapture();
            AIS_ViewController::ReleaseMouseButton( aPos, aButton, aFlags, false );
        }

        AIS_ViewController::FlushViewEvents( m_context, m_view, true );
        break;
    }
    case WM_MOUSEMOVE:
    {
        Graphic3d_Vec2i  aPos( LOWORD( aParamL ), HIWORD( aParamL ) );
        Aspect_VKeyMouse aButtons = WNT_Window::MouseButtonsFromEvent( aParamW );
        Aspect_VKeyFlags aFlags = WNT_Window::MouseKeyFlagsFromEvent( aParamW );
        CURSORINFO       aCursor;
        aCursor.cbSize = sizeof( aCursor );

        if( ::GetCursorInfo( &aCursor ) != FALSE )
        {
            POINT aCursorPnt = { aCursor.ptScreenPos.x, aCursor.ptScreenPos.y };

            if( ::ScreenToClient( aHwnd, &aCursorPnt ) )
            {
                aPos.SetValues( aCursorPnt.x, aCursorPnt.y );
                aButtons = WNT_Window::MouseButtonsAsync();
                aFlags = WNT_Window::MouseKeyFlagsAsync();
            }
        }

        AIS_ViewController::UpdateMousePosition( aPos, aButtons, aFlags, false );
        AIS_ViewController::FlushViewEvents( m_context, m_view, true );
        break;
    }
    case WM_MOUSEWHEEL:
    {
        const int              aDelta = GET_WHEEL_DELTA_WPARAM( aParamW );
        const double           aDeltaF = double( aDelta ) / double( WHEEL_DELTA );
        const Aspect_VKeyFlags aFlags = WNT_Window::MouseKeyFlagsFromEvent( aParamW );
        Graphic3d_Vec2i        aPos( int( short( LOWORD( aParamL ) ) ), int( short( HIWORD( aParamL ) ) ) );
        POINT                  aCursorPnt = { aPos.x(), aPos.y() };

        if( ::ScreenToClient( aHwnd, &aCursorPnt ) )
        {
            aPos.SetValues( aCursorPnt.x, aCursorPnt.y );
        }

        AIS_ViewController::UpdateMouseScroll( Aspect_ScrollDelta( aPos, aDeltaF, aFlags ) );
        AIS_ViewController::FlushViewEvents( m_context, m_view, true );
        break;
    }
    default:
    {
        return ::DefWindowProcW( aHwnd, aMsg, aParamW, aParamL );
    }
    }
    return 0;
}

#pragma endregion
