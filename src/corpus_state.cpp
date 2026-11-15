#include "aiws/gui/corpus_state.hpp"
#include "aiws/gui/main_window.hpp"

namespace aiws::gui{

void CorpusState::on_reset(MainWindow& window){
    window.perform_reset();
}

//not built

NotBuiltState::NotBuiltState(bool rebuild_required)
    : rebuild_required_(rebuild_required){}

void NotBuiltState::on_enter(MainWindow& window){
    if(rebuild_required_){
        window.update_corpus_ui("Corpus: <b>Rebuild Required</b>", QString(), true);
    }
    else{
        window.update_corpus_ui("Corpus: <b>Not Built</b>", "<b>Current status:</b> Ready", true);
    }
}

void NotBuiltState::on_build_requested(MainWindow& window){
    window.transition_to(std::make_unique<BuildingState>());
    window.start_build_job();
}

//building 

void BuildingState::on_enter(MainWindow& window){
    window.update_corpus_ui("Corpus: <b>Building... </b>", "<b>Current status:</b> Building Corpus...", false);
}

void BuildingState::on_document_added(MainWindow& window){
    //adding cancels build and invalidates corpus
    window.cancel_build_job();
    window.transition_to(std::make_unique<NotBuiltState>(true));
}

void BuildingState::on_build_requested(MainWindow& window){
    //building cancels build adn queues latest workspace
    window.cancel_build_job();
    window.start_build_job();
}

void BuildingState::on_reset(MainWindow& window){
    window.cancel_build_job();
    window.perform_reset();
}

void BuildingState::on_build_completed(MainWindow& window, std::shared_ptr<ProcessingCore> core, uint64_t gen, bool success, const QString& err){
    if(gen!=window.current_generation()){
        return;
    }
    if(!success){
        window.transition_to(std::make_unique<BuildFailedState>(err));
        return;
    }
    window.apply_built_core(std::move(core));
    window.transition_to(std::make_unique<ReadyState>());
}

//ready
void ReadyState::on_enter(MainWindow& window){
    window.update_corpus_ui(QString(), "<b>Current status:</b> Corpus built successfully.", true);
}

void ReadyState::on_document_added(MainWindow& window){
    window.transition_to(std::make_unique<NotBuiltState>(true));
}

void ReadyState::on_build_requested(MainWindow& window){
    window.transition_to(std::make_unique<BuildingState>());
    window.start_build_job();
}

//build failed
BuildFailedState::BuildFailedState(QString error_message)
    : error_message_(std::move(error_message)){}

void BuildFailedState::on_enter(MainWindow& window){
    window.update_corpus_ui("Corpus: <b>Build failed</b>", QString("<b>Current status:</b> Build failed: %1").arg(error_message_), false);
}

void BuildFailedState::on_document_added(MainWindow& window){
    window.transition_to(std::make_unique<NotBuiltState>(true));
}

void BuildFailedState::on_build_requested(MainWindow& window){
    window.transition_to(std::make_unique<BuildingState>());
    window.start_build_job();
}

}//namespace aiws::gui