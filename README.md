# Lab2 Final Project (BIG farm)
Documentation of the final project for the year 2021/22

## = *farm.c* =

### **Gestione_option()**:
This function evaluates the presence of optional parameters and, if any are found, it modifies the corresponding variable values (after validating the associated optional parameter values) using a switch statement. The default values are: **nthread=4**, **qlen=8**, **delay=0**. Each time an optional parameter is processed, the variable **conta_opt** is incremented by 2. This variable tracks how many optional parameters (and their associated values) were passed from the console, so that later, when evaluating the remaining parameters (binary files), we know where to start in *argv* (since `get_opt()` reorders *argv*, placing optional parameters in the first positions).

### **main()**:
The main thread reads the parameters passed from the console using a for loop, starting from **conta_opt** up to argc. The main thread synchronizes with the consumer threads via a mutex and two condition variables (**cthread**, where the parent waits for a signal from the threads, and **cpadre**, where secondary threads wait for a signal from the main thread). It waits for the necessary conditions to execute their tasks (**dati**: indicates how much data is available for processing, **free_block**: indicates how many buffer slots are free). Once the main thread has finished placing all file names from the console into the buffer, it inserts **nthread** empty strings (a file name cannot be an empty string).  
To handle the *SIGINT* signal, the default handler was replaced with the **handl()** function, which sets the global variable **segnale** to 1 upon receiving the signal. This allows the main thread to terminate early and wait for the consumer threads to finish processing the files already in the buffer.

### **tbody()**:
This function manages the consumer threads, synchronizing them with the parent as explained in **main()**. For each non-empty string in the buffer, the threads check if a file exists with the given name, and if so, they update the variable **somma** as somma += i * file[i]. After calculating the total sum for each file, they send the pair **name, sum** to the server using the **comunicazione()** function. If a consumer thread reads an empty string in the buffer, it terminates.

### **comunicazione()**:
This function establishes a connection via socket to the server and sends the pair **name, sum**. It first concatenates the file name and sum (casting from long to string) separated by the character **"/"** (chosen because a file name cannot contain this character). The entire string is then converted, character by character, into an array of integers (so it can be sent through the socket). First, the array length is sent to the server (so the server knows how many bytes to expect), and then the message is sent, integer by integer (each integer corresponds to one character in the message).

## = *client.c* =

### **main()**:
If no parameters are passed from the console, the main function calls **comunicazione(mode, long)**, passing *-1* and *0* as parameters. Otherwise, it invokes the function as many times as there are parameters, passing *1* and the *long* value to be sent to the server for each call.

### **comunicazione()**:
If the function receives *-1* as the mode, it only sends *-1* to the server, indicating that the client's request is to print all pairs, ignoring the second parameter. Since the value passed indicates a negative size, which cannot be sent, the server understands the client's request. If the mode is *1*, the function converts the *long* into a string and then into an array of integers (so it can be sent via socket). The function first sends the size of the message, then the actual message. Since the size will be positive, the server expects a message and will print the requested pair **sum, name**.

## = *collector.py* =

The server creates a thread for each connection request, using the **gestisci_connesioni** method to handle the client's request, accessing the dictionary containing the pairs in mutual exclusion.

### **gestisci_connesioni()**:
For each connection, the server waits to receive an integer indicating the size of the incoming message. If the size is *-1*, it means no further message is expected, and the request is to print all pairs currently stored, ordered in ascending order. If the size is positive, the server waits for the message, which consists of a series of integers. Once converted and concatenated, they form the message. If the message contains the character **"/"**, the client's request is to split the message and save the pair **name, sum** separated by the **"/"**. If the **"/"** is not present, the request is to print all file **names** associated with the **sum** in the message.

### **trova()**:
If the dictionary is empty or the requested key is not present, the server prints **Nessun file**. Otherwise, it prints the value associated with the key.

### **stampa()**:
The server saves all the keys in a list, sorts it, and prints each value in the dictionary by querying with the sorted keys. If the dictionary is empty, it prints **"Nessun file"**.
