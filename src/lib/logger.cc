/*
 * DICOM software development library (SDL)
 * Copyright (c) 2010-2020, Kim, Tae-Sung. All rights reserved.
 * See copyright.txt for details.
 *
 * logger.cc
 */

#include <stdarg.h>

#include <iostream>

#include "dicom.h"

namespace dicom {  //-----------------------------------------------------------

/* -----------------------------------------------------------------------------
 * default logger
 */

void default_logger_function(LogLevel::type loglevel, const char *message) {
  const char *p;
  switch (loglevel) {
    case LogLevel::DEBUG:
      p = "debug:\t";
      break;
    case LogLevel::WARN:
      p = "warn:\t";
      break;
    case LogLevel::ERROR:
      p = "error:\t";
      break;
    default:
      p = "";
      break;
  }
  std::cerr << p << message << std::endl;
}

/* -----------------------------------------------------------------------------
 * logger
 */

class Logger {
  LoggerFunctionType logger_function_;
  LogLevel::type loglevel_;

 public:
  Logger()
      : logger_function_(default_logger_function), loglevel_(LogLevel::WARN) {}

  // get the singleton instance
  static Logger *get_logger() {
    static Logger logger_instance;
    return &logger_instance;
  }

  // log_msg display message if loglevel >= loglevel,
  void log_msg(LogLevel::type loglevel, const char *format, va_list args) {
    if (logger_function_ && loglevel >= this->loglevel_) {
      char buf[MESSAGE_BUFFER_SIZE];

      vsnprintf(buf, MESSAGE_BUFFER_SIZE, format, args);
      buf[MESSAGE_BUFFER_SIZE - 1] = '\0';
      logger_function_(loglevel, buf);
    }
  }

  void set_default_logger_function() {
    logger_function_ = default_logger_function;
  }

  void set_loglevel(LogLevel::type loglevel) { loglevel_ = loglevel; }

  inline LogLevel::type get_loglevel() { return loglevel_; }

  void set_logger_function(LoggerFunctionType &logfunc) {
    if (logfunc)
      logger_function_ = logfunc;
    else
      set_default_logger_function();
  }

 private:
  static const int MESSAGE_BUFFER_SIZE = 512;
};

/* -----------------------------------------------------------------------------
 * exported functions
 */

void log_message(LogLevel::type loglevel, const char *format, ...) {
  va_list args;
  va_start(args, format);
  Logger::get_logger()->log_msg(loglevel, format, args);
  va_end(args);
}

LogLevel::type get_loglevel() { return Logger::get_logger()->get_loglevel(); }

void set_loglevel(LogLevel::type loglevel) {
  Logger::get_logger()->set_loglevel(loglevel);
}

void set_logger_function(LoggerFunctionType &logfunc) {
  Logger::get_logger()->set_logger_function(logfunc);
}

void set_default_logger_function() {
  Logger::get_logger()->set_default_logger_function();
}

}  // namespace dicom
