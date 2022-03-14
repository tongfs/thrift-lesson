// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "match_server/Match.h"
#include "save_client/Save.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <unistd.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace  ::match_service;
using namespace  ::save_service;

using namespace std;

struct Task
{
    User user;
    string type;
};

struct MessageQueue
{
    queue<Task> q;
    mutex m;
    condition_variable cv;
}message_queue;

class Pool
{
    public:
        void save_result(int a, int b)
        {
            printf("match result: %d %d\n", a, b);

            std::shared_ptr<TTransport> socket(new TSocket("123.57.47.211", 9090));
            std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            SaveClient client(protocol);

            try {
                transport->open();

                int res = client.save_data("acs_2004", "0cd537e4", a, b);

                if (!res) cout << "success" << endl;
                else cout << "failed" << endl;

                transport->close();
            } catch (TException& tx) {
                cout << "ERROR: " << tx.what() << endl;
            }
        }

        void match()
        {
            while (users.size() > 1)
            {
                sort(users.begin(), users.end(), [&](User &a, User &b){
                        return a.score < b.score;
                        });

                bool flag = true;
                for (uint32_t i = 1; i < users.size(); i ++ )
                {
                    auto a = users[i - 1], b = users[i];
                    if (b.score - a.score <= 50)
                    {
                        users.erase(users.begin() + i - 1, users.begin() + i + 1);
                        save_result(a.id, b.id);    // 保存匹配结果

                        flag = false;
                        break;
                    }
                }

                if (flag) break;
            }
        }

        void add(User user)
        {
            users.push_back(user);
        }

        void remove(User user)
        {
            for (uint32_t i = 0; i < users.size(); i ++ )
            {
                if (users[i].id == user.id)
                {
                    users.erase(users.begin() + i);
                    break;
                }
            }
        }

    private:
        vector<User> users;
}pool;

class MatchHandler : virtual public MatchIf {
    public:
        MatchHandler() {
            // Your initialization goes here
        }

        /**
         * user: 添加的用户信息
         * info: 附加信息
         * 在匹配池中添加一个名用户
         * 
         * @param user
         * @param info
         */
        int32_t add_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("add_user\n");

            unique_lock<mutex> lck(message_queue.m);    // 变量销毁时，会自动释放锁
            message_queue.q.push({user, "add"});
            message_queue.cv.notify_all();  // 唤醒所有等待的线程

            return 0;
        }

        /**
         * user: 删除的用户信息
         * info: 附加信息
         * 从匹配池中删除一名用户
         * 
         * @param user
         * @param info
         */
        int32_t remove_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("remove_user\n");

            unique_lock<mutex> lck(message_queue.m);
            message_queue.q.push({user, "remove"});
            message_queue.cv.notify_all();

            return 0;
        }

};


void consume_task()
{
    while (true)
    {
        unique_lock<mutex> lck(message_queue.m);
        if (message_queue.q.empty())
        {
            // 如果队列为空，则解锁，并且睡眠1秒
            lck.unlock();
            sleep(1);
        }
        else
        {
            auto task = message_queue.q.front();
            message_queue.q.pop();
            lck.unlock();   // 及时解锁，不要等到task完成后再解锁

            // do task
            if (task.type == "add") pool.add(task.user);
            else if (task.type == "remove") pool.remove(task.user);

            pool.match();
        }
    }
}


int main(int argc, char **argv) {
    int port = 9090;
    ::std::shared_ptr<MatchHandler> handler(new MatchHandler());
    ::std::shared_ptr<TProcessor> processor(new MatchProcessor(handler));
    ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);

    cout << "match_server starts!" << endl;

    thread match_thread(consume_task);

    server.serve();
    return 0;
}

