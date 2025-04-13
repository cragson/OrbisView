#include "orbis_view.hpp"

#include <iostream>
#include <string>
#include <print>
#include <vector>
#include <netinet/tcp.h>
#include <ranges>

#include "nihonium/source/string-utils.hpp"
#include "nihonium/source/ILibkernel.hpp"

void orbis_view::send_msg_to_client( const std::string& msg )
{
    // I always send first the length of the incoming message to the client
    // The message containing the length of the incoming message is always 8 byte long!

    const auto msg_length = msg.size();

    send( this->m_client_fd, &msg_length, sizeof( msg_length ), 0 );

    // after sending now the length of the next message, I can send the message
    send( this->m_client_fd, msg.c_str(), msg_length, 0 );
}

bool orbis_view::setup_server( const int32_t port )
{
    if( port <= 0 )
            return false;

        auto srv_fd = socket(AF_INET, SOCK_STREAM, 0 );

        if( srv_fd == -1 )
        {
            std::println("[!] Failed to create socket, while setting up TCP server.");
            return false;
        }
            

        int32_t opt = 1;
        if( setsockopt( srv_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) ) == -1 )
        {

            std::println("[!] Failed to prepare socket for quick reconnection, while setting up TCP server.");

            close( srv_fd );

            return false;
        }
        
        auto sock_addr = std::make_unique< sockaddr_in >();
        sock_addr->sin_addr.s_addr = INADDR_ANY;
        sock_addr->sin_family = AF_INET;
        sock_addr->sin_port = htons( port );

        if( bind( srv_fd, reinterpret_cast< sockaddr* >( sock_addr.get() ), sizeof( sockaddr_in ) ) == -1 ) 
        {
            std::println("[!] Failed to bind socket, while setting up TCP server.");

            close( srv_fd );

            return false;
        }

        if( listen( srv_fd, 5 ) == -1 )
        {

            std::println("[!] Failed to listen to socket, while setting up TCP server.");

            close( srv_fd );

            return false;
        }

        this->m_server_fd = srv_fd;
        this->m_port = port;
        this->m_server_addr = std::move( sock_addr );

        return true;
}

bool orbis_view::handle_command( std::string& command )
{
    StringUtils::removeSpaces( command );

        const auto cmd_data = StringUtils::split_string( command );

        const auto& cmd = cmd_data.front();

        if( cmd == "quit" || cmd == "exit" ) 
        {
            std::println( "[!] Received 'quit' command. Terminating server." );
            
            return false;
        } 
        else if( cmd == "print" ) 
        {
            for( const auto& elem : cmd_data )
                std::print("{} ", elem );

            std::println("");

            return true;
        }
        else if( cmd == "kread8")
        {
            if( cmd_data.size() != 2 )
            {
                std::println("[!] Invalid kread8 command with arg size: {}", cmd_data.size());;

                return true;
            }

            const auto addr = StringUtils::str_to_ul( cmd_data.back() );

            if( addr <= 0 )
            {
                std::println("[kread8] Invalid address");
                
                this->send_msg_to_client("NIHONIUM-INVALID-ADDRESS");

                return true;
            }

            const auto client_ret =  this->m_kmem->read_kernel< uint8_t >( addr );

            std::println( "[kread8] Read 1 byte from: {:X} => {:X}", addr, client_ret );

            this->send_msg_to_client( std::format( "{}", client_ret ) );

            return true;
        }
        else if( cmd == "kread16")
        {
            if( cmd_data.size() != 2 )
            {
                std::println("[!] Invalid kread16 command with arg size: {}", cmd_data.size());;

                return true;
            }

            const auto addr = StringUtils::str_to_ul( cmd_data.back() );

            if( addr <= 0 )
            {
                std::println("[kread16] Invalid address");
                
                this->send_msg_to_client("NIHONIUM-INVALID-ADDRESS");

                return true;
            }

            const auto client_ret =  this->m_kmem->read_kernel< uint16_t >( addr );

            std::println( "[kread16] Read 2 bytes from: {:X} => {:X}", addr, client_ret );

            this->send_msg_to_client( std::format( "{}", client_ret ) );

            return true;
        }
        else if( cmd == "kread32")
        {
            if( cmd_data.size() != 2 )
            {
                std::println("[!] Invalid kread32 command with arg size: {}", cmd_data.size());;

                return true;
            }

            const auto addr = StringUtils::str_to_ul( cmd_data.back() );

            if( addr <= 0 )
            {
                std::println("[kread32] Invalid address");
                
                this->send_msg_to_client("NIHONIUM-INVALID-ADDRESS");

                return true;
            }

            const auto client_ret =  this->m_kmem->read_kernel< uint32_t >( addr );

            std::println( "[kread32] Read 4 bytes from: {:X} => {:X}", addr, client_ret );

            this->send_msg_to_client( std::format( "{}", client_ret ) );

            return true;
        }
        else if( cmd == "kread64")
        {
            if( cmd_data.size() != 2 )
            {
                std::println("[!] Invalid kread64 command with arg size: {}", cmd_data.size());;

                return true;
            }

            const auto addr = StringUtils::str_to_ul( cmd_data.back() );

            if( addr <= 0 )
            {
                std::println("[kread64] Invalid address");
                
                this->send_msg_to_client("NIHONIUM-INVALID-ADDRESS");

                return true;
            }

            const auto client_ret =  this->m_kmem->read_kernel< std::uintptr_t >( addr );

            std::println( "[kread64] Read 8 bytes from: {:X} => {:X}", addr, client_ret );

            this->send_msg_to_client( std::format( "{}", client_ret ) );

            return true;
        }
        else if( cmd == "kreadblock")
        {
            if( cmd_data.size() != 3 )
            {
                std::println( "[!] Invalid kreadblock command with arg size: {}", cmd_data.size() );

                return true;
            }

            const auto addr = StringUtils::str_to_ul( cmd_data.at( 1 ) );

            auto size = StringUtils::str_to_ul( cmd_data.back() );

            if( addr <= 0 || size <= 0 )
            {
                std::println( "[kreadblock] Invalid address or size" );
                
                this->send_msg_to_client("NIHONIUM-INVALID-ARGUMENT");

                return true;
            }

            // make sure now size is divisable by 8 so the read block kernel will succeed
            if( size % 8 != 0 )
                size += 8 - ( size % 8 );

            const auto bytes = this->m_kmem->read_block_kernel( addr, size );

            std::println( "[kreadblock] Read {} bytes from: {:X}", bytes.size(), addr );        

            if( bytes.size() != size )
                std::println( "[!] Failed to read the requested block of {} bytes", size );

            auto client_msg = std::string();

            // convert byte stream to formatted hex string, fucking love modern cpp features    
            std::ranges::for_each( bytes, [&]( const auto& elem ){ client_msg += std::format( "{:02X} ", elem ); } );
            
            // remove last space
            client_msg.pop_back();

            this->send_msg_to_client( client_msg );

            return true;
        }
        else if( cmd == "get_procs" )
        {
            const auto current_processes = this->m_kmem->get_process_list();

            if( current_processes.empty() )
            {
                std::println("[!] Process list is empty, should not happen wtf lol!");

                this->send_msg_to_client("");

                return true;
            }

            auto client_msg = std::string();

            std::ranges::for_each( current_processes, [&]( const auto& elem ) 
            { 
                auto process_name = std::string( elem->m_name );
                auto state = std::string( elem->m_state );

                StringUtils::remove_non_printable_chars_from_string( process_name );
                StringUtils::remove_non_printable_chars_from_string( state );

                client_msg += std::format( "{} {} {},", process_name, elem->m_pid, state ); 
            } );
            
            // remove last comma from process string
            client_msg.pop_back();

            this->send_msg_to_client( client_msg );

            return true;
        }
        else if( cmd == "get_proc_info" )
        {
            // TODO: Implement logic for loaded memory regions etc.
            std::println( "[!] get_proc_info needs to be implemented!" );

            return true;
        }
        else 
        {
            std::println( "Unknown command: {}", command );
            return true;
        }
}

void orbis_view::run()
{
    if( this->m_port <= 0 || !this->m_server_fd || !this->m_server_addr )
        {
            std::println("[!] TCP server was not setup correctly, can not run.");

            return;
        }

        std::println( "[+] OrbisView is running on port: {}", this->m_port );

        bool running = true;

        while( running ) 
        {
            auto client_address = std::make_unique< sockaddr_in >();
            socklen_t client_addr_len = sizeof( client_address );

            int client_fd = accept( this->m_server_fd, reinterpret_cast< sockaddr* >( client_address.get() ), &client_addr_len );

            if( client_fd == -1 ) 
            {
                std::println("[!] Client accept failed!");
                
                // Continue accepting new connections
                continue; 
            }
            
            this->m_client_fd = client_fd;

            const auto acpt_client_ip = inet_ntoa( client_address->sin_addr );
            std::println( "[-] Accepted client: {}", acpt_client_ip );
            ILibkernel::send_kernel_notitifcation( std::format( "OrbisView:\naccepted client: {}", acpt_client_ip ) );  
            
            int32_t delay_flag = 1;

            if( setsockopt( client_fd, IPPROTO_TCP, TCP_NODELAY, &delay_flag, sizeof( delay_flag ) ) == -1 )
            {
                std::println("[!] Failed to remove the delay of the client connection");

                continue;
            }

            // Process multiple commands from the same client connection.
            bool client_connected = true;
            while( client_connected ) 
            {
                auto buffer = std::vector< char >();
                buffer.resize( ORBISVIEW_TCP_SERVER_MSG_SIZE );

                ssize_t bytes_received = read( client_fd, buffer.data(), buffer.size() );
                if( bytes_received <= 0 ) 
                {
                    std::println("[!] Client disconnected or error reading from client." );
                    
                    this->m_client_fd = -1;

                    // Exit loop if client disconnects or error occurs
                    break; 
                }

                auto recv_cmd = std::string( buffer.data(), bytes_received + 1 );

                std::println( "[-] Received cmd: {} ({} bytes)", recv_cmd, bytes_received );

                bool should_continue = handle_command( recv_cmd );

                if( !should_continue ) 
                {
                    // If "quit" was received, notify the client and exit loops.
                    //const std::string quitResponse = "NIHONIUM-TERMINATED";

                    //write( client_fd, quitResponse.c_str(), quitResponse.size() );

                    running = false;
                    client_connected = false;
                    this->m_client_fd = -1;

                    break;
                } 
            }

            this->m_client_fd = -1;

            // Close the connection with the client
            close( client_fd );
        }

        std::println( "[-] Stopping and terminating TCP server, goodbye! :-)" );

        ILibkernel::send_kernel_notitifcation( "OrbisView:\nserver says goodbye!" );

        return;
}