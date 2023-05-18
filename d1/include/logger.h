#ifndef RT_FILTERFLAG_ALL
#include "stdint.h"
#include "pstypes.h"
#include <assert.h>

typedef enum RT_LogSeverity {
	RT_LOGSERVERITY_INFO,
	RT_LOGSERVERITY_MINOR,
	RT_LOGSERVERITY_MEDIUM,
	RT_LOGSERVERITY_HIGH,
	RT_LOGSERVERITY_ASSERT
} RT_LogSeverity;

#define RT_FILTERFLAG_ALL (0 << 0)
#define RT_FILTERFLAG_INFO (1 << 0)
#define RT_FILTERFLAG_MINOR (1 << 1)
#define RT_FILTERFLAG_MEDIUM (1 << 2)
#define RT_FILTERFLAG_HIGH (1 << 3)
#define RT_FILTERFLAG_ASSERT (1 << 4)

#define RT_LOG_INIT(filter) InitLog(filter)

#define RT_LOG(severity, message)\
do{\
	Log(severity, message, __FILE__, __LINE__);\
	if (severity == RT_LOGSERVERITY_ASSERT)\
		assert(0 && "Logger assert, check log file for information");\
} while (0)

#define RT_LOGF(severity, message, ...)\
do{\
	LogF(severity, message, __FILE__, __LINE__, ##__VA_ARGS__);\
	if (severity == RT_LOGSERVERITY_ASSERT)\
		assert(0 && "Logger assert, check log file for information");\
} while (0)

void InitLog(uint8_t a_flags);
void Log(RT_LogSeverity a_serverity, char* a_message, char* a_file, int a_line);
void LogF(RT_LogSeverity a_serverity, char* a_message, char* a_file, int a_line, ...);
bool CheckSeverity(RT_LogSeverity a_serverity);
void PrintMessage(RT_LogSeverity a_serverity, char* a_message, char* a_file, int a_line);
#endif