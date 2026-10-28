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
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef __MINGW32__
// wx pins NTDDI_VERSION to Vista when it includes windows.h first, which hides these interop headers
#include <windows.h>
#include <PrintManagerInterop.h>
#include <windows.ui.xaml.hosting.desktopwindowxamlsource.h>
#endif

#include <printing.h>

#include <wx/print.h>
#include <wx/cmndata.h>

#ifndef __MINGW32__
#include <algorithm>
#include <atomic>
#include <future>
#include <memory>
#include <vector>

#include <winrt/base.h>
#include <winrt/Windows.Data.Pdf.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Graphics.Printing.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Printing.h>

#include <wx/filename.h>
#include <wx/log.h>

namespace wdp = winrt::Windows::Data::Pdf;
namespace wf = winrt::Windows::Foundation;
namespace wgi = winrt::Windows::Graphics::Imaging;
namespace wgp = winrt::Windows::Graphics::Printing;
namespace wss = winrt::Windows::Storage::Streams;
namespace wux = winrt::Windows::UI::Xaml;

namespace
{
// Paper rasters are capped because every page stays decoded until the job has spooled
constexpr double PRINT_MAX_DPI = 300.0;
constexpr double PREVIEW_MAX_DPI = 150.0;


/**
 * Run @a aFunc on a fresh MTA thread and return its result, rethrowing anything it throws.
 *
 * Blocking on a WinRT async operation from the UI STA can deadlock because the completion may
 * need that same apartment, so every wait on PDF loading and rendering goes through here.
 */
template <typename FUNC>
auto runInMta( FUNC&& aFunc )
{
    return std::async( std::launch::async,
                       [&aFunc]()
                       {
                           winrt::init_apartment( winrt::apartment_type::multi_threaded );

                           struct UNINIT
                           {
                               ~UNINIT() { winrt::uninit_apartment(); }
                           } uninit;

                           return aFunc();
                       } )
            .get();
}


/**
 * Pump window messages until @a aDone returns true or the application is quitting.
 *
 * The print UI and the XAML print document call back into this thread, so it has to keep
 * dispatching while the job is in flight.
 */
template <typename PRED>
bool pumpUntil( PRED&& aDone )
{
    MSG msg;

    while( !aDone() )
    {
        // Completion events arrive on pool threads without a message, so poll as well
        ::MsgWaitForMultipleObjectsEx( 0, nullptr, 50, QS_ALLINPUT, MWMO_INPUTAVAILABLE );

        while( ::PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            if( msg.message == WM_QUIT )
            {
                ::PostQuitMessage( static_cast<int>( msg.wParam ) );
                return false;
            }

            ::TranslateMessage( &msg );
            ::DispatchMessageW( &msg );
        }
    }

    return true;
}


/**
 * Rasterise page @a aIndex to fit the imageable area of @a aPaper at no more than @a aMaxDpi.
 *
 * Pages whose orientation differs from the paper are turned a quarter turn so a landscape
 * sheet fills portrait paper instead of shrinking to half its size.
 */
wss::InMemoryRandomAccessStream renderPage( const wdp::PdfDocument& aPdf, uint32_t aIndex,
                                            const wgp::PrintPageDescription& aPaper, double aMaxDpi )
{
    wdp::PdfPage page = aPdf.GetPage( aIndex );
    wf::Size     src = page.Size();
    wf::Rect     area = aPaper.ImageableRect;
    bool         rotate = ( src.Width > src.Height ) != ( area.Width > area.Height );
    double       fitW = rotate ? src.Height : src.Width;
    double       fitH = rotate ? src.Width : src.Height;
    double       scale = std::min( area.Width / fitW, area.Height / fitH );
    double       dpi = aPaper.DpiX > 0 ? std::min<double>( aPaper.DpiX, aMaxDpi ) : aMaxDpi;
    double       pxPerDip = scale * dpi / 96.0;

    wdp::PdfPageRenderOptions opts;
    opts.DestinationWidth( static_cast<uint32_t>( std::max( 1.0, src.Width * pxPerDip ) ) );
    opts.DestinationHeight( static_cast<uint32_t>( std::max( 1.0, src.Height * pxPerDip ) ) );

    wss::InMemoryRandomAccessStream stream;
    page.RenderToStreamAsync( stream, opts ).get();

    if( !rotate )
        return stream;

    stream.Seek( 0 );
    wgi::BitmapDecoder              decoder = wgi::BitmapDecoder::CreateAsync( stream ).get();
    wss::InMemoryRandomAccessStream rotated;
    wgi::BitmapEncoder              encoder = wgi::BitmapEncoder::CreateForTranscodingAsync( rotated, decoder ).get();

    encoder.BitmapTransform().Rotation( wgi::BitmapRotation::Clockwise270Degrees );
    encoder.FlushAsync().get();
    return rotated;
}


/**
 * Wrap a rendered page in a XAML element laid out on the imageable area of @a aPaper.
 *
 * XAML objects belong to the UI thread, so this runs there after the MTA render.
 */
wux::UIElement makePageElement( const wss::InMemoryRandomAccessStream& aStream,
                                const wgp::PrintPageDescription&       aPaper )
{
    wux::Media::Imaging::BitmapImage bitmap;
    aStream.Seek( 0 );
    bitmap.SetSource( aStream );

    wf::Rect             area = aPaper.ImageableRect;
    wux::Controls::Image image;
    image.Source( bitmap );
    image.Stretch( wux::Media::Stretch::Uniform );
    image.Width( area.Width );
    image.Height( area.Height );
    image.Margin( wux::Thickness{ static_cast<double>( area.X ), static_cast<double>( area.Y ), 0.0, 0.0 } );
    image.HorizontalAlignment( wux::HorizontalAlignment::Left );
    image.VerticalAlignment( wux::VerticalAlignment::Top );

    wux::Controls::Grid root;
    root.Children().Append( image );
    return root;
}


// Shared with the print task's Completed handler, which can outlive the printer object
struct JOB_STATE
{
    std::atomic<bool>                     done{ false };
    std::atomic<wgp::PrintTaskCompletion> completion{ wgp::PrintTaskCompletion::Abandoned };
};


class WIN_PDF_PRINTER
{
public:
    WIN_PDF_PRINTER( HWND aOwner, const wdp::PdfDocument& aPdf, const winrt::hstring& aJobName, int aCurrentPage ) :
            m_owner( aOwner ),
            m_pdf( aPdf ),
            m_jobName( aJobName ),
            m_pageCount( aPdf.PageCount() ),
            m_currentPage( aCurrentPage ),
            m_job( std::make_shared<JOB_STATE>() )
    {
    }

    ~WIN_PDF_PRINTER() { cleanup(); }

    KIPLATFORM::PRINTING::PRINT_RESULT Run()
    {
        using KIPLATFORM::PRINTING::PRINT_RESULT;

        // XAML fail-fasts if restarted on a thread after its last island closed, so pin it until exit
        [[maybe_unused]] thread_local void* xamlCore =
                winrt::detach_abi( wux::Hosting::WindowsXamlManager::InitializeForCurrentThread() );

        // The print UI needs a XAML host in this process even though nothing is shown in it
        m_xamlSource = wux::Hosting::DesktopWindowXamlSource();

        m_host = ::CreateWindowExW( 0, L"STATIC", L"", WS_CHILD, 0, 0, 1, 1, m_owner, nullptr,
                                    ::GetModuleHandleW( nullptr ), nullptr );

        if( !m_host )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to create XAML host window" );
            return PRINT_RESULT::FAILED_TO_PRINT;
        }

        winrt::check_hresult( m_xamlSource.as<IDesktopWindowXamlSourceNative>()->AttachToWindow( m_host ) );
        m_xamlSource.Content( wux::Controls::Grid() );

        m_printDoc = wux::Printing::PrintDocument();
        m_docSource = m_printDoc.DocumentSource();

        m_paginateToken = m_printDoc.Paginate(
                [this]( const wf::IInspectable&, const wux::Printing::PaginateEventArgs& aArgs )
                {
                    // Paper or orientation changed, so every cached preview is stale
                    m_previewPaper = aArgs.PrintTaskOptions().GetPageDescription( 1 );
                    m_previewPages.clear();
                    m_printDoc.SetPreviewPageCount( m_pageCount, wux::Printing::PreviewPageCountType::Final );
                } );

        m_previewToken = m_printDoc.GetPreviewPage(
                [this]( const wf::IInspectable&, const wux::Printing::GetPreviewPageEventArgs& aArgs )
                {
                    onPreviewPage( aArgs.PageNumber() );
                } );

        m_addPagesToken = m_printDoc.AddPages(
                [this]( const wf::IInspectable&, const wux::Printing::AddPagesEventArgs& aArgs )
                {
                    onAddPages( aArgs.PrintTaskOptions() );
                } );

        auto factory = winrt::get_activation_factory<wgp::PrintManager, IPrintManagerInterop>();

        winrt::check_hresult( factory->GetForWindow( m_owner, winrt::guid_of<wgp::PrintManager>(),
                                                     winrt::put_abi( m_printManager ) ) );

        std::shared_ptr<JOB_STATE> job = m_job;
        wgp::IPrintDocumentSource  source = m_docSource;
        winrt::hstring             jobName = m_jobName;
        bool                       offerCurrent = m_currentPage > 0;

        m_taskRequestedToken = m_printManager.PrintTaskRequested(
                [job, source, jobName, offerCurrent]( const wgp::PrintManager&,
                                                      const wgp::PrintTaskRequestedEventArgs& aArgs )
                {
                    wgp::PrintTask task = aArgs.Request().CreatePrintTask(
                            jobName,
                            [source]( const wgp::PrintTaskSourceRequestedArgs& aSourceArgs )
                            {
                                aSourceArgs.SetSource( source );
                            } );

                    wgp::PrintPageRangeOptions ranges = task.Options().PageRangeOptions();
                    ranges.AllowAllPages( true );
                    ranges.AllowCurrentPage( offerCurrent );
                    ranges.AllowCustomSetOfPages( true );
                    task.Options().DisplayedOptions().Append( wgp::StandardPrintTaskOptions::CustomPageRanges() );

                    task.Completed(
                            [job]( const wgp::PrintTask&, const wgp::PrintTaskCompletedEventArgs& aDone )
                            {
                                job->completion = aDone.Completion();
                                job->done = true;
                            } );
                } );

        wf::IAsyncOperation<bool> showOp{ nullptr };

        winrt::check_hresult( factory->ShowPrintUIForWindowAsync( m_owner, winrt::guid_of<wf::IAsyncOperation<bool>>(),
                                                                  winrt::put_abi( showOp ) ) );

        if( !pumpUntil(
                    [&]()
                    {
                        return showOp.Status() != wf::AsyncStatus::Started;
                    } ) )
            return PRINT_RESULT::CANCELLED;

        if( showOp.Status() != wf::AsyncStatus::Completed || !showOp.GetResults() )
        {
            int32_t hr = showOp.Status() == wf::AsyncStatus::Error ? int32_t( showOp.ErrorCode() ) : 0;
            wxLogTrace( PRINTING_TRACE, "Print UI was not shown, status %d hr 0x%08x", int( showOp.Status() ), hr );
            return PRINT_RESULT::FAILED_TO_PRINT;
        }

        // The print UI is modeless, so keep the owner from starting a second job underneath it
        m_reenableOwner = !::EnableWindow( m_owner, FALSE );

        // The rendered pages and the PDF have to outlive spooling, which ends after the UI closes
        if( !pumpUntil(
                    [&]()
                    {
                        return job->done.load();
                    } ) )
            return PRINT_RESULT::CANCELLED;

        switch( job->completion.load() )
        {
        case wgp::PrintTaskCompletion::Submitted: return PRINT_RESULT::OK;
        case wgp::PrintTaskCompletion::Failed: return PRINT_RESULT::FAILED_TO_PRINT;
        default: return PRINT_RESULT::CANCELLED;
        }
    }

private:
    void onPreviewPage( int32_t aPageNumber )
    {
        uint32_t index = static_cast<uint32_t>( aPageNumber - 1 );

        try
        {
            if( m_previewPages.size() < m_pageCount )
                m_previewPages.resize( m_pageCount, nullptr );

            if( !m_previewPages[index] )
            {
                wss::InMemoryRandomAccessStream stream = runInMta(
                        [&]()
                        {
                            return renderPage( m_pdf, index, m_previewPaper, PREVIEW_MAX_DPI );
                        } );

                m_previewPages[index] = makePageElement( stream, m_previewPaper );
            }

            m_printDoc.SetPreviewPage( aPageNumber, m_previewPages[index] );
        }
        catch( const winrt::hresult_error& e )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to preview page %d: %s", aPageNumber, winrt::to_string( e.message() ) );
        }
    }

    void onAddPages( const wgp::PrintTaskOptions& aOptions )
    {
        try
        {
            std::vector<uint32_t>                  pages = selectedPages( aOptions );
            std::vector<wgp::PrintPageDescription> papers;

            for( uint32_t i = 0; i < pages.size(); ++i )
                papers.push_back( aOptions.GetPageDescription( i + 1 ) );

            std::vector<wss::InMemoryRandomAccessStream> streams = runInMta(
                    [&]()
                    {
                        std::vector<wss::InMemoryRandomAccessStream> rendered;

                        for( uint32_t i = 0; i < pages.size(); ++i )
                            rendered.push_back( renderPage( m_pdf, pages[i], papers[i], PRINT_MAX_DPI ) );

                        return rendered;
                    } );

            for( uint32_t i = 0; i < pages.size(); ++i )
            {
                m_printPages.push_back( makePageElement( streams[i], papers[i] ) );
                m_printDoc.AddPage( m_printPages.back() );
            }
        }
        catch( const winrt::hresult_error& e )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to render pages for printing: %s", winrt::to_string( e.message() ) );
        }

        m_printDoc.AddPagesComplete();
    }

    // Zero-based PDF pages to spool, from the page selection made in the print UI
    std::vector<uint32_t> selectedPages( const wgp::PrintTaskOptions& aOptions ) const
    {
        std::vector<uint32_t> pages;

        for( const wgp::PrintPageRange& range : aOptions.CustomPageRanges() )
        {
            wxLogTrace( PRINTING_TRACE, "Selected page range %d-%d", range.FirstPageNumber(), range.LastPageNumber() );

            for( int32_t page = range.FirstPageNumber(); page <= range.LastPageNumber(); ++page )
            {
                if( page >= 1 && static_cast<uint32_t>( page ) <= m_pageCount )
                    pages.push_back( static_cast<uint32_t>( page - 1 ) );
            }
        }

        if( pages.empty() )
        {
            for( uint32_t i = 0; i < m_pageCount; ++i )
                pages.push_back( i );
        }

        return pages;
    }

    void cleanup()
    {
        if( m_reenableOwner )
        {
            ::EnableWindow( m_owner, TRUE );
            m_reenableOwner = false;
        }

        if( m_printManager )
            m_printManager.PrintTaskRequested( m_taskRequestedToken );

        if( m_printDoc )
        {
            m_printDoc.AddPages( m_addPagesToken );
            m_printDoc.GetPreviewPage( m_previewToken );
            m_printDoc.Paginate( m_paginateToken );
        }

        m_previewPages.clear();
        m_printPages.clear();
        m_printManager = nullptr;
        m_docSource = nullptr;
        m_printDoc = nullptr;

        if( m_xamlSource )
        {
            m_xamlSource.Close();
            m_xamlSource = nullptr;
        }

        if( m_host )
        {
            ::DestroyWindow( m_host );
            m_host = nullptr;
        }
    }

    HWND                       m_owner;
    wdp::PdfDocument           m_pdf;
    winrt::hstring             m_jobName;
    uint32_t                   m_pageCount;
    int                        m_currentPage;
    std::shared_ptr<JOB_STATE> m_job;

    HWND                                  m_host = nullptr;
    bool                                  m_reenableOwner = false;
    wux::Hosting::DesktopWindowXamlSource m_xamlSource{ nullptr };
    wux::Printing::PrintDocument          m_printDoc{ nullptr };
    wgp::IPrintDocumentSource             m_docSource{ nullptr };
    wgp::PrintManager                     m_printManager{ nullptr };
    wgp::PrintPageDescription             m_previewPaper{};
    std::vector<wux::UIElement>           m_previewPages;
    std::vector<wux::UIElement>           m_printPages;

    winrt::event_token m_taskRequestedToken{};
    winrt::event_token m_paginateToken{};
    winrt::event_token m_previewToken{};
    winrt::event_token m_addPagesToken{};
};
} // namespace


namespace KIPLATFORM
{
namespace PRINTING
{
    PRINT_RESULT PrintPDF( const std::string& aFile, int aCurrentPage )
    {
        wxFileName     fn( wxString::FromUTF8( aFile ) );
        winrt::hstring path( fn.GetFullPath().ToStdWstring() );

        if( !fn.FileExists() )
            return PRINT_RESULT::FILE_NOT_FOUND;

        wdp::PdfDocument pdf{ nullptr };

        try
        {
            pdf = runInMta(
                    [&]()
                    {
                        auto file = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync( path ).get();
                        return wdp::PdfDocument::LoadFromFileAsync( file ).get();
                    } );
        }
        catch( const winrt::hresult_error& e )
        {
            wxLogTrace( PRINTING_TRACE, "Failed to load %s: %s", aFile, winrt::to_string( e.message() ) );
            return PRINT_RESULT::FAILED_TO_LOAD;
        }

        if( !pdf || pdf.PageCount() == 0 )
            return PRINT_RESULT::FAILED_TO_LOAD;

        HWND owner = ::GetActiveWindow();

        if( !owner )
            owner = ::GetForegroundWindow();

        if( !owner )
            return PRINT_RESULT::FAILED_TO_PRINT;

        PRINT_RESULT result = PRINT_RESULT::FAILED_TO_PRINT;

        try
        {
            WIN_PDF_PRINTER printer( owner, pdf, winrt::hstring( fn.GetName().ToStdWstring() ), aCurrentPage );
            result = printer.Run();
        }
        catch( const winrt::hresult_error& e )
        {
            wxLogTrace( PRINTING_TRACE, "Print failed: %s", winrt::to_string( e.message() ) );
        }
        catch( const std::exception& e )
        {
            wxLogTrace( PRINTING_TRACE, "Print failed: %s", e.what() );
        }

        return result;
    }
} // namespace PRINTING
} // namespace KIPLATFORM

#else

namespace KIPLATFORM
{
namespace PRINTING
{
    PRINT_RESULT PrintPDF( std::string const&, int )
    {
        return PRINT_RESULT::UNSUPPORTED;
    }
} // namespace PRINTING
} // namespace KIPLATFORM

#endif

namespace KIPLATFORM
{
namespace PRINTING
{
    // Windows spools through the driver rather than a scratch file, so the wx-level filename
    // only ever holds the destination the user asked for
    void ResetPrintToFilePath( wxPrintData& )
    {
    }
} // namespace PRINTING
} // namespace KIPLATFORM
