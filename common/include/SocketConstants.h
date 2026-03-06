#pragma once
#include <cstddef>

namespace devana
{

    constexpr int    FD_INVALID            = -1;
    constexpr int    DEFAULT_LISTEN_BACKLOG = 10;
    constexpr size_t DEFAULT_RECV_BUFFER    = 4096;
    constexpr size_t MAX_UDP_DATAGRAM       = 65535;

} // namespace devana
