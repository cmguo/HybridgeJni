#include "jnimeta.h"
#include "jniproxyobject.h"
#include "jnivariant.h"

#include <core/metaobject.h>

JniProxyObject::JniProxyObject(JNIEnv * env, Map &&classinfo)
    : ProxyObject(std::move(classinfo))
    , env_(env)
{
    handle_ = proxyObjectClass(env).create(reinterpret_cast<jlong>(this));
    handle_ = env->NewGlobalRef(handle_);
}

JniProxyObject::~JniProxyObject()
{
    env_->DeleteGlobalRef(handle_);
    handle_ = nullptr;
}

jobject JniProxyObject::readProperty(jstring property)
{
    MetaObject const * meta = metaObj();
    JniMetaProperty jmp(JString(env_, property));
    for (size_t i = 0; i < meta->propertyCount(); ++i) {
        MetaProperty const & mp = meta->property(i);
        if (jmp == mp) {
            Array emptyArray;
            return JniVariant::fromValue(mp.read(this));
        }
    }
    return nullptr;
}

jboolean JniProxyObject::writeProperty(jstring property, jobject value)
{
    MetaObject const * meta = metaObj();
    JniMetaProperty jmp(JString(env_, property));
    for (size_t i = 0; i < meta->propertyCount(); ++i) {
        MetaProperty const & mp = meta->property(i);
        if (jmp == mp) {
            Array emptyArray;
            return mp.write(this, JniVariant::toValue(value));
        }
    }
    return false;
}

jboolean JniProxyObject::invokeMethod(jobject method, jobjectArray args, jobject onResult)
{
    MetaObject const * meta = metaObj();
    JniObjectMetaObject mo(env_);
    JniMetaMethod md(reinterpret_cast<JniMetaObject*>(&mo), method);
    for (size_t i = 0; i < meta->methodCount(); ++i) {
        if (md == meta->method(i)) {
            Array emptyArray;
            return md.invoke(this, std::move(JniVariant::toValue(args).toArray(emptyArray)),
                                  [onResult] (Value && result) {
                onResultClass().apply(onResult, JniVariant::fromValue(result));
            });
        }
    }
    return false;
}

static void handleSignal(void * handler, Object const * object, size_t index, Array && args)
{
    jobject jhandler = static_cast<jobject>(handler);
    jobject proxy = static_cast<jobject>(static_cast<JniProxyObject const*>(object)->handle());
    signalHandlerClass().apply(jhandler, proxy,
                               static_cast<int>(index),
                               static_cast<jobjectArray>(JniVariant::fromValue(args)));
}

jboolean JniProxyObject::connect(jint signalIndex, jobject handler)
{
    MetaObject const * meta = metaObj();
    size_t index = static_cast<size_t>(signalIndex);
    if (index >= meta->methodCount())
        return false;
    MetaMethod const & md = meta->method(index);
    if (!md.isSignal())
        return false;
    signalHandlerClass(env_);
    return meta->connect(MetaObject::Connection(this, index,
                                         JniVariant::registerObject(env_, handler), handleSignal));
}

jboolean JniProxyObject::disconnect(jint signalIndex, jobject handler)
{
    MetaObject const * meta = metaObj();
    size_t index = static_cast<size_t>(signalIndex);
    if (index >= meta->methodCount())
        return false;
    MetaMethod const & md = meta->method(index);
    if (!md.isSignal())
        return false;
    return meta->disconnect(MetaObject::Connection(this, index,
                                         JniVariant::deregisterObject(env_, handler), handleSignal));
}

ProxyObjectClass::ProxyObjectClass(JNIEnv *env)
    : Class(env, "com/tal/hybridge/ProxyObject")
{
    create_ = env->GetMethodID(clazz_, "<init>", "(J)V");
}

jobject ProxyObjectClass::create(jlong handle)
{
    return env_->NewObject(clazz_, create_, handle);
}

OnResultClass::OnResultClass(JNIEnv *env)
    : Class(env, "com/tal/hybridge/ProxyObject$OnResult")
{
    apply_ = env->GetMethodID(clazz_, "apply", "(Ljava/lang/Object;)V");
}

void OnResultClass::apply(jobject resp, jobject result)
{
    env_->CallVoidMethod(resp, apply_, result);
}

OnResultClass &onResultClass(JNIEnv *env)
{
    static OnResultClass c(env);
    return c;
}

SignalHandlerClass::SignalHandlerClass(JNIEnv *env)
    : Class(env, "com/tal/hybridge/ProxyObject$SignalHandler")
{
    apply_ = env->GetMethodID(clazz_, "apply", "(Lcom/tal/hybridge/ProxyObject;I[Ljava/lang/Object;)V");
}

void SignalHandlerClass::apply(jobject resp, jobject object, jint signalIndex, jobjectArray args)
{
    env_->CallVoidMethod(resp, apply_, object, signalIndex, args);
}

SignalHandlerClass &signalHandlerClass(JNIEnv *env)
{
    static SignalHandlerClass c(env);
    return c;
}

ProxyObjectClass &proxyObjectClass(JNIEnv *env)
{
    static ProxyObjectClass c(env);
    return c;
}
