/* Copyright (C) 2025 cragson */

#include <string>
#include <memory>
#include <print>
#include <format>

#include "orbis_view.hpp"
#include "nihonium/source/ILibkernel.hpp"

int main() 
{
    const auto srv = std::make_unique< orbis_view >();

    std::print( "[-] Trying to setup OrbisView.." );

    constexpr auto SERVER_PORT = 60002;

    if( srv->setup_server( SERVER_PORT ) )
        std::println( "done!" );
    else
    {
        std::println( "failed!" );

        return 1;
    }

    ILibkernel::send_kernel_notitifcation( std::format("OrbisView\nserver listening now on: {}", SERVER_PORT ) );

    srv->run();

   return 0;
}
