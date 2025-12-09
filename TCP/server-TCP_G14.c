#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#define closesocket close
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PROTOPORT 27015
#define QLEN 6

void ErrorHandler(char* errorMessage) {
  printf("%s", errorMessage);
}

void ClearWinSock() {
#if defined WIN32
  WSACleanup();
#endif
}

int main(int argc, char* argv[]) {
  int port;
  if (argc > 1) {
    port = atoi(argv[1]);
  } else
    port = PROTOPORT;

  if (port < 0) {
    printf("bad port number %s \n", argv[1]);
    return 0;
  }

#if defined WIN32
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    ErrorHandler("Error at WSAStartup()\n");
    return 0;
  }
#endif

  int MySocket;
  MySocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (MySocket < 0) {
    ErrorHandler("socket creation failed.\n");
    ClearWinSock();
    return -1;
  }

  struct sockaddr_in sockaddr;
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  sockaddr.sin_port = htons(port);

  if (bind(MySocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
    ErrorHandler("bind() failed.\n");
    closesocket(MySocket);
    ClearWinSock();
    return -1;
  }

  if (listen(MySocket, QLEN) < 0) {
    ErrorHandler("listen() failed.\n");
    closesocket(MySocket);
    ClearWinSock();
    return -1;
  }

  struct sockaddr_in clientAd;
  int clientSocket;
  int clientLen;
  printf("\n\nWaiting for a client to connect...\n");

  while (1) {

    clientLen = sizeof(clientAd);

    if ((clientSocket = accept(MySocket, (struct sockaddr*)&clientAd, &clientLen)) < 0) {
      ErrorHandler("accept() failed.\n");
      closesocket(MySocket);
      ClearWinSock();
      return 0;
    }

    printf("\nHandling client %s\n", inet_ntoa(clientAd.sin_addr));

    char inputString[256] = "connessione avvenuta\n";
    int stringLen = strlen(inputString);

    if (send(clientSocket, inputString, stringLen, 0) != stringLen) {
      ErrorHandler("send() sent a different number of bytes than expected");
      closesocket(clientSocket);
      continue; 
    } else {
      printf("Messaggio che sto inviando: %s", inputString);
    }

    char opzioneScelta;
    char* messaggioDaInviare;
    
    int chiudi_sessione = 0; 

    int n = recv(clientSocket, &opzioneScelta, 1, 0);
    opzioneScelta = tolower(opzioneScelta);

    if (n <= 0) {
      printf("recv() failed o connessione chiusa inizialmente.\n");
      closesocket(clientSocket);
      continue;
    } else {
      switch (opzioneScelta) {
        case 'a':
          messaggioDaInviare = "ADDIZIONE\n";
          break;
        case 's':
          messaggioDaInviare = "SOTTRAZIONE\n";
          break;
        case 'm':
          messaggioDaInviare = "MOLTIPLICAZIONE\n";
          break;
        case 'd':
          messaggioDaInviare = "DIVISIONE\n";
          break;
        default:
          messaggioDaInviare = "TERMINE PROCESSO CLIENT\n"; 
          chiudi_sessione = 1;
          break;
      }

      int messLenght = strlen(messaggioDaInviare);

      printf("\nOperazione scelta dal client: %s", messaggioDaInviare);

      if (send(clientSocket, messaggioDaInviare, messLenght, 0) != messLenght) {
        ErrorHandler("send() sent a different number of bytes than expected");
        closesocket(clientSocket);
        continue;
      } else {
        printf("Messaggio inviato: %s", messaggioDaInviare);
      }
      
      if (chiudi_sessione == 1) {
          printf("\nChiudo client e torno in ascolto.\n");
          closesocket(clientSocket);
          continue; // Torna al while
      }
    }

    float n1, n2;
    float result;
    int bytes_rcvd;

    if ((bytes_rcvd = recv(clientSocket, (char*)&n1, sizeof(float), 0)) <= 0) {
      printf("recv() failed leggendo n1.\n");
      closesocket(clientSocket); 
      continue; 
    }

    int n1_bits = ntohl(*(int*)&n1);
    n1 = *(float*)&n1_bits;
    
    if ((bytes_rcvd = recv(clientSocket, (char*)&n2, sizeof(float), 0)) <= 0) {
      printf("recv() failed leggendo n2.\n");
      closesocket(clientSocket);
      continue; 
    }
    
    int n2_bits = ntohl(*(int*)&n2);
    n2 = *(float*)&n2_bits;

    printf("\nNumeri ricevuti dal client: %.2f e %.2f\n", n1, n2);

    if (opzioneScelta == 'a') {
      result = n1 + n2;
    } else if (opzioneScelta == 's') {
      result = n1 - n2;
    } else if (opzioneScelta == 'm') {
      result = n1 * n2;
    } else if (opzioneScelta == 'd') {
      if (n2 != 0) {
        result = n1 / n2;
      } else {
        printf("Errore: Divisione per zero.\n");
        result = 0.0f;
      }
    }

    printf("Risultato calcolato: %.2f\n", result);

    int result_bits = htonl(*(int*)&result);
    float result_net = *(float*)&result_bits;


    if (send(clientSocket, (const char*)&result_net, sizeof(float), 0) != sizeof(float)) {
      ErrorHandler("error sending result to client.\n");
      closesocket(clientSocket); 
    } else {
      printf("\nRisultato inviato al client.\n");
    }
    
    closesocket(clientSocket);
    printf("\nChiudo client e torno in ascolto.\n");


  } 
}
