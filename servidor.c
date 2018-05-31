#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char const *argv[]) {

	int server_socket, binder, listener, porta, sock; 
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t clilen;


//***************************************************************
//					  ABERTURA DE CONEXÃO
//***************************************************************
	if(argc < 3){
		printf("Uso correto: endereco IP - porta\n");
		exit(1);
	}

	server_socket = socket(AF_INET, SOCK_STREAM, 0); 

	if(server_socket <= 0){
		printf("Erro na abertura do socket: %s\n", strerror(errno));
		exit(1);
	}
	else if(server_socket){
		do{
			printf("Aguardando cliente...\n");
		}while(!accept);
	}

	bzero(&serv_addr, sizeof(serv_addr));


	porta = atoi(argv[2]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(porta);

	binder = bind(server_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if(binder < 0){
		printf("Erro no Bind: %s\n", strerror(errno));
		exit(1);
	}

	listener = listen(server_socket, 20);

	if(listener < 0){
		printf("Erro no Listen: %s\n", strerror(errno));
		exit(1);
	}

	clilen = sizeof(cli_addr);

	sock = accept(server_socket, (struct sockaddr*) &cli_addr, &clilen);

	if(sock <= 0)
		printf("Erro no accept: %s\n", strerror(errno));
	else
		printf("Conexao recebida de %s\n", inet_ntoa(cli_addr.sin_addr));

//***************************************************************
//					  FIM ABERTURA DE CONEXÃO
//***************************************************************
	FILE *file;
	int nPacote = 0;
	char str[4096];
	int pacote[4099];
	ssize_t ler_bytes;
	ssize_t escrever;

//***************************************************************
//					   REQUISIÇÃO DE ARQUIVO
//***************************************************************
	ler_bytes = read(sock, str, sizeof(str));
	if(ler_bytes <= 0){
		printf("Erro no read: %s\n", strerror(errno));
		exit(1);
	}

	//Agora, str tem o nome do arquivo
	file = fopen(str,"rb");
	if(!file) {
		char msg[25] = "Arquivo nao existe\n"; 
		write(sock,msg,sizeof(msg));
		exit(1);
	}
	
	
	memset(pacote,0,sizeof pacote);
	while(pacote[4098] == 0) { //LOOP DE ESCREVER E ENVIAR PACOTES
		fread(pacote,4095*32,1,file);
		pacote[4096] = 1; //Campo de verificação 1 (checksum)
		pacote[4097] = 1; //Campo de verificação 2 (nº de sequencia)
		pacote[4098] = 0; //Campo de verificação 3 (temporizador)

		while(1) {
			escrever = write(sock,pacote,sizeof(pacote));

			if(escrever <= 0) 
				printf("Erro ao escrever, tentando novamente\n");
			else {
				nPacote++;
				break;
			}
			
		}

	}

	fclose(file);
	
	close(sock);
	close(server_socket);

	return 0;
}