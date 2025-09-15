#pragma once
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

inline int create_uds_server(const char* socket_path) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

    unlink(socket_path); // 기존 소켓 파일 삭제
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(1); }
    if (listen(server_fd, 1) < 0) { perror("listen"); exit(1); }

    std::cout << "[UDS] Waiting for connection on " << socket_path << std::endl;
    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) { perror("accept"); exit(1); }
    std::cout << "[UDS] Connection established!" << std::endl;
    close(server_fd); // 더 이상 필요 없음
    return client_fd; // 연결된 소켓 fd 반환
}

inline int connect_uds_client(const char* socket_path) {
    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    int client_fd;

    while (true) {
        client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (client_fd < 0) {
            perror("socket");
            exit(1);
        }

        if (connect(client_fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
            std::cout << "[UDS] Connected to " << socket_path << std::endl;
            return client_fd;
        }

        std::cerr << "[UDS] Waiting for server at " << socket_path << "...\n";
        close(client_fd); // 실패한 소켓은 닫아줘야 함
        sleep(1);
    }
}