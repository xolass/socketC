/*Compilar - gcc clienteudp.c -o clienteudp
  Executar - ./clienteudp 127.0.0.1 NOME_DO_ARQUIVO
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h> /* memset() */
#include <sys/time.h> /* select() */ 
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define REMOTE_SERVER_PORT 1500
#define MAX_MSG 100
#define SizeBuffer 1024
#define TEMPOTEPORIZADOR 1


int timer, sd, recebeu,limiteTempo, transferenciaCompleta;
char bufferResposta[2];
struct sockaddr_in remoteServAddr;

// Função que gera a soma de verificação
char doChecksum(char data[], int tamanho){

  int i;
  char checksum = 0;
  for(i = 0 ; i<tamanho ; i++){

    checksum += data[i];
    checksum &= 0xFF;
  }
  return checksum;
}


//função do temporizador, será executada em paralelo
void *timerFun(){
    sleep(TEMPOTEPORIZADOR);
    limiteTempo = 1;
    recebeu = 0;
    timer = 0;
}

// função para cnacelar um thread
int pthread_cancel(pthread_t thread);


// função que espera epla resposta do servidor, será executada em paralelo
void *respostaFunc(){
  ssize_t resposta;
  int serLen = sizeof(remoteServAddr);
  resposta = recvfrom(sd, &bufferResposta, 2, 0,(struct sockaddr *) &remoteServAddr, &serLen);
  recebeu = 1;
  timer = 0;
}


int main(int argc, char *argv[]) {

  pthread_t threadTimer, threadResposta;
  ssize_t bytesEnviados;
  long long tamanhoArquivo, quantidadeBytesEnviados=0;
  int retornoThread, retornoThread2;
  int rc, numeroPacotesEnviados = 0;
  struct sockaddr_in cliAddr;
  struct hostent *h;
  char bufferEnvio[SizeBuffer + 3];
  transferenciaCompleta = 0;
  
  /* check command line args */
  if(argc<3) {
    printf("usage : %s <server> <data1> ... <dataN> \n", argv[0]);
    exit(1);
  }
//=====================================================================================================================================
// Inicia os sockets configura a conexão
//=====================================================================================================================================
/* get server IP address (no check if input is IP address or DNS name */
  h = gethostbyname(argv[1]);
  if(h==NULL) {
    printf("%s: unknown host '%s' \n", argv[0], argv[1]);
    exit(1);
  }

  printf("%s: sending data to '%s' (IP : %s) \n", argv[0], h->h_name,
   inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));

  remoteServAddr.sin_family = h->h_addrtype;
  memcpy((char *) &remoteServAddr.sin_addr.s_addr, 
   h->h_addr_list[0], h->h_length);
  remoteServAddr.sin_port = htons(REMOTE_SERVER_PORT);


  /* socket creation */
  sd = socket(AF_INET,SOCK_DGRAM,0);
  if(sd<0) {
    printf("%s: cannot open socket \n",argv[0]);
    exit(1);
  }
  

  /* bind any port */
  cliAddr.sin_family = AF_INET;
  cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  cliAddr.sin_port = htons(0);

  
  rc = bind(sd, (struct sockaddr *) &cliAddr, sizeof(cliAddr));
  if(rc<0) {
    printf("%s: cannot bind port\n", argv[0]);
    exit(1);
  }

//=====================================================================================================================================
// Abre o arquivo a ser enviado
//=====================================================================================================================================  
  
  

  /*Abrindo arquivo*/
  FILE *arquivo;
  arquivo = fopen(argv[2],"rb");

  if(!arquivo){
    printf("%s: Nao foi possivel abrir o arquivo!\n",argv[0]);
    exit(1);
  }
  else{
    printf("%s: Arquivo Aberto com sucesso!\n",argv[0]);
  }

  //determina tamnho do arquivo
  fseek(arquivo,0,SEEK_END);
  tamanhoArquivo = ftell(arquivo);
  fseek(arquivo,0,SEEK_SET);

  printf("%s: Tamanho do Arquivo: %.2lf MegaBytes\n",argv[0],((tamanhoArquivo)/(1024.0))/1024.0);
  

//=====================================================================================================================================
// Envia o nome do arquivo para o servidor
//=====================================================================================================================================  
  
  rc = sendto(sd, argv[2], strlen(argv[2])+1, 0, (struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));

  if(rc<0) {
    printf("%s: cannot send data\n",argv[0]);
    close(sd);
    exit(1);
  }


//=====================================================================================================================================
// Transfere o arquivo
//=====================================================================================================================================  
  
  memset(bufferEnvio,0x0,SizeBuffer);
  char cont = 0;
  int flag = 0, fullORnot = 0;
  int bytesRestantes = 0;
  
  while(1){
    if(transferenciaCompleta) break;
    // verifica se existem bytes sufucientes restando do arquivo para enviar um pacote no tamanho total
    if(quantidadeBytesEnviados+SizeBuffer <= tamanhoArquivo){

      //lê do arquivo
      fread(&bufferEnvio, SizeBuffer, 1, arquivo);
      
      //adiciona bytes adionais de cabeçalho
      bufferEnvio[SizeBuffer] = 0; //DATA
      bufferEnvio[SizeBuffer+1] = cont;
      bufferEnvio[SizeBuffer+2] = doChecksum(bufferEnvio, SizeBuffer+2);  
    
      //send to retorna o número de bytes enviados
      //Envia o pacote
      bytesEnviados = sendto(sd, bufferEnvio, SizeBuffer+3, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));

      //verifica se conseguiu enviar
      if(bytesEnviados<0) {
        printf("%s: ERROR: 01\n",argv[0]);
        printf("%s: cannot send data\n",argv[0]);
        close(sd);
        exit(1);
      }

      numeroPacotesEnviados++;
      quantidadeBytesEnviados+=(bytesEnviados-3);
      memset(bufferEnvio,0x0,SizeBuffer);
      
    }else{

      bytesRestantes = tamanhoArquivo - quantidadeBytesEnviados;
      memset(bufferEnvio,0x0,SizeBuffer);
      
      //lê do arquivo
      fread(&bufferEnvio, 1, bytesRestantes, arquivo);
      
      //adiciona bytes adionais de cabeçalho
      bufferEnvio[bytesRestantes] = 0; //DATA
      bufferEnvio[bytesRestantes+1] = cont;
      bufferEnvio[bytesRestantes+2] = doChecksum(bufferEnvio, bytesRestantes+2);
      
      //send to retorna o número de bytes enviados
      //Envia o pacote
      bytesEnviados = sendto(sd, bufferEnvio, bytesRestantes+3, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));

      //flag que verifica se o pacote é menor que o tamanho padrão
      fullORnot = 1;
      if(bytesEnviados<0) {
        printf("%s: ERROR: 02\n",argv[0]);
        printf("%s: cannot send data\n",argv[0]);
        close(sd);
        exit(1);
      }
      numeroPacotesEnviados++;
      quantidadeBytesEnviados+=(bytesEnviados-3);
    }
    
    //espera pelo ack, nck ou temporizador estourar
    while(1){
      
      printf("X--------------------------------------------------------X\n");
      printf("Pacote Enviado!\n");
      printf("Tamanho:  %li\n",bytesEnviados);
      printf("Tipo: %d\n",bufferEnvio[bytesEnviados-3]);
      printf("Numero Sequencia: %d\n",bufferEnvio[bytesEnviados-2]);
      printf("Checksum: 0x%x\n\n",bufferEnvio[bytesEnviados-1]);
      printf("Envio: %lld/%lld\n",quantidadeBytesEnviados,tamanhoArquivo);
      printf("X--------------------------------------------------------X\n\n");

      timer = 1; //Segura o programa principal
      
      recebeu = 0;//Indica se o servidor recebeu resposta
      
      limiteTempo = 0;//Indica  se o temporizador estourou ou não
      
      // cria as threads de timer e receber resposta
      retornoThread = pthread_create( &threadTimer, NULL, timerFun, NULL);
      retornoThread2 = pthread_create( &threadResposta, NULL, respostaFunc, NULL);

      
      while(timer){
          //espera....
          
      }
      
      
        pthread_cancel(threadResposta);
        pthread_cancel(threadTimer);

      // se o pacote chegou inteiro e se o pacote chegou
      if(bufferResposta[0] == 1 && recebeu == 1){
        cont ++;
        cont %= 128; 
        printf("ACK RECEBIDO COM NUMERO DE SEQUENCIA: %d\n", bufferResposta[1]);
        if(fullORnot){// ultimos dados do arquivo
            transferenciaCompleta =1;
        }
        break;
        
      }      
        else{
          
          if(limiteTempo)
            printf("\nESTOURO DE TEMPORIZADOR\n");
          else
            printf("NAK RECEBIDO COM NUMERO DE SEQUENCIA: %d\n", bufferResposta[1]);
          // verifica se o pacote que esta sendo reenvidado possui o tamanho padrão ou é menor.
          if(fullORnot == 0){
            bytesEnviados = sendto(sd, bufferEnvio, SizeBuffer+3, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
          }
          else{
            bytesEnviados = sendto(sd, bufferEnvio, bytesRestantes+3, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
          }
          printf("\n\nPACOTE %d REENVIADO!\n\n", bufferResposta[1]);
        }
    }
  }

  
//=====================================================================================================================================
// Finaliza a transferencia
//=====================================================================================================================================  
  
  do{
    
    memset(bufferEnvio,0x0,SizeBuffer);
    strcpy(bufferEnvio, "X----[FIM_DO_ARQUIVO]----X");
    int tam = strlen(bufferEnvio);
    bufferEnvio[tam] = 0;
    bufferEnvio[tam+1] = (cont+1)%128;
    bufferEnvio[tam+2] = doChecksum(bufferEnvio, tam+2);
    
    bytesEnviados = sendto(sd, bufferEnvio, strlen(bufferEnvio)+3, 0,(struct sockaddr *) &remoteServAddr, sizeof(remoteServAddr));
    
    printf("X--------------------------------------------------------X\n");
    printf("Pacote de finalização de transferencia!\n");
    printf("X--------------------------------------------------------X\n");

    timer = 1;
    recebeu = 0;
    limiteTempo = 0;
    retornoThread = pthread_create( &threadTimer, NULL, timerFun, NULL);
    
    retornoThread2 = pthread_create( &threadResposta, NULL, respostaFunc, NULL);
    
    //printf("retorno da thread: %d\n", retornoThread);
    while(timer){
        //espera....
        
    }

    pthread_cancel(threadResposta);
    pthread_cancel(threadTimer);


    if(bufferResposta[0] == 1 && recebeu == 1){
        break;
    } 
    else{
      if(limiteTempo)
        printf("\nESTOURO DE TEMPORIZADOR\n");
      else
        printf("\nNAK RECEBIDO\n");
      printf("Pacote %d reenviado\n\n", bufferResposta[1]);
    }
      
  }while(1);
  
  
  printf("Envio Concluido, numero de Pacotes Enviados: %d\n",numeroPacotesEnviados);
  fclose(arquivo);
  return 1;

}

