#ifndef DATA_CENTER_HPP
#define DATA_CENTER_HPP

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <utility>
#include <map>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <thread>


#ifndef UDP_MAXSIZE
    #define UDP_MAXSIZE 1000
#endif
#ifndef PAD
    #define PAD 46
#endif


class data_center{
public:
    data_center(uint16_t, uint16_t);
    ~data_center();
private:
    void start();

    std::map<std::pair<char[INET_ADDRSTRLEN], uint16_t>,bool> list_of_servers;
    int udp_sock, tcp_sock;

    // UDP
    void udp_read_write();
    void handle_data_request(const struct sockaddr_in&);
    void send_udp(const std::string&, const struct sockaddr_in&);
    std::string generate_data_message();
    void setup_udp_socket(const uint16_t);
    void udp_session();


    // TCP
    void tcp_read_write(void *);
    void handle_list_request(int);
    void send_tcp(const std::string&, const int);
    std::string generate_list_message();
    void setup_tcp_socket(const uint16_t);
    int accept_tcp_connection();
    void tcp_session(int);

};



#endif // DATA_CENTER_HPP