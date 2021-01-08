#include "hybridgejni.h"
#include "jnichannel.h"
#include "jniclass.h"
#include "jnimeta.h"
#include "jniproxyobject.h"
#include "jnitransport.h"
#include "jnivariant.h"

#include <mutex>
#include <iostream>

static jclass sc_RuntimeException = nullptr;

HybridgeJni::HybridgeJni()
{
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*)
{
    std::cout << "JNI_OnLoad" << std::endl;
    JNIEnv *env = nullptr;
    int status = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (status != JNI_OK)
        return status;
    // RuntimeException
    sc_RuntimeException = env->FindClass("java/lang/RuntimeException");
    if (sc_RuntimeException == nullptr) {
        return JNI_ERR;
    }
    sc_RuntimeException = reinterpret_cast<jclass>(env->NewGlobalRef(sc_RuntimeException));
    // Channel methods
    JNINativeMethod methodsChannel[] = {
        {"create", "()J", reinterpret_cast<void*>(&JChannel::create)},
        {"registerObject", "(JLjava/lang/String;Ljava/lang/Object;)V", reinterpret_cast<void*>(&JChannel::registerObject)},
        {"deregisterObject", "(JLjava/lang/Object;)V", reinterpret_cast<void*>(&JChannel::deregisterObject)},
        {"blockUpdates", "(J)Z", reinterpret_cast<void*>(&JChannel::blockUpdates)},
        {"setBlockUpdates", "(JZ)V", reinterpret_cast<void*>(&JChannel::setBlockUpdates)},
        {"connectTo", "(JJLcom/tal/hybridge/ProxyObject$OnResult;)V", reinterpret_cast<void*>(&JChannel::connectTo)},
        {"disconnectFrom", "(JJ)V", reinterpret_cast<void*>(&JChannel::disconnectFrom)},
        {"propertyChanged", "(JLjava/lang/Object;Ljava/lang/String;)V", reinterpret_cast<void*>(&JChannel::propertyChanged)},
        {"timerEvent", "(J)V", reinterpret_cast<void*>(&JChannel::timerEvent)},
        {"free", "(J)V", reinterpret_cast<void*>(&JChannel::free)},
    };
    jclass clazzChannel = env->FindClass("com/tal/hybridge/Channel");
    if (clazzChannel == nullptr) {
        return JNI_ERR;
    }
    std::cout << "RegisterNatives" << std::endl;
    status = env->RegisterNatives(clazzChannel, reinterpret_cast<JNINativeMethod*>(methodsChannel), sizeof(methodsChannel) / sizeof(methodsChannel[0]));
    if (status != JNI_OK)
        return status;
    // Transport methods
    JNINativeMethod methodsTransport[] = {
        {"create", "()J", reinterpret_cast<void*>(&JTransport::create)},
        {"messageReceived", "(JLjava/lang/String;)V", reinterpret_cast<void*>(&JTransport::messageReceived)},
        {"free", "(J)V", reinterpret_cast<void*>(&JTransport::free)},
    };
    jclass clazzTransport = env->FindClass("com/tal/hybridge/Transport");
    if (clazzTransport == nullptr) {
        return JNI_ERR;
    }
    status = env->RegisterNatives(clazzTransport, reinterpret_cast<JNINativeMethod*>(methodsTransport), sizeof(methodsTransport) / sizeof(methodsTransport[0]));
    if (status != JNI_OK)
        return status;
    // ProxyObject methods
    JNINativeMethod methodsProxyObject[] = {
        {"setProperty", "(JLjava/lang/String;Ljava/lang/Object;)Z", reinterpret_cast<void*>(&JProxyObject::setProperty)},
        {"invokeMethod", "(JLjava/lang/reflect/Method;[Ljava/lang/Object;Lcom/tal/hybridge/ProxyObject$OnResult;)Z",
            reinterpret_cast<void*>(&JProxyObject::invokeMethod)},
        {"connect", "(JILcom/tal/hybridge/ProxyObject$SignalHandler;)Z", reinterpret_cast<void*>(&JProxyObject::connect)},
        {"disconnect", "(JILcom/tal/hybridge/ProxyObject$SignalHandler;)Z", reinterpret_cast<void*>(&JProxyObject::disconnect)},
    };
    jclass clazzProxyObject = env->FindClass("com/tal/hybridge/ProxyObject");
    if (clazzTransport == nullptr) {
        return JNI_ERR;
    }
    status = env->RegisterNatives(clazzProxyObject, reinterpret_cast<JNINativeMethod*>(methodsProxyObject), sizeof(methodsTransport) / sizeof(methodsTransport[0]));
    if (status != JNI_OK)
        return status;
    JniVariant::init(env);
    return JNI_VERSION_1_6;
}

static std::vector<std::shared_ptr<JniChannel>> channels(1, nullptr);
static std::vector<std::shared_ptr<JniTransport>> transports(1, nullptr);
static std::recursive_mutex smutex;

JNIEXPORT void JNI_OnUnload(JavaVM*, void*)
{
    std::cout << "JNI_OnUnload" << std::endl;
    std::lock_guard<std::recursive_mutex> l(smutex);
    transports.clear();
    channels.clear();
}

#define C(env, channel) \
    std::lock_guard<std::recursive_mutex> lc(smutex); \
    if (channel >= static_cast<jlong>(channels.size())) { \
        env->ThrowNew(sc_RuntimeException, "channel index out of range"); \
        return F; \
    } \
    std::shared_ptr<JniChannel> & c = channels[static_cast<size_t>(channel)]; \
    if (c == nullptr) { \
        env->ThrowNew(sc_RuntimeException, "channel item not found"); \
        return F; \
    }

#define T(env, transport) \
    std::lock_guard<std::recursive_mutex> lt(smutex); \
    if (transport >= static_cast<jlong>(transports.size())) { \
        env->ThrowNew(sc_RuntimeException, "transport index out of range"); \
        return F; \
    } \
    std::shared_ptr<JniTransport> & t = transports[static_cast<size_t>(transport)]; \
    if (t == nullptr) { \
        env->ThrowNew(sc_RuntimeException, "transport item not found"); \
        return F; \
    }

jlong JChannel::create(JNIEnv *env, jobject handle)
{
    std::cout << "JChannel::create" << std::endl;
    std::shared_ptr<JniChannel> c(new JniChannel(env, handle));
    std::lock_guard<std::recursive_mutex> l(smutex);
    auto iter = std::find(channels.begin() + 1, channels.end(), nullptr);
    if (iter == channels.end())
        iter = channels.insert(iter, c);
    else
        *iter = c;
    return std::distance(channels.begin(), iter);
}

void JChannel::registerObject(JNIEnv *env, jobject, jlong channel, jstring name, jobject object)
{
    std::cout << "JChannel::registerObject" << std::endl;
#undef F
#define F
    C(env, channel)
    c->registerObject(JString(env, name), object);
}

void JChannel::deregisterObject(JNIEnv *env, jobject, jlong channel, jobject object)
{
    std::cout << "JChannel::deregisterObject" << std::endl;
    C(env, channel)
    return c->deregisterObject(object);
}

jboolean JChannel::blockUpdates(JNIEnv *env, jobject, jlong channel)
{
#undef F
#define F false
    C(env, channel)
    return c->blockUpdates();
}

void JChannel::setBlockUpdates(JNIEnv *env, jobject, jlong channel, jboolean block)
{
#undef F
#define F
    C(env, channel)
    c->setBlockUpdates(block);
}

void JChannel::connectTo(JNIEnv *env, jobject, jlong channel, jlong transport, jobject response)
{
    std::cout << "JChannel::connectTo" << std::endl;
    C(env, channel)
    T(env, transport)
    c->connectTo(t.get(), response);
}

void JChannel::disconnectFrom(JNIEnv *env, jobject, jlong channel, jlong transport)
{
    std::cout << "JChannel::disconnectFrom" << std::endl;
    C(env, channel)
    T(env, transport)
    c->disconnectFrom(t.get());
}

void JChannel::propertyChanged(JNIEnv *env, jobject, jlong channel, jobject object, jstring name)
{
    std::cout << "JChannel::propertyChanged" << std::endl;
    C(env, channel)
    c->propertyChanged(object, name);
}

void JChannel::timerEvent(JNIEnv *env, jobject, jlong channel)
{
    C(env, channel)
    c->timerEvent();
}

void JChannel::free(JNIEnv *env, jobject, jlong channel)
{
    std::cout << "JChannel::free" << std::endl;
    C(env, channel)
    c.reset();
}

jlong JTransport::create(JNIEnv *env, jobject handle)
{
    std::cout << "JTransport::create" << std::endl;
    std::shared_ptr<JniTransport> t(new JniTransport(env, handle));
    std::lock_guard<std::recursive_mutex> l(smutex);
    auto iter = std::find(transports.begin() + 1, transports.end(), nullptr);
    if (iter == transports.end())
        iter = transports.insert(iter, t);
    else
        *iter = t;
    return std::distance(transports.begin(), iter);
}

void JTransport::messageReceived(JNIEnv *env, jobject, jlong transport, jstring message)
{
    std::cout << "JTransport::messageReceived" << std::endl;
    T(env, transport)
    t->messageReceived(message);
}

void JTransport::free(JNIEnv *env, jobject, jlong transport)
{
    std::cout << "JTransport::free" << std::endl;
    T(env, transport)
    t.reset();
}

jboolean JProxyObject::setProperty(JNIEnv *env, jobject, jlong handle, jstring property, jobject value)
{
    ProxyObject * po = reinterpret_cast<ProxyObject*>(handle);
    MetaObject const * meta = po->metaObj();
    JniMetaProperty jmp(JString(env, property));
    for (size_t i = 0; i < meta->propertyCount(); ++i) {
        MetaProperty const & mp = meta->property(i);
        if (jmp == mp) {
            Array emptyArray;
            return mp.write(po, JniVariant::toValue(value));
        }
    }
    return false;
}

jboolean JProxyObject::invokeMethod(JNIEnv *env, jobject, jlong handle, jobject method, jobjectArray args, jobject onResult)
{
    JniProxyObject * jpo = reinterpret_cast<JniProxyObject*>(handle);
    return jpo->invokeMethod(method, args, onResult);
}

jboolean JProxyObject::connect(JNIEnv *env, jobject, jlong handle, jint signalIndex, jobject handler)
{
    JniProxyObject * jpo = reinterpret_cast<JniProxyObject*>(handle);
    return jpo->connect(signalIndex, handler);
}

jboolean JProxyObject::disconnect(JNIEnv *env, jobject, jlong handle, jint signalIndex, jobject handler)
{
    JniProxyObject * jpo = reinterpret_cast<JniProxyObject*>(handle);
    return jpo->disconnect(signalIndex, handler);
}
