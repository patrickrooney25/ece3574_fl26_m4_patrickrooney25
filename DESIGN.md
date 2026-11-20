# M5 DESIGN.md

**1. M4 Fixes**
None.

**2. Threads and shared data**

The GUI thread in MainWindow and the Qt event loop owns all widgets, workspace_, processing_core_, current_generation_, and state_. The worker thread in MainWindow::worker_loop is spawned once in the mainWindow constructor, blocks on coord_.cv when idle without busy waiting, and joins in closeEvent() in the mainWindow destructor using stop_worker(). The ipc server thread in MainWindow::ipc_server_loop polls the UNIX domain socket adn queues add_document_file() on the GUI thread using Qt::QueuedConnection.

All mutable data shared between the GUI and the worker thread lives in BuildCoordinator. pending_workspace, job_generation, build_delay_ms, has_job, and shutdown are accessed by the GUI and worker threads and are protected using std::mutex coord_.mutex and signaled using std::condition_variable coord_.cv. This is thread safe since all multi field job handoffs and shutdown updates happen atomically under the same mutex lock without torn reads or lost wakeups. std::atomic<bool> coord_.cancel_requested is written by the GUI thread in cancel_build_job() and stop_worker() and read by the worker in worker_loop() giving data race free cancellation visibility. std::atomic<bool> ipc_running_ is written by the GUI thread in start_ipc_server() and stop_ipc_server() and read by the IPC thread in ipc_server_loop() to allow race free server shutdown. std::atomic<uint64_t> current_generation_ is incremented on the GUI thread in add_document(), add_document_file(), and perform_reset() and read in start_build_job and BuildingState::on_build_completed() to reject stale completions safely.

**3. Corpus states**
All states live exclusivley on the GUI thread. NotBuiltState(bool rebuild_required) represents an unbuilt or invalidated workspace where is_corpus_current() is false and the search, term inspector, and context are disabled. BuildingState represents an active background build where is_corpus_current() is false. ReadyState represents a current and searchable corpus where is_corpus_current() is true. BuildFailedState(QString err) represents a failed build where is_corpus_current() is false.

Corpus State     |                         Event                               | Next State
---------------------------------------------------------------------------------------------
NotBuiltState    | on_build_requested and start_build_job                      | BuildingState
NotBuiltState    | on_reset and perform_reset                                  | NotBuiltState(false)
BuildingState    | on_build_completed with matching gen and success            | ReadyState
BuildingState    | on_build_completed with matching gen and failure            | BuiledFailedState
BuildingState    | on_build_completed with stale generation                    | BuildingState
BuildingState    | on_document_added and cancel_build_job                      | NotBuiltState(true)
BuildingState    | on_build_requested and cancel_build_job and start_build_job | BuildingState
BuildingState    | on_reset and cancel_build_job and perform_reset             | NotBuiltState(false)
ReadyState       | on_document_added with clear outputs                        | NotBuiltState(true)
ReadyState       | on_build_requested or on_reset                              | BuildingState or NotBuiltState(false)
BuildFailedState | on_document_added or on_build_requested or on_reset         | NotBuiltState(true) or BuildingState or NotBuiltState(false)

**4. Event trace**

This is tracing a remote add mid build. MainWindow::build_corpus() calls NotBuiltState::on_build_requested() on the GUI thread. This transitions to BuildingState and triggers start_build_job() to take a snapshot of workspace_, set has_job=true, notify using coord_.cv. On the worker thread, the worker_loop() then wakes up from coord_.cv.wait(), moves pending_workspace, and waits per document on coord_.cv.wait_for(). On the IPC server thread, the ipc server loop receives add from aiws_send and queues add_document_file() using Qt::QueuedConnection. The GUI thread then calls add_document_file() which increments current_generation_ and calls BuildingState::on_document_added(). This invokes cancel_build_job() with has_job=false and cancel_requested=true and calls coord_.cv.notify_all() and transitions to NotBuiltState(true). coord_.cv.wait_for() wakes imeediately on the worker thread, sees canceled_requested is true, and breaks early without posting a completion and waits on coord_.cv.

**5. Design decisions**
A user may request a build during a build. BuildingState::on_build_requested() calls canel_build_job() to stop the running or queued build and start_build_job() to enqueue the newest workspace snapshot. When a remote add arrives during a build, add_document_file() increments current_generation_ and BuildingState::on_document_added() calls cancel_build_job(). This transitions immediately to NotBuiltState(true) with rebuild required. The smallest shared data my threads must share is BuildCoordinator with mutex, cv, pending_workespace, job_generation, build_delay_ms, has_job, shutdown, and cancel_requested, and ipc_running_. This is because no smaller set of data can pass workspace snapshots, wake or stop the worker thread without polling, and stop queued or active builds safely.

**6. Fault switches**
AIWS_FAULT_NO_CANCEL in MainWindow::worker_loop() skips checking coord_.cancel_requested and is caught in testing by test_cancel_mid_build() with elapsed_ms<600. AIWS_FAULT_SILENT_SHUTDOWN in MainWindow::stopworker() omits coord_.cv.notify_all() and is caught in the tests by test_shutdown() timing out on the MainWindow destructor.

**AI disclosure**
no ai was used in this project.

