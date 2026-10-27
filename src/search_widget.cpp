#include "aiws/gui/search_widget.hpp"
#include "aiws/gui/search_result_model.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QVBoxLayout>

namespace aiws::gui {

SearchWidget::SearchWidget(QWidget* parent) : QWidget(parent) {
    // TODO: Construct and lay out the Query, Search Results, and Selected Result
    // areas; configure the table/model; connect local events; establish initial state.

    //query and controls
    auto* query_label = new QLabel("<b>Query:</b>", this);
    query_edit_ = new QLineEdit(this);
    query_edit_->setPlaceholderText("Search the current corpus");

    result_count_spin_ = new QSpinBox(this);
    result_count_spin_->setRange(1, 100);
    result_count_spin_->setValue(3);

    search_button_ = new QPushButton("Search", this);

    // Layout
    auto* top_layout = new QHBoxLayout();
    top_layout->addWidget(query_label);
    top_layout->addWidget(query_edit_, 1);
    top_layout->addWidget(new QLabel("Results:", this));
    top_layout->addWidget(result_count_spin_);
    top_layout->addWidget(search_button_);

    //search results
    result_model_ = new SearchResultModel(this);
    results_view_ = new QTableView(this);
    results_view_->setModel(result_model_);
    results_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    results_view_->setSelectionMode(QAbstractItemView::SingleSelection);
    results_view_->horizontalHeader()->setStretchLastSection(true);

    results_view_->setStyleSheet("QTableView { border: 1px solid #b0b0b0; background-color: white; }" "QHeaderView::section { background-color: #f0f0f0; padding: 4px; border: 1px solid #c0c0c0; font-weight: bold; }");

    //results row
    auto* selected_result_header = new QHBoxLayout();
    auto* selected_result_title = new QLabel("<b>Selected Result:</b>", this);
    view_source_button_ = new QPushButton("View Source", this);

    selected_result_header->addWidget(selected_result_title);
    selected_result_header->addStretch();
    selected_result_header->addWidget(view_source_button_);

    //result text detail
    selected_result_text_ = new QPlainTextEdit(this);
    selected_result_text_->setReadOnly(true);
    selected_result_text_->setPlaceholderText("Select a result to inspect its details.");
    selected_result_text_->setStyleSheet("QPlainTextEdit { border: 1px solid #b0b0b0; background-color: white; }");

    //combine
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(6);
    main_layout->addLayout(top_layout);
    main_layout->addWidget(results_view_, 1);
    main_layout->addLayout(selected_result_header);
    main_layout->addWidget(selected_result_text_, 1);


    //signals
    connect(search_button_, &QPushButton::clicked, this, &SearchWidget::request_search);
    connect(query_edit_, &QLineEdit::textChanged, this, &SearchWidget::query_changed);
    connect(view_source_button_, &QPushButton::clicked, this, &SearchWidget::request_source_document);
    connect(results_view_->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SearchWidget::show_selected_result);

    reset();
}

QString SearchWidget::query() const {
    return query_edit_->text();
}

int SearchWidget::result_count() const {
    return result_count_spin_->value();
}

bool SearchWidget::has_query() const {
    return !query_edit_->text().trimmed().isEmpty();
}

void SearchWidget::set_search_enabled(bool enabled) {
    search_button_->setEnabled(enabled);
}

void SearchWidget::set_results(std::vector<SearchResult> results) {
    result_model_->set_results(std::move(results));
    results_view_->selectionModel()->clearSelection();
    selected_result_text_->clear();
    view_source_button_->setEnabled(false);
    emit selected_chunk_changed(QString());
}

void SearchWidget::clear_results() {
    result_model_->clear();
    results_view_->selectionModel()->clearSelection();
    selected_result_text_->clear();
    view_source_button_->setEnabled(false);
    emit selected_chunk_changed(QString());
}

void SearchWidget::reset() {
    query_edit_->clear();
    result_count_spin_->setValue(3);
    search_button_->setEnabled(false);
    clear_results();
}

void SearchWidget::request_search() {
    emit search_requested(query(), result_count());
}

void SearchWidget::show_selected_result() {
    const auto indexes = results_view_->selectionModel()->selectedRows();
    if(indexes.isEmpty()) {
        selected_result_text_->clear();
        view_source_button_->setEnabled(false);
        emit selected_chunk_changed(QString());
        return;
    }

    const int selected_row = indexes.first().row();
    const SearchResult* result = result_model_->result_at(selected_row);
    if(result != nullptr) {
        selected_result_text_->setPlainText(QString::fromStdString(result->text));
        view_source_button_->setEnabled(true);
        emit selected_chunk_changed(QString::fromStdString(result->chunk_id));
    }
    else {
        selected_result_text_->clear();
        view_source_button_->setEnabled(false);
        emit selected_chunk_changed(QString());
    }
}

void SearchWidget::request_source_document() {
    const auto indexes = results_view_->selectionModel()->selectedRows();
    if(indexes.isEmpty()) {
        return;
    }

    const int selected_row = indexes.first().row();
    const SearchResult* result = result_model_->result_at(selected_row);
    if(result != nullptr) {
        emit source_document_requested(QString::fromStdString(result->document_id));
    }
}

}  // namespace aiws::gui
