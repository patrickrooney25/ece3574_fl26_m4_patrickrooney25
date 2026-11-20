// Your M4 tests (specification, Section 6).
//int main() { return 0; }
#include <QTest>
#include <QCoreApplication>
#include <QLabel>
#include <QProcess>
#include <QListWidget>
#include <QList>

#include <vector>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "aiws/gui/main_window.hpp"
#include "aiws/processing_core.hpp"

class StudentTests : public QObject{
    Q_OBJECT

private slots:

    void test_stale_build_dropped(){
        aiws::gui::MainWindow window;

        std::string doc1="doc_1.txt";
        {
            std::ofstream out(doc1);
            out << "doc 1 content";
        }
        window.add_document_file(QString::fromStdString(doc1));

        //snapshot
        auto snapshot_core =std::make_shared<aiws::ProcessingCore>();

        std::string doc2 = "doc_2.txt";
        {
            std::ofstream out(doc2);
            out << "doc 2 content.";
        }
        window.add_document_file(QString::fromStdString(doc2));

        window.on_build_completed(snapshot_core, 1, true,"");

        QCoreApplication::processEvents();

        bool has_ready=false;
        for (QLabel* lbl : window.findChildren<QLabel*>()){
            if(lbl && lbl->text().contains("Ready -")){
                has_ready=true;
                break;
            }
        }

        QVERIFY2(!has_ready, "stale build accepted");

        std::filesystem::remove(doc1);
        std::filesystem::remove(doc2);
    }

    void test_build_failure_delivered(){
        aiws::gui::MainWindow window;
        window.set_build_delay_ms(500);

        std::string doc = "fail_doc.txt";
        {
            std::ofstream out(doc);
            out << "test content";
        }
        QVERIFY(window.add_document_file(QString::fromStdString(doc)));

        QMetaObject::invokeMethod(&window, "build_corpus", Qt::DirectConnection);

        window.on_build_completed(nullptr, window.current_generation(), false, "simulated crash");
        QCoreApplication::processEvents();

        bool has_failed=false;
        for (QLabel* lbl : window.findChildren<QLabel*>()){
            if(lbl&&lbl->text().contains("Build failed")){
                has_failed =true;
                break;
            }
        }
        QVERIFY2(has_failed, "failure was dropped and label never updated");
        std::filesystem::remove(doc);
    }

    void test_valid_file_and_relative_path(){
        aiws::gui::MainWindow window;
        QString sock_path = "/tmp/aiws_test_valid.sock";
        window.set_ipc_socket_path(sock_path.toStdString());
        QTest::qWait(100);

        //using spaces
        std::string rel_path = "test doc.txt";
        {
            std::ofstream out(rel_path);
            out << "test content.";
        }

        //run aiws_send
        QString bin = QCoreApplication::applicationDirPath()+ "/aiws_send";
        QProcess client;
        client.start(bin, {"--socket", sock_path, "add", QString::fromStdString(rel_path)});
        QVERIFY(client.waitForFinished(3000));

        QCOMPARE(client.exitCode(), 0);

        auto doc_list =window.findChild<QListWidget*>();
        QVERIFY(doc_list!=nullptr);
        QTRY_COMPARE_WITH_TIMEOUT(doc_list->count(), 1, 2000);

        std::filesystem::remove(rel_path);
    }

    void test_reject_missing_file(){
        aiws::gui::MainWindow window;
        QString sock_path ="/tmp/aiws_test_reject.sock";
        window.set_ipc_socket_path(sock_path.toStdString());
        QTest::qWait(100);

        QString bin = QCoreApplication::applicationDirPath()+ "/aiws_send";
        QProcess client;
        client.start(bin, {"--socket", sock_path, "add", "/tmp/missing_file.txt"});
        QVERIFY(client.waitForFinished(3000));
        QCOMPARE(client.exitCode(), 1);

        QByteArray err = client.readAllStandardError();
        QVERIFY(!err.isEmpty());

        QCoreApplication::processEvents();
        auto doc_list = window.findChild<QListWidget*>();
        QVERIFY(doc_list!=nullptr);
        QCOMPARE(doc_list->count(), 0);
    }

    void test_unreachable_server(){
        QString bin = QCoreApplication::applicationDirPath()+ "/aiws_send";
        QProcess client;
        client.start(bin, {"--socket", "/tmp/aiws_unreachable.sock", "add", "content.txt"});
        QVERIFY(client.waitForFinished(4000));

        QCOMPARE(client.exitCode(), 2);
    }

    void test_shutdown(){
        std::string sock_path = "/tmp/aiws_test_shutdown.sock";

        auto start = std::chrono::steady_clock::now();
        {
            aiws::gui::MainWindow window;
            window.set_ipc_socket_path(sock_path);

            QTest::qWait(50);
            QVERIFY(std::filesystem::exists(sock_path));
        }

        auto elapsed_ms =std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();

        QVERIFY2(elapsed_ms<500, "shudown exceeds 500ms");
        QVERIFY2(!std::filesystem::exists(sock_path), "socket file was not uplinked");
    }

    void test_cancel_mid_build(){
        std::vector<std::string> docs = {"c1.txt", "c2.txt", "c3.txt", "c4.txt", "c5.txt", "c6.txt"};
        for (std::size_t i=0; i<docs.size(); ++i){
            std::ofstream out(docs[i]);
            out << "sample text: " << docs[i];
        }
        std::chrono::steady_clock::time_point start;
        {
            aiws::gui::MainWindow window;
            window.set_build_delay_ms(300);
            for (std::size_t i=0; i<docs.size(); ++i){
                QVERIFY(window.add_document_file(QString::fromStdString(docs[i])));
            }

            QMetaObject::invokeMethod(&window, "build_corpus", Qt::DirectConnection);
            QTest::qWait(100);

            start = std::chrono::steady_clock::now();

            QMetaObject::invokeMethod(&window, "reset_application", Qt::DirectConnection);
            QCoreApplication::processEvents();

            bool has_not_built = false;
            QList<QLabel*> labels = window.findChildren<QLabel*>();
            for (int i=0; i<labels.size(); ++i){
                if(labels[i]&&labels[i]->text().contains("Not Built")){
                    has_not_built=true;
                    break;
                }
            }
            QVERIFY2(has_not_built, "Reset mid build did not transition to Not Built");
        }

        auto elapsed_ms =std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();

        QVERIFY2(elapsed_ms<600, "Build was not cancelled within 1 doc delay");
        for (std::size_t i=0; i<docs.size(); ++i){
            std::filesystem::remove(docs[i]);
        }
    }

    void test_transitions_and_rebuild_during_build(){
        aiws::gui::MainWindow window;
        window.set_build_delay_ms(150);

        std::string d1 = "r1.txt";
        std::string d2 = "r2.txt";
        {
            std::ofstream out1(d1);
            out1 << "doc1 text";
            std::ofstream out2(d2);
            out2 << "doc 2 text";
        }

        QVERIFY(window.add_document_file(QString::fromStdString(d1)));
        QMetaObject::invokeMethod(&window, "build_corpus", Qt::DirectConnection);
        QTest::qWait(50);

        QVERIFY(window.add_document_file(QString::fromStdString(d2)));
        QMetaObject::invokeMethod(&window, "build_corpus", Qt::DirectConnection);

        QTest::qWait(500);
        QCoreApplication::processEvents();

        bool has_ready=false;
        QList<QLabel*> labels = window.findChildren<QLabel*>();
        for (int i=0; i<labels.size(); ++i){
            if (labels[i]&&labels[i]->text().contains("Ready -")){
                has_ready=true;
                break;
            }
        }
        QVERIFY2(has_ready, "Second build did not complete into Ready State");

        std::filesystem::remove(d1);
        std::filesystem::remove(d2);
    }
};

void test_gui_responsiveness_and_exit_during_build(){
    std::vector<std::string> docs = {"e1.txt", "e2.txt", "e3.txt", "e4.txt"};
    for (std::size_t i=0; i<docs.size(); ++i){
        std::ofstream out(docs[i]);
        out << "content: " << docs[i];
    }

    auto start=std::chrono::steady_clock::now();
    {
        aiws::gui::MainWindow window;
        window.set_build_delay_ms(300);
        for(std::size_t i=0; i<docs.size(); ++i){
            QVERIFY(window.add_document_file(QString::fromStdString(docs[i])));
        }

        QMetaObject::invokeMethod(&window, "build_corpus", Qt::DirectConnection);
        QTest::qWait(50);

        auto doc_list = window.findChild<QListWidget*>();
        QVERIFY(doc_list!=nullptr);
        doc_list->setCurrentRow(1);
        QCoreApplication::processEvents();
        QCOMPARE(doc_list->currentRow(), 1);

        start =std::chrono::steady_clock::now();
        window.close();
    }
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    QVERIFY2(elapsed_ms<600, "Exist during build exceeds 1 doc delay");

    for (std::size_t i=0; i<docs.size(); ++i){
        std::filesystem::remove(docs[i]);
    }
}

QTEST_MAIN(StudentTests)
#include "student_tests.moc"