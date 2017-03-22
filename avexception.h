#ifndef AVEXCEPTION_H
#define AVEXCEPTION_H

#include "plsexception.h"

class AVException : public PLSException
{
public:
    AVException(QString function, int libavReturnCode) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(libavReturnCode, errbuf, AV_ERROR_MAX_STRING_SIZE);
        _message = function + ": " + QString(errbuf);}
    void raise() const { throw *this; }
    AVException *clone() const {return new AVException(*this); }
};

#endif // AVEXCEPTION_H
