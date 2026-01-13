/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiplatform/webview.h>

#include <wx/webview.h>
#include <wx/log.h>
#include <wx/ffile.h>
#include <wx/filename.h>

#include <nlohmann/json.hpp>

#include <dlfcn.h>

namespace KIPLATFORM::WEBVIEW
{

// Opaque types
typedef struct _WebKitWebView WebKitWebView;
typedef struct _WebKitWebContext WebKitWebContext;
typedef struct _WebKitWebsiteDataManager WebKitWebsiteDataManager;

// Constants
typedef enum {
    WEBKIT_WEBSITE_DATA_COOKIES = 1 << 8
} WebKitWebsiteDataTypes;

// Function pointers
typedef WebKitWebContext* (*webkit_web_view_get_context_t)(WebKitWebView*);
typedef WebKitWebsiteDataManager* (*webkit_web_context_get_website_data_manager_t)(WebKitWebContext*);
typedef void (*webkit_website_data_manager_clear_t)(WebKitWebsiteDataManager*, WebKitWebsiteDataTypes, int64_t, void*, void*, void*);

bool SaveCookies( wxWebView* aWebView, const wxString& aTargetFile )
{
    // Not implemented for GTK due to ABI issues with libsoup2/3
    return false;
}

bool LoadCookies( wxWebView* aWebView, const wxString& aSourceFile )
{
    // Not implemented for GTK due to ABI issues with libsoup2/3
    return false;
}

bool DeleteCookies( wxWebView* aWebView )
{
    if( !aWebView )
        return false;

    void* nativeBackend = aWebView->GetNativeBackend();

    if( !nativeBackend )
        return false;

    // Load symbols
    auto get_context = (webkit_web_view_get_context_t) dlsym(RTLD_DEFAULT, "webkit_web_view_get_context");
    auto get_data_manager = (webkit_web_context_get_website_data_manager_t) dlsym( RTLD_DEFAULT, "webkit_web_context_get_website_data_manager" );
    auto clear_data = (webkit_website_data_manager_clear_t) dlsym( RTLD_DEFAULT, "webkit_website_data_manager_clear" );

    if( !get_context || !get_data_manager || !clear_data )
    {
        wxLogTrace( "webview", "Failed to load WebKit symbols" );
        return false;
    }

    WebKitWebView* webView = (WebKitWebView*) nativeBackend;
    WebKitWebContext* context = get_context(webView);

    if( !context )
        return false;

    WebKitWebsiteDataManager* dataManager = get_data_manager(context);

    if( !dataManager )
        return false;

    clear_data( dataManager, WEBKIT_WEBSITE_DATA_COOKIES, 0, nullptr, nullptr, nullptr );

    return true;
}

} // namespace KIPLATFORM::WEBVIEW

