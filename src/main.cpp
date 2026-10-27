#include "aiws/gui/main_window.hpp"

#include <QApplication>
#include <QMessageBox>

// ExitCallback function used to confirm an Exit request.

static void confirm_exit_callback(QWidget* parent) {
    const auto reply = QMessageBox::question(parent, "Confirm Exit", "Are you sure you want to exit?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if(reply == QMessageBox::Yes) {
        QApplication::quit();
    }
}

int main(int argc, char* argv[]) {
    // Create the QApplication, construct/configure the MainWindow, register the ExitCallback, show the window, and run the event loop.
    QApplication app(argc, argv);

    aiws::gui::MainWindow main_window;
    main_window.set_exit_callback(confirm_exit_callback);
    main_window.show();
    return app.exec();
}
