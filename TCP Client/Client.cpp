#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <WS2tcpip.h>
#include <winsock.h>
#include <iostream>
#include <string>

#define PORT 1500

using namespace std;

int main() {
    int wsaStatus, connectStatus; //check errors
    WSADATA WSAData;
    wsaStatus = WSAStartup(MAKEWORD(2, 0), &WSAData);
    if (wsaStatus != NO_ERROR) {
        std::cout << "WSA Startup failed with error : " << wsaStatus;
    }
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cout << "INVALID SOCKET " << WSAGetLastError();
        WSACleanup();
    }

    SOCKADDR_IN sin;//information about the socket
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");//ip of the server you want to connect to
    sin.sin_family = AF_INET;//family of the socket, for internet it's AF_INET
    sin.sin_port = htons(PORT);// 23 for telnet etc, it's the port

    connectStatus = connect(sock, (SOCKADDR*)&sin, sizeof(sin)); //function to connect to the server
    if (connectStatus == SOCKET_ERROR) { //it returns 0 if no error occurs
        std::cout << "Connection failed with error : " << WSAGetLastError();
        closesocket(sock);
        WSACleanup();
    }

    char buf[128];
    std::string userInput;

    int bytesReceived = recv(sock, buf, 128, 0);
    if (bytesReceived > 0)
        cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;

    do {
        cout << "> ";
        getline(cin, userInput);
        if (userInput.size() > 0) {
            int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
            if (sendResult != SOCKET_ERROR) {
                ZeroMemory(buf, 128);
                bytesReceived = recv(sock, buf, 128, 0);
                if (bytesReceived > 0) {
                    cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;
                }
            }
        }
    } while (userInput.size() > 0);

    closesocket(sock);
    WSACleanup();

    system("pause");
}