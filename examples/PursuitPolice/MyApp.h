#include <wx/wx.h>

class MyApp : public wxApp
{
public:
    bool OnInit() override;
};

// This defines the equivalent of main() for the current platform.
wxIMPLEMENT_APP(MyApp); 