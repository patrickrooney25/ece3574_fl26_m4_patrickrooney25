// Your M4 tests (specification, Section 6).
//int main() { return 0; }
#include <QTest>
#include <QCoreApplication>
#include <QLabel>
#include <QProcess>
#include <QListWidget>

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

        window.on_build_completed(nullptr, 0, false, "simulated crash");
        QCoreApplication::processEvents();

        bool has_failed=false;
        for (QLabel* lbl : window.findChildren<QLabel*>()){
            if(lbl&&lbl->text().contains("Build failed")){
                has_failed =true;
                break;
            }
        }
        QVERIFY2(has_failed, "failure was dropped and label never updated");
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
        QProcess client;
        client.start("/mnt/build/aiws_send", {"--socket", sock_path, "add", QString::fromStdString(rel_path)});
        QVERIFY(client.waitForFinished(3000));
        qDebug() << "aiws_send stderr:" << client.readAllStandardError();
        qDebug() << "aiws_send stdout:" << client.readAllStandardOutput();

        QCOMPARE(client.exitCode(), 0);

        QCoreApplication::processEvents();
        auto doc_list = window.findChild<QListWidget*>();
        QVERIFY(doc_list!=nullptr);
        QCOMPARE(doc_list->count(), 1);

        std::filesystem::remove(rel_path);
    }

    void test_reject_missing_file(){
        aiws::gui::MainWindow window;
        QString sock_path ="/tmp/aiws_test_reject.sock";
        window.set_ipc_socket_path(sock_path.toStdString());
        QTest::qWait(100);

        QProcess client;
        client.start("/mnt/build/aiws_send", {"--socket", sock_path, "add", "/tmp/missing_file.txt"});
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
        QProcess client;
        client.start("/mnt/build/aiws_send", {"--socket", "/tmp/aiws_unreachable.sock", "add", "content.txt"});
        QVERIFY(client.waitForFinished(4000));

        QCOMPARE(client.exitCode(), 2);
    }

    void test_shutdown(){
        std::string sock_path = "/mnt/build/aiws_test_shutdown.sock";

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
};

QTEST_MAIN(StudentTests)
#include "student_tests.moc"