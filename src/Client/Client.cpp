#include "Client.h"

#include <iostream>

#include "Server_Message.h"


Client::Client(int domain, int type, int protocol, std::string_view sv_addr, int port) {
    std::cout << colors::COLOR_ARR[static_cast<size_t>(colors::Colors::BRIGHT_MAGENTA)] << "Please input your name: " <<
            colors::COLOR_ARR[static_cast<size_t>(colors::Colors::COL_END)];

    std::cout << utils::get_color(this->color_id) << std::endl;
    std::string buffer;
    std::cin >> buffer;

    this->cl_event_fd = utils::get_evfd(EFD_NONBLOCK);

    this->set_name(buffer.data());

    this->cl_socket = socket(domain, type, protocol);
    if (cl_socket < 0) {
        throw std::runtime_error("Failed to open socket - Client\n");
    }
    if (inet_pton(domain, sv_addr.data(), &this->s_addr.sin_addr) <= 0) {
        throw std::runtime_error("Failed to load network addr in struct\n");
    }
    this->s_addr.sin_family = domain;
    this->s_addr.sin_port = htons(port);

    if (connect(this->cl_socket, reinterpret_cast<sockaddr *>(&this->s_addr), sizeof(this->s_addr)) < 0) {
        throw std::runtime_error("Failed connect\n");
    }
}

Client::~Client() {
    memset(&this->s_addr, 0, sizeof(this->s_addr));
    close(this->cl_socket);
    this->cl_socket = 0;
    std::cout << C_E << std::endl;
}

void Client::send_msg(std::string &&msg) const {
    auto msg_to_send = std::move(msg);
    if (this->cl_socket >= 0) {
        if (send(cl_socket, msg_to_send.data(), msg_to_send.size(), 0) < 0) {
            throw std::runtime_error("Failed to send msg\n");
        }
    } else {
        throw std::runtime_error("The socket is not initialized\n");
    }
}


int Client::receive_msg(MessageFrame &message_frame) const {
    if (this->cl_socket < 0) {
        throw std::runtime_error("The socket is not initialized\n");
    }
    int ret = recv(this->cl_socket, &message_frame.msg, message_frame.msg_len, 0);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        throw std::runtime_error("Failed read\n");
    }
    return ret;
}

void Client::make_cl_socket_nonblocking() {
    if (cl_socket >= 0) {
        int flags = fcntl(this->cl_socket, F_GETFL, 0);
        if ((flags & O_NONBLOCK) != O_NONBLOCK) {
            fcntl(this->cl_socket, F_SETFL, flags | O_NONBLOCK);
        }
    }
}

int Client::get_cl_socket() const {
    return this->cl_socket;
}


//////////////////////////////////////////////////////////////////////////////// - helper functions

void Client::send_msg_from_stdin(std::string &buffer, Server_Message &msg) {
    try {
        if (std::getline(std::cin, buffer)) {
            if (buffer == constants::QUIT_MSG) {
                client_done_source.request_stop();
                msg.action=utils::get_action(constants::Actions::CL_DISCONNECTED);
                msg.name = this->name;
                msg.id = this->id;
                this->send_msg(msg.get_concatenated_msg());
            } else if (!buffer.empty()) {
                msg.action = utils::get_action(constants::Actions::UNICAST_REPEATED);
                msg.name = this->name;
                msg.id = this->id;
                msg.color_id = std::to_string(this->color_id);
                msg.msg_content = buffer;

                this->send_msg(msg.get_concatenated_msg());
            }
        }
        msg.reset_msg();

    } catch (const std::exception &e) {
        utils::cerr_out_err(e.what());
    }
}

void Client::update_color(int color_id) {
    std::lock_guard lk(this->data_mutex);

    std::cout<<C_E<<std::endl;
    this->color_id = color_id;
    std::cout<<utils::get_color(this->color_id)<<std::endl;
}

void Client::finalize_init(Server_Message& server_msg) {
    if (server_msg.action == utils::get_action(constants::Actions::INIT_REQ)) {
        this->update_color(std::stoi(server_msg.color_id));
        this->id = server_msg.id;
        this->init_done.test_and_set(std::memory_order_release);
        server_msg.reset_msg();
        server_msg.action = utils::get_action(constants::Actions::INIT_DONE);
        this->send_msg(std::move(server_msg.get_concatenated_msg()));
    }
    server_msg.reset_msg();
}

void Client::receive_msg_from_sv(MessageFrame &msg_frame) {
    try {
        int ret = this->receive_msg(msg_frame);
        if (ret == 0) {
            client_done_source.request_stop();
            uint64_t u = 1;
            if (write(this->cl_event_fd, &u,sizeof(uint64_t))!=sizeof(uint64_t)) {
                throw std::runtime_error("Failed to write to event fd to wake up send loop\n");
            }
            return;
        }
        if (!this->init_req.test(std::memory_order_acquire)) {
            return;
        }
        Server_Message server_msg;
        server_msg.construct_from_string(msg_frame.msg);
        if (!this->init_done.test(std::memory_order_acquire)) {
            finalize_init(server_msg);
            msg_frame.clear_msg();
            return;
        }


        utils::stdout_formatted_msg(server_msg,this->color_id);


        msg_frame.clear_msg();
        server_msg.reset_msg();
    } catch (const std::exception &e) {
        utils::cerr_out_err(e.what());
    }
}

bool Client::check_init() {
    return this->is_initialized_flag.test(std::memory_order_acquire);
}

void Client::set_init() {
    this->is_initialized_flag.test_and_set(std::memory_order_release);
}

void Client::set_name(const char *name) {
    std::lock_guard lk(this->data_mutex);
    this->name = name;
}

std::string Client::get_name() const {
    if (this->name.empty()) {
        return constants::NAME_UNINITIALIZED;
    }
    return this->name;
}

void Client::set_id(const char *id) {
    std::lock_guard lk(this->data_mutex);

    this->id = id;
}

std::string Client::get_id() const {
    if (this->id.empty()) {
        return constants::ID_UNINITIALIZED;
    }
    return this->id;
}

bool Client::is_initialized() const {
    return !this->id.empty() && !this->name.empty();
}


////////////////////////////////////////////////////////////////////////// - main loop functions

void Client::start_send_loop() {
    EpollManager em;
    Server_Message msg;
    em.add_monitored_fd(EpollPair{STDIN_FILENO, EPOLLIN});
    em.add_monitored_fd(EpollPair{this->cl_event_fd,EPOLLIN});
    int events_nr;
    std::string buffer;


    while (!client_done_source.get_token().stop_requested()) {
        if (!this->init_req.test(std::memory_order_acquire)) {
            if (!this->name.empty()) {
                msg.action = utils::get_action(constants::Actions::INIT_REQ);
                msg.name = this->name;
                msg.color_id = std::to_string(this->color_id);
                this->send_msg(std::move(msg.get_concatenated_msg()));
                this->init_req.test_and_set(std::memory_order_release);
            }
            continue;
        }
        events_nr = em.wait_events(-1);

        if (em.check_event(STDIN_FILENO, EPOLLIN, events_nr)) {
            send_msg_from_stdin(buffer, msg);
            msg.reset_msg();
        }
    }
}

void Client::start_receive_loop() {
    EpollManager em;

    em.add_monitored_fd(EpollPair{this->cl_socket, EPOLLIN});
    int events_nr;
    MessageFrame msg_frame;
    msg_frame.msg_len = constants::MAX_LEN;
    while (!client_done_source.get_token().stop_requested()) {
        events_nr = em.wait_events(-1);
        if (em.check_event(this->cl_socket, EPOLLIN, events_nr)) {
            receive_msg_from_sv(msg_frame);
        }
    }

}
