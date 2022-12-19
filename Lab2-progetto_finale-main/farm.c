#include "library.h"

#define QUI __LINE__, __FILE__
#define HOST "127.0.0.1"
#define PORT 65432
// local host e porta non nota, così da evitare conflitti

// tutti i thread condividono il buffer, indice in cui effettuare la prossima
// lettura,numero di dati ancora da consumare, numero di celle libere in cui il
// produttore può inserire i dati da consumare, mutex per sincronizzare , 2
// condition variables per sapere quando ci sono dati da consumare o è possibile
// produrre ulteriori dati.

typedef struct {
  char **buf;
  int size;
  int *idx;
  int *dati;
  int *freeblock;
  pthread_mutex_t *mutex;
  pthread_cond_t *cpadre;
  pthread_cond_t *cthread;
} dati;

volatile sig_atomic_t segnale = 0; // volatile cosi che il compilatore non possa effettuare ottimizzazioni,
       // sig_atomic_t crea un intero in cui le operazioni su di esso sono
       // atomiche, cioè arrivano a concludersi anche se vi è una system call.

void handl(int s) // gestore del segnale SIG_INT indica al thread principale di non produrre altri dati e di attendere che vengano consumati quelli già prodotti
{
  segnale = 1;
}

void comunicazione(char *messaggio, long num) // comunicazione via socket
{
  int fd_skt = 0; // file descriptor associato al socket
  struct sockaddr_in serv;
  size_t e;
  char *str=malloc(sizeof(char)*23);
	char *buf=malloc(sizeof(char)*strlen(messaggio)+23);
	strcpy(buf,"");
  // casting di un long in stringa, e concateno il nome del file con la somma,
  // separati dal carattere '/' che non può essere inserito nel nome dei file.
  
	sprintf(str, "%ld", num);
	strcat(buf,messaggio);
	strcat(buf,"/");
  strcat(buf, str);
	
  int invio[strlen(buf)];
  // trasformo in interi tutti i caratteri della stringa da inviare, così da
  // poterli inviare via socket
  for (int i = 0; i < strlen(buf); i++) {
    invio[i] = (int)buf[i]; // cast char in int
  }

  int dim = htonl(strlen(buf)); // numero di byte che il server deve leggere.

  if ((fd_skt = socket(AF_INET, SOCK_STREAM, 0)) < 0) // creazione socket
    termina("Errore creazione socket");
  // setting delle info relative al socket, come indirizzo server, porta ecc..
  serv.sin_family = AF_INET;
  serv.sin_port = htons(PORT);
  serv.sin_addr.s_addr = inet_addr(HOST);

  if (connect(fd_skt, (struct sockaddr *)&serv, sizeof(serv)) <
      0) // apertura connessione
    termina("Errore apertura connessione");

  e = writen(fd_skt, &dim,sizeof(int)); // invio n byte tramite socket, sto inviando la
                           // dimensione della stringa che dovrà ricevere
  if (e != sizeof(int))
    termina("Errore invio dati al server");

  for (int i = 0; i < strlen(buf);i++) // invio carattere per carattere (castati ad int per poter
            // utilizzare le socket)
  {

    int s = htonl(invio[i]); // pack di ogni singolo carattere (intero)
    e = writen(fd_skt, &s, sizeof(int)); // invio dati
    if (e != sizeof(int))
      termina("Errore write");
  }

  if (close(fd_skt) < 0) // interrompo la comunicazione
    perror("Errore chiusura socket");

	free(str);
	free(buf);
}

void *tbody(void *arg) // consumatori
{
  dati *d = (dati *)arg;

  int conta = 0;
  long sum = 0;

  while (true) {

    xpthread_mutex_lock(d->mutex, QUI); // acquisisco la mutex

    while (*(d->dati) ==0) 
    	xpthread_cond_wait(d->cthread, d->mutex, QUI);
    *(d->dati) -= 1; // se acquisisco la mutex indico che ci sarà un dato in
                     // meno da elaborare


    if (strcmp(d->buf[*(d->idx) % d->size], "") ==0) // se ricevo una stringa vuota vuol dire che devo terminare
    {
      *(d->idx) += 1;       // prossima cella da consumare
      *(d->freeblock) += 1; // una cella disponibile per il produttore
      xpthread_cond_signal(d->cpadre, QUI); // segnale al thread principale
      xpthread_mutex_unlock(d->mutex, QUI); // rilascio la mutex
      break;                                // esco e termino
    }

    else // se non ho ricevuto la stringa di terminazione
    {
      FILE *f = fopen(d->buf[*(d->idx) % d->size], "rb"); // apro il file in lettura binaria
      if (f == NULL) // se ci sono problemi nell'apertura segnalo l'errore
        fprintf(stderr, "[ %s ] errore apertura file\n\n",d->buf[*(d->idx) % d->size]);
      else // se il file è stato aperto correttamente
      {
        long x;
        while (true) // leggi in loop
        {
          int e = fread(&x, sizeof(long), 1, f); // leggi un long alla volta
          if (e == -1)                           // se non riesci a leggere
            termina("\nerrore fread\n");
          if (e != 1) // arrivato a EOF
            break;    // termina le letture

          sum +=conta * x; // sommatoria di -> il valore letto * la sua posizione nel file
          conta++; // prossima posizione
        }

        // inivio al server il dato elaborato
        comunicazione(d->buf[*(d->idx) % d->size], sum);
        fclose(f); // chiudo il file
      }
      
      conta = 0; // azzero per il prossimo file da leggere
      sum = 0;   // azzero per il prossimo file da leggere

    }

    *(d->idx) += 1; // prossima posizione da consumare
    *(d->freeblock) +=1; // indico che è stato consumato un dato e vi è una cella liberare
    xpthread_cond_signal(d->cpadre, QUI); // segnale al thread principale
    xpthread_mutex_unlock(d->mutex, QUI); // rilascio la mutex
  }

  pthread_exit(NULL); // ciao thread
}

// gestione dei parametri opzionali passati da console

int gestione_option(int argc, char **argv, int *nthread, int *delay,int *qlen) {
  // gestisco parametri opzionali considero come parametri opzionali quelli
  // passati come terzo parametro in getopt, separandoli con ":". i para seguiti
  // dal parametro opzionale vengono salvat in optarg.

  int opzione; // se va a -1 sono terminati i parametri opzionali
  int conta_opt = 0;
  while ((opzione = getopt(argc, argv, "n:q:t:")) != -1) {
    switch (opzione) {
    case 'n':
      conta_opt++;
      // mi assicuro che sia un parametro numerico o che
      // comunque ci sia un parametro
      if (atoi(optarg) != 0)
        if (atoi(optarg) > 0) // ignoro parametro negativo
        {
          conta_opt++;
          *nthread = atoi(optarg);
        }
      // se non ho contato un parametro non accettabile
      if (optarg != NULL && *nthread == 4 && atoi(optarg) != 4)
        conta_opt++;

      break;
    case 'q':
      conta_opt++;
      if (atoi(optarg) != 0)
        if (atoi(optarg) > 0) // ignoro parametri negativi
        {
          conta_opt++;
          *qlen = atoi(optarg);
        }
      // se non ho contato un parametro non accettabile
      if (optarg != NULL && *qlen == 8 && atoi(optarg) != 8)
        conta_opt++;
      break;
    case 't':
      conta_opt++; // parametro opzionale
      if (atoi(optarg) != 0)
        if (atoi(optarg) > 0) // ignoro valori negativi
        {
          conta_opt++; // parametro numerico ricevuto
          *delay = atoi(optarg);
        }
      // se non ho contato un parametro non accettabile
      if (optarg != NULL && delay == 0 && atoi(optarg) != 0) // se c'è qualcosa che non accetto conta
        conta_opt++;
      break;
    }
  }
  return conta_opt;
}

int main(int argc, char *argv[]) {
  // controlla numero argomenti
  if (argc < 2) {
    printf("Uso: %s file [file ...] \n", argv[0]);
    return 1;
  }

  // gestione valori opzionali
  int nthread = 4; // valori di default
  int qlen = 8;    // valori di default
  int delay = 0;   // valori di default
  int conta_opt = gestione_option(argc, argv, &nthread, &delay, &qlen); // conta da dove iniziano i parametri
                                          // non opzionali (i file da elaborare)

  // creazione thread e inizializzazione struttura
  dati d;
  char *buf[qlen];
  int messi = 0;
  int idx = 0;
  pthread_t t[nthread];
  int freeblock = qlen;
  int dati = 0;
  pthread_cond_t cpadre = PTHREAD_COND_INITIALIZER;
  pthread_cond_t cthread = PTHREAD_COND_INITIALIZER;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  d.buf = buf;
  d.size = qlen;
  d.idx = &idx;
  d.dati = &dati;
  d.freeblock = &freeblock;
  d.cpadre = &cpadre;
  d.cthread = &cthread;
  d.mutex = &mutex;

  for (int i = 0; i < nthread; i++) {
    xpthread_create(&t[i], NULL, &tbody, &d, QUI); // start nthread
  }

  // handler
  struct sigaction sa;
  sa.sa_handler = handl;        // aggiungo il mio handler
  sigaction(SIGINT, NULL, &sa); // rimuovo l'handler di default su sigint
  sigaction(SIGINT, &sa, NULL);

  // scrittura buffer

  // fprintf(stderr,"\n\n=inizio scrittura buffer=\n\n");
  for (int i = conta_opt + 1; i < argc; i++) // per ogni file passato da console
  {
    if (segnale == 1) // se ricevo il segnale sigint smetto di produrre
      break;

    xpthread_mutex_lock(&mutex, QUI); // acquisisco la mutex
    while (freeblock == 0)            // se ci sono celle libere
      xpthread_cond_wait(&cpadre, &mutex,QUI); // acquisisco la mutex se ricevo un segnale
    freeblock -= 1;            // una cella libera in meno

    buf[messi % qlen] = argv[i]; // inserisco il nome del file nel buffer

    messi += 1; // prossima posizione in cui scrivere
    dati += 1;  // un dato in più da consumare
    xpthread_cond_signal(&cthread,QUI); // segnalo ai thread che ho aggiunto qualcosa
    xpthread_mutex_unlock(&mutex, QUI); // rilascio la mutex
    sleep(delay / 1000); // per il debug rallento tutto questo processo
  }

  for (int i = 0; i < nthread; i++) // segnalo ai thread di terminare
  {
    xpthread_mutex_lock(&mutex, QUI); // uguale a prima, ma inserisco la stringa
                                      // vuota che segnala di terminare
    while (freeblock == 0)
      xpthread_cond_wait(&cpadre, &mutex, QUI);
    freeblock -= 1;

    buf[messi % qlen] = "";

    messi += 1;
    dati += 1;
    xpthread_cond_signal(&cthread, QUI);
    xpthread_mutex_unlock(&mutex, QUI);
  }

  // attesa terminazione thread
  // fprintf(stderr,"\n\n=attendo i thread=\n\n");
  for (int i = 0; i < nthread; i++){ // attendo che terminino tutti i thread
    xpthread_join(t[i], NULL, QUI);
	}	

  // pulizia memoria
  //xpthread_mutex_destroy(&mutex, QUI);
  //xpthread_cond_destroy(&cpadre, QUI);
  //xpthread_cond_destroy(&cthread, QUI);

	

  return 0;
}