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

#include <printing.h>

#ifndef __MINGW32__
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <utility>
#include <roapi.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Printing.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Printing.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Data.Pdf.h>
#include <winrt/Windows.Graphics.Imaging.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Printing.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Printing.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Data.Pdf.h>
#include <winrt/Windows.Graphics.Imaging.h>

#include <wx/log.h>

using namespace winrt;

// Manual declaration of IPrintManagerInterop to avoid missing header
MIDL_INTERFACE("C5435A42-8D43-4E7B-A68A-EF311E392087")
IPrintManagerInterop : public ::IInspectable
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetForWindow(
        /* [in] */ HWND appWindow,
        /* [in] */ REFIID riid,
        /* [iid_is][retval][out] */ void **printManager) = 0;

    virtual HRESULT STDMETHODCALLTYPE ShowPrintUIForWindowAsync(
        /* [in] */ HWND appWindow,
        /* [retval][out] */ void **operation) = 0;
};

// Manual declaration of IDesktopWindowXamlSourceNative to avoid missing header
MIDL_INTERFACE("3cbcf1bf-2f76-4e9c-96ab-e84b37972554")
IDesktopWindowXamlSourceNative : public ::IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE AttachToWindow(
        /* [in] */ HWND parentWnd) = 0;

    virtual HRESULT STDMETHODCALLTYPE get_WindowHandle(
        /* [retval][out] */ HWND *hWnd) = 0;
};

static inline std::pair<uint32_t, uint32_t> DpToPixels( winrt::Windows::Data::Pdf::PdfPage const& page, double dpi )
{
    const auto   s = page.Size(); // DIPs (1 DIP = 1/96 inch)
    const double scale = dpi / 96.0;
    uint32_t     w = static_cast<uint32_t>( std::max( 1.0, std::floor( s.Width * scale + 0.5 ) ) );
    uint32_t     h = static_cast<uint32_t>( std::max( 1.0, std::floor( s.Height * scale + 0.5 ) ) );
    return { w, h };
}

// Helper class to manage image with its associated stream
struct ManagedImage
{
    winrt::Windows::UI::Xaml::Controls::Image image;
    winrt::Windows::Storage::Streams::InMemoryRandomAccessStream stream;

    ManagedImage() = default;
    ManagedImage(winrt::Windows::UI::Xaml::Controls::Image img, winrt::Windows::Storage::Streams::InMemoryRandomAccessStream str) : image(img), stream(str) {}

    ManagedImage(ManagedImage&& other) noexcept
        : image(std::move(other.image)), stream(std::move(other.stream)) {}

    ManagedImage& operator=(ManagedImage&& other) noexcept {
        if (this != &other) {
            image = std::move(other.image);
            stream = std::move(other.stream);
        }
        return *this;
    }
};

// Render one page to a XAML Image using RenderToStreamAsync
// dpi: e.g., 300 for preview; 600 for print
// Returns a ManagedImage that keeps the stream alive
static ManagedImage RenderPdfPageToImage( winrt::Windows::Data::Pdf::PdfDocument const& pdf, uint32_t pageIndex, double dpi )
{
    auto page = pdf.GetPage( pageIndex );

    if( !page )
    {
        wxLogTrace( PRINTING_TRACE, "Failed to get page %u from PDF document", pageIndex );
        return {};
    }

    auto [pxW, pxH] = DpToPixels( page, dpi );

    winrt::Windows::Data::Pdf::PdfPageRenderOptions opts;
    opts.DestinationWidth( pxW );
    opts.DestinationHeight( pxH );

    winrt::Windows::Storage::Streams::InMemoryRandomAccessStream stream;

    try
    {
        page.RenderToStreamAsync( stream, opts ).get(); // sync for simplicity
    }
    catch( std::exception& e )
    {
        wxLogTrace( PRINTING_TRACE, "Failed to render page %u to image: %s", pageIndex, e.what() );
        return {};
    }

    // Use a BitmapImage that sources directly from the stream
    winrt::Windows::UI::Xaml::Media::Imaging::BitmapImage bmp;

    try
    {
        stream.Seek(0);
        bmp.SetSourceAsync( stream ).get();
    }
    catch( const winrt::hresult_error& e )
    {
        wxLogTrace( PRINTING_TRACE, "Failed to set BitmapImage source for page %u: %s", pageIndex, e.message().c_str() );
        return {};
    }
    catch( std::exception& e )
    {
        wxLogTrace( PRINTING_TRACE, "Failed to set BitmapImage source for page %u: %s", pageIndex, e.what() );
        return {};
    }

    winrt::Windows::UI::Xaml::Controls::Image img;
    img.Source( bmp );
    img.Stretch( winrt::Windows::UI::Xaml::Media::Stretch::Uniform );

    // Return both image and stream to keep stream alive
    return ManagedImage{ img, stream };
}


namespace KIPLATFORM {
namespace PRINTING {

class WIN_PDF_PRINTER
{
public:
    WIN_PDF_PRINTER( HWND hwndOwner, winrt::Windows::Data::Pdf::PdfDocument const& pdf ) :
            m_hwnd( hwndOwner ),
            m_pdf( pdf )
    {
    }

    PRINT_RESULT Run()
    {
        if( !m_pdf )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to load PDF document" );
            return PRINT_RESULT::FAILED_TO_LOAD;
        }

        // Create hidden XAML Island host
        m_xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();
        auto native = m_xamlSource.as<IDesktopWindowXamlSourceNative>();

        if( !native )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to create XAML Island host" );
            return PRINT_RESULT::FAILED_TO_PRINT;
        }

        RECT rc{ 0, 0, 100, 100 }; // Use larger minimum size
        m_host = ::CreateWindowExW( 0, L"STATIC", L"", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                    rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, m_hwnd, nullptr,
                                    ::GetModuleHandleW( nullptr ), nullptr );

        auto cleanup_guard = std::unique_ptr<void, std::function<void( void* )>>
                            ( (void*) 1, [this]( void* ){ this->cleanup(); } );

        if( !m_host )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to create host window" );
            return PRINT_RESULT::FAILED_TO_PRINT;
        }

        if( FAILED( native->AttachToWindow( m_host ) ) )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to attach XAML Island to host window" );
            return PRINT_RESULT::FAILED_TO_PRINT;
        }

        m_root = winrt::Windows::UI::Xaml::Controls::Grid();
        m_xamlSource.Content( m_root );

        m_printDoc = winrt::Windows::UI::Xaml::Printing::PrintDocument();
        m_docSrc = m_printDoc.DocumentSource();
        m_pageCount = std::max<uint32_t>( 1, m_pdf.PageCount() );

        m_paginateToken = m_printDoc.Paginate(
                [this]( winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Printing::PaginateEventArgs const& e )
                {
                    m_printDoc.SetPreviewPageCount( m_pageCount, winrt::Windows::UI::Xaml::Printing::PreviewPageCountType::Final );
                } );

        m_getPreviewToken = m_printDoc.GetPreviewPage(
                [this]( winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Printing::GetPreviewPageEventArgs const& e )
                {
                    const uint32_t index = e.PageNumber() - 1; // 1-based from system
                    auto managedImg = RenderPdfPageToImage( m_pdf, index, /*dpi*/ 300.0 );
                    if( managedImg.image )
                    {
                        // Store the managed image to keep stream alive
                        m_previewImages[index] = std::move(managedImg);
                        m_printDoc.SetPreviewPage( e.PageNumber(), m_previewImages[index].image );
                    }
                } );

        m_addPagesToken = m_printDoc.AddPages(
                [this]( winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::Printing::AddPagesEventArgs const& e )
                {
                    for( uint32_t i = 0; i < m_pageCount; ++i )
                    {
                        auto managedImg = RenderPdfPageToImage( m_pdf, i, /*dpi*/ 600.0 );
                        if( managedImg.image )
                        {
                            // Store the managed image to keep stream alive
                            m_printImages[i] = std::move(managedImg);
                            m_printDoc.AddPage( m_printImages[i].image );
                        }
                    }
                    m_printDoc.AddPagesComplete();
                } );

        try
        {
            auto factory = winrt::get_activation_factory<winrt::Windows::Graphics::Printing::PrintManager>();
            auto pmInterop = factory.as<IPrintManagerInterop>();

            winrt::Windows::Graphics::Printing::PrintManager printManager{ nullptr };

            if( FAILED( pmInterop->GetForWindow( m_hwnd,
                                                 winrt::guid_of<winrt::Windows::Graphics::Printing::PrintManager>(),
                                                 winrt::put_abi( printManager ) ) ) )
            {
                wxLogTrace( PRINTING_TRACE, "Failed to get PrintManager for window" );
                return PRINT_RESULT::FAILED_TO_PRINT;
            }

            // Now we have the WinRT PrintManager directly
            m_rtPM = printManager;
            m_taskRequestedToken = m_rtPM.PrintTaskRequested(
                    [this]( winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Graphics::Printing::PrintTaskRequestedEventArgs const& e )
                    {
                        auto task = e.Request().CreatePrintTask( L"KiCad PDF Print",
                                [this]( winrt::Windows::Graphics::Printing::PrintTaskSourceRequestedArgs const& sourceRequestedArgs )
                                {
                                    // Supply document source for preview
                                    sourceRequestedArgs.SetSource( m_docSrc );
                                } );
                    } );

            winrt::Windows::Foundation::IAsyncOperation<bool> asyncOp{ nullptr };

            // Immediately wait for results to keep this in thread
            if( FAILED( pmInterop->ShowPrintUIForWindowAsync( m_hwnd, winrt::put_abi(asyncOp) ) ) )
            {
                wxLogTrace( PRINTING_TRACE, "Failed to show print UI for window" );
                return PRINT_RESULT::FAILED_TO_PRINT;
            }

            bool shown = false;

            try
            {
                shown = asyncOp.GetResults();
            }
            catch( std::exception& e )
            {
                wxLogTrace( PRINTING_TRACE, "GetResults threw an exception for the print window: %s", e.what() );
                return PRINT_RESULT::FAILED_TO_PRINT;
            }

            return shown ? PRINT_RESULT::OK : PRINT_RESULT::CANCELLED;
        }
        catch( std::exception& e )
        {
            wxLogTrace( PRINTING_TRACE, "Exception caught in print operation: %s", e.what() );
            return PRINT_RESULT::FAILED_TO_PRINT;
        }
    }

private:
    void cleanup()
    {
        // Clear image containers first to release streams
        m_previewImages.clear();
        m_printImages.clear();

        if( m_rtPM )
        {
            m_rtPM.PrintTaskRequested( m_taskRequestedToken );
            m_rtPM = nullptr;
        }

        if( m_printDoc )
        {
            m_printDoc.AddPages( m_addPagesToken );
            m_printDoc.GetPreviewPage( m_getPreviewToken );
            m_printDoc.Paginate( m_paginateToken );
        }

        m_docSrc = nullptr;
        m_printDoc = nullptr;
        m_root = nullptr;

        if( m_host )
        {
            ::DestroyWindow( m_host );
            m_host = nullptr;
        }

        m_xamlSource = nullptr;
    }

private:
    HWND        m_hwnd{};
    winrt::Windows::Data::Pdf::PdfDocument m_pdf{ nullptr };

    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };
    winrt::Windows::UI::Xaml::Controls::Grid                   m_root{ nullptr };
    winrt::Windows::UI::Xaml::Printing::PrintDocument          m_printDoc{ nullptr };
    winrt::Windows::Graphics::Printing::IPrintDocumentSource   m_docSrc{ nullptr };

    uint32_t                                         m_pageCount{ 0 };
    winrt::Windows::Graphics::Printing::PrintManager m_rtPM{ nullptr };
    winrt::event_token                               m_taskRequestedToken{};

    winrt::event_token m_paginateToken{};
    winrt::event_token m_getPreviewToken{};
    winrt::event_token m_addPagesToken{};

    HWND m_host{ nullptr };

    // Store managed images to keep streams alive
    std::map<uint32_t, ManagedImage> m_previewImages;
    std::map<uint32_t, ManagedImage> m_printImages;
};


static std::wstring Utf8ToWide( std::string const& s )
{
    if( s.empty() ) return {};

    int          len = MultiByteToWideChar( CP_UTF8, 0, s.data(), (int) s.size(), nullptr, 0 );
    std::wstring out( len, L'\0' );

    MultiByteToWideChar( CP_UTF8, 0, s.data(), (int) s.size(), out.data(), len );
    return out;
}

PRINT_RESULT PrintPDF(std::string const& aFile )
{
    // Validate path
    DWORD attrs = GetFileAttributesA( aFile.c_str() );

    if( attrs == INVALID_FILE_ATTRIBUTES )
        return PRINT_RESULT::FILE_NOT_FOUND;

    // Load PDF via Windows.Data.Pdf
    winrt::Windows::Data::Pdf::PdfDocument pdf{ nullptr };

    try
    {
        auto path = Utf8ToWide( aFile );
        auto file = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync( winrt::hstring( path ) ).get();
        pdf = winrt::Windows::Data::Pdf::PdfDocument::LoadFromFileAsync( file ).get();
    }
    catch( ... )
    {
        return PRINT_RESULT::FAILED_TO_LOAD;
    }

    if( !pdf || pdf.PageCount() == 0 ) return PRINT_RESULT::FAILED_TO_LOAD;

    HWND hwndOwner = ::GetActiveWindow();
    if( !hwndOwner ) hwndOwner = ::GetForegroundWindow();
    if( !hwndOwner ) return PRINT_RESULT::FAILED_TO_PRINT;

    try
    {
        WIN_PDF_PRINTER printer( hwndOwner, pdf );
        return printer.Run();
    }
    catch( ... )
    {
        return PRINT_RESULT::FAILED_TO_PRINT;
    }
}

} // namespace PRINTING
} // namespace KIPLATFORM

#else

namespace KIPLATFORM
{
namespace PRINTING
{
    PRINT_RESULT PrintPDF( std::string const& )
    {
        return PRINT_RESULT::UNSUPPORTED;
    }
} // namespace PRINTING
} // namespace KIPLATFORM

#endif