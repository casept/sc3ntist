#pragma once

#include <QTreeView>
#include <QHeaderView>
#include <QAbstractItemModel>
#include <QTimer>
#include <memory>
#include <optional>

#include "debugger.h"
#include "parser/SCXTypes.h"

class DisassemblyView : public QTreeView {
  Q_OBJECT
 public:
  explicit DisassemblyView(
      QWidget *parent = 0,
      std::optional<std::shared_ptr<Dbg::Debugger>> dbg = {});
  ~DisassemblyView(){};

  void setModel(QAbstractItemModel *model) override;

 public slots:
  void goToAddress(SCXOffset address);
  void goToLabel(int labelId);

 private:
  void resizeEvent(QResizeEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

  QTimer *_resizeTimer;
  bool _isResizing = false;

 private slots:
  void onCommentKeyPress();
  void onNameKeyPress();
  void onXrefKeyPress();
  void onBreakpointKeyPress();
  void onContinueKeyPress();

  void onModelReset();
  void adjustHeader(int oldCount, int newCount);

  void onProjectOpened();
  void onProjectClosed();
  void onFileSwitched(int previousId);
};
