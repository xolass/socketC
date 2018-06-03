/*Compilar - gcc servidorudp.c -o servidorudp
  Executar - ./servidorudp
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close() */
#include <string.h> /* memset() */
#include <stdlib.h>

#define LOCAL_SERVER_PORT 1500
#define MAX_MSG 100
#define SizeBuffer 1024

int checkcheck(char buffer[], int tamanho){

  int i;
  char check=0;
  for(i=0 ; i < tamanho ; i++){

    check += buffer[i];
    check &= 0xFF;

  }
  printf("Verificando Checksum...\n");
  printf("Checksum recebido: 0x%x\n",buffer[tamanho-1]);
  printf("Checksum calculado: 0x%x\n",check);

  if(buffer[tamanho-1] == (check)){
      printf("Pacote não violado!\n");
      return 1;
  }
    
  else{
      printf("Pacote violado!\n");
      return 0;
  }
    

}

int main(int argc, char *argv[]) {
  ssize_t bytesRecebidos, resposta;
  int sd, rc, n, cliLen, numeroPacotesRecebidos;
  struct sockaddr_in cliAddr, servAddr;
  char  nomeArquivo[MAX_MSG], bufferEntrada[SizeBuffer], bufferResposta[2];


  
//=====================================================================================================================================
// Inicia os sockets configura a conexão
//=====================================================================================================================================
  /* socket creation */
  sd=socket(AF_INET, SOCK_DGRAM, 0);
  if(sd<0) {
    printf("%s: cannot open socket \n",argv[0]);
    exit(1);
  }

  /* bind local server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(LOCAL_SERVER_PORT);
  rc = bind (sd, (struct sockaddr *) &servAddr,sizeof(servAddr));

  if(rc<0) {
    printf("%s: cannot bind port number %d \n", 
	   argv[0], LOCAL_SERVER_PORT);
    exit(1);
  }

  printf("%s: waiting for data on port UDP %u\n", 
	   argv[0],LOCAL_SERVER_PORT);

//=====================================================================================================================================
// Espera por alguma transferencia de arquivo
//=====================================================================================================================================
  /* server infinite loop */
  while(1) {
    
    /* init buffer */
    memset(nomeArquivo,0x0,MAX_MSG);
    /* recebe nome do arquivo */
    cliLen = sizeof(cliAddr);
    n = recvfrom(sd, &nomeArquivo, MAX_MSG, 0,(struct sockaddr *) &cliAddr, &cliLen);

    if(n<0) {
      printf("%s: cannot receive data \n",argv[0]);
      continue;
    }
    
    /* print received message */
    printf("%s: from %s:UDP%u : %s \n", argv[0],inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port),nomeArquivo);

    /*Crinado Arquivo...*/
    FILE *novoArquivo;
    novoArquivo = fopen(nomeArquivo,"wb");
    numeroPacotesRecebidos = 0;
    char numeroPacote = 0;
    // recebe os pacotes
    while(1){
      bytesRecebidos = recvfrom(sd, &bufferEntrada, SizeBuffer+3, 0,(struct sockaddr *) &cliAddr, &cliLen);
      
      // verifica se recebeu
      if(bytesRecebidos<0) {
        printf("%s: cannot receive data \n",argv[0]);
        bufferResposta[0] = 2;
        bufferResposta[1] = numeroPacote;
        resposta = sendto(sd, bufferResposta, 2, 0,(struct sockaddr *) &cliAddr, sizeof(cliAddr));
        continue;
      }
      printf("\n\nX--------------------------------------------------------X\n");
      printf("Pacote Recebido!\n");
      printf("Tamanho:  %li\n",bytesRecebidos);
      printf("Tipo: %d\n",bufferEntrada[bytesRecebidos-3]);
      printf("Numero Sequencia: %d\n",bufferEntrada[bytesRecebidos-2]);
      printf("Checksum: 0x%x\n",bufferEntrada[bytesRecebidos-1]);
      printf("X--------------------------------------------------------X\n\n");
      
      // verifica se o pacote é duplicado , caso seja reenvia o ack
      if(numeroPacote > bufferEntrada[bytesRecebidos-2] || (numeroPacote == 0 && bufferEntrada[bytesRecebidos-2] == 127)){
        printf("PACOTE DUPLICADO, foi descartado\n");
        printf("ACK ENVIADO!!!\n");
        bufferResposta[0] = 1;
        bufferResposta[1] = numeroPacote;
        resposta = sendto(sd, bufferResposta, 2, 0,(struct sockaddr *) &cliAddr, sizeof(cliAddr));
        continue;
      }
      //verifica o checksum
      else if(!checkcheck(bufferEntrada, bytesRecebidos || numeroPacote < bufferEntrada[bytesRecebidos-2])){
        
        printf("NAK N = %d ENVIADO!!!\n",numeroPacote);

        bufferResposta[0] = 2;
        bufferResposta[1] = numeroPacote;
        resposta = sendto(sd, bufferResposta, 2, 0,(struct sockaddr *) &cliAddr, sizeof(cliAddr));
        continue;
      }
      else{
        printf("ACK N = %d ENVIADO!!!\n",numeroPacote);
        bufferResposta[0] = 1;
        bufferResposta[1] = numeroPacote;
        resposta = sendto(sd, bufferResposta, 2, 0,(struct sockaddr *) &cliAddr, sizeof(cliAddr));
      }

      // verifica se é o pacote de finalização de tranferencia
      if(strcmp(bufferEntrada,"X----[FIM_DO_ARQUIVO]----X")==0){
        printf("%s: Recebimento Concluido, numero de Pacotes Recebidos: %d\n",argv[0],numeroPacotesRecebidos);
        fseek(novoArquivo,0,SEEK_END);
        int tamanhoArquivo = ftell(novoArquivo);
        printf("%s: Tamanho do arquivo: %d Bytes\n",argv[0],tamanhoArquivo);
        fclose(novoArquivo);
        break;
      }
      
      printf("%s:Tamanho do Pacote Recebido: %lu\n",argv[0],bytesRecebidos);
      fwrite(&bufferEntrada,sizeof(char),bytesRecebidos-3,novoArquivo);
      memset(bufferEntrada,0x0,SizeBuffer);
      numeroPacotesRecebidos++;
      numeroPacote++;
      numeroPacote %= 128;
    }
  }/* end of server infinite loop */  

return 0;

}

