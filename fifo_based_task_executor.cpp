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

class Buffer {
private:
  deque<function<void()>> buffer_;
  int size_;
  
public:
    Buffer(int buffer_limit) {
        size_ = buffer_limit;
    };
    
    void add(function<void()> func) {
        unique_lock<std::mutex> locker(buffer_mutex);
        cond.wait(locker, [this](){return buffer_.size() < size_;});
        
        // critical section
        buffer_.push_front(func);
        
        buffer_mutex.unlock();
        cond.notify_all();
    }
    function<void()> remove() {

        unique_lock<std::mutex> locker(buffer_mutex);
        cond.wait(locker, [this](){return buffer_.size() > 0;});
        
        // critical section
        auto back = buffer_.back();
        buffer_.pop_back(); 
        
        locker.unlock();
        cond.notify_all();
        return back;
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
            function<void()> print_produced = [=]() {
                lock_guard<mutex> lock(cout_mutex);
                cout << "consumed: " << produced << endl;  
            };
            buffer_->add(print_produced);
            // scoped locking. unlocks on scope end
            ++produced;
            if (produced == produce_limit) {
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
              return;
            }
        }
    }
private:
    Buffer *buffer_;
    int consumed;
    int consume_limit;
};

int main() {

    Buffer b(5);
    int limit = 10;
    Producer p(&b, limit);
    Consumer c(&b, limit);

    thread producer_thread(&Producer::run, &p);
    thread consumer_thread(&Consumer::run, &c);

    producer_thread.join();
    consumer_thread.join();
    return 0;
}