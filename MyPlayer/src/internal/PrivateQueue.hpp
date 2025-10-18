#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template<typename T>
class PrivateQueue
{
public:
	PrivateQueue() = default;
	~PrivateQueue();

	PrivateQueue(const PrivateQueue&) = delete;//��ֹ����
	PrivateQueue& operator = (const PrivateQueue&) = delete;//��ֹ��ֵ

	void push(T value);
	bool pop(T& value);
	void shutdown();
	bool empty();
    int  size();
private:
	std::mutex m_mutex;
	std::queue<T>m_queue;
	std::condition_variable m_cond;
	bool m_isShuntdown = false;
};

template<typename T>
PrivateQueue<T>::~PrivateQueue(){
    shutdown();
}

template<typename T>
void PrivateQueue<T>::push(T data){
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_isShuntdown) return;
    m_queue.push(std::move(data));
    m_cond.notify_one();
}

template<typename T>
bool PrivateQueue<T>::pop(T& value){
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cond.wait(lock,[this] 
            {return !m_queue.empty()||m_isShuntdown;});
    if(m_isShuntdown && m_queue.empty())
        return false;
    value =std::move(m_queue.front());
    m_queue.pop();
    return true; 
}

template<typename T>
void PrivateQueue<T>::shutdown(){
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isShuntdown = true;
    }
    m_cond.notify_all();
}

template<typename T>
bool PrivateQueue<T>::empty(){
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();

}

template<typename T>
int PrivateQueue<T>::size() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}
