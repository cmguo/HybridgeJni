#ifndef JNICHANNEL_H
#define JNICHANNEL_H

#include <core/channel.h>

#include <jni.h>

class JniMetaObjectBase;

class JniChannel : public Channel
{
public:
    JniChannel(JNIEnv * env, jobject handle);

    ~JniChannel() override;

    // Bridge interface
protected:
    virtual MetaObject *metaObject(const Object *object) const override;
    virtual std::string createUuid() const override;
    virtual MetaObject::Connection connect(const Object *object, size_t signalIndex) override;
    virtual bool disconnect(const MetaObject::Connection &c) override;
    virtual void startTimer(int msec) override;
    virtual void stopTimer() override;

public:
    void registerObject(const std::string &id, jobject object);
    void deregisterObject(jobject object);
    void propertyChanged(jobject object, jstring property);

private:
    JniMetaObjectBase * metaObject2(jclass clazz) const;

private:
    friend struct JChannel;
    JNIEnv * env_;
    jobject handle_;
    jmethodID createUuid_;
    jmethodID startTimer_;
    jmethodID stopTimer_;
    std::vector<jobject> objects_;
    mutable std::map<std::string, JniMetaObjectBase*> classMetas_;
};

#endif // JNICHANNEL_H
