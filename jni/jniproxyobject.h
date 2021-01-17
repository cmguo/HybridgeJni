#ifndef JNIPROXYOBJECT_H
#define JNIPROXYOBJECT_H

#include "jniclass.h"

#include <core/proxyobject.h>

class JniProxyObject : public ProxyObject
{
public:
    JniProxyObject(JNIEnv * env);

    ~JniProxyObject() override;

    virtual void * handle() const override { return handle_; }

private:
    friend struct JProxyObject;

    jobject readProperty(jstring property);

    jboolean writeProperty(jstring property, jobject value);

    jboolean invokeMethod(jobject method, jobjectArray args, jobject onResult);

    jboolean connect(jint signalIndex, jobject handler);

    jboolean disconnect(jint signalIndex, jobject handler);

private:
    JNIEnv * env_;
    jobject handle_;
};

struct ProxyObjectClass : Class
{
    ProxyObjectClass(JNIEnv * env);
    jobject create(jlong handle);
private:
    jmethodID create_;
};

ProxyObjectClass & proxyObjectClass(JNIEnv * env = nullptr);

struct OnResultClass : Class
{
    OnResultClass(JNIEnv * env);
    void apply(jobject resp, jobject result);
private:
    jmethodID apply_;
};

OnResultClass & onResultClass(JNIEnv * env = nullptr);

struct SignalHandlerClass : Class
{
    SignalHandlerClass(JNIEnv * env);
    void apply(jobject resp, jobject object, jint signalIndex, jobjectArray args);
private:
    jmethodID apply_;
};

SignalHandlerClass & signalHandlerClass(JNIEnv * env = nullptr);


#endif // JNIPROXYOBJECT_H
