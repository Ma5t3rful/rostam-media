module;
#include "TGUI/Widgets/ChildWindow.hpp"
export module modal_overlay;


export class ModalOverlay
:public tgui::ChildWindow
{
    public:
    ModalOverlay()
    {
        const auto renderer = getRenderer();
        setSize("100%");
        renderer->setBorderColor(tgui::Color::Transparent);
        renderer->setBackgroundColor("#00000055");
        renderer->setTitleBarHeight(0); // this hack will remove the title bar of the overlay child window.
        tgui::ChildWindow::setPositionLocked(true);
    }

    void setPositionLocked (bool) = delete("An overlay must have a locked position and you should not unlock it.");

    void setTitle (const tgui::String&) = delete("An overlay does not have a title");

    private:
    using tgui::ChildWindow::setSize;
    using tgui::ChildWindow::setPosition;
};
//namespace