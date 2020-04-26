#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "se_fichier.h"

typedef struct 
{
	int nbrFrames;		//< Nombre de frames
	int taillePage;		//< Taille d'une page mémoire s
	int nbrPage;		//< Nombre de pages disponibles en mémoire lente n
	int nbrThread;		//< Nombre de threads fils
	int nbrAcces;		//< Nombre d'acces demandes par thread
	
} DATA;

typedef struct
{
	int *memRapide;		//< Memoire physique RAM
	int *memLente;		//< Memoire physique Disque
	
} MEMORY;

typedef struct 
{
	pthread_mutex_t   *mut;
	pthread_barrier_t *bar;
	pthread_t tid;
	
	int hit;		//< Nombre de hit
	int tailleMem;	//< Taille de la mémoire virtuelle
	int numReq;		//< Nombre de requête
	
	int *memVir;	//< Mémoire virtuelle
	
} ARG_T;

DATA initStruct(const char *chemin)
{
	int rd;
	
	DATA dt;
	SE_FICHIER file;
	
	file = SE_ouverture (chemin, O_RDONLY);
	
	if (file.descripteur < 0) 
	{
		fprintf(stderr, "ERR: opening cfg file\n");
		exit(EXIT_FAILURE);
	}

	rd = SE_lectureEntier(file, &dt.nbrFrames);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrFrames\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}

	rd = SE_lectureEntier(file, &dt.taillePage);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing taillePage\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrPage);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrPage\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrThread);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrThread\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	rd = SE_lectureEntier(file, &dt.nbrAcces);
	if (rd <= 0)
	{
		fprintf(stderr, "ERR: missing nbrAcces\n");
		SE_fermeture(file);
		exit(EXIT_FAILURE);
	}
	
	SE_fermeture(file);
	
	return dt;
}

MEMORY initMemoirePhysique(int nbrPage, int nbrFrames)
{
	MEMORY mem;
	
	mem.memLente = malloc(nbrPage * sizeof(int));
	for(int size = 0; size < nbrPage; size++)
		mem.memLente[size] = -1;
	
	mem.memRapide = malloc(nbrFrames * sizeof(int));
	for(int size = 0; size < nbrFrames; size++)
		mem.memRapide[size] = -1;
	
	return mem;
}

void afficheConfig(DATA dt)
{
	printf("Nombre de frame : %d\n", dt.nbrFrames);
	printf("Taille d'une page : %d = %dko\n", dt.taillePage, (dt.taillePage/1000));
	printf("Nombre de page dans la memoire lente : %d\n", dt.nbrPage);
	printf("Nombre de thread : %d\n", dt.nbrThread);
	printf("Nombre de demande par thread : %d\n", dt.nbrAcces);
}

void * demandeAcces (void * arg)
{
	ARG_T *at = (ARG_T *) arg;
	
	for(int cmpt = 0; at -> numReq; cmpt++)
	{
		//Création du tube à partir du tid du thread
		mkfifo (at -> tid, 0600);
		
		//Ectiture de la demande dans un tube en mode ecriture
		//Lecture de la reponse de la part du pere
		//Suppression du tube
	}
	
	return NULL;
}

void gestionThread(DATA dt)
{
	pthread_mutex_t   mut;
	pthread_barrier_t bar;
	
	ARG_T *at;
	
	at = malloc(dt.nbrThread * sizeof(ARG_T));
	
	//Creation mutex et barriere
	pthread_mutex_init(&mut, NULL);
	pthread_barrier_init(&bar, NULL, dt.nbrThread);
	
	//Init des infos pour la structure des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
	{
		at[cmpt].mut = &mut;
		at[cmpt].bar = &bar;
		at[cmpt].hit = 0;
	}
	
	//Creation des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
		pthread_create(&at[cmpt].tid, NULL, demandeAcces, &at[cmpt]);
	
	//Communication par tubes
	for(int cmpt = 0; cmpt < dt.nbrThread * dt.nbrAcces; cmpt++)
	{
		//Probleme comment connaitre le tid de lecture var global?
		//Lecture du tube avec comme chemin le tid du thread
		//Recuperation de l'information ecrite dans le tube 
		//Suppresion du tube 
		//Creation du tube à partir du même tid que le tube du mode lecture
		//Utiliser l'algo de LRU pout verifier la demande soit presente dans le cache
		//Si la valeur est presente renvoyer la valeur grace au tube en ecriture
		//Sinon renvoyé 0 par exemple 
		
	}
	
	//Fin des threads
	for(int cmpt = 0; cmpt < dt.nbrThread; cmpt++)
		pthread_join(at[cmpt].tid, NULL);
	
	//Suppresion des mutex et des barrieres 
	pthread_mutex_destroy(&mut);
	pthread_barrier_destroy(&bar);
	
	free(at);
}

int *LRU(int frame)
{
	int *cache;
	
	return cache;
}

int main()
{
	const char *chemin = "data.cfg";
	
	pthread_mutex_t   mut;
	pthread_barrier_t bar;
	
	DATA dt = initStruct(chemin);
	
	afficheConfig(dt);
	
	gestionThread(dt);
	
	return 0;
}
