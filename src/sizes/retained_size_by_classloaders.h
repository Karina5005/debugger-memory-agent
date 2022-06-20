// Copyright 2000-2018 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license that can be found in the LICENSE file.

#ifndef MEMORY_AGENT_RETAINED_SIZE_BY_CLASSLOADERS_H
#define MEMORY_AGENT_RETAINED_SIZE_BY_CLASSLOADERS_H

#include "../memory_agent_action.h"
#include "retained_size_action.h"
#include "retained_size_by_objects_merged.h"
#include "retained_size_by_objects.h"
#include "retained_size_by_classes.h"


class RetainedSizeByClassLoadersAction : public RetainedSizeAction<jlongArray> {
public:
    RetainedSizeByClassLoadersAction(JNIEnv *env, jvmtiEnv *jvmti, jobject object);
    RetainedSizeByObjectsMergedAction retainedSizeByObjectsMergedAction;
    RetainedSizeByObjectsAction retainedSizeByObjectsAction;

private:
    jlongArray executeOperation(jobjectArray classLoadersArray) override;
    jvmtiError getRetainedSizeByClassLoaders(jobjectArray classLoadersArray, std::vector<jlong> &result);
    jvmtiError getRetainedSizeByClassLoaderSimple(jobject classLoader, jlong* result);
    jvmtiError tagObjectsByClassLoader(jclass *classes, jint* cnt, jobject classLoader, jsize classLoaderIndex);
    jvmtiError tagObjectsByClassLoaderSimple(jclass *classes, jint* cnt, jobject classLoader);
    jlong getHeapSize();
    jlong tagOtherObjects(jclass *classes, jint* cnt, jobjectArray classLoadersArray);
};

jint JNICALL visitObject(jlong classTag, jlong size, jlong *tagPtr, jint length, void *userData);



#endif //MEMORY_AGENT_RETAINED_SIZE_BY_CLASSLOADERS_H
