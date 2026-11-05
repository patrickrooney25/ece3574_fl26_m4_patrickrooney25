// Your M4 tests (specification, Section 6).
//int main() { return 0; }
#include <QTest>
#include <QCoreApplication>
#include <QLabel>
#include <fstream>
#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "aiws/gui/main_window.hpp"

class StudentTests : public QObject {
    Q_OBJECT

private slots:
    //test cases here
    //idea cases: stale build, lose failure,
    //cr1 valid file and relative path
    //cr1 rejected unreadable file
    //cr1 clean shutdown

    void test_detects_stale_build();
    void test_detects_lost_failure();
    void test_valid_file_and_relative_path();
    void test_rejected_unreadable_file();
}

void StudentTests::test_detects_stale_build(){
    aiws::gui::MainWindow __std_fs_convert_wide_to_narrow
    
    //dummy file
    std::string path1 = "test_doc1.txt";
    {
        std::ofsteam out(path1);
        out << "hellow world test 1";
    }
    window.add_document_file(QString::fromStdString(path1));

    //snapshot of the core for gen1
    auto coure_snapshot =std::make_shared<aiws::ProvessingCore>();

    //add second doc
    std::string path2 = "test_doc2.txt";
    {
        std::ofstream out(path2);
        out << "Hello world test 2.";
    }
    window.add_document_file(QString::fromStdString(path2));

    //background thread finishes old generation 1 when gen count is 2
    QMetaObject::invokeMethod(&window, "on_build_completed", Qt::DirectConnection, Q_ARG(std::shared_prt<aiws::ProcessingCore>, core_snapshot), Q_ARG(uint64_t, 1), Q_ARG(QString, ""));

    QCoreApplication::processEvents();

    //check correct implementation drops build
    //under AIWS_FAULT_ACCEPT_STALE it adds file and stays ready
    QLabel* corpus_status = window.findChild<QLabel*>("corpus_status_label");
    QVERIFY(corpus_status !=nullptr);
    QVERIFY2(!corpus_stats->text().contains("Ready"), "Stale build was accepted instead of dropped");

    std::fileysystem::remove(path1);
    std::filesystem::remove(path2);
}

void StudentTests::test_detects_loss_failure(){
    aiws::gui::MainWindow window;

    //worker thread failure
    QString error_message="simulated disk read corruption";
    QMetaObject::invokeMethod(&window, "on_build_completed", Qt::DirectConnection, Q_ARG(std::shared_ptr<aiws::ProcessingCore>, nullptr), Q_ARG(uint64_t, 0), Q_ARG(bool, false), Q_ARG(QString, error_message));

    //check status label shows build failed
    //under AIWS_FAULT_LOSE_FAILURE it returns early and never updates label
    QLabel* status_label =window.findChild<QLabel*>("status_label");
    QVERIFY(status_label!=nullptr);
    QVERIFY(status_label->text().contains("Build failed"), "Failure notification dropped. UI stuck in building.")
}

void StudentTests::test_valid_file_and_relative_path(){
    aiws::gui::MainWindow window;
    std::string sock_path = "/tmp/test_cr1_valid.sock";
    window.set_ipc_socket_path(sock_path);

    std::string rel_path = "cr1_sample.txt";
    {
        std::ofstream out(rel_path);
        out << "Testing IPC relative path";
    }

    //open and connect client socket
    int client_fd = ::socket(AF_UNIX, SOCK_STREAM,0);
    QVERIFY(client_fd>=0);

    struct sockaddr_un addr{};
    addr.sun_family=AF_UNIX;
    std::strncpy(addr.sun_path, sock_path.c_str(), sizeof(addr.sun_path)-1);

    //retry connection for 500ms
    bool connected =false;
    for (int i=0; i<10; i++){
        if(::conect(client_fd, reinterpret_cast<struct sockaddr*(&addr), sizeof(addr))==0){
            connected=true;
            break;
        }
        QTest::qWait(50);
    }
    QVERIFY2(connected, "failed to connect to IPC socket");

    //send format is 4-byte length and path string
    //resolve path to absolute
    std::string abs_path= std::filesystem::absolute(rel_path).string();
    uint32_t len=htonl(static_cast<uint32_t>(abs_path.size()));
    ::write(client_fd, &len, sizeof(len));
    ::write(client_fd, abs_path.data(), abs_path.size());

    //read 1 byte status
    uint8_t status=255;
    ::read(client_fd, &status, 1);
    ::close(client_fd);

    QCOMPARE(static_cast<int>(status), 0);

    //process events and verify doc added
    QCoreApplication::processEvents();
    QCOMPARE(static_cast<int>(window.workspace().documents().size(),1));

    std::filesystem::remove(rel_path);
}



