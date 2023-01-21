#include "Bytes.h"

#include "jdk/jni.h"
#include "jdk/jni_md.h"
#include "jdk/jvmti.h"
#include <filesystem>
typedef jint(JNICALL* tJNI_GetCreatedJavaVMs)(JavaVM** vmBuf, jsize bufLen, jsize* numVMs);
tJNI_GetCreatedJavaVMs GetCreatedJavaVMs = (tJNI_GetCreatedJavaVMs)GetProcAddress(GetModuleHandle("jvm.dll"), "JNI_GetCreatedJavaVMs");

typedef jclass(JNICALL* tJVM_FindClassFromClassLoader)(JNIEnv* env, const char* name, jboolean init, jobject loader, jboolean throwError);
tJVM_FindClassFromClassLoader JVM_FindClassFromClassLoader = (tJVM_FindClassFromClassLoader)GetProcAddress(GetModuleHandle("jvm.dll"), "JVM_FindClassFromClassLoader");

VOID CheckJVMTIError(jvmtiEnv* tiEnv, jvmtiError tiError, const char* phase)
{
	if (tiError != JVMTI_ERROR_NONE)
	{
		char* error;
		tiEnv->GetErrorName(tiError, &error);

		MessageBoxA(NULL, error, phase, MB_OK | MB_ICONERROR);

		tiEnv->Deallocate((unsigned char*)error);
		ExitProcess(0);
	}
}

VOID JNICALL ClassFileLoadHook(jvmtiEnv* tiEnv, JNIEnv* env,
	jclass class_being_redefined, jobject loader,
	const char* name, jobject protection_domain,
	jint data_len, const unsigned char* data,
	jint* new_data_len, unsigned char** new_data)
{
	if (name != NULL && !strcmp(name, "aqz")) // your full class name (like net/minecraft/client/Minecraft or net/minecraft/util/Timer)
	{
		*new_data_len = sizeof(Block_class);

		jvmtiError tiError = tiEnv->Allocate(sizeof(Block_class), new_data);
		CheckJVMTIError(tiEnv, tiError, "Phase: Allocate");

		memcpy(*new_data, Block_class, sizeof(Block_class));
	}
}

jobject getClassLoader(JNIEnv* env, jvmtiEnv* tiEnv, const char* name)
{
	jint count;
	jthread* threads;

	tiEnv->GetAllThreads(&count, &threads);

	for (int i = 0; i < count; i++)
	{
		jthread thread = threads[i];

		jvmtiThreadInfo threadInfo;
		tiEnv->GetThreadInfo(thread, &threadInfo);

		if (threadInfo.context_class_loader != NULL)
		{
			jclass ClassLoader = env->GetObjectClass(threadInfo.context_class_loader);

			char* contextName;
			tiEnv->GetClassSignature(ClassLoader, &contextName, NULL);

			if (contextName != NULL && strstr(contextName, name))
			{
				tiEnv->Deallocate((unsigned char*)contextName);
				return threadInfo.context_class_loader;
			}

			tiEnv->Deallocate((unsigned char*)contextName);
		}
	}

	return NULL;
}

DWORD WINAPI SiberiaThread()
{

	

		jsize vmSize;
		GetCreatedJavaVMs(NULL, 0, &vmSize);

		JavaVM** jvm = new JavaVM * [vmSize];
		GetCreatedJavaVMs(jvm, vmSize, &vmSize);

		JNIEnv* env;
		jvmtiEnv* tiEnv;

		jvm[0]->AttachCurrentThread((void**)&env, NULL);
		jvm[0]->GetEnv((void**)&env, JNI_VERSION_1_8);

		jvm[0]->AttachCurrentThread((void**)&tiEnv, NULL);
		jvm[0]->GetEnv((void**)&tiEnv, JVMTI_VERSION);

		jvmtiCapabilities capabilities;
		(void)memset(&capabilities, 0, sizeof(jvmtiCapabilities));

		capabilities.can_retransform_classes = TRUE;
		capabilities.can_retransform_any_class = TRUE;
		capabilities.can_generate_all_class_hook_events = TRUE;

		jvmtiError tiError = tiEnv->AddCapabilities(&capabilities);
		CheckJVMTIError(tiEnv, tiError, "Phase: AddCapabilities");

		jvmtiEventCallbacks callbacks = { 0 };

		callbacks.ClassFileLoadHook = ClassFileLoadHook;

		tiEnv->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
		tiEnv->SetEventCallbacks(&callbacks, sizeof(callbacks));

		jobject mcLoader = getClassLoader(env, tiEnv, "ClassLoader");

		if (!mcLoader)
		{
			MessageBox(NULL, "[STERN] Classloader not found!", "Error", MB_OK | MB_ICONERROR);
			ExitProcess(0);
		}

		jclass Block = JVM_FindClassFromClassLoader(env, "aqz", true, mcLoader, false); //defini le nom de class (Block).

		if (!Block)
		{
			MessageBox(NULL, "[STERN] block not found!", "Error", MB_OK | MB_ICONERROR);
			ExitProcess(0);
		}

		tiError = tiEnv->RetransformClasses(1, &Block);
		CheckJVMTIError(tiEnv, tiError, "Phase: Retransform");
		Beep(500, 300);
		ExitThread(0);
	
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
	
		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)SiberiaThread, NULL, NULL, NULL);

		return TRUE;
	}
	

	
	Beep(400, 500);

	return FALSE;
}