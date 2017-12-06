// Copyright 2017-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the license found in the
// LICENSE file in the root directory of this source tree.
//File: executor.hh

#pragma once

#include <queue>
#include <mutex>
#include <atomic>
#include <future>
#include <thread>
#include <iostream>
#include "lib/debugutils.hh"


namespace render {


// Run something (rendering) in a dedicated thread
class ExecutorInThread {
  public:
    ExecutorInThread() {
      th_ = std::thread([=]() { this->work(); });
    }

    ~ExecutorInThread() { stop(); }

    // Run job in the dedicated thread and return the result.
    template <typename T>
    T execute_sync(std::function<T()>&& job) {
      std::packaged_task<T()> task(job);
      auto res = task.get_future();
      execute_async([&task]() { task(); });
      auto r = res.get();
      return r;
    }

    void execute_sync(std::function<void()>&& job) {
      std::packaged_task<void()> task(job);
      auto res = task.get_future();
      execute_async([&task]() { task(); });
      res.wait();
    }

    // push job to the queue for future execution in the dedicated thread
    void execute_async(std::function<void()>&& job) {
      std::lock_guard<std::mutex> lg(mutex_);
      jobs_.emplace(job);
      cv.notify_one();
    }

    void work() {
      std::unique_lock<std::mutex> lk(mutex_, std::defer_lock);
      while (!stopped.load()) {
        lk.lock();
        if (! jobs_.empty()) {
          //print_debug("Not Empty\n");
          auto job = jobs_.front();
          jobs_.pop();
          lk.unlock();
          job();
          //print_debug("Finsih Run Job\n");
        } else {  // just wait on cv
          //print_debug("Empty, watiing\n");
          cv.wait(lk);
          lk.unlock();
          //print_debug("Done watining\n");
        }
      }

      //print_debug("Clean up job queue \n");
      std::lock_guard<std::mutex> lg(mutex_);
      while (jobs_.size()) {
        auto job = jobs_.front();
        jobs_.pop();
        job();
      }
    }

    void stop() {
      //print_debug("Stopping ...\n");
      stopped.store(true);
      {
        std::lock_guard<std::mutex> lg(mutex_);
        cv.notify_one();
      }
      if (th_.joinable())
        th_.join();
    }

  private:
    std::thread th_;

    std::mutex mutex_;

    std::atomic_bool stopped{false};

    std::condition_variable cv;
    std::queue<std::function<void()>> jobs_;

};


}
