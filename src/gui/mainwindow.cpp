#include "mainwindow.h"
#include "debugger.h"
#include "ui_mainwindow.h"
#include "debuggerapplication.h"
#include "textdump.h"
#include "project.h"
#include "parser/SCXFile.h"
#include "disassemblymodel.h"
#include "disassemblyview.h"
#include <QDockWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include "memoryview.h"
#include "worklistdialog.h"
#include "newprojectdialog.h"
#include "debuggerconnectiondialog.h"

#include <exception>
#include <vector>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  setDockOptions(AnimatedDocks | AllowNestedDocks | AllowTabbedDocks |
                 GroupedDragging);

  // TODO: move children into custom subclasses

  QDockWidget *disassemblyDock = new QDockWidget("Disassembly", this);
  disassemblyDock->setFeatures(disassemblyDock->features() &
                               ~QDockWidget::DockWidgetClosable);
  disassemblyDock->setAllowedAreas(Qt::AllDockWidgetAreas);
  _disasmView = new DisassemblyView(disassemblyDock, _dbg);
  disassemblyDock->setWidget(_disasmView);
  addDockWidget(Qt::RightDockWidgetArea, disassemblyDock);

  QDockWidget *fileListDock = new QDockWidget("File list", this);
  fileListDock->setFeatures(fileListDock->features() &
                            ~QDockWidget::DockWidgetClosable);
  fileListDock->setAllowedAreas(Qt::AllDockWidgetAreas);
  _fileList = new QListWidget(fileListDock);
  fileListDock->setWidget(_fileList);
  addDockWidget(Qt::LeftDockWidgetArea, fileListDock);
  connect(_fileList, &QListWidget::itemActivated, [=]() {
    dApp->project()->switchFile(
        _fileList->currentItem()->data(Qt::UserRole).toInt());
  });

  QDockWidget *labelListDock = new QDockWidget("Labels", this);
  labelListDock->setFeatures(labelListDock->features() &
                             ~QDockWidget::DockWidgetClosable);
  labelListDock->setAllowedAreas(Qt::AllDockWidgetAreas);
  _labelList = new QListWidget(labelListDock);
  labelListDock->setWidget(_labelList);
  splitDockWidget(fileListDock, labelListDock, Qt::Vertical);
  connect(_labelList, &QListWidget::itemActivated,
          [=]() { _disasmView->goToLabel(_labelList->currentRow()); });

  QDockWidget *memoryDock = new QDockWidget("Memory", this);
  memoryDock->setFeatures(memoryDock->features() &
                          ~QDockWidget::DockWidgetClosable);
  memoryDock->setAllowedAreas(Qt::AllDockWidgetAreas);
  _memoryView = new MemoryView(this);
  memoryDock->setWidget(_memoryView);
  splitDockWidget(disassemblyDock, memoryDock, Qt::Vertical);

  // without a centralwidget we need to fill *at least* the whole width or
  // the resize is ignored
  resizeDocks({fileListDock, disassemblyDock}, {200, width()}, Qt::Horizontal);

  connect(dApp, &DebuggerApplication::projectOpened, this,
          &MainWindow::onProjectOpened);
  connect(dApp, &DebuggerApplication::projectClosed, this,
          &MainWindow::onProjectClosed);

  // Debugger may only be opened once project is opened
  ui->actionConnect_to_target->setEnabled(false);
  ui->actionDisconnect_from_target->setEnabled(false);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::onProjectOpened() {
  connect(dApp->project(), &Project::fileSwitched, this,
          &MainWindow::onFileSwitched);
  connect(dApp->project(), &Project::labelNameChanged, this,
          &MainWindow::onLabelNameChanged);
  ui->actionConnect_to_target->setEnabled(true);

  const auto &files = dApp->project()->files();
  if (files.size() == 0) return;
  for (const auto &file : files) {
    auto fileItem = new QListWidgetItem(
        QString("[%1] %2")
            .arg(file.first, 3)
            .arg(QString::fromStdString(file.second->getName())));
    fileItem->setData(Qt::UserRole, QVariant(file.first));
    _fileList->addItem(fileItem);
  }
  dApp->project()->switchFile(files.begin()->first);
}

void MainWindow::onProjectClosed() {
  _fileList->clear();
  _labelList->clear();
  ui->actionConnect_to_target->setEnabled(false);
  // Disconnect current debug session, if any
  if (_dbg.has_value()) {
    _dbg.reset();
  }
}

void MainWindow::onFileSwitched(int previousId) {
  {
    if (previousId >= 0) {
      _fileList->item(previousId)
          ->setText(
              QString("[%1] %2")
                  .arg(previousId, 3)
                  .arg(QString::fromStdString(
                      dApp->project()->files().at(previousId)->getName())));
    }
    int currentFileId = dApp->project()->currentFileId();
    auto currentFileName = dApp->project()->currentFile()->getName();
    _fileList->setCurrentRow(currentFileId);
    _fileList->currentItem()->setText(
        QString("==> [%1] %2")
            .arg(currentFileId, 3)
            .arg(QString::fromStdString(currentFileName)));
  }
  {
    const SCXFile *currentFile = dApp->project()->currentFile();
    _labelList->clear();
    for (int i = 0; i < currentFile->disassembly().size(); i++) {
      _labelList->addItem(QString("#%1").arg(
          dApp->project()->getLabelName(currentFile->getId(), i)));
    }
  }
}

void MainWindow::onLabelNameChanged(int fileId, int labelId,
                                    const QString &name) {
  if (fileId != dApp->project()->currentFileId()) return;
  auto *item = _labelList->item(labelId);
  if (item == nullptr) return;
  item->setText(QString("#%1").arg(name));
}

void MainWindow::on_actionOpen_triggered() {
  QString fileName = QFileDialog::getOpenFileName(
      this, "Open project", QString(), "Project files (*.sqlite)");
  if (!dApp->tryOpenProject(fileName)) {
    QMessageBox::critical(this, "Error", "Could not open project");
  }
}

void MainWindow::on_actionClose_triggered() { dApp->closeProject(); }

void MainWindow::on_actionGo_to_address_triggered() {
  // TODO: disable item when no script loaded
  bool ok;
  QString input = QInputDialog::getText(
      this, "Go to address", "Address:", QLineEdit::Normal, QString(), &ok);
  if (!ok) return;
  int address = input.toInt(&ok, 0);
  if (!ok) return;
  dApp->project()->goToAddress(dApp->project()->currentFileId(), address);
}

void MainWindow::on_actionEdit_stylesheet_triggered() {
  QString sheet = dApp->styleSheet();
  bool ok;
  sheet = QInputDialog::getMultiLineText(this, "Edit stylesheet",
                                         "Stylesheet:", sheet, &ok);
  if (ok) dApp->setStyleSheet(sheet);
}

void MainWindow::on_actionImport_worklist_triggered() {
  if (dApp->project() == nullptr) return;
  WorklistDialog(this).exec();
}

void MainWindow::on_actionNew_project_triggered() {
  NewProjectDialog(this).exec();
}

void MainWindow::on_actionConnect_to_target_triggered() {
  auto connDialog = DebuggerConnectionDialog(this);
  if (connDialog.exec() == QDialog::Accepted) {
    try {
      auto dbg = std::make_shared<Dbg::Debugger>(
          connDialog.getHost().toStdString().data(), connDialog.getPort());
      _dbg.emplace(dbg);
    } catch (const std::exception &e) {
      QMessageBox::critical(
          this, "Error", QString("Failed to connect to target: ") + e.what());
      return;
    }
  }
  ui->actionConnect_to_target->setEnabled(false);
  ui->actionDisconnect_from_target->setEnabled(true);
}

void MainWindow::on_actionDisconnect_from_target_triggered() {
  _dbg.reset();
  ui->actionConnect_to_target->setEnabled(true);
  ui->actionDisconnect_from_target->setEnabled(false);
}
