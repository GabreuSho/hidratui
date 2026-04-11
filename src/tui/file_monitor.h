// src/tui/file_monitor.h
#pragma once
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

class FileMonitor {
 public:
  FileMonitor(const std::string& filepath);
  ~FileMonitor();

  void set_callback(std::function<void()> on_change);
  void start();
  void stop();
  bool is_running() const { return running_.load(); }

 private:
  std::string filepath_;
  std::function<void()> on_change_callback_;
  std::thread monitor_thread_;
  std::atomic<bool> running_{false};
  int inotify_fd_;
  int watch_descriptor_;

  void monitor_loop();
};
