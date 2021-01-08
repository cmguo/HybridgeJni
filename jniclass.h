#ifndef JNIINFO_H
#define JNIINFO_H

#include <string>
#include <vector>

#include <jni.h>

struct MethodClass;
struct FieldClass;

struct Class
{
public:
    Class(JNIEnv *env, char const * className = nullptr);
    virtual ~Class();
    JNIEnv *env() const { return env_; }
    jclass clazz() const { return clazz_; }
    jobject newInstance();
    friend bool operator==(Class const & l, jclass const & r)
    {
        return l.env_->IsSameObject(l.clazz_, r);
    }
protected:
    JNIEnv *env_;
    jclass clazz_;
};

struct ClassClass
{
    ClassClass(JNIEnv *env);

    jboolean isPrimitive(jclass clazz) const;
    jboolean isArray(jclass clazz) const;
    jboolean isInstance(jclass clazz, jobject object) const;
    jboolean isAssignableFrom(jclass clazz, jclass clazz2) const;
    std::string getName(jclass clazz) const;
    std::vector<jobject> getDeclaredMethods(jclass clazz) const;
    std::vector<jobject> getDeclaredFields(jclass clazz) const;
    jclass getComponentType(jclass clazz) const;
    jclass getSuperclass(jclass clazz) const;

    jclass objectClass() const { return objectClass_; }
    jclass getClass(jobject object) const;
    std::string toString(jobject object) const;

private:
    JNIEnv * env_;
    jmethodID isPrimitive_;
    jmethodID isArray_;
    jmethodID isInstance_;
    jmethodID isAssignableFrom_;
    jmethodID getName_;
    jmethodID getDeclaredMethods_;
    jmethodID getDeclaredFields_;
    jmethodID getComponentType_;
    jmethodID getSuperclass_;

    // methods of Object
    jclass objectClass_;
    jmethodID getClass_;
    jmethodID toString_;
};

struct MemberClass
{
    MemberClass(JNIEnv *env);
    std::string getName(jobject member) const;
    jint getModifiers(jobject member) const;
    jclass getDeclaringClass(jobject member) const;

protected:
    JNIEnv * env_;
protected:
    jmethodID getName_;
    jmethodID getModifiers_;
    jmethodID getDeclaringClass_;
};

struct MethodClass : MemberClass
{
    MethodClass(JNIEnv *env);
    jint getParameterCount(jobject method) const;
    jobjectArray getParameterTypes(jobject method) const;
    jobject invoke(jobject method, jobject object, jobjectArray args) const;

private:
    jmethodID getParameterCount_;
    jmethodID getParameterTypes_;
    jmethodID invoke_;
};

struct FieldClass : MemberClass
{
    FieldClass(JNIEnv *env);
    jobject get(jobject field, jobject object) const;
    void set(jobject field, jobject object, jobject value) const;

private:
    jmethodID get_;
    jmethodID set_;
};

struct ModifierClass
{
    ModifierClass(JNIEnv *env);
    jboolean isPublic(int mod) const;
    jboolean isStatic(int mod) const;
    jboolean isAbstract(int mod) const;

private:
    JNIEnv * env_;
    jclass clazz_;
    jmethodID isPublic_;
    jmethodID isStatic_;
    jmethodID isAbstract_;
};

ClassClass & classClass(JNIEnv *env = nullptr);

FieldClass & fieldClass(JNIEnv *env = nullptr);

MethodClass & methodClass(JNIEnv *env = nullptr);

ModifierClass & modifierClass(JNIEnv *env = nullptr);

struct JThrowable
{
public:
    static void check(JNIEnv * env);
    static bool clear(JNIEnv * env);

    JThrowable(JNIEnv *env);
    jstring getMessage(jthrowable e) const;
    void printStackTrace(jthrowable e) const;

private:
    JNIEnv * env_;
    jmethodID getMessage_;
    jmethodID printStackTrace_;
};

class JObjectFinder
{
public:
    JObjectFinder(JNIEnv *env, jobject target) : env_(env), target_(target) {}
    bool operator()(jobject object) const
    {
        return env_->IsSameObject(target_, object);
    }
private:
    JNIEnv *env_;
    jobject target_;
};

template <typename T>
class JLocalRef
{
public:
    JLocalRef(JNIEnv *env, T target) : env_(env), target_(target) {}
    ~JLocalRef() { env_->DeleteLocalRef(target_); }
    operator T() { return target_; }
private:
    JNIEnv *env_;
    T target_;
};

typedef JLocalRef<jobject> JLocalObjectRef;
typedef JLocalRef<jclass> JLocalClassRef;

class JString
{
public:
    JString(JNIEnv *env, jobject jstr)
        : env_(env)
        , jstr_(static_cast<jstring>(jstr))
    {
        jboolean isCopy;
        str_ = env_->GetStringUTFChars(jstr_, &isCopy);
    }
    ~JString()
    {
        env_->ReleaseStringUTFChars(jstr_, str_);
    }
    operator std::string() const
    {
        return str_;
    }
private:
    JNIEnv *env_;
    jstring jstr_;
    char const * str_;
};

#endif // JNIINFO_H
