/*
Task executor that adds tasks with a specified delay to be executed.
Producer thread : adds tasks to queue with deelay time.
Consumer thread : waits for tasks and executes top of queue tasks.
Poller thread   : keeps polling the buffer to notify consumer if any task is
                  avaliable for execution or not.
*/

#include <bits/stdc++.h>
#include <thread>
#include <deque>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <unistd.h>
#include <functional>

using namespace std;
using std::deque;

mutex buffer_mutex, cout_mutex;
condition_variable cond;

mutex status_mutex;
bool producer_stopped = false, consumer_stopped = false;

class Buffer {
private:
  struct Cmp {
  public:
    bool operator()(pair<int, function<void()>>& a, pair<int, function<void()>>& b) {
      return a.first > b.first;
    }
  };
  priority_queue<pair<int, function<void()>>,
                 vector<pair<int, function<void()>>>, Cmp> buffer_;
  int size_;
  
public:
    Buffer(int buffer_limit) {
        size_ = buffer_limit;
    };
    
    void add(pair<int, function<void()>> func) {
        unique_lock<std::mutex> locker(buffer_mutex);
        cond.wait(locker, [this](){return buffer_.size() < size_;});
        
        // critical section
        buffer_.push(func);
        
        locker.unlock();
        cond.notify_all();
    }
    function<void()> remove() {

        unique_lock<std::mutex> locker(buffer_mutex);
        cond.wait(locker,
          [this](){return buffer_.size() > 0 && buffer_.top().first <= (int)time(nullptr);});
        
        // critical section
        auto back = buffer_.top().second;
        buffer_.pop(); 
        
        locker.unlock();
        cond.notify_all();
        return back;
    }

    void notifyConsumers() {
      bool notify = false;

      unique_lock<std::mutex> locker(buffer_mutex);
      if (buffer_.size() > 0 && buffer_.top().first <= (int)time(nullptr)) {
        notify = true;
      }  
      locker.unlock();

      if (notify) {
        cond.notify_all();
      }
    }
    
    pair<int, function<void()>> top() {
        return buffer_.top();
    }
    
    void pop() {
        buffer_.pop();
    }
};

class Producer {
public:
    Producer(Buffer* buffer, int limit) {
        this->buffer_ = buffer;
        this->produced = 0;
        this->produce_limit = limit;
    }
    void run() {
        while (true) {
            this_thread::sleep_for(chrono::milliseconds(50));
            int current_produced = produced;
            function<void()> print_produced = [=]() {
                lock_guard<mutex> lock(cout_mutex);
                cout << "consumed: " << current_produced << endl;  
            };
            
            int execution_time = (int)time(nullptr) + (produce_limit - produced);
            buffer_->add(make_pair(execution_time, print_produced));
            
            ++produced;
            
            if (produced == produce_limit) {
              unique_lock<std::mutex> locker(status_mutex);
              producer_stopped = true;
              return;
            }
        }
    }
private:
    Buffer *buffer_;
    int produced;
    int produce_limit;
};

class Consumer {
public:
    Consumer(Buffer* buffer, int limit) {
        this->buffer_ = buffer;
        this->consumed = 0;
        this->consume_limit = limit;
    }
    
    void run() {
        while (true) {
            this_thread::sleep_for(chrono::milliseconds(50));
            auto func = buffer_->remove();
            func();
            ++consumed;
            if (consumed == consume_limit) {
              unique_lock<std::mutex> locker(status_mutex);
              consumer_stopped = true;
              return;
            }
        }
    }
private:
    Buffer *buffer_;
    int consumed;
    int consume_limit;
};

class Poller {
public:
    Poller(Buffer* buffer) {
        this->buffer_ = buffer;
    }
    
    void run() {
        while (true) {
            this_thread::sleep_for(chrono::milliseconds(50));
            buffer_->notifyConsumers();
            unique_lock<std::mutex> locker(status_mutex);
            if (producer_stopped && consumer_stopped) {
              return;
            }
        }
    }
private:
    Buffer *buffer_;
};

int main() {

    Buffer b(10);
    int limit = 10;
    Producer p(&b, limit);
    Consumer c(&b, limit);
    Poller poller(&b);

    thread producer_thread(&Producer::run, &p);
    thread consumer_thread(&Consumer::run, &c);
    thread poller_thread(&Poller::run, &poller);

    producer_thread.join();
    consumer_thread.join();
    poller_thread.join();
   
   /*
   cout << b.top().first << endl;
   b.top().second();
   
   b.pop();
   
   cout << b.top().first << endl;
   b.top().second();   
    */
    
    return 0;
}
