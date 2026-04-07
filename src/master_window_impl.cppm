module;
#include <chrono>
#include <future>
#include <stdexcept>
#include <filesystem>
#include <GLFW/glfw3.h>
#include "portable_file_dialogs.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/GLFW-OpenGL3.hpp>
#include <array>

module master_window:impl;
import master_window;
import better_checkbox;
import rostam_logo;
import rostam;
import modal_dialog;
import about_dialog;

MainWindow::MainWindow(GLFWwindow* window):
tgui::Gui(window),
main_controls(tgui::GrowVerticalLayout::create()),
rostam_logo_pic(std::make_shared<rostam_logo>()),
options_button(tgui::Button::create("⁝")),
inoutstuffgrid(tgui::Grid::create()),
inputlbl(tgui::Label::create("Input TS file")),
outputlbl(tgui::Label::create("Output Folder")),
inputbtn(tgui::Button::create("Browse")),
outputbtn(tgui::Button::create("Browse")),
delCheck(std::make_shared<better_checkbox>("Delete ts files after extracting")),
progressbar(tgui::ProgressBar::create()),
bottom_box(tgui::HorizontalLayout::create({"100%",20})),
extract_btn(tgui::Button::create("Extract")),
openoutfolder(tgui::Button::create("Open output folder")),
m_rostam(std::bind_front(&MainWindow::on_extraction_progress,this))
{
    //building ui
    
    const auto master_box = tgui::Group::create({"90%","90%"});
    master_box->setOrigin(0.5f,0.5f);
    master_box->setPosition({"50%","50%"});
    master_box->add(rostam_logo_pic);
    master_box->add(main_controls);
    master_box->add(options_button);
    
    options_button->setTextSize(20);
    options_button->onClick(&MainWindow::on_options_button_clicked,this);
    
    rostam_logo_pic->setAutoLayout(tgui::AutoLayout::Fill);
    main_controls->setAutoLayout(tgui::AutoLayout::Bottom);
    main_controls->getRenderer()->setSpaceBetweenWidgets(10);
    main_controls->add(inoutstuffgrid);
    // main_controls->add(delCheck);
    main_controls->add(progressbar);
    main_controls->add(bottom_box);

    const auto notice_lbl = tgui::Label::create("*Notice: This app is NOT affiliated with or endorsed by Rostam Media.");
    notice_lbl->setTextSize(8);
    notice_lbl->getRenderer()->setTextColor("#838383");
    main_controls->add(notice_lbl);
    
    inoutstuffgrid->addWidget(inputlbl,0,0);
    inoutstuffgrid->addWidget(outputlbl,0,1);
    inoutstuffgrid->addWidget(inputbtn, 1, 0);
    inoutstuffgrid->addWidget(outputbtn, 1, 1);
    inoutstuffgrid->setAutoSize(1);
    for(const auto& w:inoutstuffgrid->getWidgets())inoutstuffgrid->setWidgetAlignment(w,tgui::Grid::Alignment::Center);
    bottom_box->getRenderer()->setSpaceBetweenWidgets(10);
    bottom_box->add(extract_btn);
    bottom_box->add(openoutfolder);

    add(master_box);
    // done building ui

    inputbtn->onClick(&MainWindow::on_input_btn_clicked,this);
    outputbtn->onClick(&MainWindow::on_output_btn_clicked,this);
    progressbar->setHeight(10);
    extract_btn->onClick(&MainWindow::on_extract_button_clicked,this);
    extract_btn->setEnabled(false);
    openoutfolder->onClick(&MainWindow::on_open_out_folder_clicked,this);
    openoutfolder->setEnabled(false);
}



void MainWindow::on_input_btn_clicked()
{
    auto selected_folder = pfd::open_file("Select a TS input file","",{"All Files","*.ts"});
    if(auto res = selected_folder.result();
    not res.empty())
    {
        m_inputaddr = std::move(res[0]);
    }
    if(not m_inputaddr.empty() and not m_outputaddr.empty())extract_btn->setEnabled(true);
}


void MainWindow::on_output_btn_clicked()
{
    auto selected_folder = pfd::select_folder("Select output folder");
    if(auto res = selected_folder.result();
      not res.empty())
    {
        m_outputaddr = std::move(res);
        openoutfolder->setEnabled(true);
    }
    if(not m_inputaddr.empty() and not m_outputaddr.empty())extract_btn->setEnabled(true);
}

void MainWindow::on_extract_button_clicked()
{
    using namespace std::literals::chrono_literals;
    [[maybe_unused]]const auto delete_ts = delCheck->is_checked();
    extract_btn->setEnabled(false);
    if(m_extraction_progress_thrd.valid() and m_extraction_progress_thrd.wait_for(0ms) != std::future_status::ready)
        throw std::logic_error("Another thread is already running and the app requests for another one. This is not intended. Terminating...");
    m_extraction_progress_thrd = std::async(std::launch::async,&rostam::extract,&m_rostam,m_inputaddr,m_outputaddr);
}

void MainWindow::on_open_out_folder_clicked()
{
    if (m_outputaddr.empty())return;
    const auto command = std::format("xdg-open '{}'",m_outputaddr.string());
    const std::string command_old = "xdg-open \'" + std::string(m_outputaddr) + '\'';
    const auto _ = std::system(command.c_str());
}


void MainWindow::on_extraction_progress(const int progress)
{
    progressbar->setValue(progress);
    if(progress == 100)
    {
        extract_btn->setEnabled(true);
        const auto result_dialog = std::make_shared<ModalDialog>("Result","Extaction is completed.");
        add(result_dialog);
    }
}

// This is deprecated
void MainWindow::add_dialog (const std::string_view title, const std::string_view text)
{
    const auto message_dialog = tgui::MessageBox::create(std::string(title),std::string(text),{"OK"});
    message_dialog->setOrigin(.5f,.5f);
    message_dialog->setPosition("50%","50%");
    message_dialog->onButtonPress(&tgui::MessageBox::close,message_dialog);
    add(message_dialog);
}

void MainWindow::on_options_button_clicked ()
{
    constexpr auto options = std::to_array({"✔ Official Website","♜ Github repo"," ℹ️  About this app"});
    const auto options_context_menu = tgui::ContextMenu::create();
    std::ranges::for_each (options,std::bind_front(static_cast<void(tgui::ContextMenu::*)(const tgui::String&)>(&tgui::ContextMenu::addMenuItem),options_context_menu));
    options_context_menu->setPosition(tgui::bindRight(options_button),tgui::bindBottom(options_button));
    add(options_context_menu);
    options_context_menu->connectMenuItem(options[0],&std::system,"xdg-open https://rostam.media");
    options_context_menu->connectMenuItem(options[1],&std::system,"xdg-open https://github.com/ma5t3rful/rostam-media");
    options_context_menu->connectMenuItem(options[2],&MainWindow::on_about_clicked,this);
    options_context_menu->openMenu();
}

void MainWindow::on_about_clicked ()
{
    const auto about_dialog = std::make_shared<AboutDialog>();
    add(about_dialog);
}

void MainWindow::on_quit_clicked()
{
    // TODO: Implement this.
}



MainWindow::~MainWindow()
{
    if(m_extraction_progress_thrd.valid() and m_extraction_progress_thrd.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)m_extraction_progress_thrd.get();
}