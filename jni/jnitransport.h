#ifndef JNITRANSPORT_H
#define JNITRANSPORT_H

#include <core/transport.h>

#include <jni.h>

class JniTransport : public Transport
{
public:
    JniTransport(JNIEnv * env, jobject handle);

    ~JniTransport() override;

    // Transport interface
public:
    virtual void sendMessage(const Message &message) override;

protected:
    void messageReceived(jstring message);

private:
    friend struct JTransport;
    JNIEnv * env_;
    jobject handle_;
    jmethodID sendMessage_;
};

#endif // JNITRANSPORT_H
