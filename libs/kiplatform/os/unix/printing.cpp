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

#include <poppler/glib/poppler.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <algorithm>
#include <memory>

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
} // namespace

namespace KIPLATFORM
{
namespace PRINTING
{
    PRINT_RESULT PrintPDF( const std::string& aFile, bool fit_to_page )
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

        // Connect callbacks
        g_signal_connect( op, "begin-print", G_CALLBACK( begin_print_callback ), &print_data );
        g_signal_connect( op, "draw-page", G_CALLBACK( draw_page ), &print_data );
        g_signal_connect( op, "request-page-setup", G_CALLBACK( request_page_setup_callback ), &print_data );

        // Run the print operation
        error = NULL;
        GtkPrintOperationResult result =
                gtk_print_operation_run( op, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, &error );

        // Handle errors and determine result
        PRINT_RESULT return_result;

        if( error )
        {
            g_error_free( error );
            return_result = PRINT_RESULT::FAILED_TO_PRINT;
        }
        else
        {
            switch( result )
            {
            case GTK_PRINT_OPERATION_RESULT_APPLY: return_result = PRINT_RESULT::OK; break;
            case GTK_PRINT_OPERATION_RESULT_CANCEL: return_result = PRINT_RESULT::CANCELLED; break;
            case GTK_PRINT_OPERATION_RESULT_ERROR:
            default: return_result = PRINT_RESULT::FAILED_TO_PRINT; break;
            }
        }

        return return_result;
    }

    PRINT_RESULT PrintPDF( const std::string& aFile )
    {
        return PrintPDF( aFile, true );
    }

} // namespace PRINTING
} // namespace KIPLATFORM