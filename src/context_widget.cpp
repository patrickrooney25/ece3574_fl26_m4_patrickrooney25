#include "aiws/gui/context_widget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QPlainTextEdit>

namespace aiws::gui {

ContextWidget::ContextWidget(QWidget* parent) : QWidget(parent) {
    // Construct and lay out the Context area, configure the token-budget, control, connect local events, and establish initial state.

    //UI components
    auto* section_title = new QLabel("<b>Context:</b>", this);

    context_budget_slider_ = new QSlider(Qt::Horizontal, this);
    context_budget_slider_->setRange(50, 1000);
    context_budget_slider_->setSingleStep(50);
    context_budget_slider_->setPageStep(50);
    context_budget_slider_->setValue(300);

    budget_value_label_ = new QLabel("300", this);
    build_context_button_ = new QPushButton("Build Context", this);
    
    //layout setup
    auto* top_layout = new QHBoxLayout();
    top_layout->addWidget(new QLabel("Token Budget:", this));
    top_layout->addWidget(context_budget_slider_, 1);
    top_layout->addWidget(budget_value_label_);
    top_layout->addWidget(build_context_button_);

    context_text_ = new QPlainTextEdit(this);
    context_text_->setReadOnly(true);
    context_text_->setPlaceholderText("Built context will appear here");
    context_text_->setStyleSheet("QPlainTextEdit { border: 1px solid #b0b0b0; background-color: white; }");

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(4, 4, 4, 4);
    main_layout->setSpacing(12);
    main_layout->addWidget(section_title);
    main_layout->addLayout(top_layout);
    main_layout->addWidget(context_text_, 1);

    //connect
    connect(context_budget_slider_, &QSlider::valueChanged, this, &ContextWidget::budget_changed);
    connect(build_context_button_, &QPushButton::clicked, this, &ContextWidget::request_context);

    reset();
}

int ContextWidget::token_budget() const {
    return context_budget_slider_->value();
}

void ContextWidget::set_build_context_enabled(bool enabled) {
    build_context_button_->setEnabled(enabled);
}

void ContextWidget::set_context(const std::vector<ContextItem>& items) {
    QString formatted_text;
    for(std::size_t i=0; i<items.size(); i++) {
        if(i > 0) {
            formatted_text.append("\n\n");
        }
        const ContextItem& item = items[i];

        QString chunk_str = QString::fromStdString(item.chunk_id);
        int hash_pos = chunk_str.indexOf('#');
        if(hash_pos != -1) {
            chunk_str = chunk_str.mid(hash_pos+1);
        }
        formatted_text.append(QString("Document: %1\nChunk: %2 Tokens: %3 Score: %4 Truncated: %5\n\n%6").arg(QString::fromStdString(item.document_id)).arg(chunk_str).arg(item.token_count).arg(item.score, 0, 'f', 6).arg(item.truncated ? "Yes" : "No").arg(QString::fromStdString(item.text)));
}
    context_text_->setPlainText(formatted_text);
}


void ContextWidget::clear_context() {
    context_text_->clear();
}

void ContextWidget::reset() {
    context_budget_slider_->setValue(300);
    budget_value_label_->setText("300");
    build_context_button_->setEnabled(false);
    clear_context();
}

void ContextWidget::request_context() {
    emit build_context_requested(token_budget());
}

void ContextWidget::budget_changed(int value) {
    budget_value_label_->setText(QString::number(value));
}

}  // namespace aiws::gui
