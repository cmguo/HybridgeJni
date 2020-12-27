#ifndef JNIMETA_H
#define JNIMETA_H

#include <core/object.h>

#include <jni.h>

class JniMetaProperty;
class JniMetaMethod;
class JniMetaEnum;

class JniMetaObjectBase : public MetaObject
{
public:
    virtual JNIEnv *env() const = 0;
};

class JniObjectMetaObject : public JniMetaObjectBase
{
public:
    JniObjectMetaObject(JNIEnv *env);

    // MetaObject interface
public:
    virtual const char *className() const override { return "Object"; }
    virtual size_t propertyCount() const override { return 0; }
    virtual const MetaProperty &property(size_t index) const override;
    virtual size_t methodCount() const override { return 0; }
    virtual const MetaMethod &method(size_t index) const override;
    virtual size_t enumeratorCount() const override { return 0; }
    virtual const MetaEnum &enumerator(size_t index) const override;

    virtual JNIEnv *env() const override { return env_; }

private:
    JNIEnv *env_;
};

class JniMetaObject : public JniMetaObjectBase
{
public:
    JniMetaObject(JniMetaObjectBase * super, jclass clazz);

    // MetaObject interface
public:
    virtual const char *className() const override;
    virtual size_t propertyCount() const override;
    virtual const MetaProperty &property(size_t index) const override;
    virtual size_t methodCount() const override;
    virtual const MetaMethod &method(size_t index) const override;
    virtual size_t enumeratorCount() const override;
    virtual const MetaEnum &enumerator(size_t index) const override;

public:
    virtual JNIEnv *env() const override { return super_->env(); }

    enum MetaType
    {
        Property,
        Method,
        Enum,
    };

    size_t metaIndexOf(void const * meta, MetaType type) const;

    size_t metaIndexOf(std::string const & name, MetaType type) const;

private:
    JniMetaObjectBase * super_;
    jclass clazz_;
    std::string className_;
    std::vector<JniMetaProperty> metaProps_;
    std::vector<JniMetaMethod> metaMethods_;
    std::vector<JniMetaEnum> metaEnums_;
};

class JniMetaProperty : public MetaProperty
{
public:
    JniMetaProperty(JniMetaObject *obj = nullptr, jobject field = nullptr);

    JniMetaProperty(JniMetaProperty && o);

    ~JniMetaProperty() override;

    void setSetter(jobject setter);

    void setGetter(jobject getter);

    // MetaProperty interface
public:
    virtual const char *name() const override;
    virtual bool isValid() const override;
    virtual int type() const override;
    virtual bool isConstant() const override;
    virtual bool hasNotifySignal() const override;
    virtual size_t notifySignalIndex() const override;
    virtual const MetaMethod &notifySignal() const override;
    virtual Value read(const Object *object) const override;
    virtual bool write(Object *object, const Value &value) const override;

private:
    JNIEnv *env() const { return obj_->env(); }

private:
    JniMetaObject *obj_;
    jobject field_;
    std::string name_;
    jobject getter_ = nullptr;
    jobject setter_ = nullptr;
};

class JniMetaMethod : public MetaMethod
{
public:
    JniMetaMethod(JniMetaObject *obj = nullptr, jobject method = nullptr);

    JniMetaMethod(JniMetaMethod && o);

    ~JniMetaMethod() override;

    // MetaMethod interface
public:
    virtual const char *name() const override;
    virtual bool isValid() const override;
    virtual bool isSignal() const override;
    virtual bool isPublic() const override;
    virtual size_t methodIndex() const override;
    virtual const char *methodSignature() const override;
    virtual size_t parameterCount() const override;
    virtual int parameterType(size_t index) const override;
    virtual const char *parameterName(size_t index) const override;
    virtual Value invoke(Object *object, const Array &args) const override;

private:
    JniMetaObject *obj_;
    jobject method_;
    std::string name_;
};

class JniMetaEnum : public MetaEnum
{
public:
    JniMetaEnum(JniMetaObject *obj = nullptr, jclass enumClass = nullptr);

    // MetaEnum interface
public:
    virtual const char *name() const override;
    virtual size_t keyCount() const override;
    virtual const char *key(size_t index) const override;
    virtual int value(size_t index) const override;

private:
    JniMetaObject *obj_;
    jclass enumClass_;
    std::string name_;
};


#endif // JNIMETA_H
