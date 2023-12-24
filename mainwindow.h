#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDropEvent>
#include <QProgressDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void addFile(const QString& str);
    void addFiles(const std::vector<const char*>& files);
    void addPath(const QString& path);
    void exportMaterials(const QString& folder);
    void updateEnabledWidgets();

public slots:
   void getCdbPath();
   void setCdbPath(const QUrl& url);
   void getMaterialPath();
   void getMaterialFolder();
   void addUrls(const QList<QUrl>& urls);
   void getExportPath();
   void showAbout();
   void showAdd();
   void removePaths();
   void clearPaths();
   void updateExport();
   void getCrcFile();
   void getCrcPath(const QUrl& url);
   void getMaterialId();

private:
    Ui::MainWindow *ui;
    QProgressDialog progressDialog;
};
#endif // MAINWINDOW_H
