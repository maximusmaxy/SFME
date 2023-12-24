#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QLabel>
#include <QFileDialog>
#include <QErrorMessage>
#include <QMimeData>
#include <QProgressDialog>
#include <QInputDialog>
#include <QtConcurrent>
#include <QDesktopServices>
#include <QShortcut>

#include <filesystem>
#include <spanstream>
#include <format>

#include <paths.h>
#include <types.h>
#include <cdb.h>
#include <bsa.h>
#include <mat.h>
#include <crc.h>

#include "aboutwindow.h"

std::filesystem::path GetStarfieldPath() {
    const wchar_t* sfRegSubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 1716740";
    return GetRegistryPath(sfRegSubkey, L"InstallLocation");
}

void Wait(int ms) {
    QTime waitTime = QTime::currentTime().addMSecs(ms);
    while (QTime::currentTime() < waitTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QErrorMessage::qtHandler()->setModal(true);

    const auto folderIcon = QStyle::SP_DialogOpenButton;

    connect(ui->buttonCdb, &QPushButton::clicked, this, &MainWindow::getCdbPath);
    ui->buttonCdb->setIcon(ui->buttonCdb->style()->standardIcon(folderIcon));

    connect(ui->buttonExport, &QPushButton::clicked, this, &MainWindow::getExportPath);

    connect(ui->listPaths, &PathListWidget::urlsAdded, this, &MainWindow::addUrls);
    ui->listPaths->setAcceptDrops(true);
    connect(ui->lineCdbPath, &TextEntryWidget::urlAdded, this, &MainWindow::setCdbPath);
    ui->lineCdbPath->setAcceptDrops(true);

    connect(ui->actionOpenMaterials, &QAction::triggered, this, &MainWindow::getCdbPath);
    connect(ui->actionOpenPaths, &QAction::triggered, this, &MainWindow::getMaterialPath);
    connect(ui->actionOpenFolder, &QAction::triggered, this, &MainWindow::getMaterialFolder);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);

    connect(ui->buttonAdd, &QPushButton::clicked, this, &MainWindow::showAdd);
    connect(ui->buttonRemove, &QPushButton::clicked, this, &MainWindow::removePaths);
    connect(ui->buttonClear, &QPushButton::clicked, this, &MainWindow::clearPaths);
    connect(ui->lineCdbPath, &QLineEdit::textChanged, this, &MainWindow::updateExport);

    auto deleteShortcut = new QShortcut(this);
    deleteShortcut->setKey(QKeySequence(Qt::Key_Delete));
    connect(deleteShortcut, &QShortcut::activated, this, &MainWindow::removePaths);

    progressDialog.cancel();
    const QSize progressSize(800, progressDialog.height());
    progressDialog.setMinimumSize(progressSize);
    progressDialog.setMaximumSize(progressSize);

    ui->lineMaterialPath->acceptedExtensions = {".mat"};
    connect(ui->lineMaterialPath, &TextEntryWidget::urlAdded, this, &MainWindow::getCrcPath);
    connect(ui->lineMaterialPath, &TextEntryWidget::textChanged, this, &MainWindow::getMaterialId);
    connect(ui->buttonCrc, &QPushButton::clicked, this, &MainWindow::getCrcFile);
    ui->buttonCrc->setIcon(ui->buttonCrc->style()->standardIcon(folderIcon));

    ui->lineCdbPath->acceptedExtensions = {".ba2", ".cdb"};
    const auto sfPath = GetStarfieldPath();
    if (!sfPath.empty()) {
        const auto cdbPath = std::filesystem::path(sfPath) / "Data\\Materials\\materialsbeta.cdb";
        if (std::filesystem::exists(cdbPath))
            ui->lineCdbPath->setText(QString::fromStdWString(cdbPath.native()));
        else {
            const auto ba2Path = std::filesystem::path(sfPath) / "Data\\Starfield - Materials.ba2";
            if (std::filesystem::exists(ba2Path))
                ui->lineCdbPath->setText(QString::fromStdWString(ba2Path.native()));
        }
    }

    updateEnabledWidgets();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showAbout()
{
    auto aboutWindow = new AboutWindow(this);
    aboutWindow->setModal(true);
    aboutWindow->show();
    aboutWindow->raise();
    aboutWindow->activateWindow();
}

void MainWindow::addPath(const QString& path) {
    if (ui->listPaths->isEmpty) {
        ui->listPaths->clear();
        ui->listPaths->isEmpty = false;
    }
    ui->listPaths->addItem(path);
}

void MainWindow::getCdbPath()
{
    const auto sfPath = GetStarfieldPath();
    const QString dir = sfPath.empty() ? "" : QString::fromStdWString(sfPath / "Data");
    const auto filename = QFileDialog::getOpenFileName(this, "Open File", dir, "Materials (*.cdb *.ba2)");
    if (ui->lineCdbPath->text().size() && filename.isEmpty())
        return;
    ui->lineCdbPath->setText(filename);
}

void MainWindow::setCdbPath(const QUrl& url)
{
    ui->lineCdbPath->setText(url.toLocalFile());
}

void MainWindow::getMaterialPath()
{
    const auto sfPath = GetStarfieldPath();
    const QString dir = sfPath.empty() ? "" : QString::fromStdWString(sfPath / "Data");
    const auto filename = QFileDialog::getOpenFileName(this, "Open File", dir, "Paths (*.nif *.ba2 *.txt *.esm *.esp *.esl)");
    if (filename.size())
        addFile(filename);
}

void MainWindow::getMaterialFolder()
{
    const auto sfPath = GetStarfieldPath();
    const QString dir = sfPath.empty() ? "" : QString::fromStdWString(sfPath / "Data");
    const auto filename = QFileDialog::getExistingDirectory(this, "Open Folder", dir);
    if (filename.size())
        addFile(filename);
}

void MainWindow::addFile(const QString& file)
{
    const auto stdString = file.toStdString();
    std::vector<const char*> data{
        ".\\MaterialExtractor.exe",
        stdString.c_str()
    };
    addFiles(data);
}

void MainWindow::addUrls(const QList<QUrl>& urls)
{
    std::vector<std::string> urlStrings;
    std::vector<const char*> urlCStrings{
        ".\\MaterialExtractor.exe"
    };
    for (auto& url : urls) {
        urlStrings.emplace_back(url.toLocalFile().toStdString());
        urlCStrings.emplace_back(urlStrings.back().c_str());
    }
    addFiles(urlCStrings);
}

void MainWindow::addFiles(const std::vector<const char*>& files)
{ 
    PathInfo paths{ [](const char*){} };
    if (GetAllPaths(paths, (int)files.size(), (char**)files.data())) {
        PathSet currentSet;
        for (int i = 0; i < ui->listPaths->count(); ++i) {
            currentSet.emplace(ui->listPaths->item(i)->text().toStdString());
        }
        for (const auto& path : paths.materials) {
            if (!currentSet.contains(path)) {
                addPath(QString::fromStdString(path));
            }
        }
        updateEnabledWidgets();
    }
    else {
        QErrorMessage::qtHandler()->showMessage(QString("Failed to find material paths in file %1").arg(files[0]));
    }
}

void MainWindow::showAdd() {
    bool ok;
    const auto path = QInputDialog::getText(this, "Add Path", "Material Path", QLineEdit::Normal, QString(), &ok);
    if (ok && path.size()) {
        auto stdPath = path.toStdString();
        SanitizePrefixedPath(stdPath, "Materials");
        const auto sanitizedPath = QString::fromStdString(stdPath);
        bool found = false;
        for (int i = 0; i < ui->listPaths->count(); ++i) {
            if (ui->listPaths->item(i)->text().compare(path, Qt::CaseInsensitive) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            addPath(sanitizedPath);
            updateEnabledWidgets();
        }
    }
}

void MainWindow::removePaths() {
    const auto selectedItems = ui->listPaths->selectedItems();
    //ui->listPaths->model()->removeRow(0);
    for (auto item : selectedItems) {
        ui->listPaths->model()->removeRow(ui->listPaths->indexFromItem(item).row());
    }
    updateEnabledWidgets();
}

void MainWindow::clearPaths() {
    ui->listPaths->clear();
    updateEnabledWidgets();
}

void MainWindow::updateExport() {
    updateEnabledWidgets();
}

void MainWindow::updateEnabledWidgets() {
    if (ui->listPaths->count() == 0) {
        auto item = new QListWidgetItem();
        auto flags = item->flags();
        flags &= ~Qt::ItemFlag::ItemIsSelectable;
        item->setFlags(flags);
        item->setText("To add paths: drag your nifs, meshes.ba2 or txt files here");
        item->setTextAlignment(Qt::AlignCenter);
        ui->listPaths->addItem(item);
        ui->listPaths->isEmpty = true;
    }
    ui->buttonExport->setEnabled(ui->lineCdbPath->text().size() && !ui->listPaths->isEmpty);
}

template<typename... Args>
void StdLog(std::format_string<Args...> fmt, Args&&... args) {
    QErrorMessage::qtHandler()->showMessage(QString::fromStdString(std::format(fmt, std::forward<Args>(args)...)));
}

void MainWindow::getExportPath()
{
    const auto& cdbPath = ui->lineCdbPath->text();
    if (!cdbPath.endsWith(".ba2") && !cdbPath.endsWith(".cdb"))
    {
        StdLog("Unknown extension for cdb file {}", cdbPath.toStdString());
        return;
    }

    const auto sfPath = GetStarfieldPath();
    QString sfFolder = sfPath.empty() ? "" : QString::fromStdWString(sfPath / "Data");
    const auto folder = QFileDialog::getExistingDirectory(this, "Export Folder", sfFolder);
    if (folder.size())
        exportMaterials(folder);
}

void MainWindow::exportMaterials(const QString& folder)
{
    using namespace cdb;

    const auto cdbPath = ui->lineCdbPath->text().toStdString();
    const auto folderPath = folder.toStdString();

    std::ifstream fstream;
    std::vector<char> ba2Buffer;
    std::ispanstream ba2Stream(std::span{ ba2Buffer });

    const auto& GetReader = [&]() -> Reader {
        if (HasExtension(cdbPath, ".cdb")) {
            fstream.open(cdbPath, std::ios::in | std::ios::binary);
            if (fstream.fail())
                StdLog("Failed to open cdb file {}", cdbPath);
            return Reader(fstream);
        }
        else if (HasExtension(cdbPath, ".ba2")) {
            bool result = GetMaterialDatabase(cdbPath, ba2Buffer);
            ba2Stream = std::ispanstream(std::span{ ba2Buffer });
            if (!result || ba2Buffer.empty()) {
                ba2Stream.setstate(std::ios::failbit);
                StdLog("Unable to find materialsbeta.cdb in {}", cdbPath);
            }
            return Reader(ba2Stream);
        }
        StdLog("Unknown extension for cdb file {}", cdbPath);
        ba2Stream.setstate(std::ios::failbit);
        return Reader(ba2Stream);
    };

    Reader in = GetReader();
    if (in.Stream().fail())
        return;

    progressDialog.reset();
    progressDialog.setRange(0, ui->listPaths->count() + 1);
    progressDialog.setWindowTitle("Exporting...");
    progressDialog.setLabelText("Reading material database");
    progressDialog.setModal(true);
    progressDialog.raise();
    progressDialog.setMinimumDuration(0);
    progressDialog.setValue(0);
    progressDialog.show();

    //Parsing the cdb to json is slow and will freeze the progress dialog as it's opening so
    //we do it on another thread and wait a bit for the progress bar to show up
    Manager header;
    auto readerFuture = QtConcurrent::run([&]() {
        if (!in.ReadHeader(header))
            return;

        if (!in.ReadAllComponents(header)) {
            StdLog("Error reading material component diffs {}", cdbPath);
            return;
        }
    });
    Wait(100);
    readerFuture.waitForFinished();

    const auto& AddPath = [](cdb::Manager& header, const std::string& path) {
        auto resourceId = GetResourceIdFromPath(path);
        auto idIt = header.resourceToDb.find(resourceId);
        if (idIt != header.resourceToDb.end()) {
            header.idToPath.emplace(idIt->second.Value, path);
        }
    };

    for (int i = 0; i < ui->listPaths->count(); ++i) {
        const auto path = ui->listPaths->item(i)->text().toStdString();
        AddPath(header, path);
    }

    for (auto& path : rootMaterialPaths) {
        AddPath(header, path);
    }

    progressDialog.setValue(1);
    progressDialog.setLabelText("Writing materials");

    for (int i = 0; i < ui->listPaths->count(); ++i) {
        const auto inPath = ui->listPaths->item(i)->text().toStdString();
        const auto outPath = std::filesystem::path(folderPath).append(inPath).string();
        progressDialog.setLabelText(QString::fromStdString(inPath));
        ExportMaterial(inPath, outPath, header);
        progressDialog.setValue(i + 1);
        if (progressDialog.wasCanceled())
            break;
    }

    progressDialog.setValue(progressDialog.maximum());
    ui->listPaths->clear();
    updateEnabledWidgets();

    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void MainWindow::getCrcFile() {
    auto filename = QFileDialog::getOpenFileName(this, "Open File", QString(), "Material (*.mat)");
    if (ui->lineMaterialPath->text().size() && filename.isEmpty())
        return;
    auto str = filename.toStdString();
    SanitizePrefixedPath(str, "Materials");
    ui->lineMaterialPath->setText(QString::fromStdString(str));
}

void MainWindow::getCrcPath(const QUrl& url) {
    auto path = url.toLocalFile().toStdString();
    SanitizePrefixedPath(path, "Materials");
    ui->lineMaterialPath->setText(QString::fromStdString(path));
}

void MainWindow::getMaterialId() {
    if (ui->lineMaterialPath->text().size()) {
        const auto path = ui->lineMaterialPath->text().trimmed().toStdString();
        ui->lineMaterialId->setText(QString::number(GetCrc(path)));
    }
    else {
        ui->lineMaterialId->clear();
    }
}
