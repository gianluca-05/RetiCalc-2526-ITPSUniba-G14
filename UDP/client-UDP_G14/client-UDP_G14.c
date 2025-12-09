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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PROTOPORT 48000
#define BUFFERSIZE 512

void ErrorHandler(char* errorMessage) {
  printf("%s\n", errorMessage);
  exit(EXIT_FAILURE);
}

void ClearWinSock() {
#if defined WIN32
  WSACleanup();
#endif
}

int main() {
#if defined WIN32
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return EXIT_FAILURE;
#endif

  int MySocket; 
  struct sockaddr_in serverAdd;
  struct hostent* ptrh;
  char host_name[BUFFERSIZE];

  MySocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (MySocket < 0) ErrorHandler("Creazione socket fallita");

  printf("\nInserisci il nome del server (es. localhost): ");
  scanf("%s", host_name);

  ptrh = gethostbyname(host_name);
  if (ptrh == NULL) ErrorHandler("\nNome host non valido\n");

  memset(&serverAdd, 0, sizeof(serverAdd));
  serverAdd.sin_family = AF_INET;
  serverAdd.sin_port = htons(PROTOPORT);
  memcpy(&serverAdd.sin_addr, ptrh->h_addr, ptrh->h_length);

  printf(
      "\nScegliere un operazione:\n A/a: Addizione\n S/s: Sottrazione\n M/m: "
      "moltiplicazione\n D/d: Divisione\n Qualsiasi carattere diverso: "
      "Terminazione client\nInserici una lettera: ");
  char operazione;
  scanf(" %c", &operazione);

  if (sendto(MySocket, &operazione, 1, 0, (struct sockaddr*)&serverAdd, sizeof(serverAdd)) != 1) {
    ErrorHandler("\nErrore nell'invio del carattere");
  }

  char buf[BUFFERSIZE];
  int serverLen = sizeof(serverAdd);
  int bytesRcvd = recvfrom(MySocket, buf, BUFFERSIZE - 1, 0, (struct sockaddr*)&serverAdd, &serverLen);

  if (bytesRcvd <= 0) {
      ErrorHandler("Nessuna risposta dal server.");
  }
  
  buf[bytesRcvd] = '\0';
  printf("\nIl server risponde: %s", buf);

  if (strcmp(buf, "ADDIZIONE\n") == 0 || strcmp(buf, "SOTTRAZIONE\n") == 0 || 
      strcmp(buf, "MOLTIPLICAZIONE\n") == 0 || strcmp(buf, "DIVISIONE\n") == 0) {
    
    float n1, n2;

    printf("Inserisci il primo numero: ");
    scanf("%f", &n1);
    int n1_bits = htonl(*(int*)&n1);
    
    sendto(MySocket, (const char*)&n1_bits, sizeof(int), 0, (struct sockaddr*)&serverAdd, sizeof(serverAdd));

    printf("Inserisci il secondo numero: ");
    scanf("%f", &n2);
    int n2_bits = htonl(*(int*)&n2); 
    
    sendto(MySocket, (const char*)&n2_bits, sizeof(int), 0, (struct sockaddr*)&serverAdd, sizeof(serverAdd));
  
  } else {
    printf("Chiusura del client come richiesto dal server.\n");
    closesocket(MySocket);
    ClearWinSock();
    return 0;
  }

  float result;
  serverLen = sizeof(serverAdd);
  if ((bytesRcvd = recvfrom(MySocket, (char*)&result, sizeof(float), 0, (struct sockaddr*)&serverAdd, &serverLen)) <= 0) {
      ErrorHandler("recvfrom() failed.\n");
      closesocket(MySocket);
      ClearWinSock();
      return -1;
  }

  int result_bits = ntohl(*(int*)&result);
  result = *(float*)&result_bits;

  if (result == 0.0f && bytesRcvd > 0 && strcmp(buf, "DIVISIONE\n") == 0) {
     printf("\n(Possibile divisione per zero o risultato zero)\n");
     printf("Il risultato dell'operazione e': %.2f\n", result);
  } else {  
     printf("\nIl risultato dell'operazione e': %.2f\n", result);
  }

  closesocket(MySocket);
  ClearWinSock();
  return 0;
}
