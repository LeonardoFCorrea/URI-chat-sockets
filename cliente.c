#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#define SERVER_PORT 8000
#define SERVER_ADDRESS "127.0.0.1"  // Use o endereço IP do servidor
#define BUFFER_SIZE 1024

int keepRunning = 1;
SOCKET clientSocket;
HANDLE readThread;

DWORD WINAPI readThreadFunction(LPVOID lpParam) {
    while (keepRunning) {
        char readBuffer[BUFFER_SIZE];

        int bytesRead = recv(clientSocket, readBuffer, sizeof(readBuffer) - 1, 0);
        if (bytesRead <= 0) {
            keepRunning = 0;
            break;
        }

        readBuffer[bytesRead] = '\0';
        printf("%s\n", readBuffer);
    }
    return 0;
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup");
        return 1;
    }

    struct sockaddr_in serverAddr;
    char inputBuffer[BUFFER_SIZE];

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        perror("Erro ao criar socket");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);  // Use inet_addr para converter o endereço IP

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        perror("Erro ao conectar ao servidor");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    readThread = CreateThread(NULL, 0, readThreadFunction, NULL, 0, NULL);

    printf("Digite uma mensagem (ou 'sair' para encerrar):\n");
    while (1) {
        fgets(inputBuffer, sizeof(inputBuffer), stdin);
        size_t len = strlen(inputBuffer);
        if (len > 0 && inputBuffer[len - 1] == '\n') {
            inputBuffer[len - 1] = '\0';
        }

        if (strcmp(inputBuffer, "sair") == 0) {
            keepRunning = 0;
            break;
        }
        send(clientSocket, inputBuffer, strlen(inputBuffer), 0);
    }

    WaitForSingleObject(readThread, INFINITE);
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}