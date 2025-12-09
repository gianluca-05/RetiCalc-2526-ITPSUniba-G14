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

#define PROTOPORT 27015
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

  int clientSocket;
  struct sockaddr_in serverAdd;
  struct hostent* ptrh;
  char host_name[BUFFERSIZE];

  clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (clientSocket < 0) ErrorHandler("Creazione socket fallita");

  printf("\nInserisci il nome del server (es. localhost): ");
  scanf("%s", host_name);

  ptrh = gethostbyname(host_name);
  if (ptrh == NULL){
    ErrorHandler("\nNome host non valido\n");
  }

  memset(&serverAdd, 0, sizeof(serverAdd));
  serverAdd.sin_family = AF_INET;
  serverAdd.sin_port = htons(PROTOPORT);
  memcpy(&serverAdd.sin_addr, ptrh->h_addr, ptrh->h_length);

  printf("\nTentativo di connessione a %s (%s)...\n", host_name, inet_ntoa(serverAdd.sin_addr));
  if (connect(clientSocket, (struct sockaddr*)&serverAdd, sizeof(serverAdd)) < 0) {
    ErrorHandler("\nConnessione fallita");
  }else {
    printf("\nConnessione stabilita!\n");
  }

  int messageComplete = 0;  // Flag: 0 = Falso, 1 = Vero
  char buf[BUFFERSIZE];     // Buffer dove salvo i dati
  int bytesRcvd;            // Numero di byte ricevuti per ogni ciclo

  printf("\nIn attesa di messaggio dal server... ");
  printf("\nIl server dice:  ");

  while (!messageComplete) {
    // lascio 1 byte libero per il terminatore \0)
    bytesRcvd = recv(clientSocket, buf, BUFFERSIZE - 1, 0);

    if (bytesRcvd < 0) {
      ErrorHandler("\nErrore nella recezzione dei dati.\n");
    } else if (bytesRcvd == 0) {
      printf("\nIl server ha chiuso la connessione.\n");
      break;  
    }

    buf[bytesRcvd] = '\0';

    printf("%s", buf);  


    if (strchr(buf, '\n') != NULL) {  
      messageComplete = 1;  
    }
  }

  printf(
      "\nScegliere un operazione:\n A/a: Addizione\n S/s: Sottrazione\n M/m: "
      "moltiplicazione\n D/d: Divisione\n Qualsiasi carettere diverso: "
      "Terminazione client\nInserici una lettera: ");
  char operazione;
  scanf(" %c", &operazione);

  if (send(clientSocket, &operazione, 1, 0) != 1) {
    ErrorHandler("\nErrore nell'invio del carattere");
  }

  messageComplete = 0; 
  memset(buf, 0, BUFFERSIZE);  
  bytesRcvd = 0;    

  while (!messageComplete) {

    bytesRcvd = recv(clientSocket, buf, BUFFERSIZE - 1, 0);

    if (bytesRcvd < 0) {
      ErrorHandler("\nErrore nella recezzione dei dati.\n");
    } else if (bytesRcvd == 0) {
      printf("\nIl server ha chiuso la connessione.\n");
      break;
    }

    buf[bytesRcvd] = '\0';

    printf("\nOperazione scelta: ");  

    printf("%s", buf); 

    if (strchr(buf, '\n') != NULL) {
      messageComplete = 1;
    }
  }


  if (strcmp(buf, "ADDIZIONE\n") == 0 || strcmp(buf, "SOTTRAZIONE\n") == 0 || strcmp(buf, "MOLTIPLICAZIONE\n") == 0 || strcmp(buf, "DIVISIONE\n") == 0) {
    float n1, n2;

    printf("\nInserisci il primo numero: ");
    scanf("%f", &n1);
    int n1_bits = htonl(*(int*)&n1);
    if (send(clientSocket, (const char*)&n1_bits, sizeof(int), 0) != sizeof(int)) { 
    ErrorHandler("\nErrore nell'invio del primo numero");
    }

    printf("Inserisci il secondo numero: ");
    scanf("%f", &n2);
    int n2_bits = htonl(*(int*)&n2); 
    if (send(clientSocket, (const char*)&n2_bits, sizeof(int), 0) != sizeof(int)) {
    ErrorHandler("\nErrore nell'invio del secondo numero");
    }
  
  }else {
    printf("Chiusura del client come richiesto dal server.\n");
    closesocket(clientSocket);
    ClearWinSock();
    return 0;
  }

   int bytes_rcvd;
   float result;

    if ((bytes_rcvd = recv(clientSocket, (char*)&result, sizeof(float), 0)) <= 0) {
      ErrorHandler("recv() failed.\n");
      closesocket(clientSocket);
      ClearWinSock();
      return -1;
    }

    int result_bits = ntohl(*(int*)&result);
    result = *(float*)&result_bits;

    if (result == 0.0f && bytes_rcvd > 0 && strcmp(buf, "DIVISIONE\n") == 0) {
      printf("\nDivisione per zero, impossibile!!!.\n");
    } else {  
    printf("\nIl risultato dell'operazione e': %.2f\n", result);
}

  closesocket(clientSocket);
  ClearWinSock();
  return 0;
}
