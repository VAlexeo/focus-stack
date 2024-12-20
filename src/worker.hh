// Work queue that distributes alignment and merging operations between threads.

#pragma once
#include <thread>
#include <deque>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <exception>
#include <opencv2/core/core.hpp>
#include "logger.hh"

namespace focusstack {

// Generic runnable task, optionally with some dependencies on other tasks
class Task
{
public:
  Task();
  virtual ~Task();

  virtual bool ready_to_run();
  virtual bool uses_opencl() { return false; }
  bool is_running() const { return m_running; }
  bool is_completed() const { return m_done; }
  void run(std::shared_ptr<Logger> logger);
  std::string filename() const { return m_filename; }
  std::string name() const { return m_name; }
  std::string basename() const;
  int index() const { return m_index; }
  void set_index(int index) { m_index = index; }
  const std::vector<std::shared_ptr<Task> > &get_depends() const { return m_depends_on; }

  void wait();

protected:
  virtual void task() { };

  std::shared_ptr<Logger> m_logger;
  std::string m_filename;
  int m_index;
  int m_jpgquality;
  std::string m_name;
  std::mutex m_mutex;
  std::vector<std::shared_ptr<Task> > m_depends_on; // List of tasks this task needs as inputs

  std::condition_variable m_wakeup;
  bool m_running;
  bool m_done;
};

// Task that has image as a result.
class ImgTask: public Task
{
public:
  ImgTask() {};
  ImgTask(cv::Mat result): m_result(result) {}
  virtual const cv::Mat &img() const { return m_result; }

protected:
  cv::Mat m_result;
};

// Work queue class that distributes tasks to threads.
class Worker
{
public:
  Worker(int max_threads, std::shared_ptr<Logger> logger);
  ~Worker();

  // Add task to the end of the queue
  void add(std::shared_ptr<Task> task);

  // Prepend task, causing it to run as soon as possible
  void prepend(std::shared_ptr<Task> task);

  // Wait until all tasks have finished
  bool wait_all(int timeout_ms = -1);

  bool failed() const { return m_failed; }

  std::string error() const { return m_error; }

  void get_status(int &total_tasks, int &completed_tasks);

private:
  std::shared_ptr<Logger> m_logger;
  std::vector<std::thread> m_threads;
  std::deque<std::shared_ptr<Task> > m_tasks;

  bool m_closed;
  int m_tasks_started;
  int m_total_tasks;
  int m_completed_tasks;
  int m_opencl_users;

  bool m_failed;
  std::string m_error;

  std::mutex m_mutex;
  std::condition_variable m_wakeup;

  std::chrono::time_point<std::chrono::steady_clock> m_start_time;
  float seconds_passed() const;

  void worker(int thread_idx);
};


}
