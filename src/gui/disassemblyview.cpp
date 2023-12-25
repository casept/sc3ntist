#include "disassemblymodel.h"
#include "disassemblyview.h"
#include "disassemblyitemdelegate.h"
#include "debugger.h"
#include "project.h"
#include "debuggerapplication.h"
#include <QAction>
#include <QInputDialog>
#include <QShortcut>
#include "xrefdialog.h"

DisassemblyView::DisassemblyView(
    QWidget* parent, std::optional<std::shared_ptr<Dbg::Debugger>> dbg)
    : QTreeView(parent) {
  connect(dApp, &DebuggerApplication::projectOpened, this,
          &DisassemblyView::onProjectOpened);
  connect(dApp, &DebuggerApplication::projectClosed, this,
          &DisassemblyView::onProjectClosed);

  connect(header(), &QHeaderView::sectionCountChanged, this,
          &DisassemblyView::adjustHeader);

  header()->setSectionsMovable(false);
  header()->setSectionsClickable(false);

  setUniformRowHeights(true);
  setItemsExpandable(false);
  setWordWrap(false);
  setIndentation(0);
  setRootIsDecorated(false);

  setItemDelegate(new DisassemblyItemDelegate(this));

  QShortcut* commentShortcut = new QShortcut(Qt::Key_C, this);
  connect(commentShortcut, &QShortcut::activated, this,
          &DisassemblyView::onCommentKeyPress);
  commentShortcut->setContext(Qt::WidgetWithChildrenShortcut);
  QShortcut* nameShortcut = new QShortcut(Qt::Key_N, this);
  connect(nameShortcut, &QShortcut::activated, this,
          &DisassemblyView::onNameKeyPress);
  nameShortcut->setContext(Qt::WidgetWithChildrenShortcut);
  QShortcut* xrefShortcut = new QShortcut(Qt::Key_X, this);
  connect(xrefShortcut, &QShortcut::activated, this,
          &DisassemblyView::onXrefKeyPress);
  xrefShortcut->setContext(Qt::WidgetWithChildrenShortcut);
  QShortcut* breakpointShortcut = new QShortcut(Qt::Key_B, this);
  connect(breakpointShortcut, &QShortcut::activated, this,
          &DisassemblyView::onBreakpointKeyPress);
  breakpointShortcut->setContext(Qt::WidgetWithChildrenShortcut);
  QShortcut* continueShortcut = new QShortcut(Qt::Key_C, this);
  connect(continueShortcut, &QShortcut::activated, this,
          &DisassemblyView::onContinueKeyPress);
  continueShortcut->setContext(Qt::WidgetWithChildrenShortcut);

  _resizeTimer = new QTimer(this);
  _resizeTimer->setSingleShot(true);
  _resizeTimer->setInterval(100);
  connect(_resizeTimer, &QTimer::timeout, [=]() {
    this->_isResizing = false;
    update();
  });
}

void DisassemblyView::resizeEvent(QResizeEvent* event) {
  QTreeView::resizeEvent(event);
  // prevent repaint during resize
  _isResizing = true;
  _resizeTimer->start();
}

void DisassemblyView::paintEvent(QPaintEvent* event) {
  if (_isResizing) return;
  QTreeView::paintEvent(event);
}

void DisassemblyView::setModel(QAbstractItemModel* model) {
  QTreeView::setModel(model);
  connect(model, &QAbstractItemModel::modelReset, this,
          &DisassemblyView::onModelReset);
  connect(model, &QAbstractItemModel::rowsInserted, this,
          &DisassemblyView::onModelReset);

  expandAll();
  scrollToTop();
}

void DisassemblyView::onModelReset() {
  setUniformRowHeights(true);
  setItemsExpandable(false);
  setWordWrap(false);
  setIndentation(0);
  setRootIsDecorated(false);

  expandAll();
}

void DisassemblyView::goToAddress(SCXOffset address) {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  QModelIndex index = disModel->firstIndexForAddress(address);
  setCurrentIndex(index);
  // in case the line was already selected, still scroll there
  scrollTo(index);
}

void DisassemblyView::goToLabel(int labelId) {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  QModelIndex index = disModel->indexForLabel(labelId);
  setCurrentIndex(index);
  // in case the line was already selected, still scroll there
  scrollTo(index);
}

void DisassemblyView::adjustHeader(int oldCount, int newCount) {
  if (newCount != (int)DisassemblyModel::ColumnType::NumColumns) return;
  header()->setSectionResizeMode((int)DisassemblyModel::ColumnType::Breakpoint,
                                 QHeaderView::Fixed);
  // TODO: make breakpoint bounding rectangle square
  header()->resizeSection((int)DisassemblyModel::ColumnType::Breakpoint, 24);
  header()->setSectionResizeMode((int)DisassemblyModel::ColumnType::Address,
                                 QHeaderView::ResizeToContents);
}

void DisassemblyView::onProjectOpened() {
  connect(dApp->project(), &Project::fileSwitched, this,
          &DisassemblyView::onFileSwitched);
  connect(dApp->project(), &Project::focusAddressSwitched, this,
          &DisassemblyView::goToAddress);
}

void DisassemblyView::onProjectClosed() {
  QAbstractItemModel* oldModel = model();
  setModel(nullptr);
  if (oldModel != nullptr) delete oldModel;
}

void DisassemblyView::onFileSwitched(int previousId) {
  QAbstractItemModel* oldModel = model();
  setModel(new DisassemblyModel(dApp->project()->currentFile()));
  if (oldModel != nullptr) delete oldModel;
}

void DisassemblyView::onCommentKeyPress() {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  SCXOffset address = disModel->addressForIndex(currentIndex());
  if (address < 0) return;

  QString oldComment =
      dApp->project()->getComment(disModel->script()->getId(), address);

  bool ok;
  QString newComment = QInputDialog::getMultiLineText(
      this, "Edit comment", "Comment:", oldComment, &ok);
  if (!ok) return;
  dApp->project()->setComment(disModel->script()->getId(), address, newComment);
}

void DisassemblyView::onNameKeyPress() {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  const DisassemblyRow* row =
      static_cast<DisassemblyRow*>(currentIndex().internalPointer());
  if (row == nullptr) return;
  if (row->type != DisassemblyModel::RowType::Label) return;

  QString oldName =
      dApp->project()->getLabelName(disModel->script()->getId(), row->id);

  bool ok;
  QString newName = QInputDialog::getText(
      this, "Edit label name", "New name:", QLineEdit::Normal, oldName, &ok);
  if (!ok) return;
  dApp->project()->setLabelName(disModel->script()->getId(), row->id, newName);
}

void DisassemblyView::onXrefKeyPress() {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  const DisassemblyRow* row =
      static_cast<DisassemblyRow*>(currentIndex().internalPointer());
  if (row == nullptr) return;
  if (row->type == DisassemblyModel::RowType::Label) {
    XrefDialog(disModel->script()->getId(), row->id, true, this).exec();
  } else {
    SCXOffset address = disModel->addressForIndex(currentIndex());
    if (address < 0) return;

    XrefDialog(disModel->script()->getId(), address, false, this).exec();
  }
}

void DisassemblyView::onBreakpointKeyPress() {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  SCXOffset address = disModel->addressForIndex(currentIndex());
  if (address < 0) return;

  bool enabled =
      dApp->project()->getBreakpoint(disModel->script()->getId(), address);
  dApp->project()->setBreakpoint(disModel->script()->getId(), address,
                                 !enabled);
}

void DisassemblyView::onContinueKeyPress() {
  const DisassemblyModel* disModel = qobject_cast<DisassemblyModel*>(model());
  if (disModel == nullptr) return;

  SCXOffset address = disModel->addressForIndex(currentIndex());
  if (address < 0) return;

  // TODO: Also check if breakpoint was actually hit
  bool breakpoint_exists =
      dApp->project()->getBreakpoint(disModel->script()->getId(), address);
  if (breakpoint_exists) {
    // TODO: Does this really belong in a view?
    // dbg->continueExecution(
    //    Dbg::Breakpoint{.address = (uint32_t)address,
    //                    .scriptBuffer = disModel->script()->getName()});
    qWarning() << "Continue not implemented yet";
  }
}
