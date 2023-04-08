#pragma once

#include "debugger.h"

#include <QMainWindow>
#include <QListWidget>
#include <optional>
#include <memory>

namespace Ui {
class MainWindow;
}

class DisassemblyView;
class MemoryView;

class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 private:
  Ui::MainWindow *ui;
  DisassemblyView *_disasmView;
  QListWidget *_fileList;
  QListWidget *_labelList;
  MemoryView *_memoryView;
  // Debugger has to be owned by main window because it has to be accessible by
  // multiple views and has to survive until disconnect which is triggered in
  // the main window.
  std::optional<std::shared_ptr<Dbg::Debugger>> _dbg;

 private slots:
  void onProjectOpened();
  void onProjectClosed();
  void onFileSwitched(int previousId);
  void onLabelNameChanged(int fileId, int labelId, const QString &name);
  void on_actionOpen_triggered();
  void on_actionClose_triggered();
  void on_actionGo_to_address_triggered();
  void on_actionEdit_stylesheet_triggered();
  void on_actionImport_worklist_triggered();
  void on_actionNew_project_triggered();
  void on_actionConnect_to_target_triggered();
  void on_actionDisconnect_from_target_triggered();
};
