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
#include <printing.h>

#include <poppler/glib/poppler.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <algorithm>
#include <memory>

#include <wx/filename.h>
#include <wx/filesys.h>
#include <wx/print.h>
#include <wx/cmndata.h>
#include <wx/regex.h>
#include <wx/toplevel.h>
#include <wx/log.h>

#if wxUSE_GTKPRINT
#include <wx/gtk/print.h>
#endif

namespace
{
struct PrintData
{
    PopplerDocument* doc;
    bool             fit_to_page;

    PrintData( PopplerDocument* d ) :
            doc( d ),
            fit_to_page( true )
    {
    }
};

void draw_page( GtkPrintOperation* operation, GtkPrintContext* context, gint page_nr, gpointer user_data )
{
    PrintData* print_data = static_cast<PrintData*>( user_data );

    if( !print_data || !print_data->doc )
        return;

    PopplerPage* page = poppler_document_get_page( print_data->doc, page_nr );

    if( !page ) return;

    auto cleanup_page = std::unique_ptr<PopplerPage, decltype( &g_object_unref )>( page, &g_object_unref );

    cairo_t* cr = gtk_print_context_get_cairo_context( context );

    if( !cr ) return;

    // Get page dimensions
    double page_width, page_height;
    poppler_page_get_size( page, &page_width, &page_height );

    // Get print context dimensions
    double print_width = gtk_print_context_get_width( context );
    double print_height = gtk_print_context_get_height( context );

    cairo_save( cr );
    auto cleanup_cairo = std::unique_ptr<cairo_t, decltype( &cairo_restore )>( cr, &cairo_restore );

    if( print_data->fit_to_page )
    {
        // Calculate scaling to fit page while maintaining aspect ratio
        double scale_x = print_width / page_width;
        double scale_y = print_height / page_height;
        double scale = std::min( scale_x, scale_y );

        // Center the page
        double scaled_width = page_width * scale;
        double scaled_height = page_height * scale;
        double offset_x = ( print_width - scaled_width ) / 2.0;
        double offset_y = ( print_height - scaled_height ) / 2.0;

        // Apply transformations
        cairo_translate( cr, offset_x, offset_y );
        cairo_scale( cr, scale, scale );
    }

    // Set white background
    cairo_set_source_rgb( cr, 1.0, 1.0, 1.0 );
    cairo_paint( cr );

    // Render the page
    poppler_page_render( page, cr );
}

void begin_print_callback( GtkPrintOperation* operation, GtkPrintContext* context, gpointer user_data )
{
    PrintData* print_data = static_cast<PrintData*>( user_data );
    if( !print_data || !print_data->doc )
    {
        gtk_print_operation_cancel( operation );
        return;
    }

    int num_pages = poppler_document_get_n_pages( print_data->doc );
    if( num_pages <= 0 )
    {
        gtk_print_operation_cancel( operation );
        return;
    }

    gtk_print_operation_set_n_pages( operation, num_pages );
}

void request_page_setup_callback( GtkPrintOperation* operation, GtkPrintContext* context, gint page_nr,
                                  GtkPageSetup* setup, gpointer user_data )
{
    PrintData* print_data = static_cast<PrintData*>( user_data );

    if( !print_data || !print_data->doc )
        return;

    PopplerPage* page = poppler_document_get_page( print_data->doc, page_nr );

    if( !page )
        return;

    // Get page dimensions to determine orientation
    double page_width, page_height;
    poppler_page_get_size( page, &page_width, &page_height );

    // Set orientation based on page dimensions
    GtkPageOrientation orientation =
            ( page_width > page_height ) ? GTK_PAGE_ORIENTATION_LANDSCAPE : GTK_PAGE_ORIENTATION_PORTRAIT;
    gtk_page_setup_set_orientation( setup, orientation );

    g_object_unref( page );
}


// GTK's scratch files use the g_mkstemp() templates "gtkprintXXXXXX" and "gtkprint_XXXXXX", so match
// the whole template to keep a destination the user named gtkprint-something
bool isSpoolName( const wxString& aFileName )
{
    static wxRegEx spoolTemplate( wxS( "^gtkprint_?[A-Z0-9]{6}$" ) );

    return spoolTemplate.Matches( aFileName );
}


// Settings from the last accepted dialog, so the next print reopens on the same printer and paper
GtkPrintSettings* s_lastSettings = nullptr;


// Keep the print-to-file folder but not the file, so the next document is offered under its own name
void rememberSettings( GtkPrintSettings* aSettings )
{
    GtkPrintSettings* settings = gtk_print_settings_copy( aSettings );
    const gchar*      uri = gtk_print_settings_get( settings, GTK_PRINT_SETTINGS_OUTPUT_URI );

    if( uri )
    {
        GFile* file = g_file_new_for_uri( uri );
        GFile* dir = g_file_get_parent( file );
        gchar* name = g_file_get_basename( file );

        // The file backend joins output-dir to the basename as a local path, so a URI there breaks it
        gchar* dirPath = dir ? g_file_get_path( dir ) : nullptr;

        if( dirPath && name && !isSpoolName( wxFileName( wxString::FromUTF8( name ) ).GetName() ) )
            gtk_print_settings_set( settings, GTK_PRINT_SETTINGS_OUTPUT_DIR, dirPath );

        g_free( dirPath );

        g_free( name );

        if( dir )
            g_object_unref( dir );

        g_object_unref( file );
        gtk_print_settings_unset( settings, GTK_PRINT_SETTINGS_OUTPUT_URI );
    }

    // A page selection only fits the document it was made for, so the next print starts from all pages
    gtk_print_settings_set_print_pages( settings, GTK_PRINT_PAGES_ALL );
    gtk_print_settings_unset( settings, GTK_PRINT_SETTINGS_PAGE_RANGES );

    if( s_lastSettings )
        g_object_unref( s_lastSettings );

    s_lastSettings = settings;
}


GtkWindow* activeToplevel()
{
    wxWindow* active = wxGetTopLevelParent( wxGetActiveWindow() );

    if( !active || !active->GetHandle() )
        return nullptr;

    GtkWidget* toplevel = gtk_widget_get_toplevel( active->GetHandle() );

    return gtk_widget_is_toplevel( toplevel ) ? GTK_WINDOW( toplevel ) : nullptr;
}
} // namespace

namespace KIPLATFORM
{
namespace PRINTING
{
    PRINT_RESULT PrintPDF( const std::string& aFile, bool fit_to_page, int aCurrentPage )
    {
        // Check file accessibility
        if( access( aFile.c_str(), R_OK ) != 0 )
            return PRINT_RESULT::FILE_NOT_FOUND;

        // Create file URI
        gchar* uri = g_filename_to_uri( aFile.c_str(), NULL, NULL );
        if( !uri )
            return PRINT_RESULT::FILE_NOT_FOUND;

        // Load the PDF document
        GError*          error = NULL;
        PopplerDocument* doc = poppler_document_new_from_file( uri, NULL, &error );
        g_free( uri );

        if( error )
        {
            wxLogTrace( PRINTING_TRACE, "Poppler could not load %s: %s", aFile, error->message );
            g_error_free( error );
            return PRINT_RESULT::FAILED_TO_LOAD;
        }

        if( !doc )
            return PRINT_RESULT::FAILED_TO_LOAD;

        auto cleanup_doc = std::unique_ptr<PopplerDocument, decltype(&g_object_unref)>(doc, &g_object_unref);

        // Check if document has pages
        int num_pages = poppler_document_get_n_pages( doc );

        if( num_pages <= 0 )
            return PRINT_RESULT::FAILED_TO_LOAD;

        // Create print data
        PrintData print_data( doc );
        print_data.fit_to_page = fit_to_page;

        // Create print operation
        GtkPrintOperation* op = gtk_print_operation_new();

        if( !op )
            return PRINT_RESULT::FAILED_TO_PRINT;

        auto cleanup_op = std::unique_ptr<GtkPrintOperation, decltype( &g_object_unref )>( op, &g_object_unref );

        // Set up print operation properties
        gtk_print_operation_set_use_full_page( op, FALSE );
        gtk_print_operation_set_unit( op, GTK_UNIT_POINTS );

        // GTK names both the spooled job and the print-to-file output after these
        std::string       jobName = wxFileName( wxString::FromUTF8( aFile ) ).GetName().ToStdString( wxConvUTF8 );
        GtkPrintSettings* settings =
                s_lastSettings ? gtk_print_settings_copy( s_lastSettings ) : gtk_print_settings_new();

        gtk_print_settings_set( settings, GTK_PRINT_SETTINGS_OUTPUT_BASENAME, jobName.c_str() );
        gtk_print_operation_set_print_settings( op, settings );
        g_object_unref( settings );
        gtk_print_operation_set_job_name( op, jobName.c_str() );

        if( aCurrentPage > 0 && aCurrentPage <= num_pages )
            gtk_print_operation_set_current_page( op, aCurrentPage - 1 );

        // Connect callbacks
        g_signal_connect( op, "begin-print", G_CALLBACK( begin_print_callback ), &print_data );
        g_signal_connect( op, "draw-page", G_CALLBACK( draw_page ), &print_data );
        g_signal_connect( op, "request-page-setup", G_CALLBACK( request_page_setup_callback ), &print_data );

        // Run the print operation
        error = NULL;
        GtkPrintOperationResult result =
                gtk_print_operation_run( op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, activeToplevel(), &error );

        // Handle errors and determine result
        PRINT_RESULT return_result;

        if( error )
        {
            wxLogTrace( PRINTING_TRACE, "GTK print failed: %s", error->message );
            g_error_free( error );
            return_result = PRINT_RESULT::FAILED_TO_PRINT;
        }
        else
        {
            switch( result )
            {
            case GTK_PRINT_OPERATION_RESULT_APPLY:
                rememberSettings( gtk_print_operation_get_print_settings( op ) );
                return_result = PRINT_RESULT::OK;
                break;
            case GTK_PRINT_OPERATION_RESULT_CANCEL: return_result = PRINT_RESULT::CANCELLED; break;
            case GTK_PRINT_OPERATION_RESULT_ERROR:
            default: return_result = PRINT_RESULT::FAILED_TO_PRINT; break;
            }
        }

        return return_result;
    }

    PRINT_RESULT PrintPDF( const std::string& aFile, int aCurrentPage )
    {
        return PrintPDF( aFile, true, aCurrentPage );
    }

    void ResetPrintToFilePath( wxPrintData& aData )
    {
        if( isSpoolName( wxFileName( aData.GetFilename() ).GetFullName() ) )
            aData.SetFilename( wxEmptyString );

#if wxUSE_GTKPRINT
        wxGtkPrintNativeData* nativeData = dynamic_cast<wxGtkPrintNativeData*>( aData.GetNativeData() );

        if( !nativeData || !nativeData->GetPrintConfig() )
            return;

        const gchar* uri = gtk_print_settings_get( nativeData->GetPrintConfig(),
                                                   GTK_PRINT_SETTINGS_OUTPUT_URI );

        if( !uri )
            return;

        wxFileName destination = wxFileSystem::URLToFileName( wxString::FromUTF8( uri ) );

        if( isSpoolName( destination.GetFullName() ) )
            gtk_print_settings_unset( nativeData->GetPrintConfig(), GTK_PRINT_SETTINGS_OUTPUT_URI );
#endif
    }

} // namespace PRINTING
} // namespace KIPLATFORM
