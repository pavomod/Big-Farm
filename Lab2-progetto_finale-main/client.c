#include "library.h"
#define HOST "127.0.0.1"
#define PORT 65432

void comunicazione(int mode,long num)
{
	
	int fd_skt = 0;      // file descriptor associato al socket
  struct sockaddr_in serv;
	size_t e;
	

	if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) //creazione socket 
    termina("Errore creazione socket");
	//setting socket
	serv.sin_family = AF_INET;
  serv.sin_port = htons(PORT);
  serv.sin_addr.s_addr = inet_addr(HOST);
	
	if (connect(fd_skt, (struct sockaddr*)&serv, sizeof(serv)) < 0)  //apertura socket
    termina("Errore apertura connessione");

	if(mode==-1) //se ricevo -1 come modalità vuol dire che devo chiedere la stampa di tutte le coppie 
	{
		int dim=-1; //invio una dimensione negativa (impossibile avere una stringa lunga -1 nemmeno per errore dato strlen restitusice un size_t)
		
		dim=htonl(dim); //pack della dimensione
		e = writen(fd_skt,&dim,sizeof(int)); //invio la dimensione che segnala di stampare tutte le coppie
  	if(e!=sizeof(int)) 
			termina("Errore write");

		
	}
	else //chiedo una o più coppie che abbiano la sommatoria richiesta
	{
		char str[20]; 
		sprintf(str, "%ld", num); //converto in string il long
		int dim=htonl(strlen(str)); //pack della dimensione che indica il numero di byte da leggere 
		
		e = writen(fd_skt,&dim,sizeof(int)); //invio la dimensione
  	if(e!=sizeof(int)) 
			termina("Errore write");
		
		for(int i=0;i< strlen(str);i++) //invio carattere per carattere la stringa (interi in realtà)
		{
			int s=htonl(str[i]);
		 	e = writen(fd_skt,&s,sizeof(int));
	  	if(e!=sizeof(int)) 
				termina("Errore write");

		}
	}

	
	
	if(close(fd_skt)<0) //termino la connessione
    perror("Errore chiusura socket");
	
}



int main(int argc, char *argv[])
{
	if(argc==1) //se non ricevo parametri (oltre "client")
	{
		comunicazione(-1,0); //chiedo solo la stampa
	}
	else
	{
		for(int i=1;i<argc;i++) //richiedo tutte le coppie che abbiano sommatoria uguale a quelle richieste come parametro
		{
			comunicazione(1,atol(argv[i]));
		}
	}
}