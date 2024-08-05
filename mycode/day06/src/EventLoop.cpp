#include "EventLoop.h"

#include "Channel.h"
#include "Epoll.h"

EventLoop::EventLoop() : ep(nullptr), quit(false) { ep = new Epoll(); }

EventLoop::~EventLoop() { delete ep; }

void EventLoop::loop() {
  while (!quit) {
    auto chs = ep->poll();
    for (auto c : chs) {
    }
  }
}

void EventLoop::updateChannel(Channel *ch) { ep->updateChannel(ch); }