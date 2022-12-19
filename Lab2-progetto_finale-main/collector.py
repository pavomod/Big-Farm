#! /usr/bin/env python3

import sys, struct, socket, threading,os
from threading import Lock

# host e porta 
HOST = "127.0.0.1"  # (localhost)
PORT = 65432  # (porta non nota)
mutex = Lock()



class ClientThread(threading.Thread):
    def __init__(self,conn,addr,diz): #costruttore thread
        threading.Thread.__init__(self)
        self.conn = conn #inizializzo il socket
        self.addr = addr #indirizzo host
        self.diz=diz
    def run(self): #thread in esecuzione
     # print("====", self.ident, "mi occupo di", self.addr)
      gestisci_connessione(self.conn,self.addr,self.diz) #gestisce ogni singola connessione
     # print("====", self.ident, "ho finito")
	


def main(host=HOST,port=PORT):
  #  creazione socket
  with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s: #s è un alias
    try:  #cattura le eccezioni
      coppie={}
      s.bind((host, port)) #lego host e porta
      s.listen() #mi metto in attesa di un client
      print("\n\n==Server attivo==\n\n")
      while True: #loop
        
        # attendo una richiesta di connessione
        conn, addr = s.accept() #accetto la connessione e salvo indirizzo client 
        t = ClientThread(conn,addr,coppie) #gestisco il client con un thread
        t.start() #avvio il thread
    except KeyboardInterrupt: #in caso di eccezione
      pass
    #print(coppie)
    s.shutdown(socket.SHUT_RDWR) #chiudo il socket
    print("\n\n\nTermino")
    


def gestisci_connessione(conn,addr,coppie):  #eseguito da ogni thread per gestire un client
  with conn:  
    #print(f"Contattato da {addr}\n\n")
    data = recv_all(conn,4) #attendo 4 byte (int)
    dim  = struct.unpack("!i",data[:4])[0] #unpacking del messaggio ricevuto, indica quanti byte sarà lungo il prossimo messaggio
    
    #print(dim)
    if dim!=-1:	#se ho ricevuto una dimensione negativa
      st=""
      for i in range(dim):
        data2 = recv_all(conn,4) #leggo 4 byte alla volta dato che il mio messaggio è castatato ad intero
        val  = struct.unpack("!i",data2[:4])[0] #leggo 4 byte
        st+=chr(val) #concateno i caratteri letti castandoli a char
      #print("ho ricevuto -> "+st+"\n\n")
      if "/" in st: # se nella stringa vi è uno "/" (i file non possono contenere "/", quindi è stato inserito come carattere per segnalare un evento)
        arr=st.split("/") #separo la somma e il nome del file
        if arr[1] not in coppie: #se non è presente nel mio dizionario lo aggiungo come "somma":"nome_file"
          mutex.acquire() #mutua esclusione
          coppie[arr[1]]=arr[0]
          mutex.release()
        else: # se  è già presente questa somma
          mutex.acquire()
          coppie[arr[1]]+=" | "+arr[0] #concateno ai file con stessa somma
          mutex.release()
      else: #se non continee / allora è una richiesta dal client di cercare una somma
        trova(st,conn,coppie) #restituisce coppie con quella somma
		
    else: #se la dimensione è negativa
      stampa(conn,coppie)#richiesta del client di stampare tutte le coppie
    
  #print(f"Terminato con {addr}\n\n")


def trova(stringa,conn,coppie):
  #invio=""
  trovato=0
  mutex.acquire()
  if len(coppie)==0: # se il dizionario è vuoto
    trovato=1
    print("Nessun file") #stampo nessun file
  else: #se ho coppie
    for i in coppie: #cerco tra tutte le coppie quella con chiave uguale alla somma cercata
      if i == stringa: #se la trovo la stampo			
        print(i+" "+coppie[i])
        trovato=1 #l'ho trovata
				
  mutex.release()
  if trovato==0:			#se non la trovo stampo "nessun file"
    print("Nessun file")


	
def stampa(conn,coppie):
  #invio=""
  mutex.acquire()
  if len(coppie)==0: #se il dizionario è vuoto
    print("Nessun file") #stampo nessun file
  else: #se non è vuoto
    l=[]
    for i in coppie:
      l.append(int(i))
    l.sort() #salvo tutte le chiavi in una lista e le ordino

    for i in range(len(coppie)):
      print(str(l[i])+" "+coppie[str(l[i])]) #stampo tutti i valori nel dizionario in ordine crescente
  mutex.release()


def recv_all(conn,n): #legge n byte, equivale alla readn in c
  chunks = b''
  bytes_recd = 0
  while bytes_recd < n: # finchè non abbiamo letto tutti i byte
    chunk = conn.recv(min(n - bytes_recd, 1024))
    if len(chunk) == 0:
      raise RuntimeError("socket connection broken")
    chunks += chunk
    bytes_recd = bytes_recd + len(chunk)
  return chunks
 




if len(sys.argv)==1:
  main()
elif len(sys.argv)==2:
  main(sys.argv[1])
elif len(sys.argv)==3:
  main(sys.argv[1], int(sys.argv[2]))
else:
  print("Uso:\n\t %s [host] [port]" % sys.argv[0])



