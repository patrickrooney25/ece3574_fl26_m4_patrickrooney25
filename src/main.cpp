#include "aiws/gui/main_window.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <unistd.h>
#include <cstdlib>
#include <string>

// ExitCallback function used to confirm an Exit request.

static void confirm_exit_callback(QWidget* parent) {
    const auto reply = QMessageBox::question(parent, "Confirm Exit", "Are you sure you want to exit?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if(reply == QMessageBox::Yes) {
        QApplication::quit();
    }
}

static std::string default_socket_path(){
    const char* user = std::getenv("USER");
    if(user!=nullptr && *user!='\0'){
        return "/tmp/aiws-" +std::string(user) +".sock";
    }
    return "/tmp/aiws-" + std::to_string(getuid()) + ".sock";
}

int main(int argc, char* argv[]) {
    // Create the QApplication, construct/configure the MainWindow, register the ExitCallback, show the window, and run the event loop.
    QApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("Responsive AI Workspace");
    parser.addHelpOption();

    QCommandLineOption delay_option(QStringList() << "build-delay-ms", "Extra build time per document in milliseconds.", "N", "0");
    parser.addOption(delay_option);

    const std::string def_sock = default_socket_path();
    QCommandLineOption ipc_option(
        QStringList() << "ipc-socket",
        "Unix domain socket path for remote document adding.",
        "PATH",
        QString::fromStdString(def_sock)
    );
    parser.addOption(ipc_option);

    parser.process(app);

    int build_delay_ms =0;
    if(parser.isSet(delay_option)){
        bool ok=false;
        int parsed_val = parser.value(delay_option).toInt(&ok);
        if(ok && parsed_val>=0){
            build_delay_ms = parsed_val;
        }
    }

    std::string socket_path = def_sock;
    if(parser.isSet(ipc_option)){
        socket_path = parser.value(ipc_option).toStdString();
    }

    aiws::gui::MainWindow main_window;
    main_window.set_exit_callback(confirm_exit_callback);
    main_window.set_build_delay_ms(build_delay_ms);
    main_window.set_ipc_socket_path(socket_path);
    main_window.show();
    return app.exec();
}
