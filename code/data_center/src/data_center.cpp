#include "../include/data_center.hpp"

std::string pad_message(const std::string& msg) {
    if (msg.size() < UDP_MAXSIZE) {
        std::ostringstream oss;
        oss << msg;
        oss << std::setw(UDP_MAXSIZE - msg.size()) << std::setfill((char)PAD) << "";
        return oss.str();
    }
    return msg;
}
std::string unpad_message(const std::string& msg) {
    auto end = std::find_if(msg.rbegin(), msg.rend(), [](char ch) {
        return ch != (char)PAD;
    }).base();
    return std::string(msg.begin(), end);
}
void sigchld_handler(int s) {
        // waitpid() might overwrite errno, so we save and restore it:
        int saved_errno = errno;
        while(waitpid(-1, nullptr, WNOHANG) > 0);
        errno = saved_errno;
}
/////////////////////////////////////////////////////////////////////////////////////
data_center::data_center(uint16_t udp_port, uint16_t tcp_port) {
    setup_udp_socket(udp_port);
    setup_tcp_socket(tcp_port);

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }   
}

data_center::~data_center() {
    close(udp_sock);
    close(tcp_sock);
}

void data_center::setup_udp_socket(uint16_t udp_port) {
    struct sockaddr_in server_addr;

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(udp_port);

    if (bind(udp_sock, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP bind failed");
        exit(EXIT_FAILURE);
    }
}

void data_center::setup_tcp_socket(uint16_t tcp_port) {
    struct addrinfo hints, *servinfo, *p;
    int yes = 1, rv;
    char port_str[6];
    snprintf(port_str, sizeof(port_str), "%u", tcp_port);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(nullptr, port_str, &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
        exit(EXIT_FAILURE);
    }

    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((tcp_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("TCP socket creation failed");
            continue;
        }

        if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        if (bind(tcp_sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(tcp_sock);
            perror("TCP bind failed");
            continue;
        }

        break; // Successfully bound
    }

    freeaddrinfo(servinfo);

    if (p == nullptr) {
        std::cerr << "TCP bind failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(tcp_sock, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
}

std::string data_center::generate_list_message() {
    size_t map_size, list_size;
    std::string list;
    std::ostringstream oss;
    
    auto it = list_of_servers.begin();
    if (it != list_of_servers.end()) {
        oss << it->second;
        oss << std::setw(INET_ADDRSTRLEN) << std::setfill('0') << it->first.first;
        oss << std::setw(5) << std::setfill('0') << it->first.second; 
        ++it; 
    }
    
    for (; it != list_of_servers.end(); ++it) {
        oss << '\n';
        oss << it->second;
        oss << std::setw(INET_ADDRSTRLEN) << std::setfill('0') << it->first.first;
        oss << std::setw(5) << std::setfill('0') << it->first.second; 
    }
    
    list = oss.str();
    map_size = list_of_servers.size();
    list_size = list.size();
    
    // Clear the ostringstream
    oss.str("");
    oss.clear();
    
    // Create the header
    oss << 'L';
    oss << std::setw(2) << std::setfill('0') << map_size;
    oss << std::setw(3) << std::setfill('0') << list_size;
    oss << list;
    
    return pad_message(oss.str());
}

void data_center::send_udp(const std::string& data, const struct sockaddr_in& receiver_addr){
    if (sendto(udp_sock, data.c_str(), UDP_MAXSIZE, MSG_CONFIRM, (const struct sockaddr *)&receiver_addr, sizeof(receiver_addr)) == -1)
        perror("sendto error");
}

void data_center::send_tcp(const std::string& data, const int receiver_sock){
    if (send(receiver_sock, data.c_str(), data.size(), 0) == -1)
        perror("send send");
}   

void data_center::udp_read_write(){
    struct sockaddr_in client_addr; socklen_t addr_len; 
    std::vector<char> buffer(UDP_MAXSIZE);

    while (true) {
        memset(&client_addr, 0, sizeof(client_addr));
        addr_len = sizeof(client_addr);
        buffer.clear();
        ssize_t n_bytes = recvfrom(udp_sock, buffer.data(), UDP_MAXSIZE, MSG_WAITALL, (struct sockaddr *)&client_addr, &addr_len);

        if (n_bytes == -1) {
            perror("recvfrom error");
            continue;
        }

        if (n_bytes > 0) {
            std::string data = unpad_message(buffer.data());
            //std::cout << "-> " << data << std::endl;
            char message_type = data[0];

            switch (message_type) {
                case 'D':
                    handle_data_request(client_addr);
                    break;
                default:
                    std::cerr << "[UDP] Unknown message type received" << std::endl;
                    break;
            }
        }
    }
}

void data_center::tcp_read_write(void *void_tcp_sock) {
    int _sock = (intptr_t)void_tcp_sock;
    std::string this_username;
    char type_buffer[1];
    
    while (true) {
        type_buffer[0] = '\n';
        if (recv(_sock, type_buffer, 1, 0) == -1) {
            perror("recv");
        }
        char message_type = type_buffer[0];

        switch (message_type) {
            case 'L':
                handle_list_request(_sock);
                break;
            default:
                std::cerr << "[TCP] Unknown message type received" << std::endl;
                break;
        }
    }
}

void data_center::start(){
    udp_session();
    while(1){
        int accepted_connection = accept_tcp_connection();
        tcp_session(accepted_connection);
    }
    
}

int data_center::accept_tcp_connection() {
    sockaddr_storage tcp_addr;
    socklen_t tcp_addr_size = sizeof tcp_addr;
    int _sock = accept(tcp_sock, (sockaddr *)&tcp_addr, &tcp_addr_size);
    if (_sock == -1){
        perror("accept");
    }
    return _sock;
}

void data_center::udp_session(){
    std::thread udp_worker_thread([this](){udp_read_write();});
    udp_worker_thread.detach();
}

void data_center::tcp_session(int _sockFD){
    std::thread worker_thread([this, _sockFD](){tcp_read_write(reinterpret_cast<void *>(_sockFD));});
    worker_thread.detach();
}

void data_center::handle_list_request(int _sock) {
    std::string list_message = generate_list_message();
    send_tcp(list_message, _sock);
}

void data_center::handle_data_request(const struct sockaddr_in& client_addr) {
    // Handle the data request from the client
}