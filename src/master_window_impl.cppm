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
#include <ranges>

module master_window:impl;
import master_window;
import better_checkbox;
import rostam_logo;
import rostam;
import modal_dialog;
import about_dialog;
import cross_platform;

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
    options_button->setTextSize(22);
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
    if(extract_btn->getText() == "Cancel")
    {
        extract_btn->setEnabled(false);
        m_rostam.request_cancel();
        return ;
    }
    [[maybe_unused]]const auto delete_ts = delCheck->is_checked();
    extract_btn->setText("Cancel");
    if(m_extraction_progress_thrd.valid() and m_extraction_progress_thrd.wait_for(0ms) != std::future_status::ready)
        throw std::logic_error("Another thread is already running and the app requests for another one. This is not intended. Terminating...");
    m_extraction_progress_thrd = std::async(std::launch::async,&rostam::extract,&m_rostam,m_inputaddr,m_outputaddr);
}

void MainWindow::on_open_out_folder_clicked()
{
    if (m_outputaddr.empty())return;
    cross_platform::open_link(m_outputaddr.string());
}


void MainWindow::on_extraction_progress(const int progress)
{
    progressbar->setValue(progress);
    if(progress == 100)
    {
        extract_btn->setText("Extract");
        extract_btn->setEnabled(true);
        const auto result_dialog = std::make_shared<ModalDialog>("Result",m_rostam.is_cancelled()?"Cancelled the extraction.":"Extaction is completed.");
        add(result_dialog);
    }
}


void MainWindow::on_options_button_clicked ()
{
    constexpr auto options = std::to_array({"✔ Official Website ↗",
                                            "♜ Github Repo ↗",
                                            "⚲ Satelite info",
                                            " ℹ️  About this app",
                                            "☓ Quit"});
    const auto options_context_menu = tgui::ContextMenu::create();
    options_context_menu->setPosition(tgui::bindRight(options_button),tgui::bindBottom(options_button));
    std::ranges::for_each (options,std::bind_front(static_cast<void(tgui::ContextMenu::*)(const tgui::String&)>(
        &tgui::ContextMenu::addMenuItem),options_context_menu)
    );

    options_context_menu->connectMenuItem(options[0], &cross_platform::open_link, "https://rostam.media");
    options_context_menu->connectMenuItem(options[1], &cross_platform::open_link, "https://github.com/ma5t3rful/rostam-media");
    options_context_menu->connectMenuItem(options[2], &MainWindow::on_satelite_info_clicked, this);
    options_context_menu->connectMenuItem(options[3], &MainWindow::on_about_clicked, this);
    options_context_menu->connectMenuItem(options[4], &glfwSetWindowShouldClose, getWindow(), true);
    add(options_context_menu);
    options_context_menu->openMenu();
}


void MainWindow::on_about_clicked ()
{
    const auto about_dialog = std::make_shared<AboutDialog>();
    add(about_dialog);
}


void MainWindow::on_satelite_info_clicked ()
{
    [[maybe_unused]] constexpr auto properties = "Satelite: Yah-sat\nFrequency: 11766\nPolarization: Vertical\nSymbol Rate: 27500";
    const auto sat_info_dialog = std::make_shared<ModalDialog>("Satelite Information","");
    const auto actual_dialog = std::static_pointer_cast<tgui::MessageBox>(sat_info_dialog->getWidgets().front());
    const auto horiz_lay = tgui::HorizontalLayout::create({"100%",100});
    actual_dialog->setSize(400,165);
    horiz_lay->getRenderer()->setSpaceBetweenWidgets(6);

    std::ranges::for_each(std::to_array({"Satelite\nYahsat","Frequency\n11766","Polarization\nVertical","Symbol Rate\n27500"})|std::views::enumerate,[&horiz_lay](const auto& str){
        constexpr auto size = tgui::Vector2f (100.f,100.f);
        const auto p = tgui::Panel::create();
        const auto label = tgui::Label::create(std::get<1>(str));
        label->setSize("100%");
        label->setHorizontalAlignment(tgui::HorizontalAlignment::Center);
        label->setVerticalAlignment(tgui::VerticalAlignment::Center);
        p->add(label);
        p->setSize(size);
        p->getRenderer()->setBackgroundColor(std::get<0>(str)%2 == 0?"#cf923d":"#c98b35");
        p->getRenderer()->setRoundedBorderRadius(10);
        horiz_lay->add(p);
    });
    
    actual_dialog->add(horiz_lay);
    add(sat_info_dialog);
}


MainWindow::~MainWindow()
{
    m_rostam.request_cancel();
    // Rethrow all exceptions thrown from the other thread.
    if(m_extraction_progress_thrd.valid() and m_extraction_progress_thrd.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)m_extraction_progress_thrd.get();
}