#ifndef JNIBRIDGE_H
#define JNIBRIDGE_H

#include <core/bridge.h>

class JniBridge : Bridge
{
public:
    JniBridge();

    // Bridge interface
protected:
    virtual MetaObject *metaObject(const Object *object) const override;
    virtual std::string createUuid() const override;
    virtual MetaObject::Connection connect(const Object *object, int signalIndex) override;
    virtual bool disconnect(const MetaObject::Connection &c) override;
    virtual void startTimer(int msec) override;
    virtual void stopTimer() override;
};

#endif // JNIBRIDGE_H
