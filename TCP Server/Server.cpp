#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <locale.h>
#include <iostream>
#include <cstdio>
#include <winsock.h>
#include <thread>
#include <ctime>

using std::endl;
using std::cout;

#define SERVICE_PORT 1500 // TCP-порт сервера

// Функция отсылки текстовой ascii строки клиенту
int send_string(SOCKET Client, const char* data) { return send(Client, data, strlen(data), 0); }

void shutdown(SOCKET Server) {
    closesocket(Server); // Закрываем серверный сокет
    WSACleanup(); // Освобождаем ресурсы библиотеки сокетов
    exit(0);
}

void client_commands(SOCKET Client, SOCKET Server) {
    char sReceiveBuffer[128] = { 0 };
    while (true)
    {
        int nReaded = recv(Client, sReceiveBuffer, sizeof(sReceiveBuffer) - 1, 0); // Получаем данные от клиента.
        if (nReaded <= 0) break; // Выходим в случае ошибки (например, отсоединения клиента).
        sReceiveBuffer[nReaded] = 0; // Мы получаем поток байт, поэтому нужно самостоятельно добавить завершающий 0 для ASCII строки 

        for (char* pPtr = sReceiveBuffer; *pPtr != 0; pPtr++) // Отбрасываем символы перевода строк
        {
            if (*pPtr == '\n' || *pPtr == '\r')
            {
                *pPtr = 0;
                break;
            }
        }
        if (sReceiveBuffer[0] == 0) continue; // Пропускаем пустые строки
        cout << "Received data from client #" << Client << " :" << sReceiveBuffer << endl;
        if (strcmp(sReceiveBuffer, "time") == 0) {
            time_t now = time(0);
            struct tm tstruct = *localtime(&now);
            char buf[80];
            strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M", &tstruct);
            char timestr[100];
            strcpy(timestr, "System time: ");
            strcat(timestr, buf);
            strcat(timestr, "\r\n");
            send_string(Client, timestr);
        }
        else if (strcmp(sReceiveBuffer, "info") == 0) {
            send_string(Client, "TCP-Server.\r\nAvailable commands: time, info, task, invert, exit, shutdown\r\n");
        }
        else if (strcmp(sReceiveBuffer, "task") == 0) {
            send_string(Client, "Variant #17. Add support for an additional command to the service, with which you can invert an arbitrary string. \r\n");
        }
        else if (strcmp(sReceiveBuffer, "exit") == 0) {
            send_string(Client, "Bye...\r\n");
            cout << "Client initialize disconnection." << endl;
            break;
        }
        else if (strcmp(sReceiveBuffer, "shutdown") == 0) {
            send_string(Client, "Server go to shutdown.\r\n");
            Sleep(500);
            closesocket(Client);
            shutdown(Server);
        }
        else if (strcmp(sReceiveBuffer, "invert") == 0) {
            send_string(Client, "Which string do you want to invert?\r\n");
            bool flag = true;
            while (flag) {
                int nReaded = recv(Client, sReceiveBuffer, sizeof(sReceiveBuffer) - 1, 0); // Получаем данные от клиента.
                if (nReaded <= 0) break; // Выходим в случае ошибки (например, отсоединения клиента).
                sReceiveBuffer[nReaded] = 0; // Мы получаем поток байт, поэтому нужно самостоятельно добавить завершающий 0 для ASCII строки 

                for (char* pPtr = sReceiveBuffer; *pPtr != 0; pPtr++) // Отбрасываем символы перевода строк
                {
                    if (*pPtr == '\n' || *pPtr == '\r')
                    {
                        *pPtr = 0;
                        break;
                    }
                }
                if (sReceiveBuffer[0] == 0) continue; // Пропускаем пустые строки
                cout << "Received data: " << sReceiveBuffer << endl;
                std::string str{ sReceiveBuffer };
                std::reverse(std::begin(str), std::end(str));
                str += "\r\n";
                const char* cstr = str.c_str();
                flag = false;
                send_string(Client, cstr);
            }
        }
        else {
            send_string(Client, "Unknown command. Type 'info' to see the available commands.\r\n");
        }
    }
    closesocket(Client); // Закрываем присоединенный сокет
    cout << "Client with socket #" << Client << " disconnected." << endl;
}

// return -1 = server shutdown
int connect_to_server(SOCKET Server, sockaddr_in serv_addr, sockaddr_in clnt_addr) {
    int addrlen = sizeof(serv_addr);
    int clientSize = sizeof(clnt_addr);
    SOCKET Client = accept(Server, (sockaddr*)&clnt_addr, &clientSize);
    if (Client == INVALID_SOCKET)
    {
        fprintf(stderr, "Can't accept connection\n");
        return -1;
    }
    // Получаем параметры присоединенного сокета NS и информацию о клиенте
    addrlen = sizeof(serv_addr);
    getsockname(Client, (sockaddr*)&serv_addr, &addrlen);
    // Функция inet_ntoa возвращает указатель на глобальный буффер, 
    // поэтому использовать ее в одном вызове printf не получится
    printf("Accepted connection on %s:%d ",
        inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
    printf("from client %s:%d\n",
        inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));

    // Отсылаем вводную информацию о сервере
    send_string(Client, "* * * Welcome to the TCP-Server * * *\r\n");
    // Получаем и обрабатываем данные от клиента
    std::thread thr(client_commands, Client, Server);
    thr.detach();

}

int main()
{
    setlocale(LC_ALL, "Russian");
    SOCKET Server;  // дескриптор прослушивающего сокета
    sockaddr_in serv_addr;
    WSADATA wsadata;
    char serverName[128];

    WSAStartup(MAKEWORD(2, 2), &wsadata); // Инициализируем библиотеку сокетов

    gethostname(serverName, sizeof(serverName)); // Получаем имя сервера
    cout << endl << "Server name is " << serverName << endl;

    if ((Server = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) // Создаем сокет. TCP-сокет SOCK_STREAM, UDP - SOCK_DGRAM 
    {
        fprintf(stderr, "Cant create socket.\n");
        exit(-1);
    }
    memset(&serv_addr, 0, sizeof(serv_addr)); // Заполняем структуру адресов 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; // Разрешаем работу на всех доступных сетевых интерфейсах, в частности на localhost
    serv_addr.sin_port = htons((u_short)SERVICE_PORT);

    if (bind(Server, (sockaddr*)&serv_addr, sizeof(serv_addr)) == INVALID_SOCKET) // Связываем сокет с заданным сетевым интерфесом и портом
    {
        fprintf(stderr, "Cant bind socket\n");
        exit(-1);
    }

    // Переводим сокет в режим прослушивания заданного порта с максимальным количеством ожидания запросов на соединение 5
    if (listen(Server, 5) == INVALID_SOCKET)
    {
        fprintf(stderr, "Cant join listening mode\n");
        exit(-1);
    }

    cout << "Server listening on " << inet_ntoa(serv_addr.sin_addr) << ":" << ntohs(serv_addr.sin_port) << endl;

    while (true) // Основной цикл обработки подключения клиентов 
    {
        cout << "Server waiting for connections..." << endl;

        sockaddr_in clnt_addr;
        int addrlen = sizeof(clnt_addr);
        memset(&clnt_addr, 0, sizeof(clnt_addr));

        // Переводим сервис в режим ожидания запроса на соединение.
        // Вызов синхронный, т.е. возвращает управление только при 
        // подключении клиента или ошибке 
        int code = connect_to_server(Server, serv_addr, clnt_addr);
        if (code == -1) break;
    }

    return 0;
}
