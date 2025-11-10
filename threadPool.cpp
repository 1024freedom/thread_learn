#include<iostream>
#include<thread>
#include<vector>
#include<queue>
#include<functional>
#include<mutex>
#include<condition_variable>
class ThreadPool {//wcnm
public:
    ThreadPool(int numThreads):stop(false){
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
    ~ThreadPool(){
        std::unique_lock<std::mutex> lock(mtx);
        stop=true;
        condition.notify_all();
        for(auto& t:threads){
            t.join();
        }
    }
    template<class F,class... Args>
    void enqueue(F&& f,Args&&... args){//&&万能引用，args是f的参数
        std::function<void()>task=
        std::bind(std::forward<F>(f),std::forward<Args>(args)...);
        std::unique_lock<std::mutex> lock(mtx);
        tasks.emplace(std::move(task));
        condition.notify_one();
    }
private:
    std::vector<std::thread> threads;//线程数组
    std::queue<std::function<void()>> tasks;//任务队列
    std::mutex mtx;
    std::condition_variable condition;
    bool stop;
};
int main(){
    ThreadPool pool(4);
    for(int i=0;i<10;i++){
        pool.enqueue([i]{
            std::cout<<"task:"<<i<<"is running"<<std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout<<"task:"<<i<<"is done"<<std::endl;
        });
    }
    return 0;
}