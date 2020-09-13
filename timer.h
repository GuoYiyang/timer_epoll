#ifndef TIMER_H_
#define TIMER_H_
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

#include <functional>
#include <iostream>

#include "error.h"

static Error CREATE_TIMER_ERROR(1001, "create timer error!");
static Error MALLOC_ERROR(1002, "malloc error!");
static Error EPOLL_CTL_ERROR(1003, "epoll ctl error!");
static Error TIMERFD_ERROR(1004, "timerfd error!");
static Error EPOLL_CREATE_ERROR(1005, "epoll create error!");

struct param {
  struct itimerspec its;
  int tfd;
  std::function<void()> callbacktask;
};

class Timer {
 private:
  int epollfd_;

 public:
  Timer() {}

  int Init() {
    epollfd_ = epoll_create(100);
    if (epollfd_ == -1) {
      return 1005;
    }
    return 0;
  }

  int CreateTimer(struct itimerspec* timeValue, int interval) {
    if (!timeValue) {
      return -1;
    }

    int timefd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timefd < 0) {
      return -1;
    }

    struct timespec nw;

    if (clock_gettime(CLOCK_MONOTONIC, &nw) != 0) {
      close(timefd);
      return -1;
    }

    timeValue->it_value.tv_sec = nw.tv_sec + interval;
    timeValue->it_value.tv_nsec = 0;
    timeValue->it_interval.tv_sec = interval;
    timeValue->it_interval.tv_nsec = 0;

    int ret = timerfd_settime(timefd, TFD_TIMER_ABSTIME, timeValue, NULL);
    if (ret < 0) {
      close(timefd);
      return -1;
    }
    return timefd;
  }

  int SetTimer(int interval, std::function<void()> task) {
    struct itimerspec timeValue;
    int timefd = CreateTimer(&timeValue, interval);
    if (timefd == -1) {
      return 1001;
    }

    struct param* pm = (struct param*)malloc(sizeof(struct param));
    if (pm == NULL) {
      return 1002;
    }
    pm->its = timeValue;
    pm->tfd = timefd;
    pm->callbacktask = task;

    struct epoll_event tevent;
    tevent.data.ptr = pm;
    tevent.events = EPOLLIN | EPOLLET;

    int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, pm->tfd, &tevent);
    if (ret == -1) {
      return 1003;
    }
    return 0;
  }

  int StartTimer() {
    while (1) {
      struct epoll_event tevents[10];
      int n = epoll_wait(epollfd_, tevents, 10, 1000);
      if (n == 0) {
        continue;
      }
      for (int i = 0; i < n; ++i) {
        int ret;
        struct param* pm = (struct param*)(tevents[i].data.ptr);
        if (tevents[i].events & EPOLLIN) {
          std::function<void()> callback = pm->callbacktask;
          callback();
          ret = epoll_ctl(epollfd_, EPOLL_CTL_DEL, pm->tfd, NULL);
          if (ret == -1) {
            return 1003;
          }
          struct epoll_event ev;
          ev.events = EPOLLIN | EPOLLET;
          pm->its.it_value.tv_sec =
              pm->its.it_value.tv_sec + pm->its.it_interval.tv_sec;
          ev.data.ptr = pm;

          ret = timerfd_settime(pm->tfd, TFD_TIMER_ABSTIME, &(pm->its), NULL);
          if (ret < 0) {
            return 1004;
          }

          int ret = epoll_ctl(epollfd_, EPOLL_CTL_ADD, pm->tfd, &ev);
          if (ret == -1) {
            return 1003;
          }
        }
      }
    }
    return 0;
  }
};
#endif