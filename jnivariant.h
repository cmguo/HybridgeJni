#ifndef JNIVARIANT_H
#define JNIVARIANT_H

#include "core/value.h"

#include <jni.h>

class JniVariant
{
public:
    static void init(JNIEnv * env);

    static Value toValue(jobject const & object);

    static jobject fromValue(Value const & value);
};

#endif // JNIVARIANT_H
