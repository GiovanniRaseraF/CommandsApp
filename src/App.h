#ifndef APP_H
#define APP_H

#include <wx/wx.h>

class App : public wxApp {
public:
    App() = default;
    virtual ~App() = default;

    // Initialization hook called when wxWidgets is ready
    virtual bool OnInit() override;
};

#endif // APP_H
