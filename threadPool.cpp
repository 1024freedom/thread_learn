#include<iostream>
#include<thread>
#include<vector>
#include<queue>
#include<functional>
#include<mutex>
#include<condition_variable>

// static std::once_flag initFlag;

class ThreadPool {
    //禁用拷贝构造和赋值运算符
    ThreadPool& operator=(const ThreadPool& threadPool)=delete;
    ThreadPool(const ThreadPool& threadPool)=delete;
    ThreadPool():stop(false){
        for(int i=0;i<numThreads;i++){
            threads.emplace_back([this]{
                //该方法相比于pushback比较节省资源（它可以直接在容器内存中构造对象，避免了额外的临时对象创建和拷贝/移动操作。）
                while(1){
                    std::unique_lock<std::mutex> lock(mtx);
                    condition.wait(lock,[this]{
                        return !tasks.empty()||stop;
                    });

                    if(stop&&tasks.empty()){
                        return;
                    }

                    std::function<void()>task(std::move(tasks.front()));//移动语义
                    tasks.pop();
                    lock.unlock();
                    task();

                }
            });
        }
    }
public:

    ~ThreadPool(){
        std::unique_lock<std::mutex> lock(mtx);
        stop=true;
        condition.notify_all();
        lock.unlock();
        for(auto& t:threads){
            t.join();
        }
    }
    
    static ThreadPool& getInstance(){
        static ThreadPool instance;
        return instance;
    }
    template<class F,class... Args>
    void enqueue(F&& f,Args&&... args){//&&万能引用，args是f的参数(可变参数模板)
        std::function<void()>task=
        std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        std::unique_lock<std::mutex> lock(mtx);
        tasks.emplace(std::move(task));
        condition.notify_one();
    }
private:
    int numThreads=10;
    std::vector<std::thread> threads;//线程数组
    std::queue<std::function<void()>> tasks;//任务队列
    std::mutex mtx;
    std::condition_variable condition;
    bool stop;
};
int main(){
    ThreadPool& pool=ThreadPool::getInstance();
    for(int i=0;i<10;i++){
        pool.enqueue([i]{
            std::cout<<"task:"<<i<<"is running"<<std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout<<"task:"<<i<<"is done"<<std::endl;
        });
    }
    return 0;
}