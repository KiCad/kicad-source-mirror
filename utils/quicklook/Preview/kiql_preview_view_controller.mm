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

#import "kiql_preview_view_controller.h"

#import "../kiql_import_queue.h"
#import "../kiql_model_scene.h"
#import "../kiql_preview_view.h"

#include <string>

#include <plugins/3dapi/model_import.h>

@implementation KIQL_PREVIEW_VIEW_CONTROLLER
{
    KIQL_PREVIEW_VIEW* m_previewView;
    std::uint64_t      m_generation;
    void ( ^m_pendingHandler )( NSError* );
}

- (void)loadView
{
    NSView* container = [[NSView alloc] initWithFrame:NSMakeRect( 0, 0, 800, 600 )];
    m_previewView = [[KIQL_PREVIEW_VIEW alloc] initWithFrame:container.bounds options:nil];
    m_previewView.translatesAutoresizingMaskIntoConstraints = NO;
    [container addSubview:m_previewView];

    [NSLayoutConstraint activateConstraints:@[
        [m_previewView.leadingAnchor constraintEqualToAnchor:container.leadingAnchor],
        [m_previewView.trailingAnchor constraintEqualToAnchor:container.trailingAnchor],
        [m_previewView.topAnchor constraintEqualToAnchor:container.topAnchor],
        [m_previewView.bottomAnchor constraintEqualToAnchor:container.bottomAnchor],
    ]];

    self.view = container;
}


- (void)preparePreviewOfFileAtURL:(NSURL*)aUrl completionHandler:(void ( ^ )( NSError* ))aHandler
{
    // Quick Look expects exactly one completion per request.  Submitting supersedes whatever was
    // running, which drops that request's completion, so finish it here before starting the next.
    [self finishPendingWithError:[NSError errorWithDomain:NSCocoaErrorDomain
                                                     code:NSUserCancelledError
                                                 userInfo:nil]];
    m_pendingHandler = [aHandler copy];

    [m_previewView displayPresentation:KIQL_PRESENTATION()];

    const bool scoped = [aUrl startAccessingSecurityScopedResource];
    std::string path( aUrl.fileSystemRepresentation );

    // Submitting supersedes whatever the previous file left running, so no separate cancel is
    // needed when Quick Look reuses this controller for another file.
    m_generation = KIQL_SharedImportQueue().Submit(
            std::move( path ), S3D::PreviewImportOptions(),
            [self, scoped, aUrl]( std::uint64_t aGeneration, S3D::MODEL_IMPORT_RESULT aResult )
            {
                // Runs on the import thread; SceneKit must be touched on the main thread only.
                KIQL_PRESENTATION presentation;

                if( aResult && aResult.GetModel() )
                    presentation = KIQL_BuildScene( *aResult.GetModel() );

                if( scoped )
                    [aUrl stopAccessingSecurityScopedResource];

                dispatch_async( dispatch_get_main_queue(), ^{
                    if( aGeneration != self->m_generation )
                        return;

                    if( presentation )
                    {
                        [self->m_previewView displayPresentation:presentation];
                        [self finishPendingWithError:nil];
                    }
                    else
                    {
                        [self->m_previewView displayPresentation:KIQL_PRESENTATION()];
                        [self finishPendingWithError:[NSError errorWithDomain:NSCocoaErrorDomain
                                                                         code:NSFileReadCorruptFileError
                                                                     userInfo:nil]];
                    }
                } );
            } );
}

- (void)finishPendingWithError:(NSError*)aError
{
    if( !m_pendingHandler )
        return;

    void ( ^handler )( NSError* ) = m_pendingHandler;
    m_pendingHandler = nil;
    handler( aError );
}


- (void)dealloc
{
    // A controller torn down mid-import must not leave Quick Look waiting.
    [self finishPendingWithError:[NSError errorWithDomain:NSCocoaErrorDomain
                                                     code:NSUserCancelledError
                                                 userInfo:nil]];
}

@end
