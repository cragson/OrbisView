#pragma once

#include "nihonium/source/tcp-server.hpp"
#include "nihonium/source/kernel_memory.hpp"

constexpr auto ORBISVIEW_TCP_SERVER_MSG_SIZE = 1024;

class orbis_view : public tcp_server
{
public:
    orbis_view()
    {
        this->m_port = {};
        this->m_server_fd = {};

        this->m_server_addr = std::make_unique< sockaddr_in >();
        this->m_client_addr = std::make_unique< sockaddr_in >();

        this->m_kmem = std::make_unique< kernel_memory >();
    }

    ~orbis_view()
    {
        close( this->m_client_fd );

        close( this->m_server_fd );  
    }

    void send_msg_to_client( const std::string& msg ) override;

    bool setup_server( const int32_t port ) override;

    virtual bool handle_command( std::string& command ) override;

    void run() override;

private:
    std::unique_ptr< kernel_memory > m_kmem;
};