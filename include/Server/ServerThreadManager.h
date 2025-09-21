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
#include <cctype>
#include <mutex>


//first int is nr of allocated clients
//second int is personal eventfd
using worker_arr_t = std::array<std::optional<std::tuple<int, int, std::jthread> >, (constants::MAX_NR_OF_THREADS)>;


inline bool is_worker_arr_empty(const worker_arr_t &arr) {
    return std::find_if(arr.begin(), arr.end(), [&](const auto &elem) { return elem.has_value(); }) == arr.end();
}

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

inline std::pair<int, int> get_max_and_idx(
    const worker_arr_t &arr) {
    int max = 0;
    int idx = 0;
    int max_idx = -1;
    for (const auto &elem: arr) {
        if (elem.has_value() && std::get<0>(elem.value()) > max) {
            max = std::get<0>(elem.value());
            max_idx = idx;
        }
        ++idx;
    }
    return {max, max_idx}; // returns min_idx == -1 on no values
}


template<concepts::NonBlockingAtomicStringQueue Queue_t>
class ServerThreadManager {
private:
    int max_clients_per_thread{1};

    int nr_of_launched_threads{0};


    Queue_t q_worker_to_listener;
    Queue_t q_listener_to_worker;

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

        const uint64_t u = 1;
        if (write(event_fd, &u, sizeof(uint64_t)) != sizeof(uint64_t)) {
            throw std::runtime_error("Failed to send event_fd");
        }
    }

    void emplace_new_worker(IServer<Queue_t> &isv, int socket, int id, int nr_thr, int listener_evfd) {
        int evfd = utils::get_evfd(EFD_NONBLOCK);
        worker_threads[this->nr_of_launched_threads].emplace(std::tuple(
            0, evfd, std::jthread([this,&isv,socket,id,nr_thr,evfd,listener_evfd]() {
                this->worker_loop(isv, socket, id, nr_thr, evfd, listener_evfd);
            })));
        ++std::get<0>(*worker_threads[nr_thr]);
        ++this->nr_of_launched_threads;
    }


    void process_new_client(IServer<Queue_t> &isv, int socket, int id, int listener_evfd) {
        if (this->nr_of_launched_threads == 0) {
            emplace_new_worker(isv, socket, id, 0, listener_evfd);
            return;
        }
        auto [min, min_idx] = get_min_and_idx(worker_threads);
        if (min_idx < 0) [[unlikely]]{
            emplace_new_worker(isv, socket, id, 0, listener_evfd);
            return;
        }
        if (min < this->max_clients_per_thread) {
            ++std::get<0>(*worker_threads[min_idx]);
            assign_client_to_worker(min_idx, socket, id,
                                    std::get<1>(*worker_threads[min_idx]));
            return;
        }
        // each thread is full, must increase max nr per thread
        if (this->nr_of_launched_threads == constants::MAX_NR_OF_THREADS) {
            // all threads launched, must increase the capacity for each one
            ++this->max_clients_per_thread;
            if (this->max_clients_per_thread > constants::MAX_CLIENTS_PER_THREAD) {
                throw utils::ServerFullException("Server full, all threads are occupied!\n");
            }
            ++std::get<0>(*worker_threads[min_idx]);
            assign_client_to_worker(min_idx, socket, id,
                                    std::get<1>(*worker_threads[min_idx]));
            return;
        }
        // not all threads launched, can launch a new one without increasing capacity
        if (this->nr_of_launched_threads >= (constants::MAX_NR_OF_THREADS)) {
            throw utils::ServerFullException("Server full, can't launch more new workers!\n");
        }
        emplace_new_worker(isv, socket, id, this->nr_of_launched_threads, listener_evfd);
    }

    void worker_push(Server_Message &server_msg, int listener_evfd) {
        if (listener_evfd < 0) {
            throw std::runtime_error("Server listener event fd is invalid!\n");
        }

        if (!this->q_worker_to_listener.try_push(server_msg.get_concatenated_msg())) {
            throw utils::QueueException("Failed to push disconnect client msg to Queue!\n");
        }
        const uint64_t u = 1;
        if (write(listener_evfd, &u, sizeof(uint64_t)) != sizeof(uint64_t)) {
            throw std::runtime_error("Failed to send to listener eventfd\n");
        }
    }


    void worker_client_disconnect(int thr_personal_idx) {
        if (!this->worker_threads[thr_personal_idx].has_value()) {
            throw utils::ThreadManagingException("Can't disconnect from a thread that isn't running\n");
        }
        auto [current_connections, current_evfd] = std::pair<int, int>{
            std::get<0>(*this->worker_threads[thr_personal_idx]), std::get<1>(*this->worker_threads[thr_personal_idx])
        };

        if (current_connections <= 0) {
            throw utils::ThreadManagingException("Can't disconnect from a thread that has no connections\n");
        }
        if (current_connections == 1) {
            this->worker_threads[thr_personal_idx].reset();
            --this->nr_of_launched_threads;
        } else {
            --std::get<0>(*this->worker_threads[thr_personal_idx]);
        }

        auto [max,max_idx] = get_max_and_idx(this->worker_threads);
        this->max_clients_per_thread = max;
        std::cout << "New max clients per thr: " << this->max_clients_per_thread << std::endl;
    }

    void unicast_repeated(IServer<Queue_t> &isv, int excepted_id, const std::vector<int> &populatedIDs,
                          std::string msg_to_push) {
        for (const auto &id: populatedIDs) {
            if (excepted_id == id) {
                continue;
            }
            if (!isv.get_client_ref(id).has_value()) {
                continue;
            }
            if (!isv.get_client_ref(id)->get().send_queue.try_push(msg_to_push)) {
                throw utils::QueueException("Failed to push message to corresponding client\n");
            }
        }
    }


    void listener_process_worker_to_listener_q(IServer<Queue_t> &isv, int listener_evfd) {
        uint64_t u;
        ssize_t bytes_read = read(listener_evfd, &u, sizeof(uint64_t));
        if (bytes_read != sizeof(uint64_t)) {
            if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // This was a spurious wake-up, not a critical error.
                return;
            }
            throw std::runtime_error("Failed read on listener evfd\n");
        }

        std::string recv_message;
        if (!this->q_worker_to_listener.try_pop(recv_message)) {
            return;
        }
        Server_Message msg;
        msg.construct_from_string(recv_message);
        auto populatedIDs = isv.get_connected_IDs();
        if (populatedIDs.empty()) {
            return;
        }


        if (msg.action == utils::get_action(constants::Actions::CL_DISCONNECTED)) {
            std::cout << "Client disconnected\n";
            isv.disconnect_client(std::stoi(msg.id));
            if (!this->worker_threads[std::stoi(msg.thread_number)].has_value()) {
                throw utils::ThreadManagingException("Can't disconnect a client from an empty optional\n");
            }
            if (std::get<0>(*this->worker_threads[std::stoi(msg.thread_number)]) == 0) {
                throw utils::ThreadManagingException(
                    "Can't disconnect a client from a thread with 0 clients\n");
            }
            msg.action = utils::get_action(constants::Actions::UNICAST_REPEATED);
            msg.msg_content = msg.name + " Has just left!";
            msg.name = constants::SERVER_NAME;
            unicast_repeated(isv,std::stoi(msg.id),populatedIDs,msg.get_concatenated_msg());
            worker_client_disconnect(std::stoi(msg.thread_number));

        } else if (msg.action == utils::get_action(constants::Actions::UNICAST_REPEATED)) {
            int msg_id = std::stoi(msg.id);
            unicast_repeated(isv, msg_id, populatedIDs, recv_message);
        } else if (msg.action == utils::get_action(constants::Actions::SV_DISCONNECTED)) {
            unicast_repeated(isv, -1, populatedIDs, recv_message);
        } else if (msg.action == utils::get_action(constants::Actions::INIT_DONE)) {
            msg.action = utils::get_action(constants::Actions::UNICAST_REPEATED);
            unicast_repeated(isv,std::stoi(msg.id),populatedIDs, msg.get_concatenated_msg());
        }
    }


    // first int represents the action
    std::tuple<int, int, int> worker_process_listener_to_worker_q(EpollManager &em, int personal_index,
                                                                  int personal_evfd) {
        std::string q_recv_msg;
        std::string q_send_msg;
        Server_Message server_msg;

        uint64_t u;
        ssize_t bytes_read = read(personal_evfd, &u, sizeof(uint64_t));
        if (bytes_read != sizeof(uint64_t)) {
            if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // This was a spurious wake-up, not a critical error.
                return {-1, -1, -1};
            }
            throw std::runtime_error("Failed read on personal thread evfd\n");
        }

        if (!this->q_listener_to_worker.try_pop(q_recv_msg)) {
            return {-1, -1, -1};
        }
        server_msg.construct_from_string(q_recv_msg);

        if (server_msg.action == utils::get_action(constants::Actions::ACCEPT)) {
            if (server_msg.thread_number != std::to_string(personal_index)) {
                return {-1, -1, -1};
            }
            int tmp_fd = std::stoi(server_msg.fd);
            int tmp_id = std::stoi(server_msg.id);
            em.add_monitored_fd(EpollPair{tmp_fd,EPOLLIN | EPOLLOUT});
            return {static_cast<int>(constants::Actions::ACCEPT), tmp_fd, tmp_id};
        }

        if (!this->q_listener_to_worker.try_push(q_recv_msg)) {
            throw utils::QueueException("Failed to put back rejected message\n");
        }
        return {-1, -1, -1};
    }

    void process_client_state(bool input, Server_Message &server_msg, IServer<Queue_t> &isv,
                              std::optional<std::reference_wrapper<Client_Data<Queue_t> > > &current_client_ref,
                              int listener_evfd) {
        auto &cl = current_client_ref->get();
        if (input) {
            if (server_msg.action == utils::get_action(constants::Actions::CL_DISCONNECTED)) {
                cl.set_state(constants::DISCONNECT_STATE);
            }
            switch (cl.get_state()) {
                case constants::INITIAL_STATE:
                    if (server_msg.action == utils::get_action(constants::Actions::INIT_REQ)) {
                        cl.inc_state();
                        cl.name = server_msg.name;
                    }
                    server_msg.reset_msg();
                    return;
                case constants::SENT_INIT_CONFIRMATION:
                    if (server_msg.action == utils::get_action(constants::Actions::INIT_DONE)) {
                        cl.inc_state();

                        server_msg.name = current_client_ref->get().name;

                        server_msg.action = utils::get_action(constants::Actions::UNICAST_REPEATED);
                        server_msg.name = constants::SERVER_NAME;
                        server_msg.id = std::to_string(cl.personal_id);
                        server_msg.color_id = std::to_string(cl.color_id);
                        server_msg.msg_content = std::string("Hello ") + cl.name + std::string(", now you can chat!");
                        std::cout << "Client ID:" << cl.personal_id << " Name: " << cl.name << " has connected!" <<
                                std::endl;
                        isv.send_msg(std::move(server_msg.get_concatenated_msg()), cl.personal_id);


                        server_msg.action = utils::get_action(constants::Actions::INIT_DONE);
                        server_msg.name = constants::SERVER_NAME;
                        server_msg.id = std::to_string(cl.personal_id);
                        server_msg.msg_content = cl.name + " Has Just Connected!";
                        worker_push(server_msg,listener_evfd);

                        server_msg.reset_msg();
                    }
                    return;
                case constants::INIT_DONE:
                    if (server_msg.action == utils::get_action(constants::Actions::UNICAST_REPEATED)) {
                        std::cout<<"RECEIVED MSG ON SV!"<<std::endl;
                        worker_push(server_msg, listener_evfd);
                        server_msg.reset_msg();
                    }
                    return;
                default:
                    return;
            }
        }

        switch (cl.get_state()) {
            case constants::REQUESTED_INIT:
                cl.inc_state();
                server_msg.action = utils::get_action(constants::Actions::INIT_REQ);
                server_msg.name = cl.name;
                server_msg.color_id = std::to_string(cl.color_id);
                server_msg.id = std::to_string(cl.personal_id);
                isv.send_msg(std::move(server_msg.get_concatenated_msg()), cl.personal_id);
                return;
            case constants::INIT_DONE:
                std::string send_msg;
                while (cl.send_queue.try_pop(send_msg)) {
                    // if (send_msg.empty()) {
                    //     return;
                    // }
                    isv.send_msg(std::move(send_msg), cl.personal_id);
                }
        }
    }


    void worker_loop(IServer<Queue_t> &isv, int cl_fd, int cl_id, int personal_index, int personal_evfd,
                     int listener_evfd) {
        EpollManager em;
        std::vector<std::pair<int, int> > clients = {{cl_fd, cl_id}};
        MessageFrame msg_frame;
        msg_frame.msg_len = constants::MAX_LEN;
        Server_Message server_msg;

        em.add_monitored_fd(EpollPair{cl_fd, EPOLLIN | EPOLLOUT});
        em.add_monitored_fd(EpollPair{personal_evfd,EPOLLIN});


        int ret = -1;
        while (!clients.empty() && !isv.check_is_done()) {
            ret = em.wait_events(-1);
            if (ret <= 0) {
                continue;
            }
            std::tuple listener_to_worker_q_tuple = {-1, -1, -1};

            if (em.check_event(personal_evfd,EPOLLIN, ret)) {
                listener_to_worker_q_tuple = this->worker_process_listener_to_worker_q(
                    em, personal_index, personal_evfd);
            }

            for (const auto &client: clients) {
                auto current_client_ref = isv.get_client_ref(client.second);

                if (em.check_event(client.first,EPOLLIN, ret)) {
                    if (isv.receive_msg(msg_frame, client.second) == 0) {
                        // client exit
                        isv.get_client_ref(client.second)->get().set_state(constants::DISCONNECT_STATE);
                        continue;
                    }
                    if (!current_client_ref.has_value()) {
                        continue;
                    }
                    server_msg.construct_from_string(std::string(msg_frame.msg));
                    msg_frame.clear_msg();
                    process_client_state(true, server_msg, isv, current_client_ref, listener_evfd);


                    // if stage == 0 take init message -> stage = 1
                    // if stage == 2 take init fin message -> stage = 3
                    // if stage == 3 take message and put on worker_to_listener queue

                    // client has a valid msg in the buffer
                    // put msg in worker_to_listener
                }
                if (em.check_event(client.first,EPOLLOUT, ret)) {
                    process_client_state(false, server_msg, isv, current_client_ref, listener_evfd);
                    // can send a message to client
                    // if stage == 1, send init msg -> stage = 2
                    // if stage == 3 : check listener_to_worker, send message to client if BROADCAST
                }
            }
            // must send cl_disconnected msg to q worker to listener with the client ID to disconnect, and thread ID
            clients.erase(
                std::remove_if(
                    clients.begin(),
                    clients.end(),
                    [&](const auto &client) {
                        bool disconnected = isv.get_client_ref(client.second)->get().get_state() ==
                                            constants::DISCONNECT_STATE;
                        if (disconnected) {
                            server_msg.action = utils::get_action(constants::Actions::CL_DISCONNECTED);
                            server_msg.name = isv.get_client_ref(client.second)->get().name;
                            server_msg.thread_number = std::to_string(personal_index);
                            server_msg.id = std::to_string(client.second);
                            server_msg.color_id = std::to_string(isv.get_client_ref(client.second)->get().color_id);
                            worker_push(server_msg, listener_evfd);
                            std::cout << "Client ID:" << client.second << " Signaled for disconnect!" << std::endl;
                        }
                        return disconnected;
                    }),
                clients.end());

            int action = std::get<0>(listener_to_worker_q_tuple);
            int new_cl_fd = std::get<1>(listener_to_worker_q_tuple);
            int new_cl_id = std::get<2>(listener_to_worker_q_tuple);

            switch (action) {
                case static_cast<int>(constants::Actions::ACCEPT):
                    if (new_cl_fd < 0 || new_cl_id < 0) {
                        throw utils::ClientConnectionException("Invalid client socket or id\n");
                    }
                    clients.emplace_back(new_cl_fd, new_cl_id);

                    break;
                default:
                    break;
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
            int evfd = utils::get_evfd(EFD_NONBLOCK);

            epoll_manager.add_monitored_fd(EpollPair{STDIN_FILENO,EPOLLIN});
            epoll_manager.add_monitored_fd(EpollPair{sv_socket,EPOLLIN});
            epoll_manager.add_monitored_fd(EpollPair{evfd,EPOLLIN});
            while (!isv.check_is_done()) {
                int ret = epoll_manager.wait_events(-1);
                if (ret <= 0) {
                    continue;
                }
                if (epoll_manager.check_event(evfd,EPOLLIN, ret)) {
                    listener_process_worker_to_listener_q(isv, evfd);
                }
                if (epoll_manager.check_event(STDIN_FILENO,EPOLLIN, ret)) {
                    if (std::getline(std::cin, buffer)) {
                        if (buffer == constants::QUIT_MSG) {
                            isv.set_is_done();
                            Server_Message msg;
                            msg.action = utils::get_action(constants::Actions::SV_DISCONNECTED);
                            msg.msg_content = "SERVER DISCONNECTED";
                            this->q_worker_to_listener.try_push(msg.get_concatenated_msg());
                        }
                    }
                }
                if (epoll_manager.check_event(sv_socket,EPOLLIN, ret)) {
                    try {
                        auto socket_id_pair = isv.accept_client(true);
                        process_new_client(isv, socket_id_pair.first, socket_id_pair.second, evfd);
                    } catch (const utils::ServerFullException &e) {
                        utils::clog_out_notif(e.what());
                    } catch (const utils::ThreadManagingException &e) {
                        utils::cerr_out_warning(e.what());
                    } catch (const std::exception &e) {
                        utils::cerr_out_err(e.what());
                        utils::cerr_out_err("Error is critical, closing..");
                        isv.set_is_done();
                    }
                }
            }
        } catch (const utils::QueueException &e) {
            utils::cerr_out_warning(e.what());
        } catch (const std::exception &e) {
            utils::cerr_out_err(e.what());
        }
    }

public:
    ServerThreadManager() = default;

    ServerThreadManager &operator=(const ServerThreadManager &other) = delete;

    ServerThreadManager(const ServerThreadManager &other) = delete;

    ServerThreadManager &operator=(ServerThreadManager &&other) noexcept = delete;

    ServerThreadManager(ServerThreadManager &&other) noexcept = delete;

    Queue_t received_msg_queue;

    std::jthread launch_listener_loop(IServer<Queue_t> &isv) {
        return std::jthread([&]() { listener_loop(isv); });
    }
};
