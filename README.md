# Progetto del corso di Advanced Operating Systems A.A. 2020-2021, Prof. F. Quaglia
## Università degli Studi di Roma Tor Vergata
#### Autore: Giorgia Marchesi

## Obiettivi del progetto
L'obiettivo del progetto è quello di implementare un sottosistema del Kernel Linux che consenta lo scambio di messaggi tra thread. Il sottosistema deve occuparsi della creazione, rimozione, apertura e gestione di 256 tags ciascuno formato da 32 livelli. 
I thread si sottoscrivono ad uno specifico livello di un determinato tag per inviare/ricevere messaggi. Si noti però che la rimozione di un servizio non puà avvenire se ci sono ancora lettori pendenti e, in tal caso,
viene messa a disposizione una funzione di risveglio per rimuoverli.
Inoltre, viene offerto un device driver per verificare qual è lo stato attuale del servizio fornendo informazioni quali la chiave del tag, il creatore del tag, i livelli associati ed i corrispettivi lettori in attesa di ricevere un messaggio su ciascun livello.

## Installazione
Per utilizzare il modulo è necessario aprire un terminale nella cartella principale e compilare i codici sorgenti utilizzando il **Makefile** a disposizione:
``` js
make clean; make; make load
```

## Test

### Esecuzione dei test
1. Per individuare il numero delle system call ed il valore del major number associato al device driver digitare:
    ```js
     dmesg 
   ```
   
2. Se necessario, modificare i numeri delle system call nel file **common_function.h** nella cartella **user**
     ``` js
    #define TAG_GET syscall_num
    #define TAG_SEND syscall_num
    #define TAG_RECEIVE syscall_num
    #define TAG_CTL syscall_num
    ```
3. nella cartella **user** avviare il **Makefile**:
    ``` js
    make 
   ```
4. avviare l'esecuzione del test desiderato (esempio);
    ``` js
    ./test_remove 
   ```
   per avviare il test sul driver eseguire il comando:
    ``` js
    sudo ./test_driver /dev/miodev <major> <minor> 
   ```
5. Per la verifica del flusso di esecuzione a run-time lato kernel digitare:
    ``` js
    dmesg -w 
    ```

### Test disponibili

I test eseguibili sono:

* *test_remove.c*: creazione, rimozione ed apertura anche di tag service già esistenti per verificare il corretto funzionamento delle funzionalità di creazione, apertura e rimozione di un tag service;
* *test_wakeupall.c*: creazione di soli thread lettori per verificarne il corretto risveglio da parte della funzione di *wake_up*;
* *test_driver.c*: test del corretto funzionamento del driver che si occupa di reperire lo snapshot del sistema (stato attuale dei tag service);
* *reader.c / writer.c*: in tal caso, avviare prima il reader e solo successivamente il writer. Test del corretto funzionamento in lettura e scrittura delle funzionalità del modulo; 
* *multi_read_write_multi_level.c*: test per analisi della corretta sincronizzazione quando letture e scritture avvengono tra più livelli dello stesso tag service;
* *multi_read_write_multi_tag.c*: test per analisi della corretta sincronizzazione quando letture e scritture avvengono tra tag services. Inoltre vengono nuovamente testate le funzioni di risveglio e rimozione dei tag service in presenza ed assenza di lettori pendenti; 

*Nota: nella cartella **test_results** sono presenti i risultati dei test effettuati in precedenza.*


### Ulteriori informazioni

Per informazioni più dettagliate sul funzionamento del sistema, viene allegata al progetto la relazione.

