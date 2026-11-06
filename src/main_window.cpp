#include "aiws/gui/main_window.hpp"
#include "aiws/gui/context_widget.hpp"
#include "aiws/gui/term_inspector_widget.hpp"
#include "aiws/gui/workspace_widget.hpp"
#include "aiws/gui/search_widget.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QFrame>
#include <chrono>

#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace aiws::gui {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // Construct the M3 application window, compose the GUI components, establish layouts/presentation, connect events, and establish initial state.

    setWindowTitle("AI Workspace");
    resize(900, 900);

    //central and child widgets
    auto* central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    //banner
    auto* banner = new QWidget(this);
    banner->setStyleSheet("background-color: #581825; color: white; padding: 6px;");
    auto* icon_label = new QLabel("AI", banner);
    icon_label->setStyleSheet("background-color: #3b1018; color: white; font-weight: bold; border-radius: 16px; padding: 6px 10px; font-size: 14px;");
    auto* title_label = new QLabel("AI Workspace", banner);
    title_label->setStyleSheet("font-size: 18pt; font-weight: bold; color: white;");
    auto* subtitle_label = new QLabel("Document Search and Context Explorer", banner);
    subtitle_label->setStyleSheet("font-size: 10pt; font-style: italic; color: #d0c0c4;");

    auto* banner_text_layout = new QVBoxLayout();
    banner_text_layout->setSpacing(0);
    banner_text_layout->setContentsMargins(0, 0, 0, 0);
    banner_text_layout->addWidget(title_label);
    banner_text_layout->addWidget(subtitle_label);

    auto* banner_layout = new QHBoxLayout(banner);
    banner_layout->setContentsMargins(10, 5, 10, 5);
    banner_layout->addWidget(icon_label);
    banner_layout->addSpacing(10);
    banner_layout->addLayout(banner_text_layout);
    banner_layout->addStretch();

    //main widgets
    workspace_widget_ = new WorkspaceWidget(this);
    search_widget_ = new SearchWidget(this);
    term_inspector_widget_ = new TermInspectorWidget(this);
    context_widget_ = new ContextWidget(this);

    auto* content_frame = new QFrame(this);
    content_frame->setObjectName("content_frame");
    content_frame->setStyleSheet("QFrame#content_frame { background-color: #f5f5f5; border: 1px solid #b0b0b0; }");
    QString line_style = "background-color: #b0b0b0; border:none;";

    //boxes for each component
    auto* h_line_1 = new QFrame(this);
    h_line_1->setFrameShape(QFrame::HLine);
    h_line_1->setFixedHeight(1);
    h_line_1->setStyleSheet(line_style);

    auto* h_line_2 = new QFrame(this);
    h_line_2->setFrameShape(QFrame::HLine);
    h_line_2->setFixedHeight(1);
    h_line_2->setStyleSheet(line_style);

    auto* v_line_bottom = new QFrame(this);
    v_line_bottom->setFrameShape(QFrame::VLine);
    v_line_bottom->setFixedWidth(1);
    v_line_bottom->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    v_line_bottom->setStyleSheet(line_style);

    //context row 
    auto* bottom_row = new QWidget(this);
    auto* bottom_row_layout = new QHBoxLayout(bottom_row);
    bottom_row_layout->setContentsMargins(0, 0, 0, 0);
    bottom_row_layout->addSpacing(0);
    bottom_row_layout->addWidget(term_inspector_widget_, 1);
    bottom_row_layout->addWidget(v_line_bottom);
    bottom_row_layout->addWidget(context_widget_, 1);

    //main content
    auto* content_layout = new QVBoxLayout(content_frame);
    content_layout->setContentsMargins(0, 0, 0, 0);
    content_layout->setSpacing(0);
    content_layout->addWidget(workspace_widget_, 2);
    content_layout->addWidget(h_line_1);
    content_layout->addWidget(search_widget_, 3);
    content_layout->addWidget(h_line_2);
    content_layout->addWidget(bottom_row, 2);
 


    // bottom actions
    auto* bottom_bar = new QHBoxLayout();
    bottom_bar->setContentsMargins(0, 4, 0, 0);
    status_label_ = new QLabel("<b>Current status:</b> Ready", this);
    
    reset_button_ = new QPushButton("Reset", this);
    reset_button_->setStyleSheet("background-color: #d8a038; font-weight: bold; padding: 6px 20px; border-radius: 3px;");

    exit_button_ = new QPushButton("Exit", this);
    exit_button_->setStyleSheet("background-color: #4a6fa5; color: white; font-weight: bold; padding: 6px 20px; border-radius: 3px;");

    bottom_bar->addWidget(status_label_);
    bottom_bar->addStretch();
    bottom_bar->addWidget(reset_button_);
    bottom_bar->addWidget(exit_button_);

    auto* main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(8, 8, 8, 8);
    main_layout->setSpacing(8);
    main_layout->addWidget(banner);
    main_layout->addWidget(content_frame, 1);
    main_layout->addLayout(bottom_bar);

    //connect workspace signals
    connect(workspace_widget_, &WorkspaceWidget::add_document_requested, this, &MainWindow::add_document);
    connect(workspace_widget_, &WorkspaceWidget::document_selected, this, &MainWindow::document_selected);
    connect(workspace_widget_, &WorkspaceWidget::build_corpus_requested, this, &MainWindow::build_corpus);

    //search signals
    connect(search_widget_, &SearchWidget::search_requested, this, &MainWindow::search_requested);
    connect(search_widget_, &SearchWidget::query_changed, this, &MainWindow::query_changed);
    connect(search_widget_, &SearchWidget::source_document_requested, this, &MainWindow::source_document_requested);
    connect(search_widget_, &SearchWidget::selected_chunk_changed, term_inspector_widget_, &TermInspectorWidget::set_selected_chunk);

    //inspector and context signals
    connect(term_inspector_widget_, &TermInspectorWidget::term_inspection_requested, this, &MainWindow::term_inspection_requested);
    connect(context_widget_, &ContextWidget::build_context_requested, this, &MainWindow::build_context_requested);
    
    connect(reset_button_, &QPushButton::clicked, this, &MainWindow::reset_application);
    connect(exit_button_, &QPushButton::clicked, this, &MainWindow::exit_application);

    reset_application();
}

MainWindow::~MainWindow() {
    stop_ipc_server();
    worker_running_=false;
    if(worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void MainWindow::set_exit_callback(ExitCallback callback) noexcept {
    exit_callback_=callback;
}

void MainWindow::add_document() {
    const QString file_path = QFileDialog::getOpenFileName(this, "Add document to workspace", QString(), "Text files (*.txt);;All files (*)");

    if (file_path.isEmpty()){
        return;
    }

    std::ifstream file(file_path.toStdString());
    if (!file.is_open()){
        status_label_->setText("Failed to open file.");
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    
    QFileInfo file_info(file_path);
    std::string doc_id = file_info.fileName().toStdString();
    std::string doc_title = file_info.baseName().toStdString();
    std::string doc_text = buffer.str();

    Document doc(doc_id, doc_title, doc_text);
    workspace_.add_document(std::move(doc));

    workspace_widget_->add_document(QString::fromStdString(doc_id));
    invalidate_corpus();
    status_label_->setText("Added document: " +QString::fromStdString(doc_id));
}

bool MainWindow::add_document_file(const QString& path){
    std::ifstream file(path.toStdString());
    if (!file.is_open()){
        return false;
    }

    std::stringstream buffer;
    buffer <<file.rdbuf();

    QFileInfo file_info(path);
    std::string doc_id = file_info.fileName().toStdString();
    std::string doc_title = file_info.baseName().toStdString();
    std::string doc_text = buffer.str();

    Document doc(doc_id, doc_title, doc_text);
    workspace_.add_document(std::move(doc));

    workspace_widget_->add_document(QString::fromStdString(doc_id));
    invalidate_corpus();
    status_label_->setText("Added document: " +QString::fromStdString(doc_id));
    return true;
}

void MainWindow::set_build_delay_ms(int ms_per_document){
    build_delay_ms_ = ms_per_document;
}

void MainWindow::build_corpus() {
    if (workspace_.documents().empty()) {
        return;
    }

    if (worker_thread_.joinable()) { //join build thread before launching another
        worker_thread_.join();
    }

    const uint64_t build_gen = ++current_generation_;
    corpus_current_=false; 
    //snapshot workspace
    Workspace snapshot = workspace_;
    const int delay = build_delay_ms_;

    //update UI to building
    workspace_widget_->set_corpus_status("Corpus: <b>Building...</b>");
    status_label_->setText("<b>Current status:</b> Building corpus...");
    update_action_states();

    worker_running_=true;

    //launch background worker thread
    worker_thread_=std::thread([this, snapshot = std::move(snapshot), build_gen, delay]() mutable{
        //heap ownership
        auto core = std::make_shared<aiws::ProcessingCore>();
        bool success = true;
        QString error_msg;

        try{
            if (delay>0){ //artificial delay
                for (std::size_t i=0; i<snapshot.documents().size(); i++){
                    const int step_ms = 20;
                    for (int elapsed=0; elapsed<delay; elapsed+=step_ms){
                        if(!worker_running_){
                            return;
                        }
                        const int current_sleep=std::min(step_ms, delay-elapsed);
                        std::this_thread::sleep_for(std::chrono::milliseconds(current_sleep));
                    }
                }
            }
            core->rebuild(snapshot);
        }catch(const std::exception& e){ //get exceptions to report failure
            success =false;
            error_msg=QString::fromStdString(e.what());
        }catch(...){
            success=false;
            error_msg="Unkown build failure.";
        }

        //completion back to gui event loop
        QMetaObject::invokeMethod(this, [this, core=std::move(core), build_gen, success, error_msg](){
            on_build_completed(std::move(core), build_gen, success, error_msg);
        });
    });
}


void MainWindow::document_selected(int row) {
    if(row<0 || static_cast<std::size_t>(row) >= workspace_.documents().size()) {
        workspace_widget_->clear_selected_document();
        return;
    }
    const auto& doc = workspace_.documents()[static_cast<std::size_t>(row)];
    workspace_widget_->show_selected_document(QString::fromStdString(doc.title()), QString::fromStdString(doc.text()));
}

void MainWindow::search_requested(const QString& query, int k) {
    if(!corpus_current_ || query.trimmed().isEmpty()){
        return;
    }

    auto results = processing_core_.search(query.toStdString(), k);
    search_widget_->set_results(results);
    status_label_->setText(QString("Search returned %1 results").arg(results.size()));
}

void MainWindow::query_changed(const QString& query) {
    (void)query;
    update_action_states();
}

void MainWindow::build_context_requested(int token_budget) {
    if(!corpus_current_ || !search_widget_->has_query()) {
        return;
    }

    int k = search_widget_->result_count();
    auto items = processing_core_.build_context(search_widget_->query().toStdString(), k, static_cast<std::size_t>(token_budget));
    context_widget_->set_context(items);
    status_label_->setText(QString("<b>Current status:</b> Context built - %1 items").arg(items.size()));

}

void MainWindow::source_document_requested(const QString& document_id) {
    const auto& docs = workspace_.documents();
    for(std::size_t i = 0; i<docs.size(); i++) {
        if(docs[i].id() == document_id.toStdString()) {
            workspace_widget_->select_document(static_cast<int>(i));
            status_label_->setText("Navigated to source document: "+ document_id);
            return;
        }
    }
}

void MainWindow::term_inspection_requested(const QString& term,
                                           const QString& chunk_id) {
    if(!corpus_current_){
        return;
    }

    const std::string term_std = term.toStdString();
    const std::size_t doc_freq = processing_core_.document_frequency(term_std);

    if(!chunk_id.isEmpty()) {
        const std::size_t chunk_freq = processing_core_.term_frequency(term_std, chunk_id.toStdString());
        term_inspector_widget_->set_statistics(doc_freq, true, chunk_freq);
        status_label_->setText("Inspected term: " + term);
        return;
    }
    term_inspector_widget_->set_statistics(doc_freq, false, 0);
    status_label_->setText("Inspected term: " + term);
}

void MainWindow::reset_application() {
    if(worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    workspace_.clear();
    corpus_current_ = false;
    current_generation_++;

    workspace_widget_->reset();
    context_widget_->reset();
    search_widget_->reset();
    term_inspector_widget_->reset();
    
    workspace_widget_->set_corpus_status("Corpus: <b>Not Built</b>");
    status_label_->setText("<b>Current status:</b> Ready");
    update_action_states();
}

void MainWindow::exit_application() {
    auto reply = QMessageBox::question(this, "Exit", "Are you sure you want to exit?");
    if(reply== QMessageBox::Yes) {
        if(worker_thread_.joinable()) {
            worker_thread_.join();
        }
        if(exit_callback_!=nullptr) {
            exit_callback_(this);
        }
        else{
            close();
        }
    }
}

void MainWindow::update_action_states() {
    const bool has_documents =!workspace_.documents().empty();
    const bool has_query = search_widget_->has_query();

    workspace_widget_->set_build_corpus_enabled(has_documents);
    search_widget_->set_search_enabled(corpus_current_ && has_query);
    term_inspector_widget_->set_inspection_enabled(corpus_current_);
    context_widget_->set_build_context_enabled(corpus_current_ && has_query);
}

void MainWindow::invalidate_corpus() {
    corpus_current_=false;
    current_generation_++;
    workspace_widget_->set_corpus_status("Corpus: <b> Rebuild Required</b>");
    clear_processing_outputs();
    update_action_states();
}

void MainWindow::clear_processing_outputs() {
    search_widget_->clear_results();
    term_inspector_widget_->clear_statistics();
    context_widget_->clear_context();
}

void MainWindow::on_build_completed(std::shared_ptr<aiws::ProcessingCore> new_core, uint64_t generation, bool success, const QString& error_message){

//handle build failure
#ifndef AIWS_FAULT_LOSE_FAILURE
    if(!success){
        status_label_->setText(QString("<b>Current status:</b> Build failed: %1").arg(error_message));
        workspace_widget_->set_corpus_status("Corpus: <b>Build failed</b>");
        update_action_states();
        return;
    }
#else
    //silently drop failure
    if(!success){
        return;
    }
#endif

//drop completion if newer mutation happened mid build
#ifndef AIWS_FAULT_ACCEPT_STALE
    if(generation!=current_generation_){
        return;
    }
#endif

    //move built core into active member
    if (new_core){
        processing_core_= std::move(*new_core);
    }
    //update gui on gui thread
    corpus_current_ = true;
    const std::size_t total_chunks = processing_core_.chunk_count();
    workspace_widget_->set_corpus_status(QString("Corpus <b>Ready - %1 chunks</b>").arg(total_chunks));
    clear_processing_outputs();
    update_action_states();
    status_label_->setText("<b>Current status:</b> Corpus built successfully.");

}

void MainWindow::set_ipc_socket_path(const std::string& path){
    ipc_socket_path_ = path;
    start_ipc_server();
}

void MainWindow::start_ipc_server(){
    if(ipc_socket_path_.empty()|| ipc_running_){
        return;
    }

    //remove any existing socket file
    ::unlink(ipc_socket_path_.c_str());

    //POSIX unix domain streaming sockets
    server_socket_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_socket_fd_<0){
        return;
    }

    //mark socket as non blocking so accept() wont freeze thread if no connection
    int flags = ::fcntl(server_socket_fd_, F_GETFL, 0);
    ::fcntl(server_socket_fd_, F_SETFL, flags | O_NONBLOCK);

    sockaddr_un addr{};
    addr.sun_family= AF_UNIX;
    std::strncpy(addr.sun_path, ipc_socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    if(::bind(server_socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0){
        ::close(server_socket_fd_);
        server_socket_fd_ = -1;
        return;
    }

    if(::listen(server_socket_fd_, 16) <0){
        ::close(server_socket_fd_);
        server_socket_fd_ = -1;
        return;
    }

    //launch server thread
    ipc_running_=true;
    ipc_thread_ = std::thread(&MainWindow::ipc_server_loop, this);
}

void MainWindow::stop_ipc_server(){
    ipc_running_=false;
    if(ipc_thread_.joinable()){
        ipc_thread_.join();
    }
    if(server_socket_fd_ >= 0){
        ::close(server_socket_fd_);
        server_socket_fd_ = -1;
    }
    if(!ipc_socket_path_.empty()){
        ::unlink(ipc_socket_path_.c_str());
    }
}

void MainWindow::ipc_server_loop(){
    //reads exact len bytes or aborts on error
    auto read_exact= [](int fd, void* buf, size_t len) ->bool{
        char* ptr = static_cast<char*>(buf);
        size_t rem=len;
        while(rem>0){
            ssize_t n = ::read(fd, ptr, rem);
            if(n<=0){
                return false;
            }
            ptr+=n;
            rem-= static_cast<size_t>(n);
        }
        return true;
    };

    //writes or aborts on error
    auto write_exact = [](int fd, const void* buf, size_t len) -> bool{
        const char* ptr = static_cast<const char*>(buf);
        size_t rem=len;
        while(rem>0){
            ssize_t n = ::write(fd, ptr, rem);
            if(n<=0){
                return false;
            }
            ptr+=n;
            rem-=static_cast<size_t>(n);
        }
        return true;
    };

    while(ipc_running_){
        //nonblocking poll for incoming connections
        int client_fd = ::accept(server_socket_fd_, nullptr, nullptr);
        if(client_fd<0){
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        //5 second timeout
        struct timeval tv{};
        tv.tv_sec =5;
        tv.tv_usec = 0;
        ::setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        //write protocol
        uint32_t path_len_net=0;
        if(!read_exact(client_fd, &path_len_net, sizeof(path_len_net))){
            ::close(client_fd);
            continue;
        }
        uint32_t path_len = ntohl(path_len_net);

        //read path
        std::string path_str(path_len, '\0');
        if(!read_exact(client_fd, &path_str[0], path_len)){
            ::close(client_fd);
            continue;
        }

        //check readability/presence
        std::ifstream test_file(path_str);
        bool readable =test_file.good();
        test_file.close();

        //wire protocol
        uint8_t status_code= 0;
        std::string reason;
        if(!readable){
            status_code=1;
            reason = "File unreadable or does not exist: "+ path_str;
        }
        uint32_t reason_len_net = htonl(static_cast<uint32_t>(reason.size()));

        //send reply
        write_exact(client_fd, &status_code, sizeof(status_code));
        write_exact(client_fd, &reason_len_net, sizeof(reason_len_net));
        if(!reason.empty()){
            write_exact(client_fd, reason.data(), reason.size());
        }
        ::close(client_fd);

        //queue adding doc to main gui thread
        if(readable){
            QString qpath =QString::fromUtf8(path_str.data(), static_cast<int>(path_str.size()));
            QMetaObject::invokeMethod(this, [this,qpath](){add_document_file(qpath);}, Qt::QueuedConnection);
        }
    
    }
}

}  // namespace aiws::gui
