#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>

#include <cstdint>

class DebuggerConnectionDialog : public QDialog {
  Q_OBJECT

 public:
  explicit DebuggerConnectionDialog(QWidget *parent = 0);
  QString getHost();
  uint16_t getPort();

 private:
  QDialogButtonBox *_buttons;
  QLineEdit *_hostEdit;
  QSpinBox *_portBox;
};
