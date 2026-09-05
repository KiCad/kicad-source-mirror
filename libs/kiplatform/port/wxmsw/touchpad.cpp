/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <kiplatform/touchpad.h>

#include <InteractionContext.h>
#include <windows.h>
#include <commctrl.h>

#include <limits>
#include <utility>
#include <vector>

#include <wx/window.h>


namespace
{
/**
 * Precision Touchpad registration and activity handler
 *
 * Precision Touchpads were introduced in Windows 8.1, but direct touchpad gesture input for
 * desktop applications uses APIs introduced in Windows 11.  Resolve them dynamically so older
 * Windows versions retain the scroll-event fallback.
 */
class MSW_TOUCHPAD_GESTURE_HANDLER final : public KIPLATFORM::UI::TOUCHPAD_GESTURE_HANDLER
{
public:
    MSW_TOUCHPAD_GESTURE_HANDLER( wxWindow* aInputWindow, KIPLATFORM::UI::TOUCHPAD_GESTURE_CALLBACK aCallback ) :
            m_inputWindow( aInputWindow ),
            m_callback( std::move( aCallback ) )
    {
        initialize();
    }

    ~MSW_TOUCHPAD_GESTURE_HANDLER() override
    {
        if( m_registered )
            m_registerTouchpadCapableWindow( getHwnd(), FALSE );

        if( m_subclassed )
            m_removeWindowSubclass( getHwnd(), windowProc, SUBCLASS_ID );

        if( m_context )
            m_destroyInteractionContext( m_context );

        if( m_ninput )
            FreeLibrary( m_ninput );

        if( m_comctl32 )
            FreeLibrary( m_comctl32 );
    }

    bool IsActive() const { return m_registered && m_subclassed; }

private:
    using REGISTER_TOUCHPAD_CAPABLE_WINDOW = BOOL( WINAPI* )( HWND, BOOL );
    using GET_POINTER_FRAME_TOUCHPAD_INFO_HISTORY = BOOL( WINAPI* )( UINT32, UINT32*, UINT32*, POINTER_TOUCH_INFO* );
    using CREATE_INTERACTION_CONTEXT = HRESULT( WINAPI* )( HINTERACTIONCONTEXT* );
    using DESTROY_INTERACTION_CONTEXT = HRESULT( WINAPI* )( HINTERACTIONCONTEXT );
    using REGISTER_OUTPUT_CALLBACK = HRESULT( WINAPI* )( HINTERACTIONCONTEXT, INTERACTION_CONTEXT_OUTPUT_CALLBACK,
                                                         void* );
    using SET_INTERACTION_CONFIGURATION = HRESULT( WINAPI* )( HINTERACTIONCONTEXT, UINT32,
                                                              const INTERACTION_CONTEXT_CONFIGURATION* );
    using SET_INTERACTION_PROPERTY = HRESULT( WINAPI* )( HINTERACTIONCONTEXT, INTERACTION_CONTEXT_PROPERTY, UINT32 );
    using PROCESS_POINTER_FRAMES_INTERACTION_CONTEXT2 = HRESULT( WINAPI* )( HINTERACTIONCONTEXT, UINT32, UINT32,
                                                                            const POINTER_TYPE_INFO* );
    using SET_WINDOW_SUBCLASS = BOOL( WINAPI* )( HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR );
    using REMOVE_WINDOW_SUBCLASS = BOOL( WINAPI* )( HWND, SUBCLASSPROC, UINT_PTR );
    using DEF_SUBCLASS_PROC = LRESULT( WINAPI* )( HWND, UINT, WPARAM, LPARAM );

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4191 )
#endif

    template <typename T>
    static T loadNamedProc( HMODULE aModule, const char* aName )
    {
        return reinterpret_cast<T>( GetProcAddress( aModule, aName ) );
    }

    template <typename T>
    static T loadOrdinalProc( HMODULE aModule, WORD aOrdinal )
    {
        return reinterpret_cast<T>( GetProcAddress( aModule, MAKEINTRESOURCEA( aOrdinal ) ) );
    }

#ifdef _MSC_VER
#pragma warning( pop )
#endif

    HWND getHwnd() const { return reinterpret_cast<HWND>( m_inputWindow->GetHandle() ); }

    void initialize()
    {
        HMODULE user32 = GetModuleHandleW( L"user32.dll" );
        m_ninput = LoadLibraryExW( L"ninput.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );
        m_comctl32 = LoadLibraryExW( L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );

        if( !user32 || !m_ninput || !m_comctl32 )
            return;

        m_registerTouchpadCapableWindow = loadOrdinalProc<REGISTER_TOUCHPAD_CAPABLE_WINDOW>( user32, 2689 );
        m_getPointerFrameTouchpadInfoHistory = loadOrdinalProc<GET_POINTER_FRAME_TOUCHPAD_INFO_HISTORY>( user32, 2694 );
        m_processPointerFramesInteractionContext2 =
                loadOrdinalProc<PROCESS_POINTER_FRAMES_INTERACTION_CONTEXT2>( m_ninput, 2507 );
        m_createInteractionContext = loadNamedProc<CREATE_INTERACTION_CONTEXT>( m_ninput, "CreateInteractionContext" );
        m_destroyInteractionContext =
                loadNamedProc<DESTROY_INTERACTION_CONTEXT>( m_ninput, "DestroyInteractionContext" );
        m_registerOutputCallback =
                loadNamedProc<REGISTER_OUTPUT_CALLBACK>( m_ninput, "RegisterOutputCallbackInteractionContext" );
        m_setInteractionConfiguration = loadNamedProc<SET_INTERACTION_CONFIGURATION>(
                m_ninput, "SetInteractionConfigurationInteractionContext" );
        m_setInteractionProperty = loadNamedProc<SET_INTERACTION_PROPERTY>( m_ninput, "SetPropertyInteractionContext" );
        m_setWindowSubclass = loadNamedProc<SET_WINDOW_SUBCLASS>( m_comctl32, "SetWindowSubclass" );
        m_removeWindowSubclass = loadNamedProc<REMOVE_WINDOW_SUBCLASS>( m_comctl32, "RemoveWindowSubclass" );
        m_defSubclassProc = loadNamedProc<DEF_SUBCLASS_PROC>( m_comctl32, "DefSubclassProc" );

        if( !m_registerTouchpadCapableWindow || !m_getPointerFrameTouchpadInfoHistory
            || !m_processPointerFramesInteractionContext2 || !m_createInteractionContext || !m_destroyInteractionContext
            || !m_registerOutputCallback || !m_setInteractionConfiguration || !m_setInteractionProperty
            || !m_setWindowSubclass || !m_removeWindowSubclass || !m_defSubclassProc )
        {
            return;
        }

        if( FAILED( m_createInteractionContext( &m_context ) ) )
            return;

        if( FAILED( m_registerOutputCallback( m_context, outputCallback, this ) ) )
            return;

        INTERACTION_CONTEXT_CONFIGURATION configuration = {
            INTERACTION_ID_MANIPULATION, INTERACTION_CONFIGURATION_FLAG_MANIPULATION
                                                 | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_X
                                                 | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_TRANSLATION_Y
                                                 | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_SCALING
                                                 | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_RAILS_X
                                                 | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_RAILS_Y
                                                 | INTERACTION_CONFIGURATION_FLAG_MANIPULATION_MULTIPLE_FINGER_PANNING
        };

        if( FAILED( m_setInteractionConfiguration( m_context, 1, &configuration ) ) )
            return;

        // Device-space output is stable across display scale factors.  It is converted to the
        // input window's effective DPI in onOutput().
        if( FAILED( m_setInteractionProperty( m_context, INTERACTION_CONTEXT_PROPERTY_MEASUREMENT_UNITS, 0 ) )
            || FAILED( m_setInteractionProperty( m_context, INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS, FALSE ) ) )
        {
            return;
        }

        m_registered = m_registerTouchpadCapableWindow( getHwnd(), TRUE ) != FALSE;

        if( !m_registered )
            return;

        if( !m_setWindowSubclass( getHwnd(), windowProc, SUBCLASS_ID, reinterpret_cast<DWORD_PTR>( this ) ) )
        {
            m_registerTouchpadCapableWindow( getHwnd(), FALSE );
            m_registered = false;
            return;
        }

        m_subclassed = true;
    }

    bool handleMessage( UINT aMessage, WPARAM aWParam )
    {
        if( !m_registered
            || ( aMessage != WM_POINTERDOWN && aMessage != WM_POINTERUPDATE && aMessage != WM_POINTERUP ) )
        {
            return false;
        }

        const UINT32 pointerId = GET_POINTERID_WPARAM( aWParam );
        POINTER_INFO pointerInfo = {};

        if( !GetPointerInfo( pointerId, &pointerInfo ) || pointerInfo.pointerType != PT_TOUCHPAD )
            return false;

        UINT32 frameCount = 0;
        UINT32 pointerCount = 0;

        if( !m_getPointerFrameTouchpadInfoHistory( pointerId, &frameCount, &pointerCount, nullptr ) || frameCount == 0
            || pointerCount == 0 || frameCount > std::numeric_limits<size_t>::max() / pointerCount )
        {
            return false;
        }

        const size_t entryCount = static_cast<size_t>( frameCount ) * pointerCount;

        // A normal touchpad frame has only a handful of contacts.  Keep corrupt input from
        // turning an input message into an unbounded allocation.
        if( entryCount > 65536 )
            return false;

        std::vector<POINTER_TOUCH_INFO> touchInfos( entryCount );

        if( !m_getPointerFrameTouchpadInfoHistory( pointerId, &frameCount, &pointerCount, touchInfos.data() ) )
            return false;

        const size_t filledEntryCount = static_cast<size_t>( frameCount ) * pointerCount;

        if( filledEntryCount > touchInfos.size() )
            return false;

        std::vector<POINTER_TYPE_INFO> typeInfos( filledEntryCount );

        for( size_t i = 0; i < filledEntryCount; ++i )
        {
            typeInfos[i].type = touchInfos[i].pointerInfo.pointerType;
            typeInfos[i].touchInfo = touchInfos[i];
        }

        if( FAILED( m_processPointerFramesInteractionContext2( m_context, frameCount, pointerCount,
                                                               typeInfos.data() ) ) )
        {
            return false;
        }

        // The entire frame was processed above; suppress its remaining per-contact messages.
        SkipPointerFrameMessages( pointerId );
        return true;
    }

    static LRESULT CALLBACK windowProc( HWND aHwnd, UINT aMessage, WPARAM aWParam, LPARAM aLParam, UINT_PTR,
                                        DWORD_PTR aReferenceData )
    {
        auto* handler = reinterpret_cast<MSW_TOUCHPAD_GESTURE_HANDLER*>( aReferenceData );

        try
        {
            if( handler && handler->handleMessage( aMessage, aWParam ) )
                return 0;
        }
        catch( ... )
        {
            // C++ exceptions must not escape a native window procedure.
        }

        if( handler && handler->m_defSubclassProc )
            return handler->m_defSubclassProc( aHwnd, aMessage, aWParam, aLParam );

        return DefWindowProcW( aHwnd, aMessage, aWParam, aLParam );
    }

    static void CALLBACK outputCallback( void* aClientData, const INTERACTION_CONTEXT_OUTPUT* aOutput )
    {
        try
        {
            static_cast<MSW_TOUCHPAD_GESTURE_HANDLER*>( aClientData )->onOutput( aOutput );
        }
        catch( ... )
        {
            // C++ exceptions must not escape an Interaction Context callback.
        }
    }

    void onOutput( const INTERACTION_CONTEXT_OUTPUT* aOutput )
    {
        if( !aOutput || aOutput->interactionId != INTERACTION_ID_MANIPULATION || aOutput->inputType != PT_TOUCHPAD )
        {
            return;
        }

        const wxSize                  dpi = m_inputWindow->GetDPI();
        constexpr double              HIMETRIC_PER_INCH = 2540.0;
        const double                  pixelsPerHimetricX = ( dpi.x > 0 ? dpi.x : 96 ) / HIMETRIC_PER_INCH;
        const double                  pixelsPerHimetricY = ( dpi.y > 0 ? dpi.y : 96 ) / HIMETRIC_PER_INCH;
        const MANIPULATION_TRANSFORM& delta = aOutput->arguments.manipulation.delta;

        POINT cursorPosition = {};

        if( !GetCursorPos( &cursorPosition ) || !ScreenToClient( getHwnd(), &cursorPosition ) )
            return;

        m_callback( { delta.translationX * pixelsPerHimetricX, delta.translationY * pixelsPerHimetricY, delta.scale,
                      wxPoint( cursorPosition.x, cursorPosition.y ) } );
    }

    static constexpr UINT_PTR SUBCLASS_ID = 1;

    wxWindow*                                   m_inputWindow = nullptr;
    KIPLATFORM::UI::TOUCHPAD_GESTURE_CALLBACK   m_callback;
    HMODULE                                     m_ninput = nullptr;
    HMODULE                                     m_comctl32 = nullptr;
    HINTERACTIONCONTEXT                         m_context = nullptr;
    REGISTER_TOUCHPAD_CAPABLE_WINDOW            m_registerTouchpadCapableWindow = nullptr;
    GET_POINTER_FRAME_TOUCHPAD_INFO_HISTORY     m_getPointerFrameTouchpadInfoHistory = nullptr;
    CREATE_INTERACTION_CONTEXT                  m_createInteractionContext = nullptr;
    DESTROY_INTERACTION_CONTEXT                 m_destroyInteractionContext = nullptr;
    REGISTER_OUTPUT_CALLBACK                    m_registerOutputCallback = nullptr;
    SET_INTERACTION_CONFIGURATION               m_setInteractionConfiguration = nullptr;
    SET_INTERACTION_PROPERTY                    m_setInteractionProperty = nullptr;
    PROCESS_POINTER_FRAMES_INTERACTION_CONTEXT2 m_processPointerFramesInteractionContext2 = nullptr;
    SET_WINDOW_SUBCLASS                         m_setWindowSubclass = nullptr;
    REMOVE_WINDOW_SUBCLASS                      m_removeWindowSubclass = nullptr;
    DEF_SUBCLASS_PROC                           m_defSubclassProc = nullptr;
    bool                                        m_registered = false;
    bool                                        m_subclassed = false;
};
} // namespace


bool KIPLATFORM::UI::IsNativeTouchpadGestureAvailable()
{
    HMODULE user32 = GetModuleHandleW( L"user32.dll" );
    HMODULE ninput = LoadLibraryExW( L"ninput.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );
    HMODULE comctl32 = LoadLibraryExW( L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32 );

    const bool available =
            user32 && ninput && comctl32 && GetProcAddress( user32, MAKEINTRESOURCEA( 2689 ) )
            && GetProcAddress( user32, MAKEINTRESOURCEA( 2694 ) ) && GetProcAddress( ninput, MAKEINTRESOURCEA( 2507 ) )
            && GetProcAddress( ninput, "CreateInteractionContext" )
            && GetProcAddress( ninput, "DestroyInteractionContext" )
            && GetProcAddress( ninput, "RegisterOutputCallbackInteractionContext" )
            && GetProcAddress( ninput, "SetInteractionConfigurationInteractionContext" )
            && GetProcAddress( ninput, "SetPropertyInteractionContext" )
            && GetProcAddress( comctl32, "SetWindowSubclass" ) && GetProcAddress( comctl32, "RemoveWindowSubclass" )
            && GetProcAddress( comctl32, "DefSubclassProc" );

    if( ninput )
        FreeLibrary( ninput );

    if( comctl32 )
        FreeLibrary( comctl32 );

    return available;
}


std::unique_ptr<KIPLATFORM::UI::TOUCHPAD_GESTURE_HANDLER>
KIPLATFORM::UI::CreateTouchpadGestureHandler( wxWindow* aInputWindow, TOUCHPAD_GESTURE_CALLBACK aCallback )
{
    if( !aInputWindow || !aCallback )
        return nullptr;

    auto handler = std::make_unique<MSW_TOUCHPAD_GESTURE_HANDLER>( aInputWindow, std::move( aCallback ) );

    if( !handler->IsActive() )
        return nullptr;

    return handler;
}
