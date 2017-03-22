#ifndef PLSEXCEPTION_H
#define PLSEXCEPTION_H

#include <QException>
#include <QString>

class PLSException : public QException
{
public:
    PLSException(QString m = "") {_message = m;}
    void raise() const { throw *this; }
    PLSException *clone() const {return new PLSException(*this); }
    QString message() const {return _message;}
protected:
    QString _message;
};

#endif // PLSEXCEPTION_H
