#ifndef JNIMETA_H
#define JNIMETA_H

#include <core/object.h>

#include <jni.h>

class JniMetaProperty;
class JniMetaMethod;
class JniMetaEnum;

class JniMetaObject : public MetaObject
{
public:
    JniMetaObject(JNIEnv *env, jclass clazz);

    // MetaObject interface
public:
    virtual const char *className() const override;
    virtual int propertyCount() const override;
    virtual const MetaProperty &property(int index) const override;
    virtual int methodCount() const override;
    virtual const MetaMethod &method(int index) const override;
    virtual int enumeratorCount() const override;
    virtual const MetaEnum &enumerator(int index) const override;

    JNIEnv *env_;
private:
    jclass clazz_;
    std::string className_;
    std::vector<JniMetaProperty> metaProps_;
    std::vector<JniMetaMethod> metaMethods_;
    std::vector<JniMetaEnum> metaEnums_;
};

class QMetaProperty;

class JniMetaProperty : public MetaProperty
{
public:
    JniMetaProperty(JniMetaObject *obj, jobject field = nullptr);

    ~JniMetaProperty() override;

    // MetaProperty interface
public:
    virtual const char *name() const override;
    virtual bool isValid() const override;
    virtual int type() const override;
    virtual bool isConstant() const override;
    virtual bool hasNotifySignal() const override;
    virtual int notifySignalIndex() const override;
    virtual const MetaMethod &notifySignal() const override;
    virtual Value read(const Object *object) const override;
    virtual bool write(Object *object, const Value &value) const override;

private:
    JniMetaObject *obj_;
    jobject field_;
};

class JniMetaMethod : public MetaMethod
{
public:
    JniMetaMethod(JniMetaObject *obj, jobject method = nullptr);

    // MetaMethod interface
public:
    virtual const char *name() const override;
    virtual bool isValid() const override;
    virtual bool isSignal() const override;
    virtual bool isPublic() const override;
    virtual int methodIndex() const override;
    virtual const char *methodSignature() const override;
    virtual int parameterCount() const override;
    virtual int parameterType(int index) const override;
    virtual const char *parameterName(int index) const override;
    virtual Value invoke(Object *object, const Array &args) const override;

private:
    JniMetaObject *obj_;
    jobject method_;
};

class JniMetaEnum : public MetaEnum
{
public:
    JniMetaEnum(jclass enumClass = nullptr);

    // MetaEnum interface
public:
    virtual const char *name() const override;
    virtual int keyCount() const override;
    virtual const char *key(int index) const override;
    virtual int value(int index) const override;

private:
    jclass enumClass_;
};


#endif // JNIMETA_H
