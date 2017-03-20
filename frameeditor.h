#ifndef FRAMEEDITOR_H
#define FRAMEEDITOR_H

#include <QDialog>

namespace Ui {
class FrameEditor;
}

class FrameEditor : public QDialog
{
    Q_OBJECT

public:
    explicit FrameEditor(QWidget *parent = 0);
    ~FrameEditor();

private:
    Ui::FrameEditor *ui;
};

#endif // FRAMEEDITOR_H
