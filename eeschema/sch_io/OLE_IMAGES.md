---
read_when: Changing embedded metafile rendering or adding an importer that uses OLE images.
---

# Embedded metafile images

`OleRenderEmf` converts an EMF buffer through the statically linked libemf2svg
converter, then rasterizes its SVG with KiCad's NanoSVG. NanoSVG outlines text
with FreeType and HarfBuzz. The converter and parser share a private Fontconfig
configuration containing bundled fonts; Calibri uses the metric-compatible
Carlito family. Missing font files are traced and skipped. The configuration is
built per conversion so a missing resource can recover on the next attempt.
It does not change the application's font configuration.

The adapter validates record framing and limits input and pixel allocations.
It returns false on failed conversion or an unsupported embedded raster so the
caller can try its WMF or native-image preview. SVG clipping is not implemented
by NanoSVG. A successful render does not guarantee support for every EMF record.

Natural image dimensions are bounded by the requested size. A positive target
aspect ratio fits the caller's frame, including nonuniform scaling when needed.
Rendering composites the transparent SVG over white, as OLE previews expect.

The vendor README files record upstream revisions and KiCad patches. Keep
parser fixes in the libraries rather than adding another EMF record dispatcher
inside an importer. Test text spacing, glyph-index fonts, rotated baselines,
malformed records and fallback behavior when changing the conversion path.
