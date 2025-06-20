#ifndef __LOGGER__H_STARK_
#define __LOGGER__H_STARK_

#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>
// ���������־��Ϣ
enum LogLevel { DEBUG = 0, INFO, WARNING, ERROR, FATAL, UNKNOWN };

//==========================================================================
// ��־��ʽ�����ӿ�
class IFormatter {
 public:
  virtual ~IFormatter() = default;
  virtual std::string format(const std::string& message, LogLevel level,
                             const std::string& logger_name,
                             const std::string& timestamp) = 0;
};

// ���׸�ʽ����
// example: [2025:06:16][error][hostscan.log]: "errmessage"
class SimpleFormatter : public IFormatter {
 public:
  std::string format(const std::string& message, LogLevel level,
                     const std::string& logger_name,
                     const std::string& timestamp) override {
    std::string level_str;
    switch (level) {
      case LogLevel::DEBUG:
        level_str = "DEBUG";
        break;
      case LogLevel::INFO:
        level_str = "INFO";
        break;
      case LogLevel::ERROR:
        level_str = "ERROR";
        break;
      case LogLevel::FATAL:
        level_str = "FATAL";
        break;
      case LogLevel::UNKNOWN:
        level_str = "UNKNOWN";
        break;
    }

    return "[" + timestamp + "]" + "[" + level_str + "]" + "[" + logger_name +
           "]" + ": \"" + message + "\"";
  }
};

//==========================================================================
// ��־���Ŀ��ӿ�
class IHandler {
 protected:
  std::shared_ptr<IFormatter> formatter;

 public:
  IHandler(std::shared_ptr<IFormatter> _formatter) : formatter(_formatter) {}
  virtual ~IHandler() = default;
  virtual void emit(const std::string& message, LogLevel level,
                    const std::string& logger_name,
                    const std::string& timestamp) = 0;
};

// ����̨���Ŀ��
class ConsoleHandler : public IHandler {
 public:
  ConsoleHandler(std::shared_ptr<IFormatter> _formatter)
      : IHandler(_formatter) {}

  void emit(const std::string& message, LogLevel level,
            const std::string& logger_name,
            const std::string& timestamp) override {
    std::string formatted =
        formatter->format(message, level, logger_name, timestamp);

    // ������־����ѡ�������
    if (level == LogLevel::ERROR || level == LogLevel::FATAL ||
        level == LogLevel::UNKNOWN) {
      std::cerr << formatted << std::endl;
    } else {
      std::cout << formatted << std::endl;
    }
  }
};

// �ļ����Ŀ��
class FileHandler : public IHandler {
 private:
  std::ofstream file;
  std::string filename;

 public:
  FileHandler(const std::string _filename,
              std::shared_ptr<IFormatter> _formatter)
      : IHandler(_formatter), filename(_filename) {
    file.open(filename, std::ios::app);
    if (!file.is_open()) {
      std::cerr << "�޷�����־�ļ���" << filename << std::endl;
    }
  }

  ~FileHandler() {
    if (file.is_open()) {
      file.close();
    }
  }
  void emit(const std::string& message, LogLevel level,
            const std::string& logger_name,
            const std::string& timestamp) override {
    if (!file.is_open()) return;
    std::string formatted =
        formatter->format(message, level, logger_name, timestamp);
    file << formatted << std::endl;
  }
};

//==========================================================================
// �������ӿ�
class IFilter {
 public:
  virtual ~IFilter() = default;
  virtual bool filter(const std::string& message, LogLevel level,
                      const std::string& logger_name) = 0;
};

//==========================================================================
// ��־��¼��
class Logger {
 private:
  std::string name;
  LogLevel level;
  std::vector<std::shared_ptr<IHandler>> handlers;
  std::vector<std::shared_ptr<IFilter>> filters;
  static std::mutex mutex;

  // ��ȡ��ǰʱ���
  std::string get_timestamp() const {
    auto now = std::time(nullptr);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
  }

  // ����Ƿ�Ӧ�ü�¼����־
  bool should_log(const std::string& message, LogLevel level) const {
    if (level < this->level) return false;
    for (const auto& filter : filters) {
      // ���������һ��������������,�Ͳ����
      if (!filter->filter(message, level, name)) return false;
    }
    return true;
  }

  // ʵ�ʼ�¼����־
  void log(const std::string& message, LogLevel level) {
    if (!should_log(message, level)) return;

    std::string timestamp = get_timestamp();

    // ʹ�û�������֤�̰߳�ȫ
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (const auto& handler : handlers) {
        handler->emit(message, level, name, timestamp);
      }
    }
  }

 public:
  Logger(const std::string& _name, LogLevel _level = LogLevel::INFO)
      : name(_name), level(_level) {}

  // ��Ӵ�����
  void add_handler(std::shared_ptr<IHandler> handler) {
    handlers.push_back(handler);
  }

  // ��ӹ�����
  void add_filter(std::shared_ptr<IFilter> filter) {
    filters.push_back(filter);
  }

  // ������־����
  void set_level(LogLevel level) { this->level = level; }

  // ��־��¼����
  void debug(const std::string& message) { log(message, LogLevel::DEBUG); }
  void info(const std::string& message) { log(message, LogLevel::INFO); }
  void warning(const std::string& message) { log(message, LogLevel::WARNING); }
  void error(const std::string& message) { log(message, LogLevel::ERROR); }
  void fatal(const std::string& message) { log(message, LogLevel::FATAL); }
  void unknown(const std::string& message) { log(message, LogLevel::UNKNOWN); }
};

// ��־������-����ģʽ
class LoggerManager {
 public:
  static LoggerManager& get_instance() { return instance; }
  std::shared_ptr<Logger> get_logger(const std::string& name) {
    auto it = loggers.find(name);
    if (it != loggers.end()) return it->second;

    auto new_logger = std::make_shared<Logger>(name);
    loggers[name] = new_logger;
    return new_logger;
  }

 private:
  static LoggerManager instance;
  std::map<std::string, std::shared_ptr<Logger>> loggers;

  LoggerManager() = default;
  LoggerManager(const LoggerManager&) = delete;
  LoggerManager& operator=(const LoggerManager&) = delete;
};

void test() {
  // ��ȡ��־��¼��
  auto logger = LoggerManager::get_instance().get_logger("test");

  // ������־����
  logger->set_level(LogLevel::DEBUG);

  // ��ӿ���̨������
  auto console_formatter = std::make_shared<SimpleFormatter>();
  auto console_handler = std::make_shared<ConsoleHandler>(console_formatter);
  logger->add_handler(console_handler);

  // ����ļ�������
  auto file_formatter = std::make_shared<SimpleFormatter>();
  auto file_handler = std::make_shared<FileHandler>("app.log", file_formatter);
  logger->add_handler(file_handler);

  // ��¼��ͬ�������־
  logger->debug("����һ��������Ϣ");
  logger->info("����һ����ͨ��Ϣ");
  logger->warning("����һ��������Ϣ");
  logger->error("����һ��������Ϣ");
  logger->fatal("����һ�����ش�����Ϣ");
  logger->unknown("����һ��δ֪������Ϣ");
}
#endif  // !__LOGGER__H_STARK_