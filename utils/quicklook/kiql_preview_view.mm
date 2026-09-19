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

#import "kiql_preview_view.h"

#import <AppKit/AppKit.h>
#import <Metal/Metal.h>


namespace
{
/// A thumbnail extension is asked for many files in a row, so the device is created once.
id<MTLDevice> sharedMetalDevice()
{
    static id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return device;
}
} // namespace


@implementation KIQL_PREVIEW_VIEW

- (void)displayPresentation:(const KIQL_PRESENTATION&)aPresentation
{
    if( !aPresentation )
    {
        self.scene = nil;
        self.pointOfView = nil;
        return;
    }

    self.scene = aPresentation.scene;
    self.allowsCameraControl = YES;
    self.backgroundColor = [NSColor clearColor];
    self.antialiasingMode = SCNAntialiasingModeMultisampling4X;

    self.pointOfView = aPresentation.camera;
    self.defaultCameraController.target = aPresentation.center;
}

@end


CGImageRef KIQL_CopyThumbnailImage( const KIQL_PRESENTATION& aPresentation, CGSize aPixelSize )
{
    id<MTLDevice> device = sharedMetalDevice();

    if( !device || !aPresentation )
        return NULL;

    // The renderer only needs the scene and its camera, so no view is created here.
    SCNRenderer* renderer = [SCNRenderer rendererWithDevice:device options:nil];
    renderer.scene = aPresentation.scene;
    renderer.pointOfView = aPresentation.camera;

    NSImage* image = [renderer snapshotAtTime:0
                                     withSize:aPixelSize
                             antialiasingMode:SCNAntialiasingModeMultisampling4X];

    if( !image )
        return NULL;

    NSRect     rect = NSMakeRect( 0, 0, image.size.width, image.size.height );
    CGImageRef cgImage = [image CGImageForProposedRect:&rect context:nil hints:nil];

    return cgImage ? CGImageRetain( cgImage ) : NULL;
}
