#include "logger.h"
#include "physfsx.h"
#include "time.h"

PHYSFS_file* log_file = NULL;
uint8_t filter = 0b00000000;

char arr[5][10] =
{ 
	"INFO:",
	"MINOR:",
	"MEDIUM:",
	"HIGH:",
	"ASSERT:"
};

void InitLog(uint8_t flags)
{
	filter |= flags;

	log_file = PHYSFS_openWrite("RTLogger.txt");
	RT_LOG(RT_LOGSERVERITY_INFO, "logger initialized");
}

void Log(RT_LogSeverity a_serverity, char* a_message, char* a_file, int a_line) {
	if (CheckSeverity(a_serverity))
		return;

	PrintMessage(a_serverity, a_message, a_file, a_line);
}

void LogF(RT_LogSeverity a_serverity, char* a_message, char* a_file, int a_line, ...) {
	if (CheckSeverity(a_serverity))
		return;
	
	va_list va_format_list;
	va_start(va_format_list, a_line);

	size_t buffersize = vsnprintf(NULL, 0, a_message, va_format_list) + 1;
	const char* formatted_message = (char*)malloc(buffersize);
	vsnprintf(formatted_message, buffersize, a_message, va_format_list);

	PrintMessage(a_serverity, formatted_message, a_file, a_line);

	free(formatted_message);
}

bool CheckSeverity(RT_LogSeverity a_serverity) {
	uint8_t currentFlag = (1 << a_serverity);
	if (filter != RT_FILTERFLAG_ALL && !(filter & currentFlag))
		return 1;

	return 0;
}

void PrintMessage(RT_LogSeverity a_serverity, char* a_message, char* a_file, int a_line) {
	struct tm* lt;
	time_t t;
	t = time(NULL);
	lt = localtime(&t);

	PHYSFSX_printf(log_file, "[%02i:%02i:%02i] %s %s - File: %s on line %d\n",
		lt->tm_hour, lt->tm_min, lt->tm_sec,
		arr[a_serverity],
		a_message,
		a_file,
		a_line);

	printf("% s % s - File: % s on line % d\n",
		arr[a_serverity],
		a_message,
		a_file,
		a_line);
}