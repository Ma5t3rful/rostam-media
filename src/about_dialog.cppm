module;
#include <format>
export module about_dialog;
import modal_messagebox;
import rostam_version;

export class AboutDialog
:public ModalMessageBox
{
    public:

    AboutDialog():
    ModalMessageBox(
    "About Rostam Media", 
    std::format("This app is an unofficial extractor for Rostam Media written in C++.\n"
                "Source Code: https://github.com/ma5t3rful/rostam-media\n\n"
                "Version: {}\n"    
                ,rostam_version::get())
    ){};
}; 