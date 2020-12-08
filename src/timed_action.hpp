// Copyright 2000-2018 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license that can be found in the LICENSE file.

#ifndef MEMORY_AGENT_TIMED_ACTION_HPP
#define MEMORY_AGENT_TIMED_ACTION_HPP

#include "timed_action.h"


template<typename RESULT_TYPE, typename... ARGS_TYPES>
MemoryAgentTimedAction<RESULT_TYPE, ARGS_TYPES...>::MemoryAgentTimedAction(JNIEnv *env, jvmtiEnv *jvmti, jobject object) :
    env(env), jvmti(jvmti) {
    jclass thisClass = env->GetObjectClass(object);
    jfieldID fileNameId = env->GetFieldID(thisClass, "cancellationFileName", "Ljava/lang/String;");
    jfieldID timeoutId = env->GetFieldID(thisClass, "timeoutInMillis", "J");
    jobject fileName = env->GetObjectField(object, fileNameId);
    jlong timeout = env->GetLongField(object, timeoutId);
    cancellationFileName = jstringTostring(env, reinterpret_cast<jstring>(fileName));
    if (timeout < 0) {
        finishTime = std::chrono::steady_clock::time_point::max();
    } else {
        finishTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
    }
}

template<typename RESULT_TYPE, typename... ARGS_TYPES>
jobjectArray MemoryAgentTimedAction<RESULT_TYPE, ARGS_TYPES...>::run(ARGS_TYPES... args) {
    RESULT_TYPE result = executeOperation(args...);
    jvmtiError err = cleanHeap();
    if (err != JVMTI_ERROR_NONE) {
        handleError(jvmti, err, "Couldn't clean heap");
    }

    jclass langObject = env->FindClass("java/lang/Object");
    jobjectArray returnValue = env->NewObjectArray(2, langObject, nullptr);
    env->SetObjectArrayElement(returnValue, 0, toJavaArray(env, static_cast<jint>(getErrorCode())));
    env->SetObjectArrayElement(returnValue, 1, result);
    std::remove(cancellationFileName.c_str());
    return returnValue;
}

template<typename RESULT_TYPE, typename... ARGS_TYPES>
jint JNICALL MemoryAgentTimedAction<RESULT_TYPE, ARGS_TYPES...>::followReferencesCallbackWrapper(jvmtiHeapReferenceKind refKind, const jvmtiHeapReferenceInfo *refInfo, jlong classTag,
                                                                                                 jlong referrerClassTag, jlong size, jlong *tagPtr,
                                                                                                 jlong *referrerTagPtr, jint length, void *userData) {
    auto *wrapperData = reinterpret_cast<CallbackWrapperData *>(userData);
    if (wrapperData->manager->shouldStopExecutionSyscallSafe()) {
        return JVMTI_VISIT_ABORT;
    }
    return reinterpret_cast<jvmtiHeapReferenceCallback>(wrapperData->callback)(refKind, refInfo, classTag, referrerClassTag, size, tagPtr, referrerTagPtr, length, wrapperData->userData);
}

template<typename RESULT_TYPE, typename... ARGS_TYPES>
jint JNICALL MemoryAgentTimedAction<RESULT_TYPE, ARGS_TYPES...>::iterateThroughHeapCallbackWrapper(jlong classTag, jlong size, jlong *tagPtr, jint length, void *userData) {
    auto *wrapperData = reinterpret_cast<CallbackWrapperData *>(userData);
    if (wrapperData->manager->shouldStopExecutionSyscallSafe()) {
        return JVMTI_ITERATION_ABORT;
    }
    return reinterpret_cast<jvmtiHeapIterationCallback>(wrapperData->callback)(classTag, size, tagPtr, length, wrapperData->userData);
}

template<typename RESULT_TYPE, typename... ARGS_TYPES>
jvmtiError MemoryAgentTimedAction<RESULT_TYPE, ARGS_TYPES...>::FollowReferences(jint heapFilter, jclass klass, jobject initialObject,
                                                                                jvmtiHeapReferenceCallback callback, void *userData,
                                                                                const char *debugMessage) const {
    if (shouldStopExecution()) return MEMORY_AGENT_INTERRUPTED_ERROR;

    if (debugMessage) {
        debug(debugMessage);
    }

    jvmtiHeapCallbacks cb;
    std::memset(&cb, 0, sizeof(jvmtiHeapCallbacks));
    cb.heap_reference_callback = followReferencesCallbackWrapper;

    CallbackWrapperData wrapperData(reinterpret_cast<void *>(callback), userData, dynamic_cast<const CancellationManager *>(this));
    return jvmti->FollowReferences(heapFilter, klass, initialObject, &cb, &wrapperData);
}

template<typename RESULT_TYPE, typename... ARGS_TYPES>
jvmtiError MemoryAgentTimedAction<RESULT_TYPE, ARGS_TYPES...>::IterateThroughHeap(jint heapFilter, jclass klass, jvmtiHeapIterationCallback callback,
                                                                                   void *userData, const char *debugMessage) const {
    if (shouldStopExecution()) return MEMORY_AGENT_INTERRUPTED_ERROR;

    if (debugMessage) {
        debug(debugMessage);
    }

    jvmtiHeapCallbacks cb;
    std::memset(&cb, 0, sizeof(jvmtiHeapCallbacks));
    cb.heap_iteration_callback = iterateThroughHeapCallbackWrapper;

    CallbackWrapperData wrapperData(reinterpret_cast<void *>(callback), userData, dynamic_cast<const CancellationManager *>(this));
    return jvmti->IterateThroughHeap(heapFilter, klass, &cb, &wrapperData);
}

template<typename RESULT_TYPE, typename... ARGS_TYPES>
ErrorCode MemoryAgentTimedAction<RESULT_TYPE, ARGS_TYPES...>::getErrorCode() const {
    if (fileExists(cancellationFileName)) {
        return ErrorCode::CANCELLED;
    } else if (finishTime < std::chrono::steady_clock::now()) {
        return ErrorCode::TIMEOUT;
    }
    return ErrorCode::OK;
}

#endif //MEMORY_AGENT_TIMED_ACTION_HPP