/*
 * KiCad Project Template HTML defaults
 * Extracted from dialog_template_selector.cpp to allow reuse and to provide
 * a styled fallback page when a template does not supply an info.html.
 */

#ifndef KICAD_TEMPLATE_DEFAULT_HTML_H
#define KICAD_TEMPLATE_DEFAULT_HTML_H

#include <wx/string.h>

// Uses _() translation macro which is defined globally in KiCad builds.

// Welcome (no template selected) page.
inline wxString GetWelcomeHtml()
{
    return wxString(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>KiCad Project Template Selector</title>"
        "<style>"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: #333; min-height: 100vh; box-sizing: border-box; }"
        ".container { max-width: 800px; margin: 0 auto; background: rgba(255, 255, 255, 0.95); border-radius: 12px; padding: 30px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1); backdrop-filter: blur(10px); }"
        ".header { text-align: center; margin-bottom: 30px; }"
        ".logo { font-size: 2.5rem; font-weight: bold; color: #4a5568; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.1); }"
        ".subtitle { font-size: 1.2rem; color: #666; margin-bottom: 20px; }"
        ".welcome-card { "
#if defined( __MINGW32__ )
        "background: #4299e1;"  // linear-gradient does not work with webview used on MSYS2
#else
        "background: linear-gradient(135deg, #4299e1, #3182ce);"
#endif
        "color: white; padding: 25px; border-radius: 10px; margin-bottom: 25px; box-shadow: 0 4px 15px rgba(66, 153, 225, 0.3); }"
        ".welcome-card h2 { margin-top: 0; font-size: 1.8rem; margin-bottom: 15px; }"
        ".instructions { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 25px; }"
        ".instruction-card { background: #f7fafc; border: 2px solid #e2e8f0; border-radius: 8px; padding: 20px; transition: all 0.3s ease; position: relative; overflow: hidden; }"
        ".instruction-card:hover { transform: translateY(-2px); box-shadow: 0 4px 20px rgba(0, 0, 0, 0.1); border-color: #4299e1; }"
        ".instruction-card::before { content: ''; position: absolute; top: 0; left: 0; width: 4px; height: 100%; background: linear-gradient(135deg, #4299e1, #3182ce); }"
        ".instruction-card h3 { color: #2d3748; margin-top: 0; margin-bottom: 10px; font-size: 1.3rem; }"
        ".instruction-card p { color: #4a5568; line-height: 1.6; margin: 0; }"
        ".features { background: #f0fff4; border: 2px solid #9ae6b4; border-radius: 8px; padding: 20px; margin-bottom: 25px; }"
        ".features h3 { color: #22543d; margin-top: 0; margin-bottom: 15px; font-size: 1.4rem; }"
        ".features ul { color: #2f855a; line-height: 1.8; margin: 0; padding-left: 20px; }"
        ".features li { margin-bottom: 8px; }"
        ".tips { background: #fffaf0; border: 2px solid #fbd38d; border-radius: 8px; padding: 20px; }"
        ".tips h3 { color: #c05621; margin-top: 0; margin-bottom: 15px; font-size: 1.4rem; }"
        ".tips p { color: #c05621; line-height: 1.6; margin: 0 0 10px 0; }"
        ".highlight { background: linear-gradient(120deg, #a8edea 0%, #fed6e3 100%); padding: 2px 6px; border-radius: 4px; font-weight: 600; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"container\">"
        "<div class=\"header\">"
        "<div class=\"logo\">KiCad 📑</div>"
        "<div class=\"subtitle\">" + _( "Project Template Selector" ) + "</div>"
        "</div>"
        "<div class=\"welcome-card\">"
        "<h2>" + _( "Welcome to Template Selection!" ) + "</h2>"
        "<p>" + _( "Choose from a variety of pre-configured project templates to jumpstart your PCB design. Templates provide ready-to-use project structures with common components, libraries, and design rules." ) + "</p>"
        "</div>"
        "<div class=\"instructions\">"
        "<div class=\"instruction-card\">"
        "<h3>→ " + _( "Browse Templates" ) + "</h3>"
        "<p>" + _( "Navigate through the template tabs above to explore different categories of project templates. Each tab contains templates organized by type or complexity." ) + "</p>"
        "</div>"
        "<div class=\"instruction-card\">"
        "<h3>→ " + _( "Select a Template" ) + "</h3>"
        "<p>" + _( "Click on any template in the list to " ) + "<span class=\"highlight\">" + _( "preview its details" ) + "</span>. " + _( "The template information will appear in this panel, showing descriptions, included components, and project structure." ) + "</p>"
        "</div>"
        "<div class=\"instruction-card\">"
        "<h3>→ " + _( "Customize Path" ) + "</h3>"
        "<p>" + _( "Use the " ) + "<span class=\"highlight\">" + _( "folder path field" ) + "</span> " + _( "above to browse custom template directories. Click the folder icon to browse, or the refresh icon to reload templates." ) + "</p>"
        "</div>"
        "<div class=\"instruction-card\">"
        "<h3>→ " + _( "Create Project" ) + "</h3>"
        "<p>" + _( "Once you've found the right template, click " ) + "<span class=\"highlight\">" + _( "OK" ) + "</span> " + _( "to create a new project based on the selected template. Your project will inherit all template settings and files." ) + "</p>"
        "</div>"
        "</div>"
        "<div class=\"features\">"
        "<h3>" + _( "What You Get with Templates" ) + "</h3>"
        "<ul>"
        "<li><strong>" + _( "Pre-configured libraries" ) + "</strong> " + _( "- Common components and footprints already linked" ) + "</li>"
        "<li><strong>" + _( "Design rules" ) + "</strong> " + _( "- Appropriate electrical and mechanical constraints" ) + "</li>"
        "<li><strong>" + _( "Layer stackups" ) + "</strong> " + _( "- Optimized for the intended application" ) + "</li>"
        "<li><strong>" + _( "Component placement" ) + "</strong> " + _( "- Basic layout and routing guidelines" ) + "</li>"
        "<li><strong>" + _( "Documentation" ) + "</strong> " + _( "- README files and design notes" ) + "</li>"
        "<li><strong>" + _( "Manufacturing files" ) + "</strong> " + _( "- Gerber and drill file configurations" ) + "</li>"
        "</ul>"
        "</div>"
        "<div class=\"tips\">"
        "<h3>" + _( "Pro Tips" ) + "</h3>"
        "<p><strong>" + _( "Start Simple:" ) + "</strong> " + _( "Begin with basic templates and add more elements as you go." ) + "</p>"
        "<p><strong>" + _( "Customize Later:" ) + "</strong> " + _( "Templates are starting points - you can modify libraries, rules, and layouts after project creation." ) + "</p>"
        "<p><strong>" + _( "Save Your Own:" ) + "</strong> " + _( "Once you develop preferred settings, create a custom template for future projects." ) + "</p>"
        "</div>"
        "</div>"
        "</body>"
        "</html>"
    );
}

// Fallback when a specific template has no meta/info.html.
inline wxString GetTemplateInfoHtml( const wxString& aTemplateName )
{
    return wxString(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>" + aTemplateName + " - KiCad Template</title>"
        "<style>"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; margin:0; padding:20px; background:#edf2f7; color:#2d3748; }"
        ".container { max-width: 780px; margin:0 auto; background:#ffffff; border-radius:12px; padding:32px; box-shadow:0 6px 24px rgba(0,0,0,0.08); }"
        ".header { display:flex; align-items:center; gap:12px; margin-bottom:24px; }"
        ".badge { background:#3182ce; color:white; padding:4px 10px; border-radius:6px; font-size:0.75rem; letter-spacing:0.05em; text-transform:uppercase; }"
        "h1 { font-size:1.9rem; margin:0; }"
        "p { line-height:1.55; }"
        ".cta { background:#ebf8ff; border:1px solid #bee3f8; padding:16px 20px; border-radius:10px; margin:28px 0 18px; }"
        ".steps { display:grid; gap:16px; margin-top:10px; }"
        ".step { background:#f7fafc; border:1px solid #e2e8f0; padding:14px 16px; border-radius:8px; }"
        ".step h3 { margin:0 0 6px 0; font-size:1.05rem; }"
        ".edit-hint { background:#fffaf0; border:1px solid #fbd38d; padding:14px 16px; border-radius:8px; margin-top:24px; }"
        "code { background:#f1f5f9; padding:2px 5px; border-radius:4px; font-size:0.85rem; }"
        "</style>"
        "</head><body><div class=\"container\">"
        "<div class=\"header\"><div class=\"badge\">" + _( "Template" ) + "</div><h1>" + aTemplateName + "</h1></div>"
        "<p>" + _( "This project template doesn't include an info page yet. You can still use it to create a new project." ) + "</p>"
        "<div class=\"cta\">"
        "<strong>" + _( "To use this template:" ) + "</strong>"
        "<div class=\"steps\">"
        "<div class=\"step\"><h3>1. " + _( "Create the project" ) + "</h3><p>" + _( "Click OK below. KiCad will create a new project folder populated with the contents of this template." ) + "</p></div>"
        "<div class=\"step\"><h3>2. " + _( "Open schematic and PCB" ) + "</h3><p>" + _( "Use the Project Manager tree or launch Schematic and PCB editors to begin designing." ) + "</p></div>"
        "<div class=\"step\"><h3>3. " + _( "Review libraries & settings" ) + "</h3><p>" + _( "Confirm symbol/footprint libraries, design rules, and board stackup match your needs." ) + "</p></div>"
        "</div>"  // steps
        "</div>"   // cta
        "<div class=\"edit-hint\">"
        "<strong>" + _( "Add an info page later:" ) + "</strong>"
        "<p>" + _( "Create a file at" ) + " <code>meta/info.html</code> " + _( "inside this template's directory to provide rich documentation, images, or guidance." ) + "</p>"
        "<p>" + _( "You can copy styling from the default pages for consistency." ) + "</p>"
        "</div>"
        "</div></body></html>"
    );
}

// Page shown when a template directory has no templates.
inline wxString GetNoTemplatesHtml()
{
    return wxString(
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "<title>No Templates - KiCad</title>"
        "<style>"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%); color: #333; min-height: 100vh; box-sizing: border-box; }"
        ".container { max-width: 600px; margin: 0 auto; background: rgba(255, 255, 255, 0.98); border-radius: 12px; padding: 40px; box-shadow: 0 8px 32px rgba(0, 0, 0, 0.15); }"
        ".content { text-align: center; }"
        ".icon { font-size: 3.5rem; margin-bottom: 20px; }"
        ".title { font-size: 1.8rem; font-weight: 600; color: #2d3748; margin-bottom: 12px; }"
        ".message { font-size: 1rem; color: #4a5568; line-height: 1.6; margin-bottom: 30px; }"
        ".suggestions { text-align: left; background: #edf2f7; border-left: 4px solid #4299e1; border-radius: 8px; padding: 20px; margin-bottom: 30px; }"
        ".suggestions h3 { color: #2d3748; margin-top: 0; margin-bottom: 12px; }"
        ".suggestions ul { color: #4a5568; margin: 0; padding-left: 20px; line-height: 1.8; }"
        ".suggestions li { margin-bottom: 8px; }"
        ".action-hint { background: #fef5e7; border: 1px solid #f9e79f; border-radius: 8px; padding: 16px; color: #7d6608; }"
        ".action-hint strong { color: #c17817; }"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"container\">"
        "<div class=\"content\">"
        "<div class=\"icon\">📂</div>"
        "<div class=\"title\">" + _( "No Templates Found" ) + "</div>"
        "<div class=\"message\">"
        + _( "The selected templates directory does not contain any project templates." )
        + "</div>"
        "<div class=\"suggestions\">"
        "<h3>" + _( "What to do:" ) + "</h3>"
        "<ul>"
        "<li>" + _( "Browse to a different templates directory using the folder icon" ) + "</li>"
        "<li>" + _( "Click the refresh icon to reload the current directory" ) + "</li>"
        "<li>" + _( "Check that the directory path contains valid templates" ) + "</li>"
        "<li>" + _( "Use the default system templates by navigating to a system templates tab" ) + "</li>"
        "</ul>"
        "</div>"
        "<div class=\"action-hint\">"
        "<strong>" + _( "Tip:" ) + "</strong> " + _( "Templates are organized in subdirectories. Each template needs a 'meta' folder with configuration files." )
        + "</div>"
        "</div>"
        "</div>"
        "</body>"
        "</html>"
    );
}

#endif // KICAD_TEMPLATE_DEFAULT_HTML_H
