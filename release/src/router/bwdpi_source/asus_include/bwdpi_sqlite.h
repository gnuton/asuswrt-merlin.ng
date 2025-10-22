
#include "sqlite3.h"

#define MON_SEC 86400 * 31
#define DAY_SEC 86400
#define HOURSEC 3600
#define QUERY_LEN 960 // BWSQL_LOG buffer size is 1024
#define LEN_MAX	32000

#define BWMON_LOCK   "/var/lock/AiProtectionMonitor.lock"
#define BWMON_PID    "/var/run/AiProtectionMonitor.pid"

#define BWHIS_LOCK   "/var/lock/WebHistory.lock"
#define BWHIS_PID    "/var/run/WebHistory.pid"

#define BWANA_LOCK   "/var/lock/TrafficAnalyzer.lock"
#define BWANA_PID    "/var/run/TrafficAnalyzer.pid"

// traffic_analyzer.c
extern int traffic_analyzer_main(int argc, char **argv);
extern int sql_integrity_check(sqlite3 *db, char *db_path);
extern int sql_remove_journal(char *db_file);

// sqlite_stat.c
extern int sql_get_table(sqlite3 *db, const char *sql, char ***pazResult, int *pnRow, int *pnColumn);
extern void bwdpi_maclist_db(char *type, int *retval, webs_t wp);
extern char *AiProtectionMontior_GetType(char *c);
extern void AiProtectionMonitor_result(int *tmp, char **result, int rows, int cols, int shift);
extern void sqlite_Stat_hook(int type, char *client, char *mode, char *dura, char *date, int *retval, webs_t wp);
extern void get_web_hook(char *client, char *page, char *num, int *retval, webs_t wp);
extern time_t Date_Of_Timestamp(time_t now);
extern void bwdpi_monitor_stat(int *retval, webs_t wp);
extern void bwdpi_monitor_info(char *type, char *event, int *retval, webs_t wp);
extern void bwdpi_monitor_ips(char *type, char *date, int *retval, webs_t wp);
extern void bwdpi_monitor_nonips(char *type, char *date, int *retval, webs_t wp);
extern void bwdpi_cgi_mon_to_json(char *type, char *start, char *end, FILE *stream);
extern void bwdpi_cgi_mon_del_db(char *type, FILE *stream);

// web_history.c
extern int web_history_main(int argc, char **argv);

// AiProctionMonitor.c
extern int aiprotection_monitor_main(int argc, char **argv);
extern void ProcessCheckPid(char *type);
