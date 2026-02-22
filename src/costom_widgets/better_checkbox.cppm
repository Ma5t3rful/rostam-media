module ;
#include <string>
#include <TGUI/TGUI.hpp>
export module better_checkbox;

export 
class better_checkbox
:public tgui::Group
{
    public:

    better_checkbox(const std::string& desc):
    m_checkbox(tgui::CheckBox::create(desc))
    {
        setHeight(m_checkbox->getSize().y);
        //m_checkbox->setOrigin(0.5,0.5);
        
        add(m_checkbox);
    }
    
    auto is_checked () -> bool const
    {
        return m_checkbox->isChecked();
    }

    private:
    tgui::CheckBox::Ptr m_checkbox;
};