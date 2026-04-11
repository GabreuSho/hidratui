// src/tui/file_monitor.cpp
#include "file_monitor.h"

#include <iostream>

FileMonitor::FileMonitor(const std::string& filepath) : filepath_(filepath) {
  inotify_fd_ = inotify_init1(IN_NONBLOCK);
  if (inotify_fd_ < 0) {
    throw std::runtime_error("Falha ao inicializar inotify");
  }

  watch_descriptor_ = inotify_add_watch(inotify_fd_, filepath.c_str(),
                                        IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB);
  if (watch_descriptor_ < 0) {
    close(inotify_fd_);
    throw std::runtime_error("Falha ao monitorar arquivo: " + filepath);
  }
}

FileMonitor::~FileMonitor() {
  stop();
  if (watch_descriptor_ >= 0) inotify_rm_watch(inotify_fd_, watch_descriptor_);
  if (inotify_fd_ >= 0) close(inotify_fd_);
}

void FileMonitor::set_callback(std::function<void()> on_change) {
  on_change_callback_ = std::move(on_change);
}

void FileMonitor::start() {
  if (running_.exchange(true)) return;  // Já está rodando
  monitor_thread_ = std::thread(&FileMonitor::monitor_loop, this);
}

void FileMonitor::stop() {
  if (!running_.exchange(false)) return;  // Já está parado
  if (monitor_thread_.joinable()) {
    monitor_thread_.join();
  }
}

void FileMonitor::monitor_loop() {
  char buffer[1024] __attribute__((aligned(8)));

  while (running_.load()) {
    pollfd pfd = {inotify_fd_, POLLIN, 0};
    int ret = poll(&pfd, 1, 200);  // Timeout de 200ms para permitir shutdown

    if (ret > 0 && (pfd.revents & POLLIN)) {
      ssize_t len = read(inotify_fd_, buffer, sizeof(buffer));
      if (len > 0 && on_change_callback_) {
        // Debounce simples: espera 50ms para evitar múltiplos triggers
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        on_change_callback_();
      }
    }
  }
}
