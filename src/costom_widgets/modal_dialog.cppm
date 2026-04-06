
module;
#include "TGUI/Widget.hpp"
#include <TGUI/TGUI.hpp>
#include <string_view>
export module modal_dialog;

export class ModalDialog :
public tgui::ChildWindow
{
    public:
    ModalDialog(const std::string_view title,const std::string_view str):
    m_message_box(tgui::MessageBox::create(tgui::String(title),tgui::String(str),{"OK"}))
    {
        setSize("100%");
        const auto renderer = getRenderer();
        renderer->setBorderColor("transparent");
        renderer->setBackgroundColor("#00000055");
        renderer->setTitleBarHeight(0); // this hack will remove the title bar of the overlay child window.
        setPositionLocked(true);
        m_message_box->setOrigin(0.5f,0.5f);
        m_message_box->setPosition("50%", "50%");
        m_message_box->onButtonPress(&ModalDialog::on_ok_clicked,this);
        add(m_message_box);
    }

    private:
    auto on_ok_clicked() -> void 
    {
        m_message_box->close();
        remove(m_message_box);
        close();
    }

    
    private:
    tgui::MessageBox::Ptr m_message_box;
};