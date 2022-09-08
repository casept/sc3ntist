#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>

class DebuggerConnectionDialog : public QDialog {
  Q_OBJECT

 public:
  explicit DebuggerConnectionDialog(QWidget *parent = 0);

 private slots:
  void okClicked();

 private:
  QDialogButtonBox *_buttons;
  QLineEdit *_hostEdit;
  QSpinBox *_portBox;
};
