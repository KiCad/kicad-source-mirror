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

#include "gerbview_jobs_handler.h"
#include "gerber_file_image.h"
#include "excellon_image.h"
#include "gerber_to_png.h"
#include "gerber_to_polyset.h"
#include "gerber_diff.h"
#include <jobs/job_gerber_info.h>
#include <jobs/job_gerber_export_png.h>
#include <jobs/job_gerber_diff.h>
#include <cli/exit_codes.h>
#include <reporter.h>
#include <base_units.h>
#include <json_common.h>
#include <wx/file.h>
#include <wx/filename.h>


GERBVIEW_JOBS_HANDLER::GERBVIEW_JOBS_HANDLER( KIWAY* aKiway ) :
        JOB_DISPATCHER( aKiway )
{
    Register( "gerber_info", std::bind( &GERBVIEW_JOBS_HANDLER::JobGerberInfo, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );

    Register( "gerber_export_png", std::bind( &GERBVIEW_JOBS_HANDLER::JobGerberExportPng, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );

    Register( "gerber_diff", std::bind( &GERBVIEW_JOBS_HANDLER::JobGerberDiff, this, std::placeholders::_1 ),
              []( JOB* job, wxWindow* aParent ) -> bool
              {
                  return true;
              } );
}


namespace
{

wxString GetFileTypeString( GERBER_FILE_IMAGE* aImage )
{
    if( dynamic_cast<EXCELLON_IMAGE*>( aImage ) )
        return wxS( "Excellon Drill" );

    return wxS( "Gerber" );
}

} // anonymous namespace


bool GERBVIEW_JOBS_HANDLER::checkStrictMode( const wxArrayString& aMessages, bool aStrict )
{
    if( aMessages.IsEmpty() )
        return true;

    for( const wxString& msg : aMessages )
    {
        if( m_reporter )
            m_reporter->Report( msg, aStrict ? RPT_SEVERITY_ERROR : RPT_SEVERITY_WARNING );
    }

    return !aStrict;
}


int GERBVIEW_JOBS_HANDLER::JobGerberInfo( JOB* aJob )
{
    JOB_GERBER_INFO* infoJob = dynamic_cast<JOB_GERBER_INFO*>( aJob );

    if( !infoJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    wxString      errorMsg;
    wxArrayString messages;
    auto          image = LoadGerberOrExcellon( infoJob->m_inputFile, &errorMsg, &messages );

    if( !image )
    {
        if( m_reporter )
            m_reporter->Report( errorMsg, RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( !checkStrictMode( messages, infoJob->m_strict ) )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    wxFileName fn( infoJob->m_inputFile );
    BOX2I      bbox = CalculateGerberBoundingBox( image.get() );

    // Convert from gerbview IU to mm (gerbview uses 10nm per IU, so IU_PER_MM = 1e5)
    double iuToMm = 1.0 / gerbIUScale.IU_PER_MM;

    // Conversion factors from mm to output units
    double   lengthScale = 1.0;
    double   areaScale = 1.0;
    wxString unitStr = wxS( "mm" );
    wxString areaUnitStr = wxS( "mm²" );

    switch( infoJob->m_units )
    {
    case JOB_GERBER_INFO::UNITS::MM:
        lengthScale = 1.0;
        areaScale = 1.0;
        unitStr = wxS( "mm" );
        areaUnitStr = wxS( "mm²" );
        break;

    case JOB_GERBER_INFO::UNITS::INCH:
        lengthScale = 1.0 / 25.4;
        areaScale = 1.0 / ( 25.4 * 25.4 );
        unitStr = wxS( "in" );
        areaUnitStr = wxS( "in²" );
        break;

    case JOB_GERBER_INFO::UNITS::MILS:
        lengthScale = 1000.0 / 25.4;
        areaScale = 1000000.0 / ( 25.4 * 25.4 );
        unitStr = wxS( "mils" );
        areaUnitStr = wxS( "mils²" );
        break;
    }

    double originX = static_cast<double>( bbox.GetOrigin().x ) * iuToMm * lengthScale;
    double originY = static_cast<double>( bbox.GetOrigin().y ) * iuToMm * lengthScale;
    double width = static_cast<double>( bbox.GetWidth() ) * iuToMm * lengthScale;
    double height = static_cast<double>( bbox.GetHeight() ) * iuToMm * lengthScale;

    int apertureCount = image->GetDcodesCount();

    if( infoJob->m_outputFormat == JOB_GERBER_INFO::OUTPUT_FORMAT::JSON )
    {
        nlohmann::json j;

        j["filename"] = fn.GetFullName().ToStdString();
        j["type"] = GetFileTypeString( image.get() ).ToStdString();
        j["item_count"] = image->GetItemsCount();
        j["units"] = unitStr.ToStdString();

        j["bounding_box"] = {
            { "origin_x", originX }, { "origin_y", originY }, { "width", width }, { "height", height }
        };

        j["aperture_count"] = apertureCount;

        if( infoJob->m_calculateArea )
        {
            double areaMm2 = image->CalculateCopperArea();
            j["copper_area"] = areaMm2 * areaScale;
        }

        if( m_reporter )
            m_reporter->Report( wxString::FromUTF8( j.dump( 2 ) ), RPT_SEVERITY_INFO );
    }
    else
    {
        wxString out;

        out += wxString::Format( wxS( "File: %s\n" ), fn.GetFullName() );
        out += wxString::Format( wxS( "Type: %s\n" ), GetFileTypeString( image.get() ) );
        out += wxString::Format( wxS( "Item count: %d\n" ), image->GetItemsCount() );
        out += wxS( "\n" );
        out += wxS( "Bounding box:\n" );
        out += wxString::Format( wxS( "  Origin: (%.3f, %.3f) %s\n" ), originX, originY, unitStr );
        out += wxString::Format( wxS( "  Size: %.3f x %.3f %s\n" ), width, height, unitStr );

        out += wxString::Format( wxS( "\nApertures defined: %d\n" ), apertureCount );

        if( infoJob->m_calculateArea )
        {
            double areaMm2 = image->CalculateCopperArea();
            out += wxString::Format( wxS( "Copper area: %.3f %s\n" ), areaMm2 * areaScale, areaUnitStr );
        }

        if( m_reporter )
            m_reporter->Report( out, RPT_SEVERITY_INFO );
    }

    return CLI::EXIT_CODES::OK;
}


int GERBVIEW_JOBS_HANDLER::JobGerberExportPng( JOB* aJob )
{
    JOB_GERBER_EXPORT_PNG* pngJob = dynamic_cast<JOB_GERBER_EXPORT_PNG*>( aJob );

    if( !pngJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    wxString      errorMsg;
    wxArrayString messages;

    if( !RenderGerberToPng( pngJob->m_inputFile, pngJob->GetConfiguredOutputPath(), *pngJob, &errorMsg, &messages ) )
    {
        if( m_reporter )
            m_reporter->Report( errorMsg, RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_UNKNOWN;
    }

    if( !checkStrictMode( messages, pngJob->m_strict ) )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    if( m_reporter )
    {
        m_reporter->Report( wxString::Format( wxS( "Exported PNG to: %s" ), pngJob->GetConfiguredOutputPath() ),
                            RPT_SEVERITY_INFO );
    }

    return CLI::EXIT_CODES::OK;
}


int GERBVIEW_JOBS_HANDLER::JobGerberDiff( JOB* aJob )
{
    JOB_GERBER_DIFF* diffJob = dynamic_cast<JOB_GERBER_DIFF*>( aJob );

    if( !diffJob )
        return CLI::EXIT_CODES::ERR_UNKNOWN;

    wxString      errorMsg;
    wxArrayString messagesA;
    wxArrayString messagesB;

    // Load both files
    auto imageA = LoadGerberOrExcellon( diffJob->m_inputFileA, &errorMsg, &messagesA );

    if( !imageA )
    {
        if( m_reporter )
            m_reporter->Report( errorMsg, RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( !checkStrictMode( messagesA, diffJob->m_strict ) )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    auto imageB = LoadGerberOrExcellon( diffJob->m_inputFileB, &errorMsg, &messagesB );

    if( !imageB )
    {
        if( m_reporter )
            m_reporter->Report( errorMsg, RPT_SEVERITY_ERROR );

        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    if( !checkStrictMode( messagesB, diffJob->m_strict ) )
        return CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE;

    // Convert to poly sets
    SHAPE_POLY_SET polyA = ConvertGerberToPolySet( imageA.get(), diffJob->m_tolerance );
    SHAPE_POLY_SET polyB = ConvertGerberToPolySet( imageB.get(), diffJob->m_tolerance );

    // Align bounding-box origins unless the caller explicitly opts out.
    // Skip alignment when testing for absolute-placement correctness (wrong origin,
    // unit conversion errors, etc.) that auto-alignment would silently hide.
    if( !diffJob->m_noAlign )
    {
        VECTOR2I alignOffset = CalculateAlignment( polyA, polyB );
        polyB.Move( alignOffset );
    }

    // Calculate differences
    GERBER_DIFF_RESULT result = CalculateGerberDiff( polyA, polyB );

    // 1e-9 mm² ≈ 1 µm², below which floating-point noise from boolean ops is expected
    constexpr double DIFF_AREA_EPSILON_MM2 = 1e-9;

    bool filesIdentical =
            ( result.additionsArea < DIFF_AREA_EPSILON_MM2 && result.removalsArea < DIFF_AREA_EPSILON_MM2 );

    if( diffJob->m_exitCodeOnly )
    {
        return filesIdentical ? CLI::EXIT_CODES::OK : CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
    }

    wxFileName fnA( diffJob->m_inputFileA );
    wxFileName fnB( diffJob->m_inputFileB );

    auto writeTextToOutput = [&]( const wxString& aText ) -> bool
    {
        wxString outputPath = diffJob->GetConfiguredOutputPath();

        if( !outputPath.IsEmpty() )
        {
            wxFile file( outputPath, wxFile::write );

            if( !file.IsOpened() )
            {
                if( m_reporter )
                {
                    m_reporter->Report( wxString::Format( wxS( "Failed to open output file: %s" ), outputPath ),
                                        RPT_SEVERITY_ERROR );
                }

                return false;
            }

            file.Write( aText );

            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( wxS( "Output written to: %s" ), outputPath ),
                                    RPT_SEVERITY_INFO );
            }
        }
        else if( m_reporter )
        {
            m_reporter->Report( aText, RPT_SEVERITY_INFO );
        }

        return true;
    };

    switch( diffJob->m_outputFormat )
    {
    case JOB_GERBER_DIFF::OUTPUT_FORMAT::TEXT:
    {
        wxString text = FormatDiffResultText( result, fnA.GetFullName(), fnB.GetFullName() );

        if( !writeTextToOutput( text ) )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        break;
    }

    case JOB_GERBER_DIFF::OUTPUT_FORMAT::JSON:
    {
        nlohmann::json j = FormatDiffResultJson( result, fnA.GetFullName(), fnB.GetFullName() );

        if( !writeTextToOutput( wxString::FromUTF8( j.dump( 2 ) ) ) )
            return CLI::EXIT_CODES::ERR_UNKNOWN;

        break;
    }

    case JOB_GERBER_DIFF::OUTPUT_FORMAT::PNG:
    {
        DIFF_RENDER_OPTIONS options;
        options.dpi = diffJob->m_dpi;
        options.antialias = diffJob->m_antialias;

        if( diffJob->m_transparentBackground )
            options.colorBackground = KIGFX::COLOR4D( 0.0, 0.0, 0.0, 0.0 );

        if( !RenderDiffToPng( result, diffJob->GetConfiguredOutputPath(), options ) )
        {
            if( m_reporter )
            {
                m_reporter->Report(
                        wxString::Format( wxS( "Failed to write PNG: %s" ), diffJob->GetConfiguredOutputPath() ),
                        RPT_SEVERITY_ERROR );
            }

            return CLI::EXIT_CODES::ERR_UNKNOWN;
        }

        if( m_reporter )
        {
            m_reporter->Report(
                    wxString::Format( wxS( "Diff PNG written to: %s" ), diffJob->GetConfiguredOutputPath() ),
                    RPT_SEVERITY_INFO );
        }

        break;
    }
    }

    return CLI::EXIT_CODES::OK;
}
