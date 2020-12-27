#ifndef HYBRIDGEJNI_H
#define HYBRIDGEJNI_H

#include "HybridgeJni_global.h"

#include <jni.h>

struct JChannel
{
    static jlong create(JNIEnv * env, jobject);
    static void registerObject(JNIEnv * env, jobject, jlong channel, jstring name, jobject object);
    static void deregisterObject(JNIEnv * env, jobject, jlong channel, jobject object);
    static jboolean blockUpdates(JNIEnv * env, jobject, jlong channel);
    static void setBlockUpdates(JNIEnv * env, jobject, jlong channel, jboolean block);
    static void connectTo(JNIEnv * env, jobject, jlong channel, jlong transport);
    static void disconnectFrom(JNIEnv * env, jobject, jlong channel, jlong transport);
    static void propertyChanged(JNIEnv * env, jobject, jlong channel, jobject object, jstring name);
    static void timerEvent(JNIEnv * env, jobject, jlong channel);
    static void free(JNIEnv * env, jobject, jlong channel);
};

struct JTransport
{
    static jlong create(JNIEnv * env, jobject);
    static void messageReceived(JNIEnv * env, jobject, jlong transport, jstring message);
    static void free(JNIEnv * env, jobject, jlong transport);
};


class HYBRIDGEJNI_EXPORT HybridgeJni
{
public:
    HybridgeJni();
};

#endif // HYBRIDGEJNI_H
