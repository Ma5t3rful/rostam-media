module;

export module about_dialog;
import modal_dialog;

export class AboutDialog
:public ModalDialog
{
    public:

    AboutDialog():
    ModalDialog(
    "About Rostam Media", 
    "This app is an unofficial extractor for Rostam Media written in C++.\n"
    "Source Code: https://github.com/ma5t3rful/rostam-media"){};
}; 