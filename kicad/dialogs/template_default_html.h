/*
 * KiCad Project Template HTML defaults
 * Extracted from dialog_template_selector.cpp to allow reuse and to provide
 * a styled fallback page when a template does not supply an info.html.
 */

#ifndef KICAD_TEMPLATE_DEFAULT_HTML_H
#define KICAD_TEMPLATE_DEFAULT_HTML_H

#include <wx/string.h>

// Uses _() translation macro which is defined globally in KiCad builds.

namespace
{

// Common CSS variables and base styles shared across all template HTML pages.
// Uses CSS custom properties for light/dark theme switching via the kicad-dark class.
inline wxString GetCommonStyles()
{
    return wxString(
        ":root {"
            "--bg-primary: #FFFFFF;"
            "--bg-secondary: #F3F3F3;"
            "--bg-elevated: #FFFFFF;"
            "--text-primary: #545454;"
            "--text-secondary: #848484;"
            "--accent: #1A81C4;"
            "--accent-subtle: rgba(26, 129, 196, 0.08);"
            "--border: #E0E0E0;"
            "--shadow: 0 1px 3px rgba(0,0,0,0.06), 0 1px 2px rgba(0,0,0,0.04);"
        "}"
        "body.kicad-dark {"
            "--bg-primary: #1E1E1E;"
            "--bg-secondary: #2D2D2D;"
            "--bg-elevated: #333333;"
            "--text-primary: #DED3DD;"
            "--text-secondary: #848484;"
            "--accent: #42B8EB;"
            "--accent-subtle: rgba(66, 184, 235, 0.1);"
            "--border: #404040;"
            "--shadow: 0 1px 3px rgba(0,0,0,0.2), 0 1px 2px rgba(0,0,0,0.15);"
        "}"
        "* { box-sizing: border-box; }"
        "body {"
            "font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;"
            "margin: 0;"
            "padding: 24px;"
            "background: var(--bg-primary);"
            "color: var(--text-primary);"
            "line-height: 1.5;"
        "}"
    );
}

// SVG icon for template/document
inline wxString GetTemplateIcon()
{
    return wxString(
        "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
            "stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
            "<path d=\"M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z\"/>"
            "<polyline points=\"14 2 14 8 20 8\"/>"
            "<line x1=\"16\" y1=\"13\" x2=\"8\" y2=\"13\"/>"
            "<line x1=\"16\" y1=\"17\" x2=\"8\" y2=\"17\"/>"
            "<polyline points=\"10 9 9 9 8 9\"/>"
        "</svg>"
    );
}

// SVG icon for settings/gear
inline wxString GetSettingsIcon()
{
    return wxString(
        "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
            "stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
            "<circle cx=\"12\" cy=\"12\" r=\"3\"/>"
            "<path d=\"M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06"
                "a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09"
                "A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83"
                "l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09"
                "A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0"
                "l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09"
                "a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83"
                "l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09"
                "a1.65 1.65 0 0 0-1.51 1z\"/>"
        "</svg>"
    );
}

// SVG icon for layers/stack
inline wxString GetLayersIcon()
{
    return wxString(
        "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
            "stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
            "<polygon points=\"12 2 2 7 12 12 22 7 12 2\"/>"
            "<polyline points=\"2 17 12 22 22 17\"/>"
            "<polyline points=\"2 12 12 17 22 12\"/>"
        "</svg>"
    );
}

// SVG icon for folder
inline wxString GetFolderIcon()
{
    return wxString(
        "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
            "stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
            "<path d=\"M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z\"/>"
        "</svg>"
    );
}

// SVG icon for info/lightbulb
inline wxString GetInfoIcon()
{
    return wxString(
        "<svg width=\"24\" height=\"24\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" "
            "stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
            "<circle cx=\"12\" cy=\"12\" r=\"10\"/>"
            "<line x1=\"12\" y1=\"16\" x2=\"12\" y2=\"12\"/>"
            "<line x1=\"12\" y1=\"8\" x2=\"12.01\" y2=\"8\"/>"
        "</svg>"
    );
}

} // anonymous namespace


// Welcome page shown when no template is selected.
inline wxString GetWelcomeHtml( bool aDarkMode )
{
    wxString bodyClass = aDarkMode ? wxS( "kicad-dark" ) : wxS( "" );

    return wxString(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>KiCad Project Template Selector</title>"
        "<style>" )
        + GetCommonStyles() +
        wxString(
        ".card {"
            "max-width: 600px;"
            "margin: 0 auto;"
            "background: var(--bg-elevated);"
            "border-radius: 8px;"
            "padding: 32px;"
            "box-shadow: var(--shadow);"
            "border: 1px solid var(--border);"
        "}"
        ".header {"
            "text-align: center;"
            "margin-bottom: 28px;"
        "}"
        ".title {"
            "font-size: 1.5rem;"
            "font-weight: 600;"
            "color: var(--text-primary);"
            "margin: 0 0 8px 0;"
        "}"
        ".subtitle {"
            "font-size: 0.95rem;"
            "color: var(--text-secondary);"
            "margin: 0;"
        "}"
        ".features {"
            "display: flex;"
            "flex-direction: column;"
            "gap: 16px;"
            "margin-bottom: 28px;"
        "}"
        ".feature {"
            "display: flex;"
            "align-items: flex-start;"
            "gap: 14px;"
            "padding: 14px;"
            "background: var(--bg-secondary);"
            "border-radius: 6px;"
        "}"
        ".feature-icon {"
            "flex-shrink: 0;"
            "width: 40px;"
            "height: 40px;"
            "display: flex;"
            "align-items: center;"
            "justify-content: center;"
            "background: var(--accent-subtle);"
            "border-radius: 8px;"
            "color: var(--accent);"
        "}"
        ".feature-content h3 {"
            "margin: 0 0 4px 0;"
            "font-size: 0.95rem;"
            "font-weight: 600;"
            "color: var(--text-primary);"
        "}"
        ".feature-content p {"
            "margin: 0;"
            "font-size: 0.875rem;"
            "color: var(--text-secondary);"
        "}"
        ".tip {"
            "display: flex;"
            "align-items: flex-start;"
            "gap: 10px;"
            "padding: 12px 14px;"
            "background: var(--accent-subtle);"
            "border-radius: 6px;"
            "border-left: 3px solid var(--accent);"
        "}"
        ".tip-icon {"
            "flex-shrink: 0;"
            "color: var(--accent);"
        "}"
        ".tip-text {"
            "font-size: 0.875rem;"
            "color: var(--text-secondary);"
            "margin: 0;"
        "}"
        "</style>"
        "</head>"
        "<body class=\"" ) + bodyClass + wxString( "\">"
        "<div class=\"card\">"
        "<div class=\"header\">"
        "<h1 class=\"title\">" ) + _( "Select a Template" ) + wxString( "</h1>"
        "<p class=\"subtitle\">" )
        + _( "Templates provide pre-configured project structures to jumpstart your design." )
        + wxString( "</p>"
        "</div>"
        "<div class=\"features\">"
        "<div class=\"feature\">"
        "<div class=\"feature-icon\">" ) + GetTemplateIcon() + wxString( "</div>"
        "<div class=\"feature-content\">"
        "<h3>" ) + _( "Pre-configured Libraries" ) + wxString( "</h3>"
        "<p>" ) + _( "Common symbols and footprints already linked and ready to use." ) + wxString( "</p>"
        "</div>"
        "</div>"
        "<div class=\"feature\">"
        "<div class=\"feature-icon\">" ) + GetSettingsIcon() + wxString( "</div>"
        "<div class=\"feature-content\">"
        "<h3>" ) + _( "Design Rules" ) + wxString( "</h3>"
        "<p>" ) + _( "Electrical and mechanical constraints configured for the intended application." ) + wxString( "</p>"
        "</div>"
        "</div>"
        "<div class=\"feature\">"
        "<div class=\"feature-icon\">" ) + GetLayersIcon() + wxString( "</div>"
        "<div class=\"feature-content\">"
        "<h3>" ) + _( "Board Stackups" ) + wxString( "</h3>"
        "<p>" ) + _( "Layer configurations optimized for common manufacturing processes." ) + wxString( "</p>"
        "</div>"
        "</div>"
        "</div>"
        "<div class=\"tip\">"
        "<div class=\"tip-icon\">" ) + GetInfoIcon() + wxString( "</div>"
        "<p class=\"tip-text\">" )
        + _( "Recently used templates appear at the top. Use the folder icon to browse custom template directories." )
        + wxString( "</p>"
        "</div>"
        "</div>"
        "</body>"
        "</html>"
    );
}


// Fallback when a specific template has no meta/info.html.
inline wxString GetTemplateInfoHtml( const wxString& aTemplateName, bool aDarkMode )
{
    wxString bodyClass = aDarkMode ? wxS( "kicad-dark" ) : wxS( "" );

    return wxString(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>" ) + aTemplateName + wxString( " - KiCad Template</title>"
        "<style>" )
        + GetCommonStyles() +
        wxString(
        ".card {"
            "max-width: 600px;"
            "margin: 0 auto;"
            "background: var(--bg-elevated);"
            "border-radius: 8px;"
            "padding: 28px;"
            "box-shadow: var(--shadow);"
            "border: 1px solid var(--border);"
        "}"
        ".header {"
            "display: flex;"
            "align-items: center;"
            "gap: 12px;"
            "margin-bottom: 20px;"
        "}"
        ".badge {"
            "background: var(--accent);"
            "color: #FFFFFF;"
            "padding: 4px 10px;"
            "border-radius: 4px;"
            "font-size: 0.75rem;"
            "font-weight: 600;"
            "letter-spacing: 0.03em;"
            "text-transform: uppercase;"
        "}"
        ".template-name {"
            "font-size: 1.25rem;"
            "font-weight: 600;"
            "color: var(--text-primary);"
            "margin: 0;"
        "}"
        ".description {"
            "color: var(--text-secondary);"
            "margin-bottom: 24px;"
        "}"
        ".steps {"
            "background: var(--bg-secondary);"
            "border-radius: 6px;"
            "padding: 20px;"
            "margin-bottom: 20px;"
        "}"
        ".steps-title {"
            "font-size: 0.875rem;"
            "font-weight: 600;"
            "color: var(--text-primary);"
            "margin: 0 0 16px 0;"
        "}"
        ".step {"
            "display: flex;"
            "gap: 12px;"
            "margin-bottom: 14px;"
        "}"
        ".step:last-child {"
            "margin-bottom: 0;"
        "}"
        ".step-number {"
            "flex-shrink: 0;"
            "width: 24px;"
            "height: 24px;"
            "display: flex;"
            "align-items: center;"
            "justify-content: center;"
            "background: var(--accent);"
            "color: #FFFFFF;"
            "border-radius: 50%;"
            "font-size: 0.75rem;"
            "font-weight: 600;"
        "}"
        ".step-content h4 {"
            "margin: 0 0 2px 0;"
            "font-size: 0.875rem;"
            "font-weight: 600;"
            "color: var(--text-primary);"
        "}"
        ".step-content p {"
            "margin: 0;"
            "font-size: 0.8125rem;"
            "color: var(--text-secondary);"
        "}"
        ".hint {"
            "display: flex;"
            "align-items: flex-start;"
            "gap: 10px;"
            "padding: 12px 14px;"
            "background: var(--accent-subtle);"
            "border-radius: 6px;"
            "border-left: 3px solid var(--accent);"
        "}"
        ".hint-icon {"
            "flex-shrink: 0;"
            "color: var(--accent);"
        "}"
        ".hint-text {"
            "font-size: 0.8125rem;"
            "color: var(--text-secondary);"
            "margin: 0;"
        "}"
        "code {"
            "background: var(--bg-secondary);"
            "padding: 2px 6px;"
            "border-radius: 3px;"
            "font-size: 0.8125rem;"
            "font-family: 'SF Mono', Monaco, Consolas, monospace;"
        "}"
        "</style>"
        "</head>"
        "<body class=\"" ) + bodyClass + wxString( "\">"
        "<div class=\"card\">"
        "<div class=\"header\">"
        "<span class=\"badge\">" ) + _( "Template" ) + wxString( "</span>"
        "<h1 class=\"template-name\">" ) + aTemplateName + wxString( "</h1>"
        "</div>"
        "<p class=\"description\">" )
        + _( "This template does not include a description. You can still use it to create a new project." )
        + wxString( "</p>"
        "<div class=\"steps\">"
        "<h3 class=\"steps-title\">" ) + _( "To use this template" ) + wxString( "</h3>"
        "<div class=\"step\">"
        "<span class=\"step-number\">1</span>"
        "<div class=\"step-content\">"
        "<h4>" ) + _( "Create the project" ) + wxString( "</h4>"
        "<p>" ) + _( "Click OK to create a new project folder with this template's contents." ) + wxString( "</p>"
        "</div>"
        "</div>"
        "<div class=\"step\">"
        "<span class=\"step-number\">2</span>"
        "<div class=\"step-content\">"
        "<h4>" ) + _( "Open schematic and PCB" ) + wxString( "</h4>"
        "<p>" ) + _( "Use the Project Manager to launch the Schematic and PCB editors." ) + wxString( "</p>"
        "</div>"
        "</div>"
        "<div class=\"step\">"
        "<span class=\"step-number\">3</span>"
        "<div class=\"step-content\">"
        "<h4>" ) + _( "Review settings" ) + wxString( "</h4>"
        "<p>" ) + _( "Verify libraries, design rules, and board stackup match your needs." ) + wxString( "</p>"
        "</div>"
        "</div>"
        "</div>"
        "<div class=\"hint\">"
        "<div class=\"hint-icon\">" ) + GetInfoIcon() + wxString( "</div>"
        "<p class=\"hint-text\">" )
        + _( "Add a description by creating" ) + wxString( " <code>meta/info.html</code> " )
        + _( "inside this template's directory." )
        + wxString( "</p>"
        "</div>"
        "</div>"
        "</body>"
        "</html>"
    );
}


// Page shown when a template directory has no templates.
inline wxString GetNoTemplatesHtml( bool aDarkMode )
{
    wxString bodyClass = aDarkMode ? wxS( "kicad-dark" ) : wxS( "" );

    return wxString(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>No Templates - KiCad</title>"
        "<style>" )
        + GetCommonStyles() +
        wxString(
        ".card {"
            "max-width: 500px;"
            "margin: 0 auto;"
            "background: var(--bg-elevated);"
            "border-radius: 8px;"
            "padding: 32px;"
            "box-shadow: var(--shadow);"
            "border: 1px solid var(--border);"
            "text-align: center;"
        "}"
        ".icon {"
            "width: 56px;"
            "height: 56px;"
            "margin: 0 auto 20px auto;"
            "display: flex;"
            "align-items: center;"
            "justify-content: center;"
            "background: var(--bg-secondary);"
            "border-radius: 50%;"
            "color: var(--text-secondary);"
        "}"
        ".icon svg {"
            "width: 28px;"
            "height: 28px;"
        "}"
        ".title {"
            "font-size: 1.25rem;"
            "font-weight: 600;"
            "color: var(--text-primary);"
            "margin: 0 0 8px 0;"
        "}"
        ".message {"
            "color: var(--text-secondary);"
            "margin: 0 0 24px 0;"
        "}"
        ".suggestions {"
            "text-align: left;"
            "background: var(--bg-secondary);"
            "border-radius: 6px;"
            "padding: 16px 20px;"
            "margin-bottom: 20px;"
        "}"
        ".suggestions-title {"
            "font-size: 0.875rem;"
            "font-weight: 600;"
            "color: var(--text-primary);"
            "margin: 0 0 12px 0;"
        "}"
        ".suggestions ul {"
            "margin: 0;"
            "padding-left: 20px;"
            "color: var(--text-secondary);"
            "font-size: 0.875rem;"
        "}"
        ".suggestions li {"
            "margin-bottom: 8px;"
        "}"
        ".suggestions li:last-child {"
            "margin-bottom: 0;"
        "}"
        ".tip {"
            "display: flex;"
            "align-items: flex-start;"
            "gap: 10px;"
            "padding: 12px 14px;"
            "background: var(--accent-subtle);"
            "border-radius: 6px;"
            "text-align: left;"
        "}"
        ".tip-icon {"
            "flex-shrink: 0;"
            "color: var(--accent);"
        "}"
        ".tip-text {"
            "font-size: 0.8125rem;"
            "color: var(--text-secondary);"
            "margin: 0;"
        "}"
        "</style>"
        "</head>"
        "<body class=\"" ) + bodyClass + wxString( "\">"
        "<div class=\"card\">"
        "<div class=\"icon\">" ) + GetFolderIcon() + wxString( "</div>"
        "<h1 class=\"title\">" ) + _( "No Templates Found" ) + wxString( "</h1>"
        "<p class=\"message\">" )
        + _( "The selected directory does not contain any project templates." )
        + wxString( "</p>"
        "<div class=\"suggestions\">"
        "<h3 class=\"suggestions-title\">" ) + _( "Suggestions" ) + wxString( "</h3>"
        "<ul>"
        "<li>" ) + _( "Browse to a different directory using the folder icon" ) + wxString( "</li>"
        "<li>" ) + _( "Use the refresh icon to reload the current directory" ) + wxString( "</li>"
        "<li>" ) + _( "Switch to a system templates tab" ) + wxString( "</li>"
        "</ul>"
        "</div>"
        "<div class=\"tip\">"
        "<div class=\"tip-icon\">" ) + GetInfoIcon() + wxString( "</div>"
        "<p class=\"tip-text\">" )
        + _( "Each template needs a 'meta' folder containing configuration files." )
        + wxString( "</p>"
        "</div>"
        "</div>"
        "</body>"
        "</html>"
    );
}

#endif // KICAD_TEMPLATE_DEFAULT_HTML_H
