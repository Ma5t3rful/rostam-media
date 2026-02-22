module;
#include <limits>
#include <array>
#include <algorithm>
#include <TGUI/TGUI.hpp>
export module rostam_logo;


export 
class rostam_logo:
public tgui::Group
{
    public:

    rostam_logo():
    m_pic(tgui::Picture::create(texture_from_memory())),
    m_max_size(std::numeric_limits<decltype(m_max_size)>::max())
    {
        const auto initial_height = m_pic->getSize().y;
        m_pic->setOrigin(.5f,.5f);
        m_pic->setPosition("50%");
        set_max_size(initial_height);
        add(m_pic);
        onSizeChange(&rostam_logo::size_change_fn,this);

    }

    private:

    void set_max_size(const float max_size)
    {
        m_max_size = max_size;
    }

    void size_change_fn (const tgui::Vector2f size)
    {
        const auto pic_size = std::min(size.y,m_max_size);
        m_pic->setSize(pic_size, pic_size);
    }

    [[nodiscard]]
    static auto texture_from_memory() -> tgui::Texture
    {
        tgui::Texture texture;
        texture.loadFromMemory(rostam_logo_data,std::size(rostam_logo_data));
        return texture;
    }

    static constexpr unsigned char rostam_logo_data[]
    {
        #embed "rostam_logo.png"
    };
    const tgui::Picture::Ptr m_pic;
    float m_max_size;
};
