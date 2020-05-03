# Memoire TD11

## Compilation
```
gcc -Wall -g -pthread main.c -L. -liof -o main
```

## Description

Thread père : programme principal
L'adresse physique : un numéro de page aléatoire allant de 0 à (nombre de page - 1)
L'adresse logique : PositionMémoireRapide * taillePage + (adresseLogique % taillePage)
Communication entre les threads : utilisation de deux FIFO

## Auteur
Théophile Molinatti 21707388 TD4
