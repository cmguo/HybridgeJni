#include "jnichannel.h"
#include "jnimeta.h"
#include "jniclass.h"
#include "jnivariant.h"
#include "jniproxyobject.h"

#include <algorithm>

JniChannel::JniChannel(JNIEnv * env, jobject handle)
    : env_(env)
    , handle_(env_->NewWeakGlobalRef(handle))
{
    jclass clazz = env->GetObjectClass(handle);
    createUuid_ = env->GetMethodID(clazz, "createUuid", "()Ljava/lang/String;");
    startTimer_ = env->GetMethodID(clazz, "startTimer", "(I)V");
    stopTimer_ = env->GetMethodID(clazz, "stopTimer", "()V");
    classMetas_.emplace("java.lang.Object", new JniObjectMetaObject(env));
}

JniChannel::~JniChannel()
{
    env_->DeleteWeakGlobalRef(handle_);
}

MetaObject *JniChannel::metaObject(const Object *object) const
{
    jobject jobj = static_cast<jobject>(const_cast<Object*>(object));
    jclass jcls = env_->GetObjectClass(jobj);
    return metaObject2(jcls);
}

std::string JniChannel::createUuid() const
{
    jobject uuid = env_->CallObjectMethod(handle_, createUuid_);
    return JString(env_, uuid);
}

ProxyObject *JniChannel::createProxyObject(Map &&classinfo) const
{
    ProxyObject * po = new JniProxyObject(env_, std::move(classinfo));
    return po;
}

void JniChannel::startTimer(int msec)
{
    env_->CallVoidMethod(handle_, startTimer_, msec);
}

void JniChannel::stopTimer()
{
    env_->CallVoidMethod(handle_, stopTimer_);
}

void JniChannel::registerObject(const std::string &id, jobject object)
{
    object = JniVariant::registerObject(env_, object);
    Channel::registerObject(id, object);
}

void JniChannel::deregisterObject(jobject object)
{
    object = JniVariant::deregisterObject(env_, object);
    if (object != nullptr) {
        Channel::deregisterObject(object);
    }
}

void JniChannel::propertyChanged(jobject object, jstring property)
{
    JLocalClassRef clazz(env_, env_->GetObjectClass(object));
    JniMetaObject * meta = static_cast<JniMetaObject*>(metaObject2(clazz));
    object = JniVariant::findObject(env_, object);
    if (object != nullptr) {
        JniMetaProperty prop(JString(env_, property));
        meta->propertyChanged(this, object, prop.propertyIndex());
    }
}

void JniChannel::connectTo(Transport *transport, jobject response)
{
    MetaMethod::Response resp;
    if (response) {
        resp = [env = env_, response] (Value && result) {
            onResultClass(env).apply(response, JniVariant::fromValue(result));
        };
    }
    Channel::connectTo(transport, resp);
}

std::map<std::string, JniMetaObject*> JniChannel::classMetas_;

JniMetaObject *JniChannel::metaObject2(jclass clazz) const
{
    std::string name = classClass(env_).getName(clazz);
    auto it = classMetas_.find(name);
    if (it == classMetas_.end()) {
        jclass super = classClass().getSuperclass(clazz);
        JniMetaObject * smeta = metaObject2(super);
        JniMetaObject * meta = new JniMetaObject(smeta, clazz);
        it = classMetas_.insert(std::make_pair(name, meta)).first;
    }
    return it->second;
}
