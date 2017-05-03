#ifndef CUSTOMLABEL_H
#define CUSTOMLABEL_H

#include <QLabel>

class CustomLabel : public QLabel
{
    Q_OBJECT
public:
    CustomLabel(QWidget* parent = nullptr) : QLabel(parent){ }

public slots:

signals:


private:
    QString holdText;

protected:
    void enterEvent(QEvent *ev) override
    {
        holdText = this->text();
        this->setText("");
        //setStyleSheet("QLabel { background-color : blue; }");
    }

    void leaveEvent(QEvent *ev) override
    {
        this->setText(holdText);
        //setStyleSheet("QLabel { background-color : green; }");
    }
};

#endif
