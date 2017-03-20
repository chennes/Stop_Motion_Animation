#include "frameeditor.h"
#include "ui_frameeditor.h"

FrameEditor::FrameEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FrameEditor)
{
    ui->setupUi(this);
}

FrameEditor::~FrameEditor()
{
    delete ui;
}
