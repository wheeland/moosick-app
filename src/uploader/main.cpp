#include <QApplication>

#include "fileview.hpp"
#include "mainwindow.hpp"
#include "connection.hpp"
#include "connectiondialog.hpp"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Connection connection;
    ConnectionDialog connectionDialog(&connection);

    MainWindow mainWindow(&connection);
    mainWindow.resize(1024, 768);

    QObject::connect(&connectionDialog, &ConnectionDialog::connected, &mainWindow, [&]() {
        connectionDialog.hide();
        mainWindow.show();
    }, Qt::QueuedConnection);

    QObject::connect(&connectionDialog, &ConnectionDialog::cancelled, &mainWindow, [&]() {
        app.quit();
    }, Qt::QueuedConnection);

    connectionDialog.show();

    return app.exec();
}
