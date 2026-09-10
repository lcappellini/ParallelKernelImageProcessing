# Parallel Kernel Image Processing

Implementazione di tecniche di elaborazione delle immagini tramite convoluzione con un kernel.

Il progetto è stato sviluppato per l'assignment del corso di Parallel Computing, il quale richiede l'implementazione del programma in modo che sfrutti almeno uno degli approcci di parallelizzazione o vettorizzazione presentati durante il corso (OpenMP, vettorizzazione SIMD, CUDA), a partire da una versione sequenziale dello stesso algoritmo, corredata da un'analisi delle performance in termini di speed-up.

## Tecniche utilizzate

* **Base**: implementazione sequenziale
* **AVX2**: vettorizzazione tramite istruzioni SIMD
* **OpenMP**: parallelizzazione su CPU (multithreading)
* **AVX2+OpenMP**: combinazione delle due tecniche
* **CUDA**: parallelizzazione su GPU

Le implementazioni vengono confrontate in termini di tempo di esecuzione al variare della risoluzione dell'immagine e della configurazione utilizzata.

## Relazione

La relazione completa, contenente dettagli su implementazione, metodologia di test e risultati sperimentali, è disponibile qui:

[**Relazione Parallel Kernel Image Processing**](Relazione%20e%20grafici/Relazione%20Parallel%20Kernel%20Image%20Processing.pdf)

## Immagine d'esempio

Immagine d'esempio a cui è applicato un kernel corrispondente all'effetto di "Edge Detection"

<a href="Relazione%20e%20grafici/Relazione%20Parallel%20Kernel%20Image%20Processing.pdf">
  <div style="display: flex; gap: 10px;">
    <img src="Relazione%20e%20grafici/Graphs/Sample%20image.png" width="48%">
    <img src="Relazione%20e%20grafici/Graphs/Sample%20image%20fx.png" width="48%">
  </div>
</a>

## 
