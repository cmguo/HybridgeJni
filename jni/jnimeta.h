#ifndef JNIMETA_H
#define JNIMETA_H

#include <core/meta.h>

#include <jni.h>

class JniMetaProperty;
class JniMetaMethod;
class JniMetaEnum;

class JniMetaObject : public MetaObject
{
public:
    JniMetaObject(JniMetaObject * super, jclass clazz);

    virtual ~JniMetaObject();

    // MetaObject interface
public:
    virtual const char *className() const override;
    virtual size_t propertyCount() const override;
    virtual const MetaProperty &property(size_t index) const override;
    virtual size_t methodCount() const override;
    virtual const MetaMethod &method(size_t index) const override;
    virtual size_t enumeratorCount() const override;
    virtual const MetaEnum &enumerator(size_t index) const override;

    virtual bool connect(const Connection &c) const override;
    virtual bool disconnect(const Connection &c) const override;

    using MetaObject::propertyChanged;

public:
    virtual JNIEnv *env() const { return super_->env(); }

    enum MetaType
    {
        Property,
        Method,
        Enum,
    };

    size_t metaIndexOf(void const * meta, MetaType type) const;

protected:
    JniMetaObject(JNIEnv *env);

protected:
    union {
        JNIEnv *env_;
        JniMetaObject * super_;
    };
    jclass clazz_;
    std::string className_;
    std::vector<JniMetaProperty> metaProps_;
    std::vector<JniMetaMethod> metaMethods_;
    std::vector<JniMetaEnum> metaEnums_;
};

class JniMetaObjectBase : public MetaObject
{
public:
    virtual JNIEnv *env() const = 0;
};

class JniObjectMetaObject : public JniMetaObject
{
public:
    JniObjectMetaObject(JNIEnv *env);

    // MetaObject interface
public:
    virtual const char *className() const override;
    virtual size_t propertyCount() const override { return 0; }
    virtual size_t methodCount() const override;
    virtual const MetaMethod &method(size_t index) const override;
    virtual size_t enumeratorCount() const override { return 0; }

    virtual JNIEnv *env() const override { return env_; }
};

class JniMetaProperty : public MetaProperty
{
public:
    JniMetaProperty(JniMetaObject *obj = nullptr, jobject field = nullptr);

    JniMetaProperty(JniMetaProperty && o);

    JniMetaProperty(std::string const & name);

    ~JniMetaProperty() override;

    void setSetter(jobject setter);

    void setGetter(jobject getter);

    friend bool operator==(JniMetaProperty const & l, JniMetaProperty const & r);

    bool operator==(MetaProperty const & o);

    // MetaProperty interface
public:
    virtual const char *name() const override;
    virtual bool isValid() const override;
    virtual Value::Type type() const override;
    virtual bool isConstant() const override;
    virtual bool hasNotifySignal() const override;
    virtual size_t notifySignalIndex() const override;
    virtual const MetaMethod &notifySignal() const override;
    virtual Value read(const Object *object) const override;
    virtual bool write(Object *object, Value &&value) const override;

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

    friend bool operator==(JniMetaMethod const & l, JniMetaMethod const & r);

    bool operator==(MetaMethod const & o);

    // MetaMethod interface
public:
    virtual const char *name() const override;
    virtual bool isValid() const override;
    virtual bool isSignal() const override;
    virtual bool isPublic() const override;
    virtual size_t methodIndex() const override;
    virtual const char *methodSignature() const override;
    virtual Value::Type returnType() const override;
    virtual size_t parameterCount() const override;
    virtual Value::Type parameterType(size_t index) const override;
    virtual const char *parameterName(size_t index) const override;
    virtual bool invoke(Object *object, Array &&args, Response const & resp) const override;

private:
    JniMetaObject *obj_;
    jobject method_;
    Value::Type returnType_;
    std::string name_;
    std::vector<Value::Type> paramTypes_;
};

class JniMetaEnum : public MetaEnum
{
public:
    JniMetaEnum(JniMetaObject *obj = nullptr, jclass enumClass = nullptr);

    JniMetaEnum(std::string const & name);

    friend bool operator==(JniMetaEnum const & l, JniMetaEnum const & r);

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
