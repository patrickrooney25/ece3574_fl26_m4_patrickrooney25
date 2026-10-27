#include "aiws/gui/term_inspector_widget.hpp"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace aiws::gui {

TermInspectorWidget::TermInspectorWidget(QWidget* parent) : QWidget(parent) {
    // Construct and lay out the Term Inspector, connect local events, and establish initial state.
    
    //UI components
    auto* section_title = new QLabel("<b>Term Inspector:</b>", this);

    auto* input_layout = new QHBoxLayout();
    input_layout->addWidget(new QLabel("Term:", this));
    term_edit_ = new QLineEdit(this);
    term_edit_->setPlaceholderText("Enter a term");
    inspect_button_ = new QPushButton("Inspect", this);

    input_layout->addWidget(term_edit_, 1);
    input_layout->addWidget(inspect_button_);

    document_frequency_value_ = new QLabel("<b>&mdash;</b>", this);
    chunk_frequency_value_ = new QLabel("<b>&mdash;</b>", this);

    auto* stats_layout = new QHBoxLayout();
    stats_layout->addWidget(new QLabel("Document Frequency:", this));
    stats_layout->addWidget(document_frequency_value_);
    stats_layout->addSpacing(20);
    stats_layout->addWidget(new QLabel("Selected Chunk TF:", this));
    stats_layout->addWidget(chunk_frequency_value_);
    stats_layout->addStretch();

    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->addWidget(section_title);
    main_layout->addLayout(input_layout);
    main_layout->addLayout(stats_layout);
    main_layout->addStretch();

    connect(inspect_button_, &QPushButton::clicked, this, &TermInspectorWidget::request_inspection);

    reset();
}

void TermInspectorWidget::set_inspection_enabled(bool enabled) {
    inspect_button_->setEnabled(enabled);
}

void TermInspectorWidget::set_selected_chunk(const QString& chunk_id) {
    selected_chunk_id_ = chunk_id;
}

void TermInspectorWidget::set_statistics(std::size_t document_frequency,
                                         bool has_chunk_frequency,
                                         std::size_t chunk_frequency) {
    document_frequency_value_->setText(QString("<b>%1</b>").arg(document_frequency));
    if (has_chunk_frequency) {
        chunk_frequency_value_->setText(QString("<b>%1</b>").arg(chunk_frequency));
    } else {
        chunk_frequency_value_->setText("<b>&mdash;</b>");
    }
}

void TermInspectorWidget::clear_statistics() {
    document_frequency_value_->setText("<b>&mdash;</b>");
    chunk_frequency_value_->setText("<b>&mdash;</b>");
}

void TermInspectorWidget::reset() {
    term_edit_->clear();
    clear_statistics();
    selected_chunk_id_.clear();
    inspect_button_->setEnabled(false);
}

void TermInspectorWidget::request_inspection() {
    const QString term = term_edit_->text().trimmed();
    if (!term.isEmpty()) {
        emit term_inspection_requested(term, selected_chunk_id_);
    }
}

}  // namespace aiws::gui
