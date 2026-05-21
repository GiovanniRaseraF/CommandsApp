#include "App.h"
#include "MainFrame.h"

bool App::OnInit() {
    // On macOS, it's nice to prevent standard console window startup
    // by ensuring we handle exceptions gracefully
    try {
        MainFrame* mainFrame = new MainFrame("Commands Clipboard Manager");
        mainFrame->Show(true);
        return true;
    } catch (const std::exception& e) {
        wxMessageBox(wxString::Format("An initialization error occurred: %s", e.what()), 
                     "Startup Error", 
                     wxOK | wxICON_ERROR);
        return false;
    } catch (...) {
        wxMessageBox("An unknown error occurred during application startup.", 
                     "Startup Error", 
                     wxOK | wxICON_ERROR);
        return false;
    }
}
