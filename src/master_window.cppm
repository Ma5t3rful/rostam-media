module;
#include <GLFW/glfw3.h>
#include <filesystem>
#include <memory>
#include <future>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/GLFW-OpenGL3.hpp>
export module master_window;
import rostam;
import better_checkbox;
import rostam_logo;


export 
class MainWindow:
public tgui::Gui
{
    public:
    MainWindow(GLFWwindow*);
    ~MainWindow();
    private:

    tgui::GrowVerticalLayout::Ptr main_controls;
    std::shared_ptr<rostam_logo> rostam_logo_pic;

    tgui::Grid::Ptr inoutstuffgrid;
    tgui::Label::Ptr inputlbl;
    tgui::Label::Ptr outputlbl;
    tgui::Button::Ptr inputbtn;
    tgui::Button::Ptr outputbtn;
    std::shared_ptr<better_checkbox> delCheck;

    tgui::ProgressBar::Ptr progressbar;

    tgui::HorizontalLayout::Ptr bottom_box;
    tgui::Button::Ptr extract_btn;
    tgui::Button::Ptr openoutfolder;
    
    
    std::filesystem::path m_inputaddr;
    std::filesystem::path m_outputaddr;
    
    rostam m_rostam;
    std::future<void> m_extraction_progress_thrd;
    void on_input_btn_clicked();
    void on_output_btn_clicked();
    void onInputDialResponded(const int response);
    void onOutputDialResponded(const int response);
    void on_open_out_folder_clicked();
    void on_extract_button_clicked();
    void on_extraction_progress(const int progress);
    void add_dialog (const std::string_view title, const std::string_view text);
};