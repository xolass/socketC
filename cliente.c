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

int main(int argc, char const *argv[]) {
	int porta, connector;
	ssize_t ler_bytes, escrever_bytes;  
	int client_socket;
	struct sockaddr_in serv_addr;
	char str[4096];

	if(argc < 3){ //Checa se na execução do programa vc colocou ./cliente <ip> <porta>
		printf("Uso correto: endereco IP - porta\n");
		exit(1);
	}

	client_socket = socket(AF_INET, SOCK_STREAM, 0); //Cria uma conexão socket no client_socket
    //AF_INET = ipv4, SOCK_STREAM = TCP(SOCK_DGRAM = UDP), 0 padrão

	if(client_socket <= 0){ //A conexão é vista como um número inteiro, 0 ou negativo, conexão invalida
		printf("Erro no socket: %s\n", strerror(errno));
		exit(1);
	}

	bzero(&serv_addr, sizeof(serv_addr)); //da memset no valor 0 em todo serv_addr
	porta = atoi(argv[2]);//Armazena na variavel porta o 2º argumento colocado na execução do programa atoi string to integer
	serv_addr.sin_family = AF_INET; //Termo que guarda o tipo de conexão (tcp,udp)
	serv_addr.sin_port = htons(porta); //Guarda a porta a se checar

	connector = connect(client_socket, (const struct sockaddr*) &serv_addr, sizeof(serv_addr));
    //Recebe o socket, a struct com as informações do server e o tamanho da struct com as informações do server
    //Essa função conecta cliente ao servidor e retorna negativo se a conexão falhou
    if(connector < 0){
		fprintf(stderr, "%s", "Falha na conecao\n");
		exit(1);
	}else{
		printf("Conectado com: %s\n", argv[1]);
	}

	printf("Mensagem: ");
	fgets(str, sizeof(str), stdin); //lê mensagem do terminal e armazena em str
	escrever_bytes = write(client_socket, str, sizeof(str)); //envia pelo client_socket o str
	if(escrever_bytes == 0){    //se vc n enviar nada
		printf("Erro no write: %s\n",strerror(errno));
		printf("Nada escrito.\n");
		exit(1);
	}

	close(client_socket); //Fecha a conexão socket

	return 0;
}