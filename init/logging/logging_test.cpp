#include <cstdio>
#include <cstdlib>
#include "logging.h"
#include "logprint.h"
#include "list.h"
#include <string.h>

int main()
{
	// Create a new log format instance
	LinuxLogFormat *log_format = linux_log_format_new();
	if (!log_format) {
		fprintf(stderr, "Failed to create LinuxLogFormat\n");
		return EXIT_FAILURE;
	}

	// Set the desired log format and settings
	linux_log_setPrintFormat(log_format,
				 FORMAT_TIME); // Use the "time" log format
	//log_format->colored_output = true;                 // Enable colored output
	//log_format->printable_output = true;               // Escape unprintable characters

	// Create a sample log entry
	LinuxLogEntry log_entry;
	log_entry.priority = LINUX_LOG_INFO;
	log_entry.tv_sec = time(nullptr); // Current time in seconds
	log_entry.tv_nsec = 123456789; // Sample nanoseconds
	log_entry.pid = getpid(); // Process ID
	log_entry.tid = pthread_self(); // Thread ID
	log_entry.uid = getuid(); // User ID
	log_entry.tag = "TEST_LOGGER"; // Tag for the log
	log_entry.tagLen = strlen(log_entry.tag);
	log_entry.message =
		"This is a test log message.\nIt spans multiple lines.";
	log_entry.messageLen = strlen(log_entry.message);

	// Print the log entry to stdout
	linux_log_printLogLine(log_format, stdout, &log_entry);

	// Clean up resources
	linux_log_format_free(log_format);

	return EXIT_SUCCESS;
}
