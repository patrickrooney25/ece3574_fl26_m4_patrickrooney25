#include "aiws/gui/search_result_model.hpp"

#include <utility>

namespace aiws::gui {

SearchResultModel::SearchResultModel(QObject* parent)
    : QAbstractTableModel(parent) {
}

int SearchResultModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(results_.size());
}

int SearchResultModel::columnCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        return 0;
    }
    return 5;  //rank, doc, chunk, score, text
}

QVariant SearchResultModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid()|| role!=Qt::DisplayRole) {
        return {};
    }

    const auto& item = results_[static_cast<std::size_t>(index.row())];
    switch(index.column()){
        case 0: //rank
            return static_cast<qlonglong>(index.row() + 1);
        case 1://doc
            return QString::fromStdString(item.document_id);
        case 2: //chunk
            return static_cast<qlonglong>(item.chunk_sequence);
        case 3://score
            return QString::number(item.score, 'f', 6);
        case 4://text
            return QString::fromStdString(item.text);
        default:
            return {};
    }
}

QVariant SearchResultModel::headerData(int section,
                                       Qt::Orientation orientation,
                                       int role) const {
    if(role!=Qt::DisplayRole || orientation!=Qt::Horizontal) {
        return {};
    }
    switch(section) {
        case 0:
            return "Rank";
        case 1:
            return "Document";
        case 2:
            return "Chunk";
        case 3:
            return "Score";
        case 4:
            return "Text";
        default:
            return {};
    }
}

void SearchResultModel::set_results(std::vector<SearchResult> results) {
    beginResetModel();
    results_ =std::move(results);
    endResetModel();
}

void SearchResultModel::clear() {
    beginResetModel();
    results_.clear();
    endResetModel();
}

const SearchResult* SearchResultModel::result_at(int row) const noexcept {
    if(row <0 || static_cast<std::size_t>(row) >= results_.size()){
        return nullptr;
    }
    return &results_[static_cast<std::size_t>(row)];
}

}  // namespace aiws::gui
