#include <bits/stdc++.h>
#include <thread>
#include <deque>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <unistd.h>


using namespace std;
using std::deque;

mutex buffer_mutex, cout_mutex;
condition_variable cond;

class Buffer {
private:
  deque<int> buffer_;
    
public:
    Buffer() {};
    void add(int num) {
            buffer_mutex.lock();
            buffer_.push_front(num);
            buffer_mutex.unlock();
    }
    int remove() {
            int back = -1;
            buffer_mutex.lock();
            if (buffer_.size() > 0) {
              back = buffer_.back();
              buffer_.pop_back(); 
            }
            buffer_mutex.unlock();
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
            buffer_->add(produced);
            // scoped locking. unlocks on scope end
            lock_guard<mutex> lock(cout_mutex);
            cout << "Produced: " << produced << endl;
            ++produced;
            if (produced == produce_limit) {
              cout << "Produced total " << produce_limit << ", exiting producer\n";
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
            int num = buffer_->remove();
            if (num != -1) ++consumed;
            lock_guard<mutex> lock(cout_mutex);
            cout << "Consumed: " << num << endl;
            if (consumed == consume_limit) {
              cout << "Consumed total " << consume_limit << ", Exiting consumer\n";
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

    Buffer b;
    int limit = 10;
    Producer p(&b, limit);
    Consumer c(&b, limit);

    thread producer_thread(&Producer::run, &p);
    thread consumer_thread(&Consumer::run, &c);

    producer_thread.join();
    consumer_thread.join();
    return 0;
}