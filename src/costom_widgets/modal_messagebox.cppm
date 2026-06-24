
module;
#include "TGUI/Widget.hpp"
#include <TGUI/TGUI.hpp>
#include <string_view>
export module modal_messagebox;
import modal_overlay;

export class ModalMessageBox 
:public ModalOverlay
{
    public:
    ModalMessageBox(const std::string_view title,const std::string_view str):
    m_message_box(tgui::MessageBox::create(tgui::String(title),tgui::String(str),{"OK"}))
    {   
        m_message_box->setOrigin(0.5f,0.5f);
        m_message_box->setPosition("50%", "50%");
        m_message_box->onButtonPress(&ModalMessageBox::on_ok_clicked,this);
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