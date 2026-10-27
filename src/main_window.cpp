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
#include <QSplitter>
#include <QWidget>
#include <QFrame>

#include <fstream>
#include <sstream>

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

void MainWindow::build_corpus() {
    if (workspace_.documents().empty()) {
        return;
    }
    processing_core_.rebuild(workspace_);
    corpus_current_=true;

    const std::size_t total_chunks = processing_core_.chunk_count();
    workspace_widget_->set_corpus_status(QString("Corpus: <b>Ready - %1 chunks</b>").arg(total_chunks));

    clear_processing_outputs();
    update_action_states();
    status_label_->setText(QString("<b> Current status:</b> Corpus built succesfully."));
    
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
    workspace_.clear();
    corpus_current_ = false;

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
    workspace_widget_->set_corpus_status("Corpus: <b> Rebuild Required</b>");
    clear_processing_outputs();
    update_action_states();
}

void MainWindow::clear_processing_outputs() {
    search_widget_->clear_results();
    term_inspector_widget_->clear_statistics();
    context_widget_->clear_context();
}

}  // namespace aiws::gui
