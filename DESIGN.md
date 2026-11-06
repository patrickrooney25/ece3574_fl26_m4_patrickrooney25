# M4 Decision Record

See specification Section 8.

**Threads and Ownership**

Three threads were used in the development of m4. The main GUI thread has exclusive ownership of the UI in MainWindow and all of its widgets (WorkspaceWidget, SearchWidget, TermInspectorWidget, and ContextWidget). This keeps all widget allocations, signals, and visual updates strictly on the main GUI thread. When corpus indexing is initiated through MainWindow::build_corpus(), a bakcground worker thread is made using std::thread. The worker thread uses a snapshot of the workspace and injects artificial delay (build_delay_ms_). It then executes ProcessingCore::rebuild() and communicates completion onto the main thread using QMetaObject::invokeMethod. start_ipc_server() also runs an independent thread that polls an AF_UNIX streaming socket for client commands without frezzing the GUI event loop.

The ownership lifecycle of a built corpus begins inside the bakground worker thread. This thread allocates the new structure as std::make_shared<aiws::ProcessingCore>(). This pointer is passed through a queued event into MainWindow::on_build_completed() when indexing is successful. The handler then confirms that the generation is current and transitions ownership to MainWindow::processing_core where the previous corpus is destroyed. The active corpus remains owned by MainWindow until overwritten by a valid build, cleared by reset_application(), or destroyed when MainWindow goes out of scope on an exit.

All mutable data that is accessible accross threads is bounded and synchronized to comply with constraint 5. The generation counter (current_generation_) is defined as a std::atomic<uint64_t> and is incremented on the GUI thread and read across thread boundaries without a race condition. The server status flag (ipc_running_) is a std::atomic<bool> and written by the main thread during shutdown and read by ipc_thread_ as the termination condition. The workspace instance is also never shared. MainWindow::build_corpus() copies the workspace on the GUI thread before thread creation. This allows the worker thread ot use its own private snapshot while the GUI thread continues to modify the live workspace. Both the worker thread and ipc thread are joined before launching replacements or destroying MainWindow to ensure no detatched threads exist.

**Judging Finished Build Valid**

A completed background build is accepted and allowed to update the corpus only if it reflects the current workspace stat to satisfy requirement R2. The invariant is maintained by tracking generation using current_generation_. When an event like adding a document to the GUI invalidates the corpus, current_generation_ is incremented immediately on the GUI thread. The generation number is captured and passed to the worker thread when build_corpus() initiates background indexing. on_build_completed then compares the workers generation number with the value of current_generation_ when the worker finishes. If the workspace changed or reset during indexing the values are different and causes the method to discard the build without changing the processing core or UI labels. If the generations match, the build and status labels update to show the new workspace.

**Changes Since Checkpoint**

A few aspects of my design changed since submitting my design note. In my initial design, I planned to allocate the core inside the inside the worker thread as a std::unique_ptr<ProcessingCore> and move the pointer through the Qt event queue into a member on MainWindow. Moving move only types like unique pointers got buggy so I changed on_build_complete() to accept std::shared_ptr<aiws::ProcessingCore>. This kept ownership transfer safe while fitting Qt's queued event system cleanly. I also kept MainWindow holding processing core directly as a value member rather than managing pointer dereferencing or reset calls.

**CR-1 Impact**

CR-1 added external document ingestion using POSIX Unix domain stream sockets. In MainWindow, I added set_ipc_socket_path(), start_ipc_server(), stop_ipc_server(), and ipc_server_loop() to manage the socket lifecycle on a seperate thread to keep socket handling off the GUI thread.

tools/aiws_send was created as a binary to parse --socket and add arguments, connect to the socket with connection retries and read/write timeouts, and handle message framing. The wire protocol is length prefixed. The client sends a 4-byte network endien integer (htonl) for the path length followed by the UTF-8 file path bytes. Th server reads this on the worker thread and cheks if the file exists and is readable. It then replies immediately without waiting for the GUI thread. The response format is 1-byte status (0 for success and 1 for failure) followed by a 4 bute network endian integer for the error messegae length with the error string attached. If it is valid the document addition is queued to the GUI thread with QMetaObject::invokeMethod with Qt::QueuedConnection. The rest of the app remained untouched because IPC rputes through add_document_file() reuisng the existing document addition and corpus invalidation flow.

**Fault Switches**

Both fault macros were placed in on_build_completed() which evaluates completion lgoic to satisfy the section 6 requirements. AIWS_FAULT_ACCEPT_STALE surrounds the generation validation check. When it is defined, the stale build bypasses the guard and overwrites the newer workspace stats which violates R2. This fault is detected by test_stale_build_dropped(). This triggers an intervening workspace mutation and asserts that the UI label never displays "Ready - " for a stale generation. AIWS_FAUL_LOSE_FAILURE arrounds the failure dispatch pathway. When this is defined, the build errors are ignored and the UI is permanently left in the building state violating R3. This is detected by test_build_failure_delivered(). This injects a failed build and verifies that the status label updates to "Build failed".

**AI Disclosure**

Gemini was used slightly to debug my code. My test cases were having issues passing. I origionally used QDebug in test_reject_missing_file and QVerify(!client.readAllStandardError().isEmpty()) falsely returned the test as failed. 



**Pair Programming Log**
Partners Name: Res Sharpe
Date: 11/05/2026

* Turn 1 (partner drives - his code) 12:47 - 1:02- Res read over m4 requirements, Res coded bool MainWindow::add_document_file() in mainWindow.cpp. I helped guide Res on his implementation and gave feedback on how to properly edit void MainWindow::add_document() to meet project requirements.
* Turn 2 (driving - his code) 1:02 - 1:17 - I began outlining a default socket path in aiws_send.cpp and worked with Res on editing his header file to account for a thread based design.
* Turn 3 (partner drives - my code) 1:17-1:32 - Res looked over my aiws_send.cpp and looked at architecture and possible bug concerns. I was not able to give any real feedback because he did not edit my code, I mostly explained how the coded worked to him.
* Turn 4 (driving - his code) 1:32- 1:47 - I started building a test case, void StudentTests::valid_file_and_relative_path(), to test CR-1 implementation. Res gave feedback on ways to test.

Comments: This assignment was difficult as my partner agreed to meet and work on it before starting his own m4 project. With that being said, he was still helpful in implementing tests for CR-1, and I believe I was helpful in his understanding of CR-1 as a whole. We worked best when we both thought out loud and treated the assignment as both of us working on the same code with the same responsibility, except one person was just typing.
