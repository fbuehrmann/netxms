/*
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: main.cpp
**
**/

#include "nxcore.h"
#include <netxmsdb.h>
#include <netxms_mt.h>
#include <hdlink.h>

#if !defined(_WIN32) && HAVE_READLINE_READLINE_H && HAVE_READLINE && !defined(UNICODE)
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#endif

#ifdef _WIN32
#include <errno.h>
#include <psapi.h>
#include <conio.h>
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#ifdef WITH_ZMQ
#include "zeromq.h"
#endif

/**
 * Messages generated by mc.pl (for UNIX version only)
 */
#ifndef _WIN32
extern unsigned int g_dwNumMessages;
extern const TCHAR *g_szMessages[];
#endif

/**
 * Shutdown reasons
 */
#define SHUTDOWN_DEFAULT      0
#define SHUTDOWN_FROM_CONSOLE 1
#define SHUTDOWN_BY_SIGNAL    2

/**
 * Externals
 */
extern Queue g_dciCacheLoaderQueue;

void InitClientListeners();
void InitMobileDeviceListeners();
void InitCertificates();
bool LoadServerCertificate(RSA **serverKey);
void InitUsers();
void CleanupUsers();
void LoadPerfDataStorageDrivers();
void ImportLocalConfiguration();
void RegisterPredictionEngines();
void ExecuteStartupScripts();
void CloseAgentTunnels();

void ExecuteScheduledScript(const ScheduledTaskParameters *param);
void MaintenanceModeEnter(const ScheduledTaskParameters *params);
void MaintenanceModeLeave(const ScheduledTaskParameters *params);
void ScheduleDeployPolicy(const ScheduledTaskParameters *params);
void ScheduleUninstallPolicy(const ScheduledTaskParameters * params);

void InitCountryList();
void InitCurrencyList();

#if XMPP_SUPPORTED
void StartXMPPConnector();
void StopXMPPConnector();
#endif

/**
 * Syslog server control
 */
void StartSyslogServer();
void StopSyslogServer();

/**
 * Thread functions
 */
THREAD_RESULT THREAD_CALL Syncer(void *);
THREAD_RESULT THREAD_CALL NodePoller(void *);
THREAD_RESULT THREAD_CALL PollManager(void *);
THREAD_RESULT THREAD_CALL EventProcessor(void *);
THREAD_RESULT THREAD_CALL WatchdogThread(void *);
THREAD_RESULT THREAD_CALL ClientListenerThread(void *);
THREAD_RESULT THREAD_CALL MobileDeviceListenerThread(void *);
THREAD_RESULT THREAD_CALL ISCListener(void *);
THREAD_RESULT THREAD_CALL LocalAdminListener(void *);
THREAD_RESULT THREAD_CALL SNMPTrapReceiver(void *);
THREAD_RESULT THREAD_CALL BeaconPoller(void *);
THREAD_RESULT THREAD_CALL JobManagerThread(void *);
THREAD_RESULT THREAD_CALL UptimeCalculator(void *);
THREAD_RESULT THREAD_CALL ReportingServerConnector(void *);
THREAD_RESULT THREAD_CALL TunnelListener(void *arg);

/**
 * Global variables
 */
TCHAR NXCORE_EXPORTABLE g_szConfigFile[MAX_PATH] = _T("{search}");
TCHAR NXCORE_EXPORTABLE g_szLogFile[MAX_PATH] = DEFAULT_LOG_FILE;
UINT32 g_logRotationMode = NXLOG_ROTATION_BY_SIZE;
UINT64 g_maxLogSize = 16384 * 1024;
UINT32 g_logHistorySize = 4;
TCHAR g_szDailyLogFileSuffix[64] = _T("");
TCHAR NXCORE_EXPORTABLE g_szDumpDir[MAX_PATH] = DEFAULT_DUMP_DIR;
char g_szCodePage[256] = ICONV_DEFAULT_CODEPAGE;
TCHAR NXCORE_EXPORTABLE g_szListenAddress[MAX_PATH] = _T("*");
#ifndef _WIN32
TCHAR NXCORE_EXPORTABLE g_szPIDFile[MAX_PATH] = _T("/var/run/netxmsd.pid");
#endif
UINT32 g_dwDiscoveryPollingInterval;
UINT32 g_dwStatusPollingInterval;
UINT32 g_dwConfigurationPollingInterval;
UINT32 g_dwRoutingTableUpdateInterval;
UINT32 g_dwTopologyPollingInterval;
UINT32 g_dwConditionPollingInterval;
UINT32 g_instancePollingInterval;
UINT32 g_icmpPingSize;
UINT32 g_icmpPingTimeout = 1500;    // ICMP ping timeout (milliseconds)
UINT32 g_auditFlags;
UINT32 g_slmPollingInterval;
UINT32 g_offlineDataRelevanceTime = 86400;
TCHAR NXCORE_EXPORTABLE g_netxmsdDataDir[MAX_PATH] = _T("");
TCHAR NXCORE_EXPORTABLE g_netxmsdLibDir[MAX_PATH] = _T("");
int g_dbSyntax = DB_SYNTAX_UNKNOWN;
UINT32 NXCORE_EXPORTABLE g_processAffinityMask = DEFAULT_AFFINITY_MASK;
UINT64 g_serverId = 0;
RSA *g_pServerKey = NULL;
time_t g_serverStartTime = 0;
UINT32 g_lockTimeout = 60000;   // Default timeout for acquiring mutex
UINT32 g_agentCommandTimeout = 4000;  // Default timeout for requests to agent
UINT32 g_thresholdRepeatInterval = 0;	// Disabled by default
int g_requiredPolls = 1;
DB_DRIVER g_dbDriver = NULL;
ThreadPool NXCORE_EXPORTABLE *g_mainThreadPool = NULL;
INT16 g_defaultAgentCacheMode = AGENT_CACHE_OFF;
InetAddressList g_peerNodeAddrList;
Condition g_dbPasswordReady(true);

/**
 * Static data
 */
static CONDITION m_condShutdown = INVALID_CONDITION_HANDLE;
static THREAD m_thPollManager = INVALID_THREAD_HANDLE;
static THREAD m_thSyncer = INVALID_THREAD_HANDLE;
static THREAD s_tunnelListenerThread = INVALID_THREAD_HANDLE;
static int m_nShutdownReason = SHUTDOWN_DEFAULT;
static StringSet s_components;

#ifndef _WIN32
static pthread_t m_signalHandlerThread;
#endif

/**
 * Register component
 */
void NXCORE_EXPORTABLE RegisterComponent(const TCHAR *id)
{
   s_components.add(id);
}

/**
 * Check if component with given ID is registered
 */
bool NXCORE_EXPORTABLE IsComponentRegistered(const TCHAR *id)
{
   return s_components.contains(id);
}

/**
 * Fill NXCP message with components data
 */
void FillComponentsMessage(NXCPMessage *msg)
{
   msg->setField(VID_NUM_COMPONENTS, (INT32)s_components.size());
   UINT32 fieldId = VID_COMPONENT_LIST_BASE;
   Iterator<const TCHAR> *it = s_components.iterator();
   while(it->hasNext())
   {
      msg->setField(fieldId++, it->next());
   }
   delete it;
}

/**
 * Sleep for specified number of seconds or until system shutdown arrives
 * Function will return TRUE if shutdown event occurs
 *
 * @param seconds seconds to sleep
 * @return true if server is shutting down
 */
bool NXCORE_EXPORTABLE SleepAndCheckForShutdown(int seconds)
{
	return ConditionWait(m_condShutdown, seconds * 1000);
}

/**
 * Disconnect from database (exportable function for startup module)
 */
void NXCORE_EXPORTABLE ShutdownDB()
{
   DBConnectionPoolShutdown();
	DBUnloadDriver(g_dbDriver);
}

/**
 * Check data directory for existence
 */
static BOOL CheckDataDir()
{
	TCHAR szBuffer[MAX_PATH];

	if (_tchdir(g_netxmsdDataDir) == -1)
	{
		nxlog_write(MSG_INVALID_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", g_netxmsdDataDir);
		return FALSE;
	}

#ifdef _WIN32
#define MKDIR(name) _tmkdir(name)
#else
#define MKDIR(name) _tmkdir(name, 0700)
#endif

	// Create directory for package files if it doesn't exist
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_PACKAGES);
	if (MKDIR(szBuffer) == -1)
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}

	// Create directory for map background images if it doesn't exist
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_BACKGROUNDS);
	if (MKDIR(szBuffer) == -1)
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}

	// Create directory for image library is if does't exists
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_IMAGES);
	if (MKDIR(szBuffer) == -1)
	{
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}
	}

	// Create directory for file store if does't exists
	_tcscpy(szBuffer, g_netxmsdDataDir);
	_tcscat(szBuffer, DDIR_FILES);
	if (MKDIR(szBuffer) == -1)
	{
		if (errno != EEXIST)
		{
			nxlog_write(MSG_ERROR_CREATING_DATA_DIR, EVENTLOG_ERROR_TYPE, "s", szBuffer);
			return FALSE;
		}
	}

#undef MKDIR

	return TRUE;
}

/**
 * Load global configuration parameters
 */
static void LoadGlobalConfig()
{
	g_dwDiscoveryPollingInterval = ConfigReadInt(_T("DiscoveryPollingInterval"), 900);
	g_dwStatusPollingInterval = ConfigReadInt(_T("StatusPollingInterval"), 60);
	g_dwConfigurationPollingInterval = ConfigReadInt(_T("ConfigurationPollingInterval"), 3600);
	g_instancePollingInterval = ConfigReadInt(_T("InstancePollingInterval"), 600);
	g_dwRoutingTableUpdateInterval = ConfigReadInt(_T("RoutingTableUpdateInterval"), 300);
	g_dwTopologyPollingInterval = ConfigReadInt(_T("TopologyPollingInterval"), 1800);
	g_dwConditionPollingInterval = ConfigReadInt(_T("ConditionPollingInterval"), 60);
	g_slmPollingInterval = ConfigReadInt(_T("SlmPollingInterval"), 60);
	DCObject::m_defaultPollingInterval = ConfigReadInt(_T("DefaultDCIPollingInterval"), 60);
   DCObject::m_defaultRetentionTime = ConfigReadInt(_T("DefaultDCIRetentionTime"), 30);
   g_defaultAgentCacheMode = (INT16)ConfigReadInt(_T("DefaultAgentCacheMode"), AGENT_CACHE_OFF);
   if ((g_defaultAgentCacheMode != AGENT_CACHE_ON) && (g_defaultAgentCacheMode != AGENT_CACHE_OFF))
   {
      DbgPrintf(1, _T("Invalid value %d of DefaultAgentCacheMode: reset to %d (OFF)"), g_defaultAgentCacheMode, AGENT_CACHE_OFF);
      ConfigWriteInt(_T("DefaultAgentCacheMode"), AGENT_CACHE_OFF, true, true, true);
      g_defaultAgentCacheMode = AGENT_CACHE_OFF;
   }
	if (ConfigReadInt(_T("DeleteEmptySubnets"), 1))
		g_flags |= AF_DELETE_EMPTY_SUBNETS;
	if (ConfigReadInt(_T("EnableSNMPTraps"), 1))
		g_flags |= AF_ENABLE_SNMP_TRAPD;
	if (ConfigReadInt(_T("ProcessTrapsFromUnmanagedNodes"), 0))
		g_flags |= AF_TRAPS_FROM_UNMANAGED_NODES;
	if (ConfigReadInt(_T("EnableZoning"), 0))
		g_flags |= AF_ENABLE_ZONING;
	if (ConfigReadInt(_T("EnableObjectTransactions"), 0))
		g_flags |= AF_ENABLE_OBJECT_TRANSACTIONS;
	if (ConfigReadInt(_T("RunNetworkDiscovery"), 0))
		g_flags |= AF_ENABLE_NETWORK_DISCOVERY;
	if (ConfigReadInt(_T("ActiveNetworkDiscovery"), 0))
		g_flags |= AF_ACTIVE_NETWORK_DISCOVERY;
   if (ConfigReadInt(_T("UseSNMPTrapsForDiscovery"), 0))
      g_flags |= AF_SNMP_TRAP_DISCOVERY;
   if (ConfigReadInt(_T("UseSyslogForDiscovery"), 0))
      g_flags |= AF_SYSLOG_DISCOVERY;
	if (ConfigReadInt(_T("ResolveNodeNames"), 1))
		g_flags |= AF_RESOLVE_NODE_NAMES;
	if (ConfigReadInt(_T("SyncNodeNamesWithDNS"), 0))
		g_flags |= AF_SYNC_NODE_NAMES_WITH_DNS;
	if (ConfigReadInt(_T("CheckTrustedNodes"), 1))
		g_flags |= AF_CHECK_TRUSTED_NODES;
	if (ConfigReadInt(_T("EnableNXSLContainerFunctions"), 1))
		g_flags |= AF_ENABLE_NXSL_CONTAINER_FUNCS;
   if (ConfigReadInt(_T("UseFQDNForNodeNames"), 1))
      g_flags |= AF_USE_FQDN_FOR_NODE_NAMES;
   if (ConfigReadInt(_T("ApplyDCIFromTemplateToDisabledDCI"), 1))
      g_flags |= AF_APPLY_TO_DISABLED_DCI_FROM_TEMPLATE;
   if (ConfigReadInt(_T("ResolveDNSToIPOnStatusPoll"), 0))
      g_flags |= AF_RESOLVE_IP_FOR_EACH_STATUS_POLL;
   if (ConfigReadInt(_T("CaseInsensitiveLoginNames"), 0))
      g_flags |= AF_CASE_INSENSITIVE_LOGINS;
   if (ConfigReadInt(_T("TrapSourcesInAllZones"), 0))
      g_flags |= AF_TRAP_SOURCES_IN_ALL_ZONES;

   if (g_netxmsdDataDir[0] == 0)
   {
      GetNetXMSDirectory(nxDirData, g_netxmsdDataDir);
      DbgPrintf(1, _T("Data directory set to %s"), g_netxmsdDataDir);
   }
   else
   {
      DbgPrintf(1, _T("Using data directory %s"), g_netxmsdDataDir);
   }

   g_icmpPingTimeout = ConfigReadInt(_T("IcmpPingTimeout"), 1500);
	g_icmpPingSize = ConfigReadInt(_T("IcmpPingSize"), 46);
	g_lockTimeout = ConfigReadInt(_T("LockTimeout"), 60000);
	g_agentCommandTimeout = ConfigReadInt(_T("AgentCommandTimeout"), 4000);
	g_thresholdRepeatInterval = ConfigReadInt(_T("ThresholdRepeatInterval"), 0);
	g_requiredPolls = ConfigReadInt(_T("PollCountForStatusChange"), 1);
	g_offlineDataRelevanceTime = ConfigReadInt(_T("OfflineDataRelevanceTime"), 86400);

	UINT32 snmpTimeout = ConfigReadInt(_T("SNMPRequestTimeout"), 1500);
   SnmpSetDefaultTimeout(snmpTimeout);
}

/**
 * Initialize cryptografic functions
 */
static bool InitCryptografy()
{
#ifdef _WITH_ENCRYPTION
	bool success = false;

   if (!InitCryptoLib(ConfigReadULong(_T("AllowedCiphers"), 0x7F)))
		return FALSE;
   nxlog_debug(4, _T("Supported ciphers: %s"), (const TCHAR *)NXCPGetSupportedCiphersAsText());

   SSL_library_init();
   SSL_load_error_strings();

   if (LoadServerCertificate(&g_pServerKey))
   {
      nxlog_debug(1, _T("Server certificate loaded"));
   }
   if (g_pServerKey != NULL)
   {
      nxlog_debug(1, _T("Using server certificate key"));
      success = true;
   }
   else
   {
      TCHAR szKeyFile[MAX_PATH];
      _tcscpy(szKeyFile, g_netxmsdDataDir);
      _tcscat(szKeyFile, DFILE_KEYS);
      g_pServerKey = LoadRSAKeys(szKeyFile);
      if (g_pServerKey == NULL)
      {
         nxlog_debug(1, _T("Generating RSA key pair..."));
         g_pServerKey = RSA_generate_key(NETXMS_RSA_KEYLEN, 17, NULL, NULL);
         if (g_pServerKey != NULL)
         {
            int fd = _topen(szKeyFile, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0600);
            if (fd != -1)
            {
               UINT32 dwLen = i2d_RSAPublicKey(g_pServerKey, NULL);
               dwLen += i2d_RSAPrivateKey(g_pServerKey, NULL);
               BYTE *pKeyBuffer = (BYTE *)malloc(dwLen);

               BYTE *pBufPos = pKeyBuffer;
               i2d_RSAPublicKey(g_pServerKey, &pBufPos);
               i2d_RSAPrivateKey(g_pServerKey, &pBufPos);
               _write(fd, &dwLen, sizeof(UINT32));
               _write(fd, pKeyBuffer, dwLen);

               BYTE hash[SHA1_DIGEST_SIZE];
               CalculateSHA1Hash(pKeyBuffer, dwLen, hash);
               _write(fd, hash, SHA1_DIGEST_SIZE);

               _close(fd);
               free(pKeyBuffer);
               success = true;
            }
            else
            {
               nxlog_debug(0, _T("Failed to open %s for writing"), szKeyFile);
            }
         }
         else
         {
            nxlog_debug(0, _T("Failed to generate RSA key"));
         }
      }
      else
      {
         success = true;
      }
   }

	int iPolicy = ConfigReadInt(_T("DefaultEncryptionPolicy"), 1);
	if ((iPolicy < 0) || (iPolicy > 3))
		iPolicy = 1;
	SetAgentDEP(iPolicy);

	return success;
#else
	return TRUE;
#endif
}

/**
 * Check if process with given PID exists and is a NetXMS server process
 */
static bool IsNetxmsdProcess(UINT32 pid)
{
#ifdef _WIN32
	bool result = false;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (hProcess != NULL)
	{
	   TCHAR szExtModule[MAX_PATH], szIntModule[MAX_PATH];
		if ((GetModuleBaseName(hProcess, NULL, szExtModule, MAX_PATH) > 0) &&
			 (GetModuleBaseName(GetCurrentProcess(), NULL, szIntModule, MAX_PATH) > 0))
		{
			result = (_tcsicmp(szExtModule, szIntModule) == 0);
		}
		else
		{
			// Cannot read process name, for safety assume that it's a server process
			result = true;
		}
		CloseHandle(hProcess);
	}
	return result;
#else
	return kill((pid_t)pid, 0) != -1;
#endif
}

/**
 * Check if remote netxmsd is running
 */
static bool PeerNodeIsRunning(const InetAddress& addr)
{
   bool result = false;

   TCHAR keyFile[MAX_PATH];
   _tcscpy(keyFile, g_netxmsdDataDir);
   _tcscat(keyFile, DFILE_KEYS);
   RSA *key = LoadRSAKeys(keyFile);

   AgentConnection *ac = new AgentConnection(addr);
   if (ac->connect(key))
   {
      TCHAR result[MAX_RESULT_LENGTH];
#ifdef _WIN32
      UINT32 rcc = ac->getParameter(_T("Process.Count(netxmsd.exe)"), MAX_RESULT_LENGTH, result);
#else
      UINT32 rcc = ac->getParameter(_T("Process.Count(netxmsd)"), MAX_RESULT_LENGTH, result);
#endif
      ac->decRefCount();
      if (key != NULL)
         RSA_free(key);
      if (rcc == ERR_SUCCESS)
      {
         return _tcstol(result, NULL, 10) > 0;
      }
   }
   else
   {
      ac->decRefCount();
      if (key != NULL)
         RSA_free(key);
   }

   UINT16 port = (UINT16)ConfigReadInt(_T("ClientListenerPort"), SERVER_LISTEN_PORT_FOR_CLIENTS);
   SOCKET s = ConnectToHost(addr, port, 5000);
   if (s != INVALID_SOCKET)
   {
      shutdown(s, SHUT_RDWR);
      closesocket(s);
      result = true;
   }
   return result;
}

/**
 * Database event handler
 */
static void DBEventHandler(DWORD dwEvent, const WCHAR *pszArg1, const WCHAR *pszArg2, bool connLost, void *userArg)
{
	if (!(g_flags & AF_SERVER_INITIALIZED))
		return;     // Don't try to do anything if server is not ready yet

	switch(dwEvent)
	{
		case DBEVENT_CONNECTION_LOST:
			PostEvent(EVENT_DB_CONNECTION_LOST, g_dwMgmtNode, NULL);
			g_flags |= AF_DB_CONNECTION_LOST;
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, FALSE);
			break;
		case DBEVENT_CONNECTION_RESTORED:
			PostEvent(EVENT_DB_CONNECTION_RESTORED, g_dwMgmtNode, NULL);
			g_flags &= ~AF_DB_CONNECTION_LOST;
			NotifyClientSessions(NX_NOTIFY_DBCONN_STATUS, TRUE);
			break;
		case DBEVENT_QUERY_FAILED:
			PostEvent(EVENT_DB_QUERY_FAILED, g_dwMgmtNode, "uud", pszArg1, pszArg2, connLost ? 1 : 0);
			break;
		default:
			break;
	}
}

/**
 * Send console message to session with open console
 */
static void SendConsoleMessage(ClientSession *session, void *arg)
{
	if (session->isConsoleOpen())
	{
		NXCPMessage msg;

		msg.setCode(CMD_ADM_MESSAGE);
		msg.setField(VID_MESSAGE, (TCHAR *)arg);
		session->postMessage(&msg);
	}
}

/**
 * Console writer
 */
static void LogConsoleWriter(const TCHAR *format, ...)
{
	TCHAR buffer[8192];
	va_list args;

	va_start(args, format);
	_vsntprintf(buffer, 8192, format, args);
	buffer[8191] = 0;
	va_end(args);

	WriteToTerminal(buffer);

	EnumerateClientSessions(SendConsoleMessage, buffer);
}

/**
 * Oracle session init callback
 */
static void OracleSessionInitCallback(DB_HANDLE hdb)
{
   DBQuery(hdb, _T("ALTER SESSION SET DDL_LOCK_TIMEOUT = 60"));
}

/**
 * Get database password
 */
static void GetDatabasePassword()
{
   if (_tcscmp(g_szDbPassword, _T("?")))
      return;

   nxlog_write(MSG_WAITING_FOR_DB_PASSWORD, NXLOG_INFO, NULL);
   g_dbPasswordReady.wait(INFINITE);
   nxlog_debug(1, _T("Database password received"));
}

/**
 * Server initialization
 */
BOOL NXCORE_EXPORTABLE Initialize()
{
	s_components.add(_T("CORE"));

	g_serverStartTime = time(NULL);
	srand((unsigned int)g_serverStartTime);

	if (!(g_flags & AF_USE_SYSLOG))
	{
		if (!nxlog_set_rotation_policy((int)g_logRotationMode, g_maxLogSize, (int)g_logHistorySize, g_szDailyLogFileSuffix))
			if (!(g_flags & AF_DAEMON))
				_tprintf(_T("WARNING: cannot set log rotation policy; using default values\n"));
	}
   if (!nxlog_open((g_flags & AF_USE_SYSLOG) ? NETXMSD_SYSLOG_NAME : g_szLogFile,
	                ((g_flags & AF_USE_SYSLOG) ? NXLOG_USE_SYSLOG : 0) |
	                ((g_flags & AF_BACKGROUND_LOG_WRITER) ? NXLOG_BACKGROUND_WRITER : 0) |
                   ((g_flags & AF_DAEMON) ? 0 : NXLOG_PRINT_TO_STDOUT),
                   _T("LIBNXSRV.DLL"),
#ifdef _WIN32
				       0, NULL, MSG_DEBUG, MSG_OTHER))
#else
				       g_dwNumMessages, g_szMessages, MSG_DEBUG, MSG_OTHER))
#endif
   {
		_ftprintf(stderr, _T("FATAL ERROR: Cannot open log file\n"));
      return FALSE;
   }
	nxlog_set_console_writer(LogConsoleWriter);

   if (g_netxmsdLibDir[0] == 0)
   {
      GetNetXMSDirectory(nxDirLib, g_netxmsdLibDir);
      nxlog_debug(1, _T("LIB directory set to %s"), g_netxmsdLibDir);
   }

	// Set code page
#ifndef _WIN32
	if (SetDefaultCodepage(g_szCodePage))
	{
		DbgPrintf(1, _T("Code page set to %hs"), g_szCodePage);
	}
	else
	{
		nxlog_write(MSG_CODEPAGE_ERROR, EVENTLOG_WARNING_TYPE, "m", g_szCodePage);
	}
#endif

	// Set process affinity mask
	if (g_processAffinityMask != DEFAULT_AFFINITY_MASK)
	{
#ifdef _WIN32
		if (SetProcessAffinityMask(GetCurrentProcess(), g_processAffinityMask))
		   nxlog_debug(1, _T("Process affinity mask set to 0x%08X"), g_processAffinityMask);
#else
		nxlog_write(MSG_SET_PROCESS_AFFINITY_NOT_SUPPORTED, EVENTLOG_WARNING_TYPE, NULL);
#endif
	}

#ifdef _WIN32
	WSADATA wsaData;
	int wrc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (wrc != 0)
	{
		nxlog_write(MSG_WSASTARTUP_FAILED, EVENTLOG_ERROR_TYPE, "e", wrc);
		return FALSE;
	}
#endif

	InitLocalNetInfo();
   SnmpSetMessageIds(MSG_OID_PARSE_ERROR, MSG_SNMP_UNKNOWN_TYPE, MSG_SNMP_GET_ERROR);

	// Create queue for delayed SQL queries
	g_dbWriterQueue = new Queue(256, 64);
	g_dciDataWriterQueue = new Queue(1024, 1024);
	g_dciRawDataWriterQueue = new Queue(1024, 1024);

	// Initialize database driver and connect to database
	if (!DBInit(MSG_OTHER, (g_flags & AF_LOG_SQL_ERRORS) ? MSG_SQL_ERROR : 0))
		return FALSE;
	g_dbDriver = DBLoadDriver(g_szDbDriver, g_szDbDrvParams, (nxlog_get_debug_level() >= 9), DBEventHandler, NULL);
	if (g_dbDriver == NULL)
		return FALSE;

   // Start local administrative interface listener if required
   if (g_flags & AF_ENABLE_LOCAL_CONSOLE)
      ThreadCreate(LocalAdminListener, 0, NULL);

	// Wait for database password if needed
	GetDatabasePassword();

	// Connect to database
	DB_HANDLE hdbBootstrap = NULL;
	TCHAR errorText[DBDRV_MAX_ERROR_TEXT];
	for(int i = 0; ; i++)
	{
	   hdbBootstrap = DBConnect(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, errorText);
		if ((hdbBootstrap != NULL) || (i == 5))
			break;
		ThreadSleep(5);
	}
	if (hdbBootstrap == NULL)
	{
		nxlog_write(MSG_DB_CONNFAIL, EVENTLOG_ERROR_TYPE, "s", errorText);
		return FALSE;
	}
	nxlog_debug(1, _T("Successfully connected to database %s@%s"), g_szDbName, g_szDbServer);

	// Check database schema version
	int schemaVersion = DBGetSchemaVersion(hdbBootstrap);
	if (schemaVersion != DB_FORMAT_VERSION)
	{
		nxlog_write(MSG_WRONG_DB_VERSION, EVENTLOG_ERROR_TYPE, "dd", schemaVersion, DB_FORMAT_VERSION);
		DBDisconnect(hdbBootstrap);
		return FALSE;
	}

	// Read database syntax
	g_dbSyntax = DBGetSyntax(hdbBootstrap);
   if (g_dbSyntax == DB_SYNTAX_ORACLE)
   {
      DBSetSessionInitCallback(OracleSessionInitCallback);
   }

	int baseSize = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolBaseSize"), 10);
	int maxSize = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolMaxSize"), 30);
	int cooldownTime = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolCooldownTime"), 300);
	int ttl = ConfigReadIntEx(hdbBootstrap, _T("DBConnectionPoolMaxLifetime"), 14400);

   DBDisconnect(hdbBootstrap);

	if (!DBConnectionPoolStartup(g_dbDriver, g_szDbServer, g_szDbName, g_szDbLogin, g_szDbPassword, g_szDbSchema, baseSize, maxSize, cooldownTime, ttl))
	{
      nxlog_write(MSG_DBCONNPOOL_INIT_FAILED, EVENTLOG_ERROR_TYPE, NULL);
	   return FALSE;
	}

   UINT32 lrt = ConfigReadULong(_T("LongRunningQueryThreshold"), 0);
   if (lrt != 0)
   {
      DBSetLongRunningThreshold(lrt);
      nxlog_write_generic(NXLOG_INFO, _T("Long running query threshold set at %d milliseconds"), lrt);
   }

   MetaDataPreLoad();

	// Read server ID
   TCHAR buffer[256];
	MetaDataReadStr(_T("ServerID"), buffer, 256, _T(""));
	StrStrip(buffer);
	if (buffer[0] != 0)
	{
      g_serverId = _tcstoull(buffer, NULL, 16);
	}
	else
	{
		// Generate new ID
		g_serverId = ((UINT64)time(NULL) << 31) | (UINT64)((UINT32)rand() & 0x7FFFFFFF);
      _sntprintf(buffer, 256, UINT64X_FMT(_T("016")), g_serverId);
		MetaDataWriteStr(_T("ServerID"), buffer);
	}
	nxlog_write_generic(NXLOG_INFO, _T("Server ID ") UINT64X_FMT(_T("016")), g_serverId);

	// Initialize locks
retry_db_lock:
   InetAddress addr;
	if (!InitLocks(&addr, buffer))
	{
		if (addr.isValidUnicast())     // Database already locked by another server instance
		{
			// Check for lock from crashed/terminated local process
			if (GetLocalIpAddr().equals(addr))
			{
				UINT32 pid = ConfigReadULong(_T("DBLockPID"), 0);
				if (!IsNetxmsdProcess(pid) || (pid == GetCurrentProcessId()))
				{
					UnlockDB();
					nxlog_write(MSG_DB_LOCK_REMOVED, EVENTLOG_INFORMATION_TYPE, NULL);
					goto retry_db_lock;
				}
			}
			else if (g_peerNodeAddrList.hasAddress(addr))
			{
			   if (!PeerNodeIsRunning(addr))
			   {
               UnlockDB();
               nxlog_write(MSG_DB_LOCK_REMOVED, EVENTLOG_INFORMATION_TYPE, NULL);
               goto retry_db_lock;
			   }
			}
			nxlog_write(MSG_DB_LOCKED, EVENTLOG_ERROR_TYPE, "Is", &addr, buffer);
		}
		else
      {
         nxlog_write(MSG_INIT_LOCKS_FAILED, EVENTLOG_ERROR_TYPE, NULL);
      }
		return FALSE;
	}
	g_flags |= AF_DB_LOCKED;

	// Load global configuration parameters
	LoadGlobalConfig();
   CASReadSettings();
   nxlog_debug(1, _T("Global configuration loaded"));

	// Check data directory
	if (!CheckDataDir())
		return FALSE;

	// Initialize cryptografy
	if (!InitCryptografy())
	{
		nxlog_write(MSG_INIT_CRYPTO_FAILED, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}

	// Initialize certificate store and CA
	InitCertificates();

	// Create synchronization stuff
	m_condShutdown = ConditionCreate(TRUE);

   // Create thread pools
	nxlog_debug(2, _T("Creating thread pools"));
   g_mainThreadPool = ThreadPoolCreate(8, 256, _T("MAIN"));
   g_agentConnectionThreadPool = ThreadPoolCreate(4, 256, _T("AGENT"));

	// Setup unique identifiers table
	if (!InitIdTable())
		return FALSE;
	nxlog_debug(2, _T("ID table created"));

	InitCountryList();
	InitCurrencyList();

	// Update status for unfinished jobs in job history
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DBQuery(hdb, _T("UPDATE job_history SET status=4,failure_message='Aborted due to server shutdown or crash' WHERE status NOT IN (3,4,5)"));
	DBConnectionPoolReleaseConnection(hdb);

	// Load and compile scripts
	LoadScripts();

	// Initialize persistent storage
	PersistentStorageInit();

	// Initialize watchdog
	WatchdogInit();

	// Load modules
	if (!LoadNetXMSModules())
		return FALSE;	// Mandatory module not loaded
	RegisterPredictionEngines();

	// Initialize mailer and SMS sender
	InitMailer();
	InitSMSSender();

	// Load users from database
	InitUsers();
	if (!LoadUsers())
	{
		nxlog_write(MSG_ERROR_LOADING_USERS, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}
	nxlog_debug(2, _T("User accounts loaded"));

	// Initialize audit
	InitAuditLog();

	// Initialize event handling subsystem
	if (!InitEventSubsystem())
		return FALSE;

	// Initialize alarms
   LoadAlarmCategories();
	if (!InitAlarmManager())
		return FALSE;

	// Initialize objects infrastructure and load objects from database
	LoadNetworkDeviceDrivers();
	ObjectsInit();
	if (!LoadObjects())
		return FALSE;
	nxlog_debug(1, _T("Objects loaded and initialized"));

	// Initialize and load event actions
	if (!InitActions())
	{
		nxlog_write(MSG_ACTION_INIT_ERROR, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}

   // Initialize helpdesk link
   SetHDLinkEntryPoints(ResolveAlarmByHDRef, TerminateAlarmByHDRef);
   LoadHelpDeskLink();

	// Initialize data collection subsystem
   LoadPerfDataStorageDrivers();
	if (!InitDataCollector())
		return FALSE;

	InitLogAccess();
	FileUploadJob::init();
	InitMappingTables();

   InitClientListeners();
	if (ConfigReadInt(_T("ImportConfigurationOnStartup"), 1))
	   ImportLocalConfiguration();

	// Check if management node object presented in database
	CheckForMgmtNode();
	if (g_dwMgmtNode == 0)
	{
		nxlog_write(MSG_CANNOT_FIND_SELF, EVENTLOG_ERROR_TYPE, NULL);
		return FALSE;
	}

	// Start threads
	ThreadCreate(WatchdogThread, 0, NULL);
	ThreadCreate(NodePoller, 0, NULL);
	ThreadCreate(JobManagerThread, 0, NULL);
	m_thSyncer = ThreadCreateEx(Syncer, 0, NULL);
	m_thPollManager = ThreadCreateEx(PollManager, 0, NULL);

   StartHouseKeeper();

	// Start event processor
	ThreadCreate(EventProcessor, 0, NULL);

	// Start SNMP trapper
	InitTraps();
	if (ConfigReadInt(_T("EnableSNMPTraps"), 1))
		ThreadCreate(SNMPTrapReceiver, 0, NULL);

	// Start built-in syslog daemon
   StartSyslogServer();

	// Start database _T("lazy") write thread
	StartDBWriter();

	// Start beacon host poller
	ThreadCreate(BeaconPoller, 0, NULL);

	// Start inter-server communication listener
	if (ConfigReadInt(_T("EnableISCListener"), 0))
		ThreadCreate(ISCListener, 0, NULL);

	// Start reporting server connector
	if (ConfigReadInt(_T("EnableReportingServer"), 0))
		ThreadCreate(ReportingServerConnector, 0, NULL);

   //Start ldap synchronization
   if (ConfigReadInt(_T("LdapSyncInterval"), 0))
		ThreadCreate(SyncLDAPUsers, 0, NULL);

   RegisterSchedulerTaskHandler(_T("Execute.Script"), ExecuteScheduledScript, SYSTEM_ACCESS_SCHEDULE_SCRIPT);
   RegisterSchedulerTaskHandler(_T("Maintenance.Enter"), MaintenanceModeEnter, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
   RegisterSchedulerTaskHandler(_T("Maintenance.Leave"), MaintenanceModeLeave, SYSTEM_ACCESS_SCHEDULE_MAINTENANCE);
	RegisterSchedulerTaskHandler(_T("Policy.Deploy"), ScheduleDeployPolicy, 0); //No access right beacause it will be used only by server
	RegisterSchedulerTaskHandler(_T("Policy.Uninstall"), ScheduleUninstallPolicy, 0); //No access right beacause it will be used only by server
   RegisterSchedulerTaskHandler(ALARM_SUMMARY_EMAIL_TASK_ID, SendAlarmSummaryEmail, 0); //No access right beacause it will be used only by server
   InitializeTaskScheduler();

   // Send summary emails
   if (ConfigReadInt(_T("EnableAlarmSummaryEmails"), 0))
      EnableAlarmSummaryEmails();
   else
      RemoveScheduledTaskByHandlerId(ALARM_SUMMARY_EMAIL_TASK_ID);

	// Allow clients to connect
	ThreadCreate(ClientListenerThread, 0, NULL);

	// Allow mobile devices to connect
	InitMobileDeviceListeners();
	ThreadCreate(MobileDeviceListenerThread, 0, NULL);

	// Agent tunnels
   s_tunnelListenerThread = ThreadCreateEx(TunnelListener, 0, NULL);

	// Start uptime calculator for SLM
	ThreadCreate(UptimeCalculator, 0, NULL);

	nxlog_debug(2, _T("LIBDIR: %s"), g_netxmsdLibDir);

	// Call startup functions for the modules
   CALL_ALL_MODULES(pfServerStarted, ());

#if XMPP_SUPPORTED
   if (ConfigReadInt(_T("EnableXMPPConnector"), 1))
   {
      StartXMPPConnector();
   }
#endif

#if WITH_ZMQ
   StartZMQConnector();
#endif

   ExecuteStartupScripts();

	g_flags |= AF_SERVER_INITIALIZED;
	nxlog_debug(1, _T("Server initialization completed"));
	return TRUE;
}

/**
 * Server shutdown
 */
void NXCORE_EXPORTABLE Shutdown()
{
	// Notify clients
	NotifyClientSessions(NX_NOTIFY_SHUTDOWN, 0);

	nxlog_write(MSG_SERVER_STOPPED, EVENTLOG_INFORMATION_TYPE, NULL);
	g_flags |= AF_SHUTDOWN;     // Set shutdown flag
	ConditionSet(m_condShutdown);

   CloseTaskScheduler();

   // Stop DCI cache loading thread
   g_dciCacheLoaderQueue.setShutdownMode();

#if XMPP_SUPPORTED
   StopXMPPConnector();
#endif

#if WITH_ZMQ
   StopZMQConnector();
#endif

	g_pEventQueue->clear();
	g_pEventQueue->put(INVALID_POINTER_VALUE);

	ShutdownMailer();
	ShutdownSMSSender();

	ThreadSleep(1);     // Give other threads a chance to terminate in a safe way
	nxlog_debug(2, _T("All threads was notified, continue with shutdown"));

	StopSyslogServer();
	StopHouseKeeper();

	// Wait for critical threads
	ThreadJoin(m_thPollManager);
	ThreadJoin(m_thSyncer);
	ThreadJoin(s_tunnelListenerThread);

	CloseAgentTunnels();

	// Call shutdown functions for the modules
   // CALL_ALL_MODULES cannot be used here because it checks for shutdown flag
   for(UINT32 i = 0; i < g_dwNumModules; i++)
	{
		if (g_pModuleList[i].pfShutdown != NULL)
			g_pModuleList[i].pfShutdown();
	}

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	SaveObjects(hdb, INVALID_INDEX);
	nxlog_debug(2, _T("All objects saved to database"));
	SaveUsers(hdb, INVALID_INDEX);
	nxlog_debug(2, _T("All users saved to database"));
   UpdatePStorageDatabase(hdb, INVALID_INDEX);
	nxlog_debug(2, _T("All persistent storage values saved"));
	DBConnectionPoolReleaseConnection(hdb);

	StopDBWriter();
	nxlog_debug(1, _T("Database writer stopped"));

	CleanupUsers();
	PersistentStorageDestroy();

	// Remove database lock
	UnlockDB();

	DBConnectionPoolShutdown();
	DBUnloadDriver(g_dbDriver);
	nxlog_debug(1, _T("Database driver unloaded"));

	CleanupActions();
	ShutdownEventSubsystem();
   ShutdownAlarmManager();
   nxlog_debug(1, _T("Event processing stopped"));

	ThreadPoolDestroy(g_agentConnectionThreadPool);
   ThreadPoolDestroy(g_mainThreadPool);
   MsgWaitQueue::shutdown();
   WatchdogShutdown();

	nxlog_debug(1, _T("Server shutdown complete"));
	nxlog_close();

	// Remove PID file
#ifndef _WIN32
	_tremove(g_szPIDFile);
#endif

	// Terminate process
#ifdef _WIN32
	if (!(g_flags & AF_DAEMON))
		ExitProcess(0);
#else
	exit(0);
#endif
}

/**
 * Fast server shutdown - normally called only by Windows service on system shutdown
 */
void NXCORE_EXPORTABLE FastShutdown()
{
   DbgPrintf(1, _T("Using fast shutdown procedure"));

	g_flags |= AF_SHUTDOWN;     // Set shutdown flag
	ConditionSet(m_condShutdown);

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	SaveObjects(hdb, INVALID_INDEX);
	DbgPrintf(2, _T("All objects saved to database"));
	SaveUsers(hdb, INVALID_INDEX);
	DbgPrintf(2, _T("All users saved to database"));
   UpdatePStorageDatabase(hdb, INVALID_INDEX);
	DbgPrintf(2, _T("All persistent storage values saved"));
	DBConnectionPoolReleaseConnection(hdb);

	// Remove database lock first, because we have a chance to lose DB connection
	UnlockDB();

	// Stop database writers
	StopDBWriter();
	DbgPrintf(1, _T("Database writer stopped"));

   DbgPrintf(1, _T("Server shutdown complete"));
	nxlog_close();
}

/**
 * Signal handler for UNIX platforms
 */
#ifndef _WIN32

void SignalHandlerStub(int nSignal)
{
	// should be unused, but JIC...
	if (nSignal == SIGCHLD)
	{
		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
	}
}

THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL SignalHandler(void *pArg)
{
	sigset_t signals;
	int nSignal;

	m_signalHandlerThread = pthread_self();

	// default for SIGCHLD: ignore
	signal(SIGCHLD, &SignalHandlerStub);

	sigemptyset(&signals);
	sigaddset(&signals, SIGTERM);
	sigaddset(&signals, SIGINT);
	sigaddset(&signals, SIGSEGV);
	sigaddset(&signals, SIGCHLD);
	sigaddset(&signals, SIGHUP);
	sigaddset(&signals, SIGUSR1);
	sigaddset(&signals, SIGUSR2);
#if !defined(__sun) && !defined(_AIX) && !defined(__hpux)
	sigaddset(&signals, SIGPIPE);
#endif

	sigprocmask(SIG_BLOCK, &signals, NULL);

	while(1)
	{
		if (sigwait(&signals, &nSignal) == 0)
		{
			switch(nSignal)
			{
				case SIGTERM:
				case SIGINT:
				   // avoid repeat Shutdown() call
				   if (!(g_flags & AF_SHUTDOWN))
				   {
                  m_nShutdownReason = SHUTDOWN_BY_SIGNAL;
                  if (IsStandalone())
                     Shutdown(); // will never return
                  else
                     ConditionSet(m_condShutdown);
				   }
				   break;
				case SIGSEGV:
					abort();
					break;
				case SIGCHLD:
					while (waitpid(-1, NULL, WNOHANG) > 0)
						;
					break;
				case SIGUSR1:
					if (g_flags & AF_SHUTDOWN)
						goto stop_handler;
					break;
				default:
					break;
			}
		}
		else
		{
			ThreadSleepMs(100);
		}
	}

stop_handler:
	sigprocmask(SIG_UNBLOCK, &signals, NULL);
	return THREAD_OK;
}

#endif

/**
 * Common main()
 */
THREAD_RESULT NXCORE_EXPORTABLE THREAD_CALL Main(void *pArg)
{
	nxlog_write(MSG_SERVER_STARTED, EVENTLOG_INFORMATION_TYPE, NULL);

	if (IsStandalone())
   {
      if (!(g_flags & AF_DEBUG_CONSOLE_DISABLED))
	   {
		   char *ptr, szCommand[256];
		   struct __console_ctx ctx;
#ifdef UNICODE
   		WCHAR wcCommand[256];
#endif

		   ctx.hSocket = -1;
		   ctx.socketMutex = INVALID_MUTEX_HANDLE;
		   ctx.pMsg = NULL;
		   ctx.session = NULL;
         ctx.output = NULL;
		   WriteToTerminal(_T("\nNetXMS Server V") NETXMS_VERSION_STRING _T(" Build ") NETXMS_VERSION_BUILD_STRING IS_UNICODE_BUILD_STRING _T(" Ready\n")
				             _T("Enter \"\x1b[1mhelp\x1b[0m\" for command list or \"\x1b[1mdown\x1b[0m\" for server shutdown\n")
				             _T("System Console\n\n"));

#if USE_READLINE
		   // Initialize readline library if we use it
		   rl_bind_key('\t', RL_INSERT_CAST rl_insert);
#endif

		   while(1)
		   {
#if USE_READLINE
   			ptr = readline("\x1b[33mnetxmsd:\x1b[0m ");
#else
			   WriteToTerminal(_T("\x1b[33mnetxmsd:\x1b[0m "));
			   fflush(stdout);
			   if (fgets(szCommand, 255, stdin) == NULL)
				   break;   // Error reading stdin
			   ptr = strchr(szCommand, '\n');
			   if (ptr != NULL)
				   *ptr = 0;
			   ptr = szCommand;
#endif

			   if (ptr != NULL)
			   {
#ifdef UNICODE
#if HAVE_MBSTOWCS
			      mbstowcs(wcCommand, ptr, 255);
#else
				   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, ptr, -1, wcCommand, 256);
#endif
				   wcCommand[255] = 0;
				   StrStrip(wcCommand);
				   if (wcCommand[0] != 0)
				   {
					   if (ProcessConsoleCommand(wcCommand, &ctx) == CMD_EXIT_SHUTDOWN)
#else
				   StrStrip(ptr);
				   if (*ptr != 0)
				   {
					   if (ProcessConsoleCommand(ptr, &ctx) == CMD_EXIT_SHUTDOWN)
#endif
						   break;
#if USE_READLINE
					   add_history(ptr);
#endif
				   }
#if USE_READLINE
				   free(ptr);
#endif
			   }
			   else
			   {
				   _tprintf(_T("\n"));
			   }
		   }

#if USE_READLINE
   		free(ptr);
#endif
   		if (!(g_flags & AF_SHUTDOWN))
   		{
            m_nShutdownReason = SHUTDOWN_FROM_CONSOLE;
            Shutdown();
   		}
      }
      else
      {
         // standalone with debug console disabled
#ifdef _WIN32
         _tprintf(_T("Server running. Press ESC to shutdown.\n"));
         while(1)
         {
            if (_getch() == 27)
               break;
         }
         _tprintf(_T("Server shutting down...\n"));
         Shutdown();
#else
         _tprintf(_T("Server running. Press Ctrl+C to shutdown.\n"));
         // Shutdown will be called from signal handler
   		ConditionWait(m_condShutdown, INFINITE);
#endif
      }
	}
	else
	{
		ConditionWait(m_condShutdown, INFINITE);
		// On Win32, Shutdown() will be called by service control handler
#ifndef _WIN32
		Shutdown();
#endif
	}
	return THREAD_OK;
}

/**
 * Initiate server shutdown
 */
void InitiateShutdown()
{
#ifdef _WIN32
	Shutdown();
#else
	if (IsStandalone())
	{
		Shutdown();
	}
	else
	{
		pthread_kill(m_signalHandlerThread, SIGTERM);
	}
#endif
}

/**
 *DLL Entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
