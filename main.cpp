#include <iostream>
#include <winsock2.h>
#include <thread>
#include <string>
#include <fstream>


#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#define PORT 3030
#define BUFFER_SIZE 1024

using namespace std;

class HttpServer {
private:
    SOCKET serverSocket;
    sockaddr_in serverAddr;

    void receiveRequest(SOCKET clientSocket, string& fileName) {
        char buffer[BUFFER_SIZE] = {0};
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived > 0) {
            string request(buffer, bytesReceived);
            size_t firstNewLine = request.find_first_of('\n');
            if (firstNewLine != string::npos) {
                string firstLine = request.substr(0, firstNewLine);
                size_t start = firstLine.find_first_of(" /") + 2;
                size_t end = firstLine.find_last_of(" ");

                if (start != string::npos && end != string::npos && start < end) {
                    fileName = firstLine.substr(start, end - start);
                }
            }
        } else {
            cerr << "[Error] Failed to receive data from client." << endl;
        }
    }

    void sendResponse(SOCKET clientSocket, string& fileName) {
        if (fileName.empty() || fileName == "/") {
            fileName = "index.html";
        }

        ifstream file(fileName, ios::binary);
        string response;

        if (file) {
            string fileContent((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + to_string(fileContent.length()) + "\r\n\r\n";
            response += fileContent;
        } else {
            string errorContent = "<html><body><h1>404 Not Found</h1></body></html>";
            response = "HTTP/1.1 404 Not Found\r\n";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + to_string(errorContent.length()) + "\r\n\r\n";
            response += errorContent;
        }

        send(clientSocket, response.c_str(), (int)response.length(), 0);
    }

    void sessionHandler(SOCKET clientSocket) {
        string fileName;
        receiveRequest(clientSocket, fileName);
        sendResponse(clientSocket, fileName);
        closesocket(clientSocket);
    }

public:
    HttpServer() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "[Error] WinSock initialization failed." << endl;
            return;
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            cerr << "[Error] Could not create socket." << endl;
            WSACleanup();
            return;
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "[Error] Bind failed." << endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            cerr << "[Error] Listen failed." << endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        cout << "[Success] Server is running! Click to open: http://localhost:" << PORT << endl;
    }

    ~HttpServer() {
        closesocket(serverSocket);
        WSACleanup();
    }

    void run() {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        while (true) {
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
            if (clientSocket != INVALID_SOCKET) {
                cout << "[Info] New client connected." << endl;


                thread t(&HttpServer::sessionHandler, this, clientSocket);
                t.detach();
            }
        }
    }
};

int main() {
    HttpServer server;
    server.run();
    return 0;
}