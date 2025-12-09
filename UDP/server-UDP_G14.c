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

#define PROTOPORT 48000

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

  #if defined WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    printf("Errore WSAStartup\n");
    return 0;
  }
  #endif

  int MySocket;
  MySocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (MySocket < 0) {
    ErrorHandler("socket creation failed.\n");
    return -1;
  }

  struct sockaddr_in sockaddr;
  memset(&sockaddr, 0, sizeof(sockaddr));
  sockaddr.sin_family = AF_INET;
  
  sockaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
  sockaddr.sin_port = htons(port);

  if (bind(MySocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
    ErrorHandler("bind() failed.\n");
    closesocket(MySocket);
    return -1;
  }

  struct sockaddr_in clientAd;
  int clientLen;

   printf("\nServer in attesa di richieste...\n");

  while (1) {

    clientLen = sizeof(clientAd); 
    char opzioneScelta;
    char* messaggioDaInviare;
    int chiudi_sessione = 0;

    int n = recvfrom(MySocket, &opzioneScelta, 1, 0, (struct sockaddr*)&clientAd, &clientLen);
    
    if (n < 0) {
        printf("\nErrore in recvfrom (nessun dato o errore rete)\n");
        continue;
    }

    opzioneScelta = tolower(opzioneScelta);

    switch (opzioneScelta) {
      case 'a': messaggioDaInviare = "ADDIZIONE\n"; break;
      case 's': messaggioDaInviare = "SOTTRAZIONE\n"; break;
      case 'm': messaggioDaInviare = "MOLTIPLICAZIONE\n"; break;
      case 'd': messaggioDaInviare = "DIVISIONE\n"; break;
      default:
        messaggioDaInviare = "TERMINE PROCESSO CLIENT\n";
        chiudi_sessione = 1;
        break;
    }

    int messLenght = strlen(messaggioDaInviare);
    
    if (sendto(MySocket, messaggioDaInviare, messLenght, 0, (struct sockaddr*)&clientAd, clientLen) != messLenght) {
      printf("\nErrore sendto (impossibile rispondere al client)\n");
    } else {
      printf("\nRisposta inviata: %s", messaggioDaInviare);
    }

    if (chiudi_sessione) {
        printf("\nSessione chiusa, attendo prossimo comando...\n");
        continue; 
    }

    float n1, n2;
    float result;

    printf("Attendo i numeri...\n");

    clientLen = sizeof(clientAd);
    if (recvfrom(MySocket, (char*)&n1, sizeof(float), 0, (struct sockaddr*)&clientAd, &clientLen) <= 0) {
        printf("Errore ricezione N1\n"); continue;
    }
    int n1_bits = ntohl(*(int*)&n1);
    n1 = *(float*)&n1_bits;

    clientLen = sizeof(clientAd);
    if (recvfrom(MySocket, (char*)&n2, sizeof(float), 0, (struct sockaddr*)&clientAd, &clientLen) <= 0) {
        printf("Errore ricezione N2\n"); continue;
    }
    int n2_bits = ntohl(*(int*)&n2);
    n2 = *(float*)&n2_bits;

    printf("Numeri: %.2f, %.2f\n", n1, n2);

    if (opzioneScelta == 'a') result = n1 + n2;
    else if (opzioneScelta == 's') result = n1 - n2;
    else if (opzioneScelta == 'm') result = n1 * n2;
    else if (opzioneScelta == 'd') {
      if (n2 != 0) result = n1 / n2;
      else result = 0.0f;
    }

    int result_bits = htonl(*(int*)&result);
    float result_net = *(float*)&result_bits;

    sendto(MySocket, (const char*)&result_net, sizeof(float), 0, (struct sockaddr*)&clientAd, clientLen);
    printf("\nRisultato inviato.\n");
  }
}
