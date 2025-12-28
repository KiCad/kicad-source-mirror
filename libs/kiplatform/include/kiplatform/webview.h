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

#ifndef KIPLATFORM_WEBVIEW_H_
#define KIPLATFORM_WEBVIEW_H_

#include <wx/string.h>

class wxWebView;

namespace KIPLATFORM
{
namespace WEBVIEW
{
    /**
     * Save cookies from the given WebView to the specified file.
     *
     * This function retrieves cookies from the WebView's native backend
     * and serializes them to a JSON file. The format is platform-independent
     * so cookies can theoretically be transferred between systems (though
     * typically they are only restored on the same machine).
     *
     * @param aWebView The WebView to extract cookies from. Must not be null.
     * @param aTargetFile Full path to the file where cookies will be saved.
     * @return true if cookies were successfully saved, false otherwise.
     */
    bool SaveCookies( wxWebView* aWebView, const wxString& aTargetFile );

    /**
     * Load cookies from the specified file into the given WebView.
     *
     * This function reads cookies from a JSON file and injects them into
     * the WebView's native backend. The WebView should already be created
     * and initialized before calling this function.
     *
     * @param aWebView The WebView to load cookies into. Must not be null.
     * @param aSourceFile Full path to the file containing saved cookies.
     * @return true if cookies were successfully loaded, false otherwise.
     */
    bool LoadCookies( wxWebView* aWebView, const wxString& aSourceFile );

    /**
     * Delete all cookies from the given WebView.
     *
     * This function removes all cookies from the WebView's native backend.
     *
     * @param aWebView The WebView to delete cookies from. Must not be null.
     * @return true if cookies were successfully deleted, false otherwise.
     */
    bool DeleteCookies( wxWebView* aWebView );

} // namespace WEBVIEW
} // namespace KIPLATFORM

#endif // KIPLATFORM_WEBVIEW_H_
