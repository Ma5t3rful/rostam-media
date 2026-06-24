module;
#include "TGUI/Layout.hpp"
#include "TGUI/Renderers/ChildWindowRenderer.hpp"
#include "TGUI/Widget.hpp"
#include "TGUI/Widgets/ChildWindow.hpp"
export module modal_window;
import modal_overlay;

export class ModalDialog
:public ModalOverlay
{
    public: 
    ModalDialog(const std::string& title=""):
    m_inner_window(tgui::ChildWindow::create(title))
    {
        m_inner_window->setOrigin(0.5,0.5);
        m_inner_window->setPosition("50%","50%");
        m_inner_window->onClose(&tgui::ChildWindow::close,this);
        ModalOverlay::add(m_inner_window);
    }

    auto add (const tgui::Widget::Ptr& widget, const tgui::String& name="") -> void override
    {
        m_inner_window->add(widget,name);
    }

    auto set_size (const tgui::Layout2d& size2d) -> void
    {
        m_inner_window->setSize(size2d);
    }

    auto set_title (const std::string& title) -> void
    {
        m_inner_window->setTitle(title);
    }

    auto remove_all_from_inner () -> void
    {
        m_inner_window->removeAllWidgets();
    }
    
    [[nodiscard]]
    auto get_inner_renderer () const -> tgui::ChildWindowRenderer*
    {
        return m_inner_window->getRenderer();
    }

    private:
    const tgui::ChildWindow::Ptr m_inner_window;
};
