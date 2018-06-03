#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <errno.h>

typedef enum { false, true } bool;

int main(int argc, char const *argv[]) {
	int porta, connector;
	ssize_t ler_bytes, escrever_bytes;  
	int client_socket,binder;
	struct sockaddr_in serv_addr;
	char arqName[200];


	//Checa se na execução do programa vc colocou ./cliente <ip> <porta>
	if(argc < 3) { 
		printf("Uso correto: endereco IP - porta\n");
		exit(1);
	}

	//Cria uma conexão socket no client_socket
	client_socket = socket(AF_INET, SOCK_DGRAM, 0); 

	if(client_socket <= 0){
		printf("Erro no socket: %s\n", strerror(errno));
		exit(1);
	}

	//da memset no valor 0 em todo serv_addr
	bzero(&serv_addr, sizeof(serv_addr)); 
	
	//Armazena na variavel porta o 2º argumento colocado na execução do programa atoi string to integer
	porta = atoi(argv[2]);

	//Termo que guarda o tipo de conexão (ipv4,ipv6...)
	serv_addr.sin_family = AF_INET; 

	//Guarda a porta a se checar
	serv_addr.sin_port = htons(porta); 



    //Recebe o socket, a struct com as informações do server e o tamanho da struct com as informações do server
    //Essa função conecta cliente ao servidor e retorna negativo se a conexão falhou
	binder = bind(client_socket, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if(connector < 0){
		fprintf(stderr, "%s", "Falha na conecao\n");
		exit(1);
	}else{
		printf("Conectado com: %s\n", argv[1]);
	}



//***************************************************************
//			           REQUISIÇÃO DE ARQUIVO
//***************************************************************

	printf("Qual arquivo pedir: ");

	fgets(arqName, sizeof(arqName), stdin); //lê mensagem do terminal e armazena em arqName
	
	escrever_bytes = sendto(client_socket, arqName, sizeof(arqName),0,(const struct sockaddr*) &serv_addr,sizeof(serv_addr)); //envia pelo client_socket o arqName
	if(escrever_bytes == 0) {    //se vc n enviar nada
		printf("Erro no write: %s\n",strerror(errno));
		printf("Nada escrito.\n");
		exit(1);
	}

//***************************************************************
//			         FIM REQUISIÇÃO DE ARQUIVO
//***************************************************************
	FILE *file;
	char pacote[4099];
	int ack[2];
	ssize_t ler;

//***************************************************************
//			           RECEBIMENTO DO ARQUIVO
//***************************************************************
	
	file = fopen(arqName,"wb");
	
	while(1) {
		int servlen = sizeof(serv_addr);
		ler = recvfrom(client_socket,pacote,sizeof(pacote),0,(struct sockaddr*) &serv_addr,&servlen);
		if(ler<=0) {
			break;
		}

		// ack[0] = 1;
		// ack[1] = pacote[];
		// write(client_socket,ack,sizeof(ack));
	}
	
	close(client_socket); //Fecha a conexão socket

	return 0;
}