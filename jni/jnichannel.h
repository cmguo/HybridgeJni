#ifndef JNICHANNEL_H
#define JNICHANNEL_H

#include <core/channel.h>

#include <jni.h>

#include <core/proxyobject.h>

class JniMetaObject;

class JniChannel : public Channel
{
public:
    JniChannel(JNIEnv * env, jobject handle);

    ~JniChannel() override;

    // Bridge interface
protected:
    virtual MetaObject *metaObject(const Object *object) const override;

    virtual std::string createUuid() const override;

    virtual ProxyObject * createProxyObject(Map &&classinfo) const override;

    virtual void startTimer(int msec) override;

    virtual void stopTimer() override;

protected:
    void registerObject(const std::string &id, jobject object);

    void deregisterObject(jobject object);

    void propertyChanged(jobject object, jstring property);

    void connectTo(Transport *transport, jobject response);

protected:
    void invokeMethod(Object *object, jobject method, jobjectArray args, jobject response);

private:
    JniMetaObject * metaObject2(jclass clazz) const;

private:
    friend struct JChannel;

    JNIEnv * env_;
    jobject handle_;
    jmethodID createUuid_;
    jmethodID startTimer_;
    jmethodID stopTimer_;
    static std::map<std::string, JniMetaObject*> classMetas_;
};

#endif // JNICHANNEL_H
