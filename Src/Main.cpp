/* @@@LICENSE
*
*      Copyright (c) 2008-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */




#include "Common.h"

#include "HostBase.h"
#include "ApplicationDescription.h"
#include "ApplicationManager.h"
#include "BootupAnimation.h"
#include "CpuAffinity.h"
//MDK-LAUNCHER #include "DockPositionManager.h"
#include "HapticsController.h"
#include "IpcServer.h"
#include "Localization.h"
#include "WindowServer.h"
#include "WebAppManager.h"
#include "WebAppMgrProxy.h"
#include "MemoryMonitor.h"
#include "Settings.h"
#include "SystemService.h"
#include "ApplicationInstaller.h"
#include "Preferences.h"
#include "DeviceInfo.h"
#include "Security.h"
#include "EASPolicyManager.h"
#include "Logging.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <ucontext.h>
#include <fcntl.h>
#include <sys/file.h>

#include <QApplication>
#include <QtGui>
#include <QtGlobal> 

/* Convenience macro for simulating crashes for debugging purposes only:
#define crash() {                               \
        volatile int *ip = (volatile int *)0;   \
        *ip = 0;                                \
    }
*/


//#define PRINT_MALLOC_STATS
#ifdef __cplusplus
extern "C" {
#endif
extern void malloc_stats(void);
#ifdef __cplusplus
};
#endif

static gchar* s_uiStr = NULL;
static gchar* s_appToLaunchStr = NULL;
static gchar* s_logLevelStr = NULL;
static gboolean s_useSysLog = false;
static gboolean s_colorLog = true;
static gboolean s_useTerminal = false;
static gchar* s_debugTrapStr = NULL;
static gboolean s_forceSoftwareRendering = false;
static gchar* s_mallocStatsFileStr = NULL;
static int s_mallocStatsInterval = -1;

// debugCrashes indicates whether the user has specified "-x on" in the command
// line args to enable debugging of crashes.
static bool debugCrashes = false;

// Used to allow us to bail from the handler when we're done debugging.  This
// is meant to be set only from within a gdb seesion.
volatile bool stayInLoop = true;

int crashLogFD = -1;
pid_t sysmgrPid;

pid_t bootAnimPid;
int   bootAnimPipeFd=-1, sysmgrPipeFd=-1;
int   WebAppMgrPipeFd=-1, IpcServerPipeFd=-1;
char  msgOkToContinue = 0xAB;

// Safe printf that does not call malloc (to avoid re-entrancy issue when
// called from the signal handler.
static void crash_printf(const char *format, ...) {
    // Allocate the buffer staticly because we don't want to assume that
    // there's a lot of stack space left at the time of the crash:
	static char buf[512];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	ssize_t result = write(crashLogFD, buf, strlen(buf));
	Q_UNUSED(result);
}

static void crash_flush() {
    fsync(crashLogFD);
}

#ifdef __i386__

// Register context dumper for ia32 / x64:
#if __WORDSIZE == 64
const char *const regNames[NGREG] = {
    "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15",
    "RDI", "RSI", "RBP" "RBX", "RDX", "RAX", "RCX", "RIP",
    "EFL", "CSGSFS", "ERR", "TRAPNO", "OLDMASK", "CR2"
};
#else // __WORDSIZE == 32
const char *const regNames[NGREG] = {
    "GS", "FS", "ES", "DS", "EDI", "ESI", "EBP", "ESP",
    "EBX", "EDX", "ECX", "EAX", "TRAPNO", "ERR", "EIP",
    "CS", "EFL", "UESP", "SS"
};
#endif

void logCrashRegisterContext(int sig, siginfo_t *info, void *data) {
    ucontext_t *context = reinterpret_cast<ucontext_t *>(data);
    crash_printf("reg context {\n");

    // Note: For ia32/x64, we dump all the registers because this is not for
    // field deployment.  Hence, there is no risk of violation of user privacy.
    for (int i = 0; i < NGREG; i++) {
        int value = context->uc_mcontext.gregs[i];
        crash_printf("  %2d %8s = 0x%08x %d\n", i, regNames[i], value, value);
    }

    crash_printf("}\n");
    crash_flush();
}

#elif defined(__arm__)

// Register context dumper for ARM:
void logCrashRegisterContext(int sig, siginfo_t *info, void *data) {
    ucontext_t *context = reinterpret_cast<ucontext_t *>(data);
    sigcontext *scon = &context->uc_mcontext;
    crash_printf("reg context {\n");

    // Dumping non-sensitive register content:
    crash_printf("  // non-sensitive register content:\n");
    crash_printf("  trap_no       = 0x%08x %d\n", scon->trap_no, scon->trap_no);
    crash_printf("  error_code    = 0x%08x %d\n", scon->error_code, scon->error_code);
    crash_printf("  oldmask       = 0x%08x %d\n", scon->oldmask, scon->oldmask);

    crash_printf("  arm_sp        = 0x%08x %d\n", scon->arm_sp, scon->arm_sp);
    crash_printf("  arm_lr        = 0x%08x %d\n", scon->arm_lr, scon->arm_lr);
    crash_printf("  arm_pc        = 0x%08x %d\n", scon->arm_pc, scon->arm_pc);
    crash_printf("  arm_cpsr      = 0x%08x %d\n", scon->arm_cpsr, scon->arm_cpsr);
    crash_printf("  fault_address = 0x%08x %d\n", scon->fault_address, scon->fault_address);

    if (Settings::LunaSettings()->debug_doVerboseCrashLogging) {
        // Dumping sensitive register content for internal debugging only.
        // By default, this data should not be dumped in the field (to avoid
        // potential violation of user privacy issues):
        crash_printf("  arm_r0        = 0x%08x %d\n", scon->arm_r0, scon->arm_r0);
        crash_printf("  arm_r1        = 0x%08x %d\n", scon->arm_r1, scon->arm_r1);
        crash_printf("  arm_r2        = 0x%08x %d\n", scon->arm_r2, scon->arm_r2);
        crash_printf("  arm_r3        = 0x%08x %d\n", scon->arm_r3, scon->arm_r3);
        crash_printf("  arm_r4        = 0x%08x %d\n", scon->arm_r4, scon->arm_r4);
        crash_printf("  arm_r5        = 0x%08x %d\n", scon->arm_r5, scon->arm_r5);
        crash_printf("  arm_r6        = 0x%08x %d\n", scon->arm_r6, scon->arm_r6);
        crash_printf("  arm_r7        = 0x%08x %d\n", scon->arm_r7, scon->arm_r7);
        crash_printf("  arm_r8        = 0x%08x %d\n", scon->arm_r8, scon->arm_r8);
        crash_printf("  arm_r9        = 0x%08x %d\n", scon->arm_r9, scon->arm_r9);
        crash_printf("  arm_r10       = 0x%08x %d\n", scon->arm_r10, scon->arm_r10);
        crash_printf("  arm_fp        = 0x%08x %d\n", scon->arm_fp, scon->arm_fp);
        crash_printf("  arm_ip        = 0x%08x %d\n", scon->arm_ip, scon->arm_ip);
    }

    crash_printf("}\n");
    crash_flush();
}

#endif // __arm__

static volatile bool hasCrashedInCrashHandler = false;

static void innerCrashHandler(int sig, siginfo_t *info, void *data);
static void outerCrashHandler(int sig, siginfo_t *info, void *data);

static void installInnerCrashHandler(int sig,
                                     struct sigaction *previous_crash_action)
{
    struct sigaction crash_action;
    sigset_t block_mask;
    sigfillset(&block_mask);
    crash_action.sa_flags = SA_SIGINFO | SA_RESETHAND;
    crash_action.sa_mask = block_mask;
    crash_action.sa_sigaction = &innerCrashHandler;

    // Install the handler for signals that we want to trap:
    sigaction(sig, &crash_action, previous_crash_action);
}

static void installOuterCrashHandler(int sig)
{
    struct sigaction crash_action;
    sigset_t block_mask;
    sigfillset(&block_mask);
    sigdelset(&block_mask, SIGSEGV);
    crash_action.sa_flags = SA_SIGINFO | SA_RESETHAND | SA_NODEFER;
    crash_action.sa_mask = block_mask;
    crash_action.sa_sigaction = &outerCrashHandler;

    // Install the handler for signals that we want to trap:
    sigaction(sig, &crash_action, NULL);
}

static gboolean mallocStatsCb(gpointer data)
{
	char buf[30];
	time_t cur_time;
	time(&cur_time);
	static pid_t my_pid = 0;
	static char process_name[16] = { 0 };

	if (!my_pid) {
		my_pid = getpid();
	}

	if (!process_name[0]) {
		::prctl(PR_GET_NAME, (unsigned long)process_name, 0, 0, 0);
		process_name[sizeof(process_name) - 1] = '\0';
	}

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

    flock(STDERR_FILENO, LOCK_EX);

	fflush(stderr);
	fprintf(stderr, "\nMALLOC STATS FOR PROCESS: \"%s\" (PID: %d) AT [%ld.%ld] %s", process_name, my_pid, ts.tv_sec, ts.tv_nsec, ctime_r(&cur_time, buf));
	fflush(stderr);
	malloc_stats();
	fprintf(stderr, "\n\n");
	fflush(stderr);
	fsync(STDERR_FILENO);

    flock(STDERR_FILENO, LOCK_UN);
	
	
	return TRUE;
}

static void initMallocStatsCb(GMainLoop* mainLoop, int secs)
{
	// negative means no stats
	if ((secs < 0) || (s_mallocStatsFileStr == NULL)) return;

	GSource *timeoutSource = g_timeout_source_new_seconds(secs);
	g_source_set_callback(timeoutSource, mallocStatsCb, NULL, NULL);
	g_source_attach(timeoutSource, g_main_loop_get_context(mainLoop));
}

static void setupMallocStats(const char* mallocStatsFile)
{
	FILE* file = ::freopen(mallocStatsFile, "a+", stderr);

	if (!file) {
		g_critical("Unable to open file: %s", mallocStatsFile);
	}
}

static void innerCrashHandler(int sig, siginfo_t *info, void *data)
{
    if (sig == SIGSEGV) {
        // The inner crash handler is expecting to only see SIGSEGVs as a
        // result of memory dumps that may have unknowingly touched a
        // non-accessible page of memory.  In this case, we simply bypass
        // the instruction of the memory access and flag the error so that
        // the outer crash handler know not to continue with this region
        // of memory.
#if defined(__arm__)
        ucontext_t *context = reinterpret_cast<ucontext_t *>(data);
        sigcontext *scon = &context->uc_mcontext;
        scon->arm_pc = scon->arm_pc + 4;
#endif
        hasCrashedInCrashHandler = true;

        // Note: We need to reinstall the inner signal handler because the
        // outer handler may rely on it to safe access multiple regions of
        // memory.

        // Note: We only reinstall the inner signal handler if the fault is a
        // known SIGSEGV fault that we know how to handle.  Otherwise, let the
        // normal faulting system handle the crash.
        installInnerCrashHandler(sig, NULL);
    }
}

static void outerCrashHandler(int sig, siginfo_t *info, void *data)
{
    // Let webkit handle the crash if it wants to.  If it returns true,
    // the signal has been handled.  Just return:
    /*if (Palm::WebGlobal::handleSignal(sig, info, data)) {

        // Webkit side has handled the signal.  Hence, we're not going to crash,
        // and will keep running.  Therefore, we need to re-install the signal
        // handler:
        installOuterCrashHandler(sig);
        return;
    }*/

    // Otherwise, report the crash diagnostics:
    struct sigaction previous_crash_action;
    installInnerCrashHandler(sig, &previous_crash_action);

    const char *logFileName = Settings::LunaSettings()->logFileName.c_str();
    char logFileNameBuffer[64];
    if (Settings::LunaSettings()->debug_doVerboseCrashLogging) {
        snprintf(logFileNameBuffer, 64,
                 "/media/internal/lunasysmgr.%d.verbose.log", sysmgrPid);
        logFileName = logFileNameBuffer;
    }
    
    crashLogFD = open(logFileName, O_WRONLY | O_CREAT | O_TRUNC);

#if 0 // Disable until we can do this without calling malloc and free:
	int memTotal, memFree, swapTotal, swapFree, memchuteFree, memUsage;
#endif


    if (crashLogFD != -1) {
        crash_printf("LunaSysMgr.%d: Caught signal %d\n", sysmgrPid, sig);
        crash_flush();

        // Dump the register context for the crash to the logs:
        logCrashRegisterContext(sig, info, data);

        // Let webkit do some analysis on the crash information and log report
        // as appropriate:
#ifndef FIX_FOR_QT
        Palm::WebGlobal::reportCrashDiagnostics(sig, info, data, crashLogFD,
            Settings::LunaSettings()->debug_doVerboseCrashLogging,
            &hasCrashedInCrashHandler);
#endif
        crash_flush();

        crash_printf("LunaSysMgr.%d: Caught signal %d END report\n",
                     sysmgrPid, sig);
        crash_flush();
    }

    // If the option to debug crashes is enabled, then we'll enter an infinite
    // loop here to capture the crash conditions until a gdb session can be
    // attached.
    //
    // By default, this option is disabled, and the crashHandler will return
    // to the crashing instruction.  Since the crashHandler is set up to
    // fire only once (see SA_RESETHAND option which resets the handler after
    // it has fired), when we return to the crashing pc, we'll fault again.
    // This time, the fault will be handled by the default system handler and
    // generate a core or minicore as appropriate.

    if (debugCrashes || Settings::LunaSettings()->debug_loopInCrashHandler) {
        // Infinite loop until we turn off stayInLoop using gdb:
        while (stayInLoop) {
            // If we get here, sit and wait for someone to come and debug
            // this device.
        }
    }

    // Restore the previous handler before we return.  Otherwise, the inner
    // handler will perpetually skip over faulting addresses:
    sigaction(sig, &previous_crash_action, NULL);

    // Close the file:
    fsync(crashLogFD);
    close(crashLogFD);
    crashLogFD = -1;
}

void qtMsgHandler(QtMsgType type, const char *str) {
    switch(type)
    {
	case QtDebugMsg:
	    g_debug("QDebug: %s", str);
	    break;
	case QtWarningMsg:
	    g_warning("QWarning: %s", str);
	    break;
	case QtCriticalMsg:
	    g_critical("QCritical: %s", str);
	    break;
	case QtFatalMsg:
	    g_error("QFatal: %s", str);
	    break;
	default:
	    g_message("QMessage: %s", str);
	    break;
    }
}

static void parseCommandlineOptions(int argc, char** argv)
{
    GError* error = NULL;
    GOptionContext* context = NULL;

	static GOptionEntry entries[] = {
		{ "ui",  'u',  0, G_OPTION_ARG_STRING, &s_uiStr, "UI type to launch (minimal, luna)", "name" },
		{ "app", 'a', 0, G_OPTION_ARG_STRING,  &s_appToLaunchStr, "App Id of app to launch", "id" },
		{ "logger", 'l', 0, G_OPTION_ARG_STRING,  &s_logLevelStr, "log level", "level"},
		{ "syslog", 's', 0, G_OPTION_ARG_NONE, &s_useSysLog, "Use syslog", NULL },
		{ "colorlogging", 'c', 0, G_OPTION_ARG_NONE, &s_colorLog, "Color logging on or off", NULL },	// use -c=0 or --colorlogging=0 to disable color logging
		{ "terminal", 't', 0, G_OPTION_ARG_NONE, &s_useTerminal, "Use terminal for logs", NULL },
		{ "debug-trap", 'x', 0, G_OPTION_ARG_STRING,  &s_debugTrapStr, "debug trap (on/ off)", "state"},
		{ "force-software-rendering", 'S', 0, G_OPTION_ARG_NONE, &s_forceSoftwareRendering, "Force Software rendering", NULL},
		{ "malloc-stats-file", 'm', 0, G_OPTION_ARG_STRING,  &s_mallocStatsFileStr, "File for logging malloc stats", "file" },
		{ "malloc-stats-interval", 'i', 0, G_OPTION_ARG_INT,  &s_mallocStatsInterval, "Interval at which to log malloc stats", "seconds" },
		{ NULL }
	};

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, &error);

#if !defined(HAVE_OPENGL)
	// if there's no OpenGL, then implicitely force software rendering
	s_forceSoftwareRendering = true;
#endif

	if (s_uiStr && strcasecmp(s_uiStr, "minimal") == 0)
		Settings::LunaSettings()->uiType = Settings::UI_MINIMAL;
	else
		Settings::LunaSettings()->uiType = Settings::UI_LUNA;

	Settings::LunaSettings()->logger_useSyslog = s_useSysLog;
	Settings::LunaSettings()->logger_useColor = s_colorLog;
	Settings::LunaSettings()->logger_useTerminal = s_useTerminal;
#if SHIPPING_VERSION
	Settings::LunaSettings()->logger_level = G_LOG_LEVEL_CRITICAL;
#else
	Settings::LunaSettings()->logger_level = G_LOG_LEVEL_DEBUG;
#endif
	Settings::LunaSettings()->forceSoftwareRendering = s_forceSoftwareRendering;
	if (s_forceSoftwareRendering)
		Settings::LunaSettings()->atlasEnabled = false;

    if (s_logLevelStr)
    {
        if (0 == strcasecmp(s_logLevelStr, "error"))
            Settings::LunaSettings()->logger_level = G_LOG_LEVEL_ERROR;
        else if (0 == strcasecmp(s_logLevelStr, "critical"))
            Settings::LunaSettings()->logger_level = G_LOG_LEVEL_CRITICAL;
        else if (0 == strcasecmp(s_logLevelStr, "warning"))
            Settings::LunaSettings()->logger_level = G_LOG_LEVEL_WARNING;
        else if (0 == strcasecmp(s_logLevelStr, "message"))
            Settings::LunaSettings()->logger_level = G_LOG_LEVEL_MESSAGE;
        else if (0 == strcasecmp(s_logLevelStr, "info"))
            Settings::LunaSettings()->logger_level = G_LOG_LEVEL_INFO;
        else if (0 == strcasecmp(s_logLevelStr, "debug"))
            Settings::LunaSettings()->logger_level = G_LOG_LEVEL_DEBUG;
    }

    g_option_context_free(context);
}

static void generateGoodBacktraceTerminateHandler()
{
	volatile int* p = 0;
	*p = 0;
	exit(-1);
}

static int RunWebAppManagerTask(void* data)
{
    // Install the handler for signals that we want to trap:
    // Note: We install the handlers after we initialize the setting because
    // we may do something different depending on the settings values.
    installOuterCrashHandler(SIGILL);
    installOuterCrashHandler(SIGSEGV);
    installOuterCrashHandler(SIGTERM);

	::prctl(PR_SET_NAME, (unsigned long) "WebAppMgr", 0, 0, 0);
	::prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0);


	char msg = 0x00;
	int len = 0;

	// block here until the IpcServer in the main process is ready
	while (len != 1 || msg != msgOkToContinue)
		len = ::read(IpcServerPipeFd, &msg, 1);

	::close(IpcServerPipeFd);
	IpcServerPipeFd = -1;

	const HostInfo* info = &(HostBase::instance()->getInfo());
	WebAppManager::instance()->setHostInfo(info);

	initMallocStatsCb(WebAppManager::instance()->mainLoop(), s_mallocStatsInterval);

	logInit();

	// Start the Browser App Launcher
#ifdef NO_WEBKIT_INIT
	WindowServer::instance()->bootupFinished();
#else
	WebAppManager::instance()->run(); // Sync execution of the task
#endif

	return 0;
}


int appArgc = 0;
char** appArgv = 0;

static int RunBootupAnimationTask(void* data)
{
	::prctl(PR_SET_NAME, (unsigned long) "BootupAnimation", 0, 0, 0);
	::prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0);

	// Set "nice" property
	setpriority(PRIO_PROCESS,getpid(),1);

	const HostInfo* info = &(HostBase::instance()->getInfo());
	QCoreApplication bootApp(appArgc, appArgv);

//	BootupAnimation* bootAnim = new BootupAnimation(sysmgrPipeFd);
	//bootAnim->setFont(QFont("Prelude", 12));

	//bootAnim->resize(info->displayWidth,info->displayHeight);
	//bootAnim->show();
	//bootAnim->start();

	bootApp.exec();

	return 0;
}

pid_t spawnWebKitProcess()
{
	int fd[2];
	::pipe(fd);

	pid_t pid = ::fork();
	if (pid < 0)
		return pid;


	if (pid == 0) {
		// child closed the WRITE end of the pipe
		::close(fd[1]);
		IpcServerPipeFd = fd[0];
		RunWebAppManagerTask((void*) 0);
		exit(-1);
	} else {
		// parent closes the READ end of the pipe
		::close(fd[0]);
		WebAppMgrPipeFd = fd[1];
	}

	return pid;
}

pid_t spawnBootupAnimationProcess()
{
	int fd[2];
	::pipe(fd);

	pid_t pid = ::fork();
	if (pid < 0) {
		return pid;
	}

	if (pid == 0) {
		// child closed the WRITE end of the pipe
		::close(fd[1]);
		sysmgrPipeFd = fd[0];
		RunBootupAnimationTask((void*) 0);
		exit(-1);
	} else {
		bootAnimPid = pid;
		// parent closed the READ end of the pipe
		::close(fd[0]);
		bootAnimPipeFd = fd[1];
	}

	return pid;
}

gboolean finishBootup(gpointer data)
{
    WindowServer::instance()->bootupFinished();
    return FALSE;
}

int main( int argc, char** argv)
{
	appArgc = argc;
	appArgv = argv;

	std::set_terminate(generateGoodBacktraceTerminateHandler);

	g_thread_init(NULL);

	const char *renderMode;
#if defined(TARGET_DEVICE) && defined(HAVE_OPENGL)
	::setenv("QT_PLUGIN_PATH", "/usr/plugins", 1);
	renderMode = "HW egl";
#elif defined(TARGET_DEVICE) || defined(TARGET_EMULATOR)
	::setenv("QT_PLUGIN_PATH", "/usr/plugins", 1);	
	renderMode = "Software";
#elif defined(HAVE_OPENGL)
	renderMode = "HW OpenGL";
#else
	renderMode = "Software";
#endif

	WindowServer::markBootStart();

	g_debug("SysMgr compiled against Qt %s, running on %s, %s render mode requested", QT_VERSION_STR, qVersion(), renderMode);

	// Command-Line options
  	parseCommandlineOptions(argc, argv);

    if (s_debugTrapStr && 0 == strcasecmp(s_debugTrapStr, "on")) {
        debugCrashes = true;
    }

	if (s_mallocStatsFileStr) {
		setupMallocStats(s_mallocStatsFileStr);
	}

    sysmgrPid = getpid();

	// Load Settings (first!)
	Settings* settings = Settings::LunaSettings();

	// Initialize logging handler
	g_log_set_default_handler(logFilter, NULL);

#if defined(TARGET_DESKTOP)
	// use terminal logging when running on desktop
	settings->logger_useTerminal = true;
#endif

	// disable color logging using an environment variable. Useful when run from QtCreator
	const char* useColor = ::getenv("COLOR_LOGGING");
	if (useColor)
		settings->logger_useColor = (useColor[0] != 0 && useColor[0] != '0');

	HostBase* host = HostBase::instance();
	// the resolution is just a hint, the actual
	// resolution may get picked up from the fb driver on arm
	host->init(settings->displayWidth, settings->displayHeight);

#if defined(TARGET_DEVICE)
	pid_t animPid= spawnBootupAnimationProcess();
	if(animPid < 0) { // failed to start the Animation process
		return -1;
	}
#endif

#if defined(TARGET_DEVICE) && defined(HAVE_OPENGL)
	if (settings->forceSoftwareRendering)
		::setenv("QT_QPA_PLATFORM", "palm-soft", 1);
	else
		::setenv("QT_QPA_PLATFORM", "palm", 1);
#else
		::setenv("QT_QPA_PLATFORM", "palm", 1);
#endif
	
	
#if defined(TARGET_DEVICE) && defined(HAVE_OPENGL)
	if (!settings->forceSoftwareRendering)
		::setenv("QWS_DISPLAY", "egl", 1);
#endif


	pid_t webKitPid= spawnWebKitProcess();
	if(webKitPid < 0) { // failed to start the WebKit process
		return -1;
	}

	// Tie LunaSysMgr to Processor 0
	setCpuAffinity(getpid(), 1);

	// Tie WebAppMgr to Processor 1
	setCpuAffinity(webKitPid, 0);

	// Safe to create logging threads now
	logInit();

	// Initialize Ipc Server
	(void) IpcServer::instance();

	// Ipc Server is ready, so signal the WebAppMgr process (via pipe) to go ahead and connect
	::write(WebAppMgrPipeFd, &msgOkToContinue, 1);
	::close(WebAppMgrPipeFd);
	WebAppMgrPipeFd = -1;


#if !defined(TARGET_DESKTOP)
	// Set "nice" property
	setpriority(PRIO_PROCESS,getpid(),-1);
#endif

	qInstallMsgHandler(qtMsgHandler);
	QApplication app(argc, argv);
	QApplication::setStartDragDistance(settings->tapRadius);
	QApplication::setDoubleClickInterval (Settings::LunaSettings()->tapDoubleClickDuration);
	host->show();
	
	initMallocStatsCb(HostBase::instance()->mainLoop(), s_mallocStatsInterval);

	// Initialize Preferences handler
	(void) Preferences::instance();

	// Initialize Localization handler
	(void) Localization::instance();

	//Register vibration/haptics support
	HapticsController::instance()->startService();

	(void) DeviceInfo::instance();

	// Initialize Security handler
	(void) Security::instance();

	// Initialize the System Service
	SystemService::instance()->init();

	// Initialize the application mgr
	ApplicationManager::instance()->init();

	// Initialize the Application Installer
	ApplicationInstaller::instance();

	// Start the window manager
	WindowServer *windowServer = WindowServer::instance();
	windowServer->installEventFilter(windowServer);

	// Initialize the SysMgr MemoryMonitor
	MemoryMonitor::instance();

	// load all set policies
	EASPolicyManager::instance()->load();

	// Launching of the System UI launcher and headless apps has been moved to WebAppMgrProxy::connectWebAppMgr

	// Did user specify an app to launch
	if (s_appToLaunchStr) {
		WebAppMgrProxy::setAppToLaunchUponConnection(s_appToLaunchStr);
	}
	else
	    g_timeout_add(0, finishBootup, 0);
	
	app.exec();
	return 0;
}
