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

#import "kiql_thumbnail_provider.h"

#import <AppKit/AppKit.h>

#import "../kiql_import_queue.h"
#import "../kiql_model_scene.h"
#import "../kiql_preview_view.h"

#include <algorithm>
#include <string>

#include <plugins/3dapi/model_import.h>

@implementation KIQL_THUMBNAIL_PROVIDER

- (void)provideThumbnailForFileRequest:(QLFileThumbnailRequest*)aRequest
                     completionHandler:(void ( ^ )( QLThumbnailReply*, NSError* ))aHandler
{
    const bool  scoped = [aRequest.fileURL startAccessingSecurityScopedResource];
    std::string path( aRequest.fileURL.fileSystemRepresentation );

    const CGFloat pointSide = std::min( aRequest.maximumSize.width, aRequest.maximumSize.height );
    const CGFloat pixelSide = std::clamp( pointSide * aRequest.scale, CGFloat( 64 ), CGFloat( 2048 ) );
    NSURL*        url = aRequest.fileURL;

    KIQL_SharedImportQueue().Submit(
            std::move( path ), S3D::PreviewImportOptions(),
            [scoped, url, pointSide, pixelSide, aHandler]( std::uint64_t, S3D::MODEL_IMPORT_RESULT aResult )
            {
                // Runs on the import thread.  QLThumbnailProvider has no main-thread requirement,
                // so the render stays here and only the reply block is handed back.
                KIQL_PRESENTATION presentation;

                if( aResult && aResult.GetModel() )
                    presentation = KIQL_BuildScene( *aResult.GetModel() );

                if( scoped )
                    [url stopAccessingSecurityScopedResource];

                CGImageRef image = KIQL_CopyThumbnailImage( presentation, CGSizeMake( pixelSide, pixelSide ) );

                if( !image )
                {
                    aHandler( nil, [NSError errorWithDomain:NSCocoaErrorDomain
                                                       code:NSFileReadCorruptFileError
                                                   userInfo:nil] );
                    return;
                }

                // Hand the image to ARC before capturing it: Quick Look gives no guarantee that a
                // drawing block runs exactly once, so releasing inside the block would leak when it
                // is dropped and double-free when it is replayed.
                id retainedImage = CFBridgingRelease( image );

                const CGSize replySize = CGSizeMake( pointSide, pointSide );
                QLThumbnailReply* reply = [QLThumbnailReply replyWithContextSize:replySize
                                                                    drawingBlock:^BOOL( CGContextRef aContext )
                {
                    CGContextSetInterpolationQuality( aContext, kCGInterpolationHigh );
                    CGContextDrawImage( aContext, CGRectMake( 0, 0, replySize.width, replySize.height ),
                                        (__bridge CGImageRef) retainedImage );

                    return YES;
                }];

                aHandler( reply, nil );
            } );
}

@end
