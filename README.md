# Memoire

## Compilation
```
gcc -Wall -g -pthread main.c -L. -liof -o main
```

## Description du projet
Le prgramme créer un nombre de thread et de demande d'accés en fonction    
des informations du fichier de configuration. Un thread fils fait une    
demande d'accès à la mémoire en envoyant un numéro de page compris entre   
0 et le nombre de pages en mémoire lente moins un. Une fois la demande    
faite le père récupère l'information, vérifie avec l'algorithme de LRU      
si la mémoire rapide est pleine. Si elle ne l'est pas il envoie l'adresse    
logique correspondante à la demande sinon il renvoie -1. La communication  
entre les threads fils et le thread père se fait avec deux fifo, la première    
fifo s'occupe de l'écriture de la demande d'adresse physique et la lecture   
par le thread père de l'information. La seconde fifo s'occupe de l'envoie   
de l'adresse logique au thread fils. Lorsqu'un thread fils obtient l'adresse    
logique il incrémente son compteur de hit. Avec un fichier de configuration  
contenant les information suivante : 4 4096 256 10 100, le pourcentage de hit   
pour un thread varie de 0 à 9% avec une moyenne total pour tous les threads        
de 0 à 3 pourcent.

## Auteur
Théophile Molinatti 21707388 TD4
