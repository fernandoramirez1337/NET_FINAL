#ifndef SERVER_HPP
#define SERVER_HPP

#include "neuronal_network.hpp"
#include "game.hpp"

class server{
public:
    server();
    ~server();
private:
    neuronal_network nn;
    tictactoe game;
};

#endif // SERVER_HPP