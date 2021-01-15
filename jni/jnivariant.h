#ifndef JNIVARIANT_H
#define JNIVARIANT_H

#include "core/value.h"

#include <jni.h>

class JniVariant
{
public:
    static void init(JNIEnv * env);

    static Value::Type type(jclass clazz);

    static Value toValue(jobject object);

    static jobject fromValue(Value const & value);

    static jobject registerObject(JNIEnv * env, jobject object);

    static jobject findObject(JNIEnv * env, jobject object);

    static jobject deregisterObject(JNIEnv * env, jobject object);
};

#endif // JNIVARIANT_H
