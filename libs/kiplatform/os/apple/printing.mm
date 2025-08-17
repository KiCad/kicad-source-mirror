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

#import <AppKit/AppKit.h>
#import <PDFKit/PDFKit.h>

namespace KIPLATFORM
{
namespace PRINTING
{

BOOL hasMultipleOrientations( PDFDocument* document )
{
    if( [document pageCount] == 0 )
        return NO;

    PDFPage* firstPage = [document pageAtIndex:0];
    NSRect firstBounds = [firstPage boundsForBox:kPDFDisplayBoxMediaBox];
    BOOL firstIsLandscape = firstBounds.size.width > firstBounds.size.height;

    for( NSUInteger i = 1; i < [document pageCount]; i++ )
    {
        PDFPage* page = [document pageAtIndex:i];
        NSRect bounds = [page boundsForBox:kPDFDisplayBoxMediaBox];
        BOOL isLandscape = bounds.size.width > bounds.size.height;

        if( isLandscape != firstIsLandscape )
            return YES;
    }

    return NO;
}

PRINT_RESULT PrintPDF( const std::string& aFile, bool fit_to_page)
{
    @autoreleasepool
    {
        NSString* path = [NSString stringWithUTF8String:aFile.c_str()];

        if( ![[NSFileManager defaultManager] isReadableFileAtPath:path] )
            return PRINT_RESULT::FILE_NOT_FOUND;

        NSURL* url = [NSURL fileURLWithPath:path];
        PDFDocument* document = [[PDFDocument alloc] initWithURL:url];

        if( !document || [document pageCount] == 0 )
        {
            [document release];
            return PRINT_RESULT::FAILED_TO_LOAD;
        }

        PDFView* pdfView = [[PDFView alloc] init];
        [pdfView setDocument:document];

        NSPrintInfo* printInfo = [[NSPrintInfo sharedPrintInfo] copy];

        BOOL hasMixed = hasMultipleOrientations( document );

        if( hasMixed )
        {
            [printInfo setOrientation:NSPrintingOrientationPortrait];
            [printInfo setHorizontallyCentered:YES];
            [printInfo setVerticallyCentered:YES];
        }

        if( fit_to_page )
        {
            NSMutableDictionary* settings = [[printInfo printSettings] mutableCopy];
            [settings setObject:@YES forKey:@"com.apple.print.PrintSettings.PMScaleToFit"];
            [printInfo setPrintSettings:settings];
            [settings release];
        }

        NSPrintOperation* op = [NSPrintOperation printOperationWithView:pdfView printInfo:printInfo];

        [printInfo release];

        if( !op )
        {
            [pdfView release];
            [document release];
            return PRINT_RESULT::FAILED_TO_PRINT;
        }

        [op setShowsPrintPanel:YES];
        [op setShowsProgressPanel:YES];

        BOOL success = [op runOperation];

        PRINT_RESULT result;

        if( success )
        {
            result = PRINT_RESULT::OK;
        } else
        {
            NSPrintInfo* info = [op printInfo];
            NSDictionary* settings = [info printSettings];

            if( [[settings objectForKey:NSPrintJobDisposition] isEqualToString:NSPrintCancelJob] )
            {
                result = PRINT_RESULT::CANCELLED;
            } else
            {
                result = PRINT_RESULT::FAILED_TO_PRINT;
            }
        }

        [pdfView release];
        [document release];
        return result;
    }
}

PRINT_RESULT PrintPDF(const std::string& aFile)
{
    return PrintPDF(aFile, true);
}

} // namespace PRINTING
} // namespace KIPLATFORM