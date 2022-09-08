#include "debuggerconnectiondialog.h"
#include "debuggerapplication.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QSpinBox>

#include <stdexcept>

DebuggerConnectionDialog::DebuggerConnectionDialog(QWidget *parent)
    : QDialog(parent) {
  _buttons =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(_buttons, &QDialogButtonBox::accepted, this,
          &DebuggerConnectionDialog::okClicked);
  connect(_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *layout = new QVBoxLayout(this);

  QHBoxLayout *hostLayout = new QHBoxLayout(this);
  QLabel *hostLabel = new QLabel("Host:", this);
  _hostEdit = new QLineEdit(this);

  hostLayout->addWidget(hostLabel);
  hostLayout->addWidget(_hostEdit);

  QHBoxLayout *portLayout = new QHBoxLayout(this);
  QLabel *portLabel = new QLabel("Port:", this);
  _portBox = new QSpinBox(this);
  _portBox->setMinimum(1);
  _portBox->setMaximum(65535);

  portLayout->addWidget(portLabel);
  portLayout->addWidget(_portBox);

  layout->addLayout(hostLayout);
  layout->addLayout(portLayout);
  layout->addWidget(_buttons);
  setLayout(layout);

  resize(500, height());
}

void DebuggerConnectionDialog::okClicked() {
  // TODO: Validate inputs
  accept();
}
