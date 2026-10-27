#include "aiws/gui/workspace_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPlainTextEdit>

namespace aiws::gui {

WorkspaceWidget::WorkspaceWidget(QWidget* parent) : QWidget(parent) {
    // Construct and lay out the Workspace and Selected Document areas, connect local widget events, and establish initial presentation state.

    //title and buttons left section
    auto* workspace_header_layout = new QHBoxLayout;
    workspace_header_layout->setSpacing(6);
    
    auto* workspace_title = new QLabel("<b>Workspace</b>", this);
    add_document_button_ = new QPushButton("Add Document", this);
    build_corpus_button_ = new QPushButton("Build Corpus", this);
    
    workspace_header_layout->addWidget(workspace_title);
    workspace_header_layout->addWidget(add_document_button_);
    workspace_header_layout->addWidget(build_corpus_button_);
    workspace_header_layout->addStretch();

    document_list_ = new QListWidget(this);
    corpus_status_label_ = new QLabel("Corpus: <b> Not Built</b>", this);

    auto* left_layout = new QVBoxLayout;
    left_layout->addLayout(workspace_header_layout);
    left_layout->addWidget(document_list_, 1);
    left_layout->addWidget(corpus_status_label_);

    auto* v_line = new QFrame(this);
    v_line->setFrameShape(QFrame::VLine);
    v_line->setFixedWidth(1);
    v_line->setStyleSheet("background-color: #d0d0d0; border: none;");

    //right section
    document_title_label_ = new QLabel("<b>Selected Document -</b>", this);
    document_text_ = new QPlainTextEdit(this);
    document_text_->setReadOnly(true);
    document_text_->setPlaceholderText("Select a document to view its contents.");

    auto* right_layout = new QVBoxLayout;
    right_layout->addWidget(document_title_label_);
    right_layout->addWidget(document_text_, 1);
    auto* bottom_spacer = new QWidget(this);
    bottom_spacer->setFixedHeight(corpus_status_label_->fontMetrics().height());
    right_layout->addWidget(bottom_spacer);

    auto* main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(8);
    main_layout->addLayout(left_layout, 1);
    main_layout->addWidget(v_line);
    main_layout->addLayout(right_layout, 1);

    //connect clicks to signals
    connect(add_document_button_, &QPushButton::clicked, this, &WorkspaceWidget::add_document_requested);
    connect(build_corpus_button_, &QPushButton::clicked, this, &WorkspaceWidget::build_corpus_requested);
    connect(document_list_, &QListWidget::currentRowChanged, this, &WorkspaceWidget::document_selected);

    reset();
}

void WorkspaceWidget::add_document(const QString& title) {
    document_list_->addItem(title);
}

void WorkspaceWidget::select_document(int row) {
    if(row>=0 && row< document_list_->count()){
        document_list_->setCurrentRow(row);
    }
    else{
        document_list_->clearSelection();
        document_list_->setCurrentRow(-1);
    }
}

void WorkspaceWidget::show_selected_document(const QString& title,
                                              const QString& text) {
    document_title_label_->setText(QString("<b>Selected Document - %1</b>").arg(title));
    document_text_->setPlainText(text);
}

void WorkspaceWidget::clear_selected_document() {
    document_title_label_->setText("<b>Select Document -</b>");
    document_text_->clear();
}

void WorkspaceWidget::set_corpus_status(const QString& text) {
    corpus_status_label_->setText(text);
}

void WorkspaceWidget::set_build_corpus_enabled(bool enabled) {
    build_corpus_button_->setEnabled(enabled);
}

void WorkspaceWidget::reset() {
    document_list_->clear();
    clear_selected_document();
    corpus_status_label_->setText("Corpus: <b>Not built</b>");
    build_corpus_button_->setEnabled(false);
}

}  // namespace aiws::gui
