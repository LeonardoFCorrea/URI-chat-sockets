#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#define MAX_CLIENTS 10
#define PORT 8000

SOCKET clientSockets[MAX_CLIENTS];
int clientCount = 0;
HANDLE threads[MAX_CLIENTS];
CRITICAL_SECTION mutex;

DWORD WINAPI clientHandler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char message[1024];
    int bytesRead;

    int clientNumber;

    EnterCriticalSection(&mutex);
    for (int i = 0; i < clientCount; i++) {
        if (clientSockets[i] == clientSocket) {
            clientNumber = i + 1;
            break;
        }
    }
    LeaveCriticalSection(&mutex);

    while (1) {
        bytesRead = recv(clientSocket, message, sizeof(message), 0);
        if (bytesRead <= 0) {
            EnterCriticalSection(&mutex);
            for (int i = 0; i < clientCount; i++) {
                if (clientSockets[i] == clientSocket) {
                    for (int j = i; j < clientCount - 1; j++) {
                        clientSockets[j] = clientSockets[j + 1];
                    }
                    clientCount--;
                }
            }
            LeaveCriticalSection(&mutex);
            closesocket(clientSocket);
            break;
        }
        message[bytesRead] = '\0';

        char messageWithPrefix[2048];
        snprintf(messageWithPrefix, sizeof(messageWithPrefix), "Cliente %d: %s", clientNumber, message);

        printf("%s\n", messageWithPrefix);

        EnterCriticalSection(&mutex);
        for (int i = 0; i < clientCount; i++) {
            if (clientSockets[i] != clientSocket) {
                send(clientSockets[i], messageWithPrefix, strlen(messageWithPrefix), 0);
            }
        }
        LeaveCriticalSection(&mutex);
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup");
        return 1;
    }

    SOCKET serverSocket;
    struct sockaddr_in serverAddr;

    InitializeCriticalSection(&mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientSockets[i] = INVALID_SOCKET;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        perror("Socket");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("Unable to bind");
        return 1;
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        perror("Listen");
        return 1;
    }

    printf("Servidor de chat está ouvindo na porta %d...\n", PORT);

    while (1) {
        SOCKET clientSocket;
        struct sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            perror("Accept");
            continue;
        }

        EnterCriticalSection(&mutex);
        if (clientCount < MAX_CLIENTS) {
            clientSockets[clientCount] = clientSocket;
            clientCount++;
            threads[clientCount - 1] = CreateThread(NULL, 0, clientHandler, (LPVOID)clientSocket, 0, NULL);
        } else {
            printf("Número máximo de clientes atingido. Rejeitando nova conexão.\n");
            closesocket(clientSocket);
        }
        LeaveCriticalSection(&mutex);
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}