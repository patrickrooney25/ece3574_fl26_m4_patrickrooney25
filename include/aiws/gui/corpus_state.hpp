#pragma once

#include <QString>
#include <cstdint>
#include <memory>
#include "aiws/processing_core.hpp"

namespace aiws::gui{
    
class MainWindow;

// used for state specific responses to workspace edits, build requests, resets, and worker build completions
class CorpusState{
public:
    virtual ~CorpusState()=default;

    virtual void on_enter(MainWindow& window)=0;

    virtual void on_document_added(MainWindow&) {}
    virtual void on_build_requested(MainWindow&) {}
    virtual void on_reset(MainWindow&);
    virtual void on_build_completed(MainWindow&, std::shared_ptr<ProcessingCore>,uint64_t, bool, const QString&){}

    //returns true when processing core matches current workspace
    virtual bool is_corpus_current() const noexcept {
        return false;
    }
};

//initial or invalidated corpus state
class NotBuiltState final : public CorpusState{
public:
    explicit NotBuiltState(bool rebuild_required=false);
    void on_enter(MainWindow& window) override;
    void on_build_requested(MainWindow& window) override;
private:
    bool rebuild_required_;
};

//active background build state
class BuildingState final : public CorpusState{
public:
    void on_enter(MainWindow& window) override;
    void on_document_added(MainWindow& window) override;
    void on_build_requested(MainWindow& window) override;
    void on_reset(MainWindow& window) override;
    void on_build_completed(MainWindow& window, std::shared_ptr<ProcessingCore> core, uint64_t gen, bool success, const QString& err) override;
};

//corpus up to date with current workspace
class ReadyState final : public CorpusState{
public: 
    void on_enter(MainWindow& window) override;
    void on_document_added(MainWindow& window) override;
    void on_build_requested(MainWindow& window) override;
    bool is_corpus_current() const noexcept override{
        return true;
    }
};

//state for a failed build generation
class BuildFailedState final : public CorpusState{
public:
    explicit BuildFailedState(QString error_message);
    void on_enter(MainWindow& window) override;
    void on_document_added(MainWindow& window) override;
    void on_build_requested(MainWindow& window) override;
private:
    QString error_message_;
};


}
