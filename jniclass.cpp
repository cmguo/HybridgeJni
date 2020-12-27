#include "jniclass.h"

ClassClass::ClassClass(JNIEnv *env)
    : env_(env)
{
    jclass clazz = env->FindClass("java/lang/Class");
    isPrimitive_ = env->GetMethodID(clazz, "isPrimitive", "()Z");
    isArray_ = env->GetMethodID(clazz, "isArray", "()Z");
    isInstance_ = env->GetMethodID(clazz, "isInstance", "(Ljava/lang/Object;)Z");
    getName_ = env->GetMethodID(clazz, "getName", "()Ljava/lang/String;");
    getDeclaredMethods_ = env->GetMethodID(clazz, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    getDeclaredFields_ = env->GetMethodID(clazz, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
    getComponentType_ = env->GetMethodID(clazz, "getComponentType", "()Ljava/lang/Class;");
    getSuperclass_ = env->GetMethodID(clazz, "getSuperclass", "()Ljava/lang/Class;");

    clazz = env->FindClass("java/lang/Object");
    objectClass_ = static_cast<jclass>(env->NewGlobalRef(clazz));
    getClass_ = env->GetMethodID(clazz, "getClass", "()Ljava/lang/Class;");
    toString_ = env->GetMethodID(clazz, "toString", "()Ljava/lang/String;");
    JThrowable::check(env);
}

jboolean ClassClass::isPrimitive(jclass clazz) const
{
    return env_->CallBooleanMethod(clazz, isPrimitive_);
}

jboolean ClassClass::isArray(jclass clazz) const
{
    return env_->CallBooleanMethod(clazz, isArray_);
}

jboolean ClassClass::isInstance(jclass clazz, jobject object) const
{
    return env_->CallBooleanMethod(clazz, isInstance_, object);
}

std::string ClassClass::getName(jclass clazz) const
{
    return JString(env_, env_->CallObjectMethod(clazz, getName_));
}

std::vector<jobject> ClassClass::getDeclaredMethods(jclass clazz) const
{
    jobjectArray methods = static_cast<jobjectArray>(
                env_->CallObjectMethod(clazz, getDeclaredMethods_));
    int n = env_->GetArrayLength(methods);
    std::vector<jobject> methods2;
    for (int i = 0; i < n; ++i) {
        methods2.push_back(env_->GetObjectArrayElement(methods, i));
    }
    return methods2;
}

std::vector<jobject> ClassClass::getDeclaredFields(jclass clazz) const
{
    jobjectArray fields = static_cast<jobjectArray>(
                env_->CallObjectMethod(clazz, getDeclaredFields_));
    int n = env_->GetArrayLength(fields);
    std::vector<jobject> fields2;
    for (int i = 0; i < n; ++i) {
        fields2.push_back(env_->GetObjectArrayElement(fields, i));
    }
    return fields2;
}

jclass ClassClass::getComponentType(jclass clazz) const
{
    return static_cast<jclass>(env_->CallObjectMethod(clazz, getComponentType_));
}

jclass ClassClass::getSuperclass(jclass clazz) const
{
    return static_cast<jclass>(env_->CallObjectMethod(clazz, getSuperclass_));
}

jclass ClassClass::getClass(jobject object) const
{
    return static_cast<jclass>(env_->CallObjectMethod(object, getClass_));
}

std::string ClassClass::toString(jobject object) const
{
    return JString(env_, env_->CallObjectMethod(object, toString_));
}

MemberClass::MemberClass(JNIEnv *env)
	: env_(env)
{
    jclass clazz = env->FindClass("java/lang/reflect/Member");
    getName_ = env->GetMethodID(clazz, "getName", "()Ljava/lang/String;");
    getModifiers_ = env->GetMethodID(clazz, "getModifiers", "()I");
    getDeclaringClass_ = env->GetMethodID(clazz, "getDeclaringClass", "()Ljava/lang/Class;");
}

std::string MemberClass::getName(jobject member) const
{
    return JString(env_, env_->CallObjectMethod(member, getName_));
}

jint MemberClass::getModifiers(jobject member) const
{
    return env_->CallIntMethod(member, getModifiers_);
}

jclass MemberClass::getDeclaringClass(jobject member) const
{
    return static_cast<jclass>(env_->CallObjectMethod(member, getDeclaringClass_));
}

MethodClass::MethodClass(JNIEnv *env)
    : MemberClass(env)
{
    jclass clazz = env->FindClass("java/lang/reflect/Method");
    getParameterCount_ = env->GetMethodID(clazz, "getParameterCount", "()I");
    invoke_ = env->GetMethodID(clazz, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    JThrowable::check(env);
}

jint MethodClass::getParameterCount(jobject method) const
{
    return env_->CallIntMethod(method, getParameterCount_);
}

jobject MethodClass::invoke(jobject method, jobject object, jobjectArray args) const
{
    return env_->CallObjectMethod(method, invoke_, object, args);
}

FieldClass::FieldClass(JNIEnv *env)
    : MemberClass(env)
{
    jclass clazz = env->FindClass("java/lang/reflect/Field");
    get_ = env->GetMethodID(clazz, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
    set_ = env->GetMethodID(clazz, "set", "(Ljava/lang/Object;Ljava/lang/Object;)V");
    JThrowable::check(env);
}

jobject FieldClass::get(jobject field, jobject object) const
{
    return env_->CallObjectMethod(field, get_, object);
}

void FieldClass::set(jobject field, jobject object, jobject value) const
{
    env_->CallObjectMethod(field, set_, object, value);
}

ClassClass &classClass(JNIEnv *env)
{
    static ClassClass clazz(env);
    return clazz;
}

FieldClass &fieldClass(JNIEnv *env)
{
    static FieldClass clazz(env);
    return clazz;
}

MethodClass &methodClass(JNIEnv *env)
{
    static MethodClass clazz(env);
    return clazz;
}

void JThrowable::check(JNIEnv *env)
{
    jthrowable e = env->ExceptionOccurred();
    if (e) {
        static JThrowable th(env);
        th.printStackTrace(e);
        throw std::runtime_error(JString(env, th.getMessage(e)));
    }
}

JThrowable::JThrowable(JNIEnv *env)
    : env_(env)
{
    jclass clazz = env->FindClass("java/lang/Throwable");
    getMessage_ = env->GetMethodID(clazz, "getMessage", "()Ljava/lang/String;");
    printStackTrace_ = env->GetMethodID(clazz, "printStackTrace", "()V");
}

jstring JThrowable::getMessage(jthrowable e) const
{
    return static_cast<jstring>(env_->CallObjectMethod(e, getMessage_));
}

void JThrowable::printStackTrace(jthrowable e) const
{
    env_->CallVoidMethod(e, printStackTrace_);
}

ModifierClass::ModifierClass(JNIEnv *env)
    : env_(env)
{
    jclass clazz = env->FindClass("java/lang/reflect/Modifier");
    clazz_ = static_cast<jclass>(env->NewGlobalRef(clazz));
    isPublic_ = env->GetStaticMethodID(clazz, "isPublic", "(I)Z");
    isStatic_ = env->GetStaticMethodID(clazz, "isStatic", "(I)Z");
    isAbstract_ = env->GetStaticMethodID(clazz, "isAbstract", "(I)Z");
    JThrowable::check(env);
}

jboolean ModifierClass::isPublic(int mod) const
{
    return env_->CallStaticBooleanMethod(clazz_, isPublic_, mod);
}

jboolean ModifierClass::isStatic(int mod) const
{
    return env_->CallStaticBooleanMethod(clazz_, isStatic_, mod);
}

jboolean ModifierClass::isAbstract(int mod) const
{
    return env_->CallStaticBooleanMethod(clazz_, isAbstract_, mod);
}

ModifierClass &modifierClass(JNIEnv *env)
{
    static ModifierClass clazz(env);
    return clazz;
}
