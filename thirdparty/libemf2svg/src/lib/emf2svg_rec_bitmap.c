#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg_img_utils.h"
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include <png.h>
#include <stdio.h>
#include <stdlib.h>

void U_EMRALPHABLEND_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRALPHABLEND_print(contents, states);
    }

    PU_EMRALPHABLEND pEmr = (PU_EMRALPHABLEND)(contents);

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);

    float alpha = (float)pEmr->Blend.Global / 255.0;
    fprintf(out, " fill-opacity=\"%.4f\" ", alpha);
    clipset_draw(states, out);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc, false);
    fprintf(out, "/>\n");
}
void U_EMRBITBLT_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRBITBLT_print(contents, states);
    }
    PU_EMRBITBLT pEmr = (PU_EMRBITBLT)(contents);

    // if no bitmap, check for pattern brush
    // Should fill the output with the current brush and the raster operation
    if (pEmr->cbBitsSrc == 0) {
        char style[256];
        if (pEmr->dwRop == U_NOOP)
            return;
        if (states->currentDeviceContext.fill_mode == U_BS_MONOPATTERN) {
            sprintf(style, "fill:url(#img-%d-ref);",
                    states->currentDeviceContext.fill_idx);
        } else if (states->currentDeviceContext.fill_mode == U_BS_SOLID) {
            sprintf(style, "fill:#%02x%02x%02x",
                    states->currentDeviceContext.fill_red,
                    states->currentDeviceContext.fill_green,
                    states->currentDeviceContext.fill_blue);
        } else {
            style[0] = '\0';
        }
        if (style[0]) {
            POINT_D size =
                point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
            POINT_D position =
                point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
            fprintf(out, "<%spath style=\"%s", states->nameSpaceString, style);
            fprintf(
                out,
                "\" d=\"M %.4f,%.4f L %.4f,%.4f L %.4f,%.4f L %.4f,%.4f Z\" />",
                position.x, position.y, position.x + size.x, position.y,
                position.x + size.x, position.y + size.y, position.x,
                position.y + size.y);
        }
        // else
        // FIXME - non MONOBRUSH
        return;
    }

    // FIXME doesn't handle ternary raster operation

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);
    clipset_draw(states, out);

    // float alpha = (float)pEmr->Blend.Global / 255.0;
    // fprintf(out, " fill-opacity=\"%.4f\" ", alpha);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc, false);
    fprintf(out, "/>\n");
}
void U_EMRMASKBLT_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRMASKBLT_print(contents, states);
    }
    // PU_EMRMASKBLT pEmr = (PU_EMRMASKBLT) (contents);
}
void U_EMRPLGBLT_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRPLGBLT_print(contents, states);
    }
    // PU_EMRPLGBLT pEmr = (PU_EMRPLGBLT) (contents);
}
void U_EMRSETDIBITSTODEVICE_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSETDIBITSTODEVICE_print(contents, states);
    }
    // PU_EMRSETDIBITSTODEVICE pEmr = (PU_EMRSETDIBITSTODEVICE) (contents);
}
void U_EMRSTRETCHBLT_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSTRETCHBLT_print(contents, states);
    }
    PU_EMRSTRETCHBLT pEmr = (PU_EMRSTRETCHBLT)(contents);
    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);
    clipset_draw(states, out);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc, false);
    fprintf(out, "/>\n");
}
void U_EMRSTRETCHDIBITS_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSTRETCHDIBITS_print(contents, states);
    }
    PU_EMRSTRETCHDIBITS pEmr = (PU_EMRSTRETCHDIBITS)(contents);

    // check that the header is not outside of the emf file
    returnOutOfEmf(contents + pEmr->offBmiSrc);
    returnOutOfEmf(contents + pEmr->offBmiSrc + sizeof(U_BITMAPINFOHEADER));

    // get the header
    PU_BITMAPINFOHEADER BmiSrc =
        (PU_BITMAPINFOHEADER)(contents + pEmr->offBmiSrc);

    // check that the bitmap is not outside the emf file
    returnOutOfEmf(contents + pEmr->offBitsSrc);
    returnOutOfEmf(contents + pEmr->offBitsSrc + pEmr->cbBitsSrc);

    const unsigned char *BmpSrc =
        (const unsigned char *)(contents + pEmr->offBitsSrc);

    POINT_D size =
        point_cal(states, (double)pEmr->cDest.x, (double)pEmr->cDest.y);
    POINT_D position =
        point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);
    fprintf(out, "<image width=\"%.4f\" height=\"%.4f\" x=\"%.4f\" y=\"%.4f\" ",
            size.x, size.y, position.x, position.y);
    clipset_draw(states, out);

    dib_img_writer(contents, out, states, BmiSrc, BmpSrc,
                   (size_t)pEmr->cbBitsSrc, false);
    fprintf(out, "/>\n");
}
void U_EMRTRANSPARENTBLT_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRTRANSPARENTBLT_print(contents, states);
    }
}

void dib_img_writer(const char *contents, FILE *out, drawingStates *states,
                    PU_BITMAPINFOHEADER BmiSrc, const unsigned char *BmpSrc,
                    size_t size, bool assign_mono_colors_from_dc) {
    char *b64Bmp = NULL;
    size_t b64s;
    char *tmp = NULL;

    // Handle simple cases first, no treatment needed for them
    switch (BmiSrc->biCompression) {
    case U_BI_JPEG:
        b64Bmp = base64_encode(BmpSrc, size, &b64s);
        fprintf(out, "xlink:href=\"data:image/jpg;base64,");
        break;
    case U_BI_PNG:
        b64Bmp = base64_encode(BmpSrc, size, &b64s);
        fprintf(out, "xlink:href=\"data:image/png;base64,");
        break;
    }
    if (b64Bmp != NULL) {
        fprintf(out, "%s\" ", b64Bmp);
        free(b64Bmp);
        return;
    }

    // more complexe treatment, with conversion to png
    RGBBitmap convert_in;
    convert_in.size = size;
    convert_in.width = BmiSrc->biWidth;
    convert_in.height = BmiSrc->biHeight;
    convert_in.pixels = (RGBPixel *)BmpSrc;
    convert_in.bytewidth = BmiSrc->biWidth * 3;
    convert_in.bytes_per_pixel = 3;

    RGBBitmap convert_out;
    convert_out.pixels = NULL;
    const U_RGBQUAD *ct = NULL;
    U_RGBQUAD monoCt[2];
    uint32_t width, height, colortype, numCt, invert;
    char *rgba_px = NULL;
    int dibparams;
    char *in;
    size_t img_size;

    RGBABitmap convert_inpng;

    // In any cases after that, we get a png blob
    fprintf(out, "xlink:href=\"data:image/png;base64,");

    switch (BmiSrc->biCompression) {
    case U_BI_RLE8:
        convert_out = rle8ToRGB8(convert_in);
        break;
    case U_BI_RLE4:
        convert_out = rle4ToRGB(convert_in);
        break;
    }

    if (convert_out.pixels != NULL) {
        in = (char *)convert_out.pixels;
        img_size = convert_out.size;
    } else {
        in = (char *)convert_in.pixels;
        img_size = convert_in.size;
    }

    dibparams =
        e2s_get_DIB_params((PU_BITMAPINFO)BmiSrc, (const U_RGBQUAD **)&ct,
                           &numCt, &width, &height, &colortype, &invert);
    // if enable to read header, then exit
    if (dibparams || width > MAX_BMP_WIDTH || height > MAX_BMP_HEIGHT) {
        free(convert_out.pixels);
        states->Error = true;
        return;
    }
    // check that what we will read in the DIB_to_RGBA conversion is actually
    // there
    size_t offset_check =
        (size_t)((float)width * (float)height * get_pixel_size(colortype));
    if (((in + img_size) < in + offset_check)) {
        free(convert_out.pixels);
        states->Error = true;
        return;
    }
    if (colortype == U_BCBM_MONOCHROME) {
        if (assign_mono_colors_from_dc) {
            monoCt[0].Red = states->currentDeviceContext.text_red;
            monoCt[0].Green = states->currentDeviceContext.text_green;
            monoCt[0].Blue = states->currentDeviceContext.text_blue;
            monoCt[0].Reserved = 0xff;
            monoCt[1].Red = states->currentDeviceContext.bk_red;
            monoCt[1].Green = states->currentDeviceContext.bk_green;
            monoCt[1].Blue = states->currentDeviceContext.bk_blue;
            monoCt[1].Reserved =
                0xff; // states->currentDeviceContext.bk_mode ? 0xff : 0;
            ct = monoCt;
        }
    }
    DIB_to_RGBA(in, ct, numCt, &rgba_px, width, height, colortype, numCt,
                invert);

    if (rgba_px != NULL) {
        convert_inpng.size = width * 4 * height;
        convert_inpng.width = width;
        convert_inpng.height = height;
        convert_inpng.pixels = (RGBAPixel *)rgba_px;
        convert_inpng.bytewidth = BmiSrc->biWidth * 3;
        convert_inpng.bytes_per_pixel = 3;

        rgb2png(&convert_inpng, &b64Bmp, &b64s);
        tmp = (char *)b64Bmp;
        b64Bmp = base64_encode((unsigned char *)b64Bmp, b64s, &b64s);
        free(convert_out.pixels);
        free(tmp);
        free(rgba_px);
    }

    if (b64Bmp != NULL) {
        fprintf(out, "%s\" ", b64Bmp);
        free(b64Bmp);
    } else {
        // transparent 5x5 px png
        fprintf(out, "iVBORw0KGgoAAAANSUhEUgAAAAUAAAAFCAYAAACNbyblAAAABGdBTUEAA"
                     "LGPC/xhBQAAAAZiS0dEAP8A/wD/"
                     "oL2nkwAAAAlwSFlzAAALEwAACxMBAJqcGAAAAAd0SU1FB+"
                     "ABFREtOJX7FAkAAAAIdEVYdENvbW1lbnQA9syWvwAAAAxJREFUCNdjYKA"
                     "TAAAAaQABwB3y+AAAAABJRU5ErkJggg==\" ");
    }
}

// Find an image that matches (otherwise return NULL)
emfImageLibrary *image_library_find(emfImageLibrary *lib,
                                    PU_BITMAPINFOHEADER BmiSrc, size_t size) {
    while (lib) {
        if (memcmp(BmiSrc, lib->content, size) == 0)
            return lib;
        lib = lib->next;
    }
    return NULL;
}

// Create a new image
emfImageLibrary *image_library_create(int id, PU_BITMAPINFOHEADER BmiSrc,
                                      size_t size) {
    emfImageLibrary *image =
        (emfImageLibrary *)calloc(1, sizeof(emfImageLibrary) + size);
    image->id = id;
    image->content = (PU_BITMAPINFOHEADER)(image + 1);
    memcpy(image->content, BmiSrc, size);
    return image;
}

// Add an image to the states image 'library'
emfImageLibrary *image_library_add(drawingStates *states,
                                   PU_BITMAPINFOHEADER BmiSrc, size_t size) {
    ++states->count_images;
    emfImageLibrary *image =
        image_library_create(states->count_images, BmiSrc, size);
    if (states->library) {
        emfImageLibrary *last = states->library;
        while (last->next) {
            last = last->next;
        }
        last->next = image;
    } else {
        states->library = image;
    }
    return image;
}

// Release image library;
void freeEmfImageLibrary(drawingStates *states) {
    emfImageLibrary *last = states->library;
    while (last) {
        emfImageLibrary *next = last->next;
        free(last);
        last = next;
    }
}

// Lookup existing - or create and emit new image reference for use with image
// brush
emfImageLibrary *image_library_writer(const char *contents, FILE *out,
                                      drawingStates *states,
                                      PU_BITMAPINFOHEADER BmiSrc, size_t size,
                                      const unsigned char *BmpSrc) {
    emfImageLibrary *image = image_library_find(states->library, BmiSrc, size);
    if (!image) {
        image = image_library_add(states, BmiSrc, size);
        if (image) {
            const U_RGBQUAD *ct = NULL;
            uint32_t width = 0, height = 0, colortype, numCt, invert;
            e2s_get_DIB_params((PU_BITMAPINFO)BmiSrc, (const U_RGBQUAD **)&ct,
                               &numCt, &width, &height, &colortype, &invert);
            if (width > 0 && height > 0) {
                fprintf(out, "<%sdefs><%simage id=\"img-%d\" x=\"0\" y=\"0\" "
                             "width=\"%d\" height=\"%d\" ",
                        states->nameSpaceString, states->nameSpaceString,
                        image->id, width, height);
                dib_img_writer(contents, out, states, BmiSrc, BmpSrc, size,
                               true);
                fprintf(out, " preserveAspectRatio=\"none\" />");
                fprintf(out, "<%spattern id=\"img-%d-ref\" x=\"0\" y=\"0\" "
                             "width=\"%d\" height=\"%d\" "
                             "patternUnits=\"userSpaceOnUse\" >\n",
                        states->nameSpaceString, image->id, width, height);
                fprintf(out,
                        "<%suse id=\"img-%d-ign\" xlink:href=\"#img-%d\" />",
                        states->nameSpaceString, image->id, image->id);
                fprintf(out, "</%spattern></%sdefs>\n", states->nameSpaceString,
                        states->nameSpaceString);
            };
        }
    }
    return image;
}
#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
