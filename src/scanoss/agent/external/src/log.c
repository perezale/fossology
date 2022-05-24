
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "log.h"

static const char *level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

// Default log level is INFO
static int LEVEL = LOG_INFO;

// Default log file is STDERR
static FILE *LOG_FILE = NULL;

char *format_timestamp()
{
  // I. Format timestamp
  // Always use UTC time
  char *out = malloc(64);
  time_t t = time(NULL);
  struct tm *utc = gmtime(&t);
  strftime(out, 64, "%d-%m-%y %H:%M:%S", utc);
  return out;
}

/**
 * Formats UTC timestamp in common log format.
 */
char *format_ts_common_log()
{
  //%d/%b/%Y:%H:%M:%S %z
  char *out = malloc(64);
  time_t t = time(NULL);
  struct tm *utc = gmtime(&t);
  strftime(out, 64, "%d/%b/%Y:%H:%M:%S %z", utc);
  return out;
}

void __logger(int level, const char *file, int line, const char *func, const char *format, ...)
{
  if (level < LEVEL)
  {
    return;
  }

  if (LOG_FILE == NULL)
  {
    LOG_FILE = stderr;
  }

  char *buf = format_timestamp();
  va_list args;

  // II. Format log
  fprintf(LOG_FILE, "%s %lu %-5s %s:%s:%d: ", buf, (unsigned long) pthread_self(), level_names[level], file, func, line);
  va_start(args, format);
  vfprintf(LOG_FILE, format, args);
  va_end(args);
  fprintf(LOG_FILE, "\n");
  fflush(LOG_FILE);
  free(buf);

  // Exit with error if log level is FATAL
  if (level == LOG_FATAL)
  {
    exit(EXIT_FAILURE);
  }
}

bool log_level_is_enabled(int level)
{
  return level >= LEVEL;
}

void log_set_level(int level)
{
  LEVEL = level;
}

void log_set_file(char *filename)
{
  LOG_FILE = fopen(filename, "a+");
  if (LOG_FILE == NULL)
  {
    fprintf(stderr, "ERROR SETTING LOG FILE: %s\n", filename);
    exit(EXIT_FAILURE);
  }
}

void log_close_file()
{
  fclose(LOG_FILE);
}

