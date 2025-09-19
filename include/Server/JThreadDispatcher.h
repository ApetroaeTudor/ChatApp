#pragma once

#include <iostream>

#include "resources.h"
#include "Server.h"
#include "Epoll.h"
#include "MessageFrame.h"
#include "Server_Message.h"
#include <utility>
#include <sys/eventfd.h>
#include <tuple>
#include <ctype.h>


//first int is nr of allocated clients
//second int is personal eventfd
using worker_arr_t = std::array<std::optional<std::tuple<int, int, std::jthread> >, (constants::MAX_NR_OF_THREADS)>;


inline std::pair<int, int> get_min_and_idx(
    const worker_arr_t &arr) {
    int min = std::numeric_limits<int>::max();
    int idx = 0;
    int min_idx = -1;
    for (const auto &elem: arr) {
        if (elem.has_value() && std::get<0>(elem.value()) < min) {
            min = std::get<0>(elem.value());
            min_idx = idx;
        }
        ++idx;
    }
    return {min, min_idx}; // returns min_idx == -1 on no values
}


template<concepts::NonBlockingAtomicStringQueue Queue_t>
class ServerManager {
private:
    std::atomic<int> nr_of_connected_clients{0};
    int max_clients_per_thread{1};

    int nr_of_launched_threads{0};


    Queue_t q_worker_to_listener;
    Queue_t q_listener_to_worker;

    // first int is number of clients on that thread
    worker_arr_t worker_threads;

    void assign_client_to_worker(int thread_index, int socket, int id, int event_fd) {
        Server_Message msg{
            std::to_string(thread_index),
            utils::get_action(constants::Actions::ACCEPT),
            std::to_string(socket),
            std::to_string(id)
        };
        if (!q_listener_to_worker.try_push(msg.get_concatenated_msg()) || event_fd < 0) {
            throw utils::QueueException("Failed to push new worker to queue");
        }

        uint64_t u = 1;
        if (write(event_fd, &u, sizeof(uint64_t)) != sizeof(uint64_t)) {
            throw std::runtime_error("Failed to send event_fd");
        }
    }

    void process_new_client(IServer<Queue_t> &isv, int socket, int id) {
        if (this->nr_of_launched_threads == 0) {
            int evfd = utils::get_evfd(EFD_NONBLOCK);
            worker_threads[0].emplace(std::tuple(0, evfd, std::jthread([this, &isv, socket,id,evfd]() {
                this->worker_loop(isv, socket, id, 0, evfd);
            })));
            ++this->nr_of_launched_threads;
            ++std::get<0>(*worker_threads[0]);
        } else {
            auto min_minidx = get_min_and_idx(this->worker_threads);
            if (min_minidx.second < 0) [[unlikely]]{
                int evfd = utils::get_evfd(EFD_NONBLOCK);
                worker_threads[0].emplace(std::tuple(0, evfd, std::jthread([this,&isv,socket,id,evfd]() {
                    this->worker_loop(isv, socket, id, 0, evfd);
                })));
                ++this->nr_of_launched_threads;
                ++std::get<0>(*worker_threads[0]);
            } else {
                if (min_minidx.first < this->max_clients_per_thread) {
                    ++std::get<0>(*worker_threads[min_minidx.second]);
                    assign_client_to_worker(min_minidx.second, socket, id,
                                            std::get<1>(*worker_threads[min_minidx.second]));
                } else {
                    // each thread is full, must increase max nr per thread
                    if (this->nr_of_launched_threads == constants::MAX_NR_OF_THREADS) {
                        // all threads launched, must increase the capacity for each one
                        ++this->max_clients_per_thread;
                        if (this->max_clients_per_thread > constants::MAX_CLIENTS_PER_THREAD) {
                            throw utils::ServerFullException("Server full, all threads are occupied!\n");
                        }
                        ++std::get<0>(*worker_threads[min_minidx.second]);
                        assign_client_to_worker(min_minidx.second, socket, id,
                                                std::get<1>(*worker_threads[min_minidx.second]));
                    } else {
                        // not all threads launched, can launch a new one without increasing capacity
                        if (this->nr_of_launched_threads >= (constants::MAX_NR_OF_THREADS)) {
                            throw utils::ServerFullException("Server full, can't launch more new workers!\n");
                        }
                        int nr_thr = this->nr_of_launched_threads;
                        int evfd = utils::get_evfd(EFD_NONBLOCK);
                        worker_threads[this->nr_of_launched_threads].emplace(std::tuple(
                            0, evfd, std::jthread([this,&isv,socket,id,nr_thr,evfd]() {
                                this->worker_loop(isv, socket, id, nr_thr, evfd);
                            })));
                        ++std::get<0>(*worker_threads[nr_thr]);
                        ++this->nr_of_launched_threads;
                    }
                }
            }
        }
    }

    // first int represents the action
    std::tuple<int, int, int> worker_check_listener_to_worker_q(EpollManager &em, int personal_index) {
        std::string q_recv_msg;
        std::string q_send_msg;
        Server_Message server_msg;

        if (this->q_listener_to_worker.try_pop(q_recv_msg)) {
            // std::cout<<"Thread nr: "<<personal_index<<" msg: "<<q_recv_msg<<std::endl;
            server_msg.construct_from_string(q_recv_msg);
            if (server_msg.thread_number == std::to_string(personal_index)) {
                if (server_msg.action == utils::get_action(constants::Actions::ACCEPT)) {
                    try {
                        int tmp_fd = std::stoi(server_msg.fd);
                        int tmp_id = std::stoi(server_msg.id);
                        em.add_monitored_fd(EpollPair{tmp_fd,EPOLLIN | EPOLLOUT});
                        return {static_cast<int>(constants::Actions::ACCEPT), tmp_fd, tmp_id};
                    } catch (...) {
                        utils::cerr_out_red("Bad string to int cast in worker loop - fd or id\n");
                    }
                }
            }
            else if (this->q_listener_to_worker.try_push(q_recv_msg)) {
                std::this_thread::sleep_for(utils::time_millis(10));
            }
            else {
                throw utils::QueueException("Failed to put back rejected message\n");
            }
        }
        return {-1, -1, -1};
    }


    void worker_loop(IServer<Queue_t> &isv, int cl_fd, int cl_id, int personal_index, int personal_eventfd) {
        std::cout << personal_eventfd << std::endl;
        EpollManager em;
        std::vector<std::pair<int, int> > clients = {{cl_fd, cl_id}};
        MessageFrame msg_frame;
        msg_frame.msg_len = constants::MAX_LEN;

        em.add_monitored_fd(EpollPair{cl_fd, EPOLLIN | EPOLLOUT});
        em.add_monitored_fd(EpollPair{personal_eventfd,EPOLLIN});


        int ret = -1;
        while (!clients.empty()) {
            ret = em.wait_events(-1);
            if (ret > 0) {
                auto client_to_remove = clients.end();
                std::tuple<int,int,int> listener_to_worker_q_tuple = {-1,-1,-1};

                for (const auto &client: clients) {
                    if (em.check_event(personal_eventfd,EPOLLIN, ret)) {
                        listener_to_worker_q_tuple = this->worker_check_listener_to_worker_q(em, personal_index);
                    }

                    if (em.check_event(client.first,EPOLLIN, ret)) {
                        if (isv.receive_msg(msg_frame, client.second) == 0) {
                            // client exit
                            isv.disconnect_client(client.second);
                            // send message to server Queue
                            client_to_remove = std::find_if(clients.begin(), clients.end(),
                                                            [&client](std::pair<int, int> a) {
                                                                return a.second == client.second;
                                                            });
                            std::cout << "Client disconnected from thread: " << personal_index << std::endl;
                            continue;
                        }
                        std::cout << "Received msg on thr " << personal_index << ":" << msg_frame.msg << std::endl;
                        msg_frame.clear_msg();
                        // if stage == 0 take init message -> stage = 1
                        // if stage == 2 take init fin message -> stage = 3
                        // if stage == 3 take message and put on worker_to_listener queue

                        // client has a valid msg in the buffer
                        // put msg in worker_to_listener
                        if (em.check_event(client.first,EPOLLOUT, ret)) {
                            // can send a message to client
                            // if stage == 1, send init msg -> stage = 2
                            // if stage == 3 : check listener_to_worker, send message to client if BROADCAST
                        }
                    }
                }
                if (client_to_remove != clients.end()) {
                    clients.erase(client_to_remove);
                }
                int action = std::get<0>(listener_to_worker_q_tuple);
                int new_cl_fd = std::get<1>(listener_to_worker_q_tuple);
                int new_cl_id = std::get<2>(listener_to_worker_q_tuple);

                switch (action) {
                    case static_cast<int>(constants::Actions::ACCEPT):
                        if (new_cl_fd<0 || new_cl_id<0) {
                            throw utils::ClientConnectionException("Invalid client socket or id\n");
                        }
                        clients.push_back({new_cl_fd,new_cl_id});
                        std::cout<<"Added new client\n";
                    break;
                    default:
                        break;
                }
            }
        }
    }


    void listener_loop(IServer<Queue_t> &isv) {
        try {
            // must check server queue to print messages to server stdout

            // must check worker_to_listener to check for disconnects and broadcasts
            // must write to listener_to_worker to send broadcasts and new clients
            EpollManager epoll_manager{};
            std::string buffer;
            int sv_socket = isv.get_sv_socket();
            epoll_manager.add_monitored_fd(EpollPair{STDIN_FILENO,EPOLLIN});
            epoll_manager.add_monitored_fd(EpollPair{sv_socket,EPOLLIN});
            while (!isv.check_is_done()) {
                int ret = epoll_manager.wait_events(0);
                if (epoll_manager.check_event(STDIN_FILENO,EPOLLIN, ret)) {
                    if (std::getline(std::cin, buffer)) {
                        if (buffer == "#quit") {
                            isv.set_is_done();
                        }
                    }
                }
                if (epoll_manager.check_event(sv_socket,EPOLLIN, ret)) {
                    try {
                        auto socket_id_pair = isv.accept_client(true);
                        process_new_client(isv, socket_id_pair.first, socket_id_pair.second);
                    } catch (const utils::ServerFullException &e) {
                        utils::cerr_out_red(e.what());
                    } catch (const std::exception &e) {
                        utils::cerr_out_red(e.what());
                        utils::cerr_out_red("Error is critical, closing..");
                        isv.set_is_done();
                    }
                }
                std::this_thread::sleep_for(utils::time_millis(50));
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }

public:
    ServerManager() = default;

    ServerManager &operator=(const ServerManager &other) = delete;

    ServerManager(const ServerManager &other) = delete;

    ServerManager &operator=(ServerManager &&other) noexcept = delete;

    ServerManager(ServerManager &&other) noexcept = delete;

    Queue_t received_msg_queue;

    std::jthread launch_listener_loop(IServer<Queue_t> &isv) {
        return std::jthread([&]() { listener_loop(isv); });
    }
};
