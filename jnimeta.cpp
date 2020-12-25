#include "jnimeta.h"

#include <natiflect/class.h>
//#include "variant.h"

struct ClassInfo
{
    ClassInfo(JNIEnv *env)
    {
        jclass clazz = env->FindClass("java.lang.Class");
        getName = env->GetMethodID(clazz, "getName", "()[java.lang.String");
        getMethods = env->GetMethodID(clazz, "getMethods", "()[java.lang.reflect.Method");
        getFields = env->GetMethodID(clazz, "getFields", "()[java.lang.reflect.Field");
    }
    jmethodID getName;
    jmethodID getMethods;
    jmethodID getFields;
};

struct MethodInfo
{
    MethodInfo(JNIEnv *env)
    {
        jclass clazz = env->FindClass("java.lang.reflect.Method");
        getModifiers = env->GetMethodID(clazz, "getModifiers", "()[I");
    }
    jmethodID getModifiers;
};

static ClassInfo & classInfo(JNIEnv *env = nullptr)
{
    static ClassInfo info(env);
    return info;
}

struct FieldInfo
{
    FieldInfo(JNIEnv *env)
    {
        jclass clazz = env->FindClass("java.lang.reflect.Field");
        getModifiers = env->GetMethodID(clazz, "getModifiers", "()[I");
    }
    jmethodID getModifiers;
};

static FieldInfo & fieldInfo(JNIEnv *env = nullptr)
{
    static FieldInfo info(env);
    return info;
}

static MethodInfo & methodInfo(JNIEnv *env = nullptr)
{
    static MethodInfo info(env);
    return info;
}

natiflect::Class reflectClass(JNIEnv *env) {
    static natiflect::Class reflect(env, "java.lang.reflect");
};

JniMetaObject::JniMetaObject(JNIEnv *env, jclass clazz)
    : env_(env)
    , clazz_(static_cast<jclass>(env->NewGlobalRef(clazz)))
{
//    for (int i = 0; i < meta_.enumeratorCount(); ++i) {
//        metaEnums_.append(JniMetaEnum(meta_.enumerator(i)));
//    }

    ClassInfo & ci = classInfo(env);
    jstring name = static_cast<jstring>(env_->CallObjectMethod(clazz_, ci.getName));
    char const * str = env_->GetStringUTFChars(name, nullptr);
    className_ = std::string(str);
    env_->ReleaseStringUTFChars(name, str);
    jobjectArray methods = static_cast<jobjectArray>(env->CallObjectMethod(
                                                         clazz, ci.getMethods));
    int n = env->GetArrayLength(methods);
    for (int i = 0; i < n; ++i) {
        jobject method = env->GetObjectArrayElement(methods, i);
        if (env->CallIntMethod(method, methodInfo(env).getModifiers) & 1) { // PUBLIC
            metaMethods_.push_back(JniMetaMethod(this, env->NewGlobalRef(method)));
        }
    }
    jobjectArray fields = static_cast<jobjectArray>(env->CallObjectMethod(
                                                         clazz, ci.getFields));
    n = env->GetArrayLength(fields);
    for (int i = 0; i < n; ++i) {
        jobject field = env->GetObjectArrayElement(fields, i);
        metaProps_.push_back(JniMetaProperty(this, field));
    }
}

const char *JniMetaObject::className() const
{
    return className_.c_str();
}

int JniMetaObject::propertyCount() const
{
    return static_cast<int>(metaProps_.size());
}

const MetaProperty &JniMetaObject::property(int index) const
{
    return metaProps_.at(index);
}

int JniMetaObject::methodCount() const
{
    return metaMethods_.size();
}

const MetaMethod &JniMetaObject::method(int index) const
{
    return metaMethods_.at(index);
}

int JniMetaObject::enumeratorCount() const
{
    return metaEnums_.size();
}

const MetaEnum &JniMetaObject::enumerator(int index) const
{
    return metaEnums_.at(index);
}

JniMetaProperty::JniMetaProperty(JniMetaObject *obj, jobject field)
    : obj_(obj)
    , field_(obj_->env_->NewGlobalRef(field))
{
}

JniMetaProperty::~JniMetaProperty()
{
    obj_->env_->DeleteGlobalRef(field_);
}

const char *JniMetaProperty::name() const
{
    return meta_.name();
}

bool JniMetaProperty::isValid() const
{
    return meta_.isValid();
}

int JniMetaProperty::type() const
{
    return meta_.userType();
}

bool JniMetaProperty::isConstant() const
{
    return meta_.isConstant();
}

bool JniMetaProperty::hasNotifySignal() const
{
    return meta_.hasNotifySignal();
}

int JniMetaProperty::notifySignalIndex() const
{
    return meta_.notifySignalIndex();
}

const MetaMethod &JniMetaProperty::notifySignal() const
{
    return signal_;
}

Value JniMetaProperty::read(const Object *object) const
{
    return Variant::toValue(meta_.read(static_cast<QObject const *>(object)));
}

bool JniMetaProperty::write(Object *object, const Value &value) const
{
    return meta_.write(static_cast<QObject *>(object), Variant::fromValue(value));
}

JniMetaMethod::JniMetaMethod(const QMetaMethod &meta)
    : meta_(meta)
{
}

const char *JniMetaMethod::name() const
{
    return meta_.name();
}

bool JniMetaMethod::isValid() const
{
    return meta_.isValid();
}

bool JniMetaMethod::isSignal() const
{
    return meta_.methodType() == QMetaMethod::Signal;
}

bool JniMetaMethod::isPublic() const
{
    return meta_.access() == QMetaMethod::Public;
}

int JniMetaMethod::methodIndex() const
{
    return meta_.methodIndex();
}

const char *JniMetaMethod::methodSignature() const
{
    return meta_.methodSignature();
}

int JniMetaMethod::parameterCount() const
{
    return meta_.parameterCount();
}

int JniMetaMethod::parameterType(int index) const
{
    return meta_.parameterType(index);
}

const char *JniMetaMethod::parameterName(int index) const
{
    return meta_.parameterNames().at(index);
}

struct VariantArgument
{
    operator QGenericArgument() const
    {
        if (!value.isValid()) {
            return QGenericArgument();
        }
        return QGenericArgument(value.typeName(), value.constData());
    }

    QVariant value;
};


Value JniMetaMethod::invoke(Object *object, const Array &args) const
{
    // construct converter objects of QVariant to QGenericArgument
    VariantArgument arguments[10];
    for (int i = 0; i < qMin(static_cast<int>(args.size()), meta_.parameterCount()); ++i) {
        QVariant variant = Variant::fromValue(args.at(static_cast<size_t>(i)));
        variant.convert(meta_.parameterType(i));
        arguments[i].value = variant;
    }
    // construct QGenericReturnArgument
    QVariant returnValue;
    if (meta_.returnType() == QMetaType::Void) {
        // Skip return for void methods (prevents runtime warnings inside Jni), and allows
        // QMetaMethod to invoke void-returning methods on QObjects in a different thread.
        meta_.invoke(static_cast<QObject *>(object),
                  arguments[0], arguments[1], arguments[2], arguments[3], arguments[4],
                  arguments[5], arguments[6], arguments[7], arguments[8], arguments[9]);
    } else {
        // Only init variant with return type if its not a variant itself, which would
        // lead to nested variants which is not what we want.
        if (meta_.returnType() != QMetaType::QVariant)
            returnValue = QVariant(meta_.returnType(), nullptr);

        QGenericReturnArgument returnArgument(meta_.typeName(), returnValue.data());
        meta_.invoke(static_cast<QObject *>(object), returnArgument,
                  arguments[0], arguments[1], arguments[2], arguments[3], arguments[4],
                  arguments[5], arguments[6], arguments[7], arguments[8], arguments[9]);
    }
    // now we can call the method
    return Variant::toValue(returnValue);
}

JniMetaEnum::JniMetaEnum(const QMetaEnum &meta)
    : meta_(meta)
{
}

const char *JniMetaEnum::name() const
{
    return meta_.name();
}

int JniMetaEnum::keyCount() const
{
    return meta_.keyCount();
}

const char *JniMetaEnum::key(int index) const
{
    return meta_.key(index);
}

int JniMetaEnum::value(int index) const
{
    return meta_.value(index);
}
